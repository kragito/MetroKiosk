// Runs the default 3D mode: spawns the concourse, locks the camera on the kiosk (E / Esc), drives the cursor, the hardware parts, the wallet and the collection tray, and hosts the self-test.
#include "KioskWorld.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "KioskEnvironment.h"
#include "KioskLoc.h"
#include "KioskStyle.h"
#include "KioskSession.h"
#include "KioskVisitorPawn.h"
#include "Widgets/Components/KioskAutomation.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SKioskRoot.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogKiosk3D, Display, All);

namespace
{
	constexpr float NearDistanceCm = 260.f;
	constexpr float NearFacingDot = 0.6f;
	constexpr float ShotDelaySeconds = 4.f;
	constexpr float QuitAfterShotSeconds = 3.f;
	constexpr float LockOnFov = 90.f;
	constexpr double ToastSeconds = 4.0;

	/** Wallet item i is chosen with number key i + 1 (the tenth with 0). */
	const FKey& NumberKey(int32 Index)
	{
		static const FKey Keys[10] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero };
		return Keys[FMath::Clamp(Index, 0, 9)];
	}

	/** Sees E, Esc and the number keys before the viewport does, so they work in both walking and locked-on modes. */
	class FKioskWorldInput : public IInputProcessor
	{
	public:
		explicit FKioskWorldInput(FKioskWorld* InOwner) : Owner(InOwner) {}
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			return Owner && Owner->HandleKey(InKeyEvent.GetKey());
		}
		virtual const TCHAR* GetDebugName() const override { return TEXT("KioskWorldInput"); }

	private:
		FKioskWorld* Owner;
	};
}

bool FKioskWorld::IsRequested()
{
	return !FParse::Param(FCommandLine::Get(), TEXT("Kiosk2D"));
}

FKioskWorld::FKioskWorld(UWorld* InWorld, UGameViewportClient* InViewport, const TSharedRef<SKioskRoot>& InRoot, const TSharedRef<FKioskSession>& InSession)
	: World(InWorld)
	, Viewport(InViewport)
	, Root(InRoot)
	, Session(InSession)
{
}

FKioskWorld::~FKioskWorld()
{
	Shutdown();
}

void FKioskWorld::Start()
{
	UWorld* PlayWorld = World.Get();
	UGameViewportClient* GameViewport = Viewport.Get();
	if (!PlayWorld || !GameViewport || bStarted)
	{
		return;
	}
	bStarted = true;

	AKioskEnvironment* Env = PlayWorld->SpawnActor<AKioskEnvironment>(FVector::ZeroVector, FRotator::ZeroRotator);
	Environment = Env;
	if (Env)
	{
		Env->Build();
		Env->SetScreenContent(Root.ToSharedRef());
	}

	APlayerController* Controller = PlayWorld->GetFirstPlayerController();
	if (Controller)
	{
		AKioskVisitorPawn* Visitor = Cast<AKioskVisitorPawn>(Controller->GetPawn());
		if (!Visitor)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Visitor = PlayWorld->SpawnActor<AKioskVisitorPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
			if (Visitor)
			{
				Controller->Possess(Visitor);
			}
		}
		Pawn = Visitor;
		if (Visitor && Env)
		{
			const FTransform StartTransform = Env->GetVisitorStart();
			Visitor->SetActorLocation(StartTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			FRotator Look = StartTransform.Rotator();
			Look.Pitch = -4.f; // the screen centre is a little below eye height
			Controller->SetControlRotation(Look);
		}
	}

	FString ShotView;
	if (FParse::Value(FCommandLine::Get(), TEXT("-Kiosk3DShotView="), ShotView) && Pawn.IsValid() && Controller && Env)
	{
		if (ShotView.Equals(TEXT("wide"), ESearchCase::IgnoreCase))
		{
			// Overview for the screenshot: back and to the side, raised, wide lens, so one frame shows the kiosk, floor, pillar, benches, lights and platform edge.
			Pawn->SetActorLocation(FVector(620.f, -720.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
			Controller->SetControlRotation(FRotator(-14.f, 96.f, 0.f));
			Pawn->SetFieldOfView(105.f);
		}
		else if (ShotView.Equals(TEXT("kiosk"), ESearchCase::IgnoreCase))
		{
			// Hardware close-up: a card on the pad, the guide lights on and the tray holding a ticket, a receipt and change.
			bHardwareDemo = true;
			Pawn->SetWalkingEnabled(false);
			Pawn->LockView(Env->GetHardwareShotView(), 80.f);
			bCardOnPad = true;
			Env->SetCardOnPad(true);
			Env->SetTrayContents(true, true, 1310);
			for (const EKioskPart Part : { EKioskPart::Contactless, EKioskPart::ChipSlot, EKioskPart::NoteSlot, EKioskPart::CoinSlot, EKioskPart::CardPad, EKioskPart::ChangeTray })
			{
				Env->SetPartLight(Part, true);
			}
		}
	}
	bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("Kiosk3DSelfTest"));
	if (FParse::Value(FCommandLine::Get(), TEXT("-Kiosk3DShotDir="), ShotDir))
	{
		ShotDir = ShotDir.TrimQuotes();
	}
	if (bSelfTest)
	{
		BuildSelfTestPlan();
	}

	Hud = BuildHud();
	GameViewport->AddViewportWidgetContent(Hud.ToSharedRef(), 200);
	WalletHud = BuildWallet();
	GameViewport->AddViewportWidgetContent(WalletHud.ToSharedRef(), 210);
	if (bHardwareDemo)
	{
		Hud->SetVisibility(EVisibility::Collapsed);
		WalletHud->SetVisibility(EVisibility::Collapsed);
	}

	InputProcessor = MakeShared<FKioskWorldInput>(this);
	FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);

	FString StartMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("-Kiosk3DStart="), StartMode) && StartMode.Equals(TEXT("kiosk"), ESearchCase::IgnoreCase))
	{
		EnterKioskView();
	}
	else
	{
		SetWalkingInput();
	}

	if (FParse::Value(FCommandLine::Get(), TEXT("-Kiosk3DShot="), ShotPath))
	{
		ShotPath = ShotPath.TrimQuotes();
		ShotTimer = ShotDelaySeconds;
	}
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FKioskWorld::Tick));
}

void FKioskWorld::Shutdown()
{
	if (!bStarted)
	{
		return;
	}
	bStarted = false;
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	if (UGameViewportClient* GameViewport = Viewport.Get())
	{
		if (Hud.IsValid())
		{
			GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
		}
		if (WalletHud.IsValid())
		{
			GameViewport->RemoveViewportWidgetContent(WalletHud.ToSharedRef());
		}
	}
	Hud.Reset();
	WalletHud.Reset();
	if (AKioskEnvironment* Env = Environment.Get())
	{
		Env->ClearScreenContent();
	}
	bKioskView = false;
}

void FKioskWorld::SetWalkingInput()
{
	UWorld* PlayWorld = World.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	if (!Controller)
	{
		return;
	}
	Controller->bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	Controller->SetInputMode(InputMode);
}

void FKioskWorld::EnterKioskView()
{
	UWorld* PlayWorld = World.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	AKioskVisitorPawn* Visitor = Pawn.Get();
	AKioskEnvironment* Env = Environment.Get();
	if (bKioskView || !Controller || !Visitor || !Env)
	{
		return;
	}
	bKioskView = true;
	Visitor->SetWalkingEnabled(false);
	Visitor->LockView(Env->GetLockOnView(), LockOnFov);
	// The mouse moves a cursor over the 3D screen and the kiosk parts; the viewport keeps the clicks that Slate (the wallet) does not take.
	Controller->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	Controller->SetInputMode(InputMode);
}

void FKioskWorld::ExitKioskView()
{
	AKioskVisitorPawn* Visitor = Pawn.Get();
	if (!bKioskView || !Visitor)
	{
		return;
	}
	bKioskView = false;
	if (bScreenPressed)
	{
		Visitor->InjectPointer(false);
		bScreenPressed = false;
	}
	bSyntheticCursor = false;
	SyntheticClickFrames = 0;
	Visitor->ReleaseView();
	if (AKioskEnvironment* Env = Environment.Get())
	{
		Env->SetPartHighlight(EKioskPart::None);
	}
	CursorPart = EKioskPart::None;
	SetWalkingInput(); // walking itself resumes once the camera is back at the eye (TickKiosk)
}

bool FKioskWorld::IsNearKiosk() const
{
	const AKioskEnvironment* Env = Environment.Get();
	const AKioskVisitorPawn* Visitor = Pawn.Get();
	if (!Env || !Visitor)
	{
		return false;
	}
	const FVector ToScreen = Env->GetScreenCenter() - Visitor->GetEyeLocation();
	const float Distance = ToScreen.Size();
	if (Distance > NearDistanceCm || Distance < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	return FVector::DotProduct(ToScreen / Distance, Visitor->GetEyeForward()) > NearFacingDot;
}

bool FKioskWorld::HandleKey(const FKey& Key)
{
	if (bHardwareDemo)
	{
		return false;
	}
	if (bKioskView)
	{
		if (Key == EKeys::Escape)
		{
			ExitKioskView();
			return true;
		}
		for (int32 Index = 0; Index < KioskWallet::Items().Num() && Index < 10; ++Index)
		{
			if (Key == NumberKey(Index))
			{
				SelectWalletItem(Index);
				return true;
			}
		}
		return false;
	}
	if (Key == EKeys::E && IsNearKiosk())
	{
		EnterKioskView();
		return true;
	}
	return false;
}

void FKioskWorld::SelectWalletItem(int32 Index)
{
	SelectedItem = KioskWallet::Items().IsValidIndex(Index) ? Index : INDEX_NONE;
}

// --- parts, cursor, tray ---------------------------------------------------------------------

void FKioskWorld::ShowToast(const FText& Text)
{
	ToastText = Text;
	ToastUntil = FPlatformTime::Seconds() + ToastSeconds;
}

void FKioskWorld::SyncTray()
{
	AKioskEnvironment* Env = Environment.Get();
	const EKioskScreen Now = Session->Current();
	if (Now == LastScreen || !Env)
	{
		return;
	}
	LastScreen = Now;
	if (Now == EKioskScreen::Home)
	{
		SelectedItem = INDEX_NONE; // a new visitor starts with empty hands
	}
	if (Now == EKioskScreen::Receipt)
	{
		bTrayTicket = true; // the printed ticket and the receipt drop into the tray
		bTrayReceipt = true;
		TrayChangeCents += Session->ChangeCents();
	}
	else if (Now == EKioskScreen::DoneLiftCard)
	{
		bTrayReceipt = true;
		TrayChangeCents += Session->ChangeCents();
	}
	else
	{
		return;
	}
	Env->SetTrayContents(bTrayTicket, bTrayReceipt, TrayChangeCents);
}

void FKioskWorld::PickUpFromTray()
{
	AKioskEnvironment* Env = Environment.Get();
	const int32 Change = TrayChangeCents;
	bTrayTicket = false;
	bTrayReceipt = false;
	TrayChangeCents = 0;
	if (Env)
	{
		Env->SetTrayContents(false, false, 0);
	}
	ShowToast(Change > 0 ? FText::Format(KT("You collected your things and {0} change"), FKioskSession::Money(Change)) : KT("You collected your things from the tray"));
}

void FKioskWorld::OnPartClicked(EKioskPart Part)
{
	const TArray<FKioskWalletItem>& Items = KioskWallet::Items();
	const FKioskWalletItem* Item = Items.IsValidIndex(SelectedItem) ? &Items[SelectedItem] : nullptr;
	++UseCount;

	if (Session->IsIdlePrompt() || Session->IsCallActive())
	{
		bLastUseAccepted = false;
		ShowToast(KT("Answer the message on the screen first."));
		return;
	}

	if (Part == EKioskPart::ChangeTray && !Item)
	{
		bLastUseAccepted = TrayHasContents();
		if (bLastUseAccepted)
		{
			PickUpFromTray();
		}
		else
		{
			ShowToast(KT("The tray is empty."));
		}
		return;
	}

	const bool bWasLift = Session->Current() == EKioskScreen::DoneLiftCard;
	const KioskWallet::FUseResult Result = KioskWallet::Use(*Session, Item, Part);
	bLastUseAccepted = Result.bAccepted;
	if (!Result.Message.IsEmpty())
	{
		ShowToast(Result.Message);
	}
	if (Result.bAccepted)
	{
		Session->NoteActivity();
	}
	if (Result.bAccepted && Part == EKioskPart::CardPad)
	{
		bCardOnPad = !bWasLift; // tapped on: the card rests on the pad; lifted: it is back in the hand
	}
}

void FKioskWorld::TickKiosk(float DeltaSeconds)
{
	AKioskVisitorPawn* Visitor = Pawn.Get();
	AKioskEnvironment* Env = Environment.Get();
	UWorld* PlayWorld = World.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	if (!Visitor || !Env || !Controller || !Session.IsValid())
	{
		return;
	}

	// Back from the kiosk: walking resumes once the camera has returned to the eye.
	if (!bKioskView && !bHardwareDemo && !Visitor->IsWalkingEnabled() && Visitor->IsViewFree())
	{
		Visitor->SetWalkingEnabled(true);
		Visitor->ResetPointerAim();
	}

	// Guide lights, the card on the pad and the tray contents follow the session.
	if (!bHardwareDemo)
	{
		SyncTray();
		const EKioskScreen Screen = Session->Current();
		if (Screen == EKioskScreen::Home)
		{
			bCardOnPad = false;
		}
		if (Env->IsCardOnPad() != bCardOnPad)
		{
			Env->SetCardOnPad(bCardOnPad);
		}
		for (const EKioskPart Part : { EKioskPart::Contactless, EKioskPart::ChipSlot, EKioskPart::NoteSlot, EKioskPart::CoinSlot, EKioskPart::CardPad })
		{
			Env->SetPartLight(Part, KioskWallet::IsPartWanted(*Session, Part));
		}
		Env->SetPartLight(EKioskPart::TicketSlot, Screen == EKioskScreen::Processing);
		Env->SetPartLight(EKioskPart::ChangeTray, TrayHasContents());
	}

	if (!bKioskView)
	{
		Env->SetPartHighlight(EKioskPart::None);
		CursorPart = EKioskPart::None;
		return;
	}

	// The cursor: a ray from the camera through the mouse (or the self-test's synthetic cursor).
	EKioskPart Hover = EKioskPart::None;
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	bool bHaveRay = false;
	if (bSyntheticCursor)
	{
		Origin = Visitor->GetEyeLocation();
		Direction = (SyntheticTarget - Origin).GetSafeNormal();
		bHaveRay = true;
	}
	else
	{
		bHaveRay = Controller->DeprojectMousePositionToWorld(Origin, Direction);
	}
	if (bHaveRay && Visitor->IsViewSettled())
	{
		FCollisionQueryParams Params(TEXT("KioskCursor"), false);
		Params.AddIgnoredActor(Visitor);
		FHitResult Hit;
		if (PlayWorld->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * 800.f, ECC_Visibility, Params))
		{
			Hover = Env->PartForComponent(Hit.GetComponent());
			if (Hover == EKioskPart::Screen)
			{
				Visitor->AimPointerAt(Hit.ImpactPoint);
			}
		}
	}
	if (Hover != EKioskPart::Screen)
	{
		Visitor->AimPointerAway();
	}
	CursorPart = Hover;
	Env->SetPartHighlight(Hover == EKioskPart::Screen ? EKioskPart::None : Hover);

	// Clicks: on the screen they go to the widget pointer, on a part to the wallet rules.
	bool bDown = false;
	bool bUp = false;
	if (SyntheticClickFrames > 0)
	{
		bDown = SyntheticClickFrames == 4;
		bUp = SyntheticClickFrames == 1;
		--SyntheticClickFrames;
	}
	else if (!bSyntheticCursor)
	{
		bDown = Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton);
		bUp = Controller->WasInputKeyJustReleased(EKeys::LeftMouseButton);
	}
	if (bDown && Visitor->IsViewSettled())
	{
		if (Hover == EKioskPart::Screen)
		{
			Visitor->InjectPointer(true);
			bScreenPressed = true;
		}
		else if (Hover != EKioskPart::None)
		{
			OnPartClicked(Hover);
		}
	}
	if (bUp && bScreenPressed)
	{
		Visitor->InjectPointer(false);
		bScreenPressed = false;
	}
	const float Wheel = bSyntheticCursor ? 0.f : Controller->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (Wheel != 0.f && Hover == EKioskPart::Screen)
	{
		Visitor->ScrollPointer(Wheel);
	}
}

bool FKioskWorld::Tick(float DeltaSeconds)
{
	TickKiosk(DeltaSeconds);
	if (bSelfTest && !TickSelfTest(DeltaSeconds))
	{
		return false;
	}
	if (ShotTimer >= 0.f)
	{
		ShotTimer -= DeltaSeconds;
		if (ShotTimer < 0.f)
		{
			FScreenshotRequest::RequestScreenshot(ShotPath, true, false);
			UE_LOG(LogTemp, Display, TEXT("Kiosk3D: screenshot requested: %s"), *ShotPath);
			QuitTimer = QuitAfterShotSeconds;
		}
	}
	if (QuitTimer >= 0.f)
	{
		QuitTimer -= DeltaSeconds;
		if (QuitTimer < 0.f)
		{
			FPlatformMisc::RequestExit(false);
			return false;
		}
	}
	return true;
}

// --- HUD and wallet --------------------------------------------------------------------------

TSharedRef<SWidget> FKioskWorld::BuildHud()
{
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = KC();

	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		// Crosshair while walking.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(8.f)
			.HeightOverride(8.f)
			.Visibility_Lambda([this]() { return bKioskView ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			[
				SNew(SBorder)
				.BorderImage(Style.Round(C.White, 4.f, C.Ink, 1.f))
			]
		]
		// Hint pill at the bottom while walking.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, 36.f)
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return bKioskView ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			.BorderImage(Style.Round(C.Scrim, 14.f))
			.Padding(FMargin(22.f, 10.f))
			[
				SNew(STextBlock)
				.Font(Style.Font(KioskType::Body, true))
				.ColorAndOpacity(C.White)
				.Text_Lambda([this]()
				{
					if (IsNearKiosk())
					{
						return KT("Press E to use the kiosk");
					}
					return KT("Move with the keyboard, look with the mouse");
				})
			]
		]
		// While stepped up to the kiosk the way back sits at the top, clear of the kiosk.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.f, 16.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.Visibility_Lambda([this]() { return bKioskView ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderImage(Style.Round(C.Scrim, 14.f))
			.Padding(FMargin(22.f, 8.f))
			[
				SNew(STextBlock)
				.Font(Style.Font(KioskType::Body, true))
				.ColorAndOpacity(C.White)
				.Text_Lambda([]() { return KT("Esc to step back"); })
			]
		];
}

TSharedRef<SWidget> FKioskWorld::BuildWallet()
{
	const TArray<FKioskWalletItem>& Items = KioskWallet::Items();
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FKioskWalletItem& Item = Items[Index];
		const FString Number = FString::FromInt((Index + 1) % 10);
		Row->AddSlot()
		.AutoWidth()
		.Padding(4.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage_Lambda([this, Index]()
			{
				FKioskStyle& Style = FKioskStyle::Get();
				const FKioskColors& C = Style.C;
				return SelectedItem == Index ? Style.Round(C.PrimaryTint, 12.f, C.Primary, 4.f) : Style.Round(C.White, 12.f, C.BorderStrong, 2.f);
			})
			.Padding(FMargin(14.f, 8.f))
			.OnMouseButtonDown_Lambda([this, Index](const FGeometry&, const FPointerEvent& Event)
			{
				if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
				{
					return FReply::Unhandled();
				}
				SelectWalletItem(SelectedItem == Index ? INDEX_NONE : Index);
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(FKioskStyle::Get().Font(KioskType::Caption))
					.ColorAndOpacity(KC().Muted)
					.Text(FText::FromString(Number))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(FKioskStyle::Get().Font(KioskType::Body, true))
					.ColorAndOpacity(KC().Ink)
					.Text(KioskWallet::ItemLabel(Item))
				]
			]
		];
	}

	return SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, 24.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return bKioskView ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			// What to do next, or why a part refused.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 10.f)
			[
				SNew(SBorder)
				.Visibility(EVisibility::HitTestInvisible)
				.BorderImage(FKioskStyle::Get().Round(KC().Scrim, 14.f))
				.Padding(FMargin(22.f, 10.f))
				[
					SNew(STextBlock)
				.Font(FKioskStyle::Get().Font(KioskType::Body, true))
				.ColorAndOpacity(KC().White)
				.WrapTextAt(900.f)
				.Justification(ETextJustify::Center)
				.Text_Lambda([this]()
				{
					return FPlatformTime::Seconds() < ToastUntil ? ToastText : KT("Choose an item from your wallet, then click the kiosk part to use it");
				})
				]
			]
			// The wallet: one tile per item.
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FKioskStyle::Get().Round(KC().Scrim, 16.f))
				.Padding(FMargin(10.f))
				[
					Row
				]
			]
		];
}

// --- self-test -------------------------------------------------------------------------------

void FKioskWorld::SelfTestOk(const FString& Step, const FString& Detail)
{
	UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST step %d %s: ok (%s)"), TestStep, *Step, *Detail);
	++TestStep;
	TestFrames = 0;
	TestPhase = 0;
	TestSub = 0;
	SubFrames = 0;
	TestTime = 0.f;
}

void FKioskWorld::SelfTestFail(const FString& Step, const FString& Detail)
{
	UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST FAIL: %s: %s"), *Step, *Detail);
	bSelfTest = false;
	FPlatformMisc::RequestExit(false);
}

bool FKioskWorld::FindButtonPixel(const FString& Name, FVector2D& OutPixel) const
{
	bool bFound = false;
	TFunction<void(SWidget&)> Visit = [&](SWidget& Widget)
	{
		if (bFound || !Widget.GetVisibility().IsVisible())
		{
			return;
		}
		if (TSharedPtr<FKioskAutomationMeta> Meta = Widget.GetMetaData<FKioskAutomationMeta>())
		{
			const FGeometry Geo = Widget.GetTickSpaceGeometry();
			if (Meta->Name.Equals(Name, ESearchCase::IgnoreCase) && Geo.GetLocalSize().X > 1.f)
			{
				const FGeometry RootGeo = Root->GetTickSpaceGeometry();
				const FVector2f Local = RootGeo.AbsoluteToLocal(Geo.GetAbsolutePosition() + Geo.GetAbsoluteSize() * 0.5f);
				const FVector2f RootSize = RootGeo.GetLocalSize();
				if (RootSize.X > 1.f && RootSize.Y > 1.f)
				{
					OutPixel = FVector2D(Local.X / RootSize.X * SKioskRoot::CanvasWidth, Local.Y / RootSize.Y * SKioskRoot::CanvasHeight);
					bFound = true;
					return;
				}
			}
		}
		if (FChildren* Children = Widget.GetChildren())
		{
			Children->ForEachWidget([&Visit](SWidget& Child) { Visit(Child); });
		}
	};
	if (Root.IsValid())
	{
		Visit(*Root);
	}
	return bFound;
}

void FKioskWorld::PressKeyThroughProcessor(const FKey& Key)
{
	const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
	if (InputProcessor.IsValid())
	{
		InputProcessor->HandleKeyDownEvent(FSlateApplication::Get(), Event);
	}
}

void FKioskWorld::SetSyntheticCursor(const FVector& WorldTarget)
{
	bSyntheticCursor = true;
	SyntheticTarget = WorldTarget;
}

void FKioskWorld::RequestShot(const FString& FilePath)
{
	FScreenshotRequest::RequestScreenshot(FilePath, true, false);
	UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST screenshot requested: %s"), *FilePath);
}

void FKioskWorld::BuildSelfTestPlan()
{
	TestPlan.Reset();
	auto Add = [this](ETestAct Type, const FString& Name = FString(), int32 A = 0, EKioskPart PartArg = EKioskPart::None)
	{
		FTestAct Act;
		Act.Type = Type;
		Act.Name = Name;
		Act.A = A;
		Act.PartArg = PartArg;
		TestPlan.Add(Act);
	};

	Add(ETestAct::Spawn);
	Add(ETestAct::Walk);
	Add(ETestAct::LockOn);
	Add(ETestAct::Shot, TEXT("1_locked_on"));

	// One-way ticket paid with notes through the wallet and the note slot.
	for (const TCHAR* Button : { TEXT("Buy a ticket"), TEXT("Choose One-way"), TEXT("Station HarbourFront"), TEXT("Confirm selected ticket"), TEXT("Go to payment"), TEXT("Pay with cash") })
	{
		Add(ETestAct::Click, Button);
	}
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::WaitCash));
	Add(ETestAct::Select, FString(), 0); // the Metro card does not fit the note slot
	Add(ETestAct::Part, FString(), 0, EKioskPart::NoteSlot);
	Add(ETestAct::ExpectUse, FString(), 0);
	Add(ETestAct::Shot, TEXT("2_cash_refused"));
	Add(ETestAct::Select, FString(), 2); // $2 note
	Add(ETestAct::CashUntilPaid);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Receipt));
	Add(ETestAct::ExpectTray, FString(), 1);
	Add(ETestAct::Shot, TEXT("3_cash_receipt_tray"));
	Add(ETestAct::Select, FString(), -1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChangeTray);
	Add(ETestAct::ExpectTray, FString(), 0);
	Add(ETestAct::Click, TEXT("No receipt · I'm done"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	// Day pass paid with a card tap on the contactless reader.
	for (const TCHAR* Button : { TEXT("Buy a ticket"), TEXT("Choose Day pass"), TEXT("Go to payment"), TEXT("Pay with card") })
	{
		Add(ETestAct::Click, Button);
	}
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::WaitTapCard));
	Add(ETestAct::Shot, TEXT("4_card_waiting"));
	Add(ETestAct::Select, FString(), 1); // the bank card
	Add(ETestAct::Part, FString(), 0, EKioskPart::Contactless);
	Add(ETestAct::ExpectUse, FString(), 1);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Receipt));
	Add(ETestAct::ExpectTray, FString(), 1);
	Add(ETestAct::Shot, TEXT("5_card_receipt_tray"));
	Add(ETestAct::Select, FString(), -1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChangeTray);
	Add(ETestAct::ExpectTray, FString(), 0);

	Add(ETestAct::Click, TEXT("No receipt · I'm done"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	// One-way part-paid with a coin through the coin slot, the rest with notes.
	for (const TCHAR* Button : { TEXT("Buy a ticket"), TEXT("Choose One-way"), TEXT("Station HarbourFront"), TEXT("Confirm selected ticket"), TEXT("Go to payment"), TEXT("Pay with cash") })
	{
		Add(ETestAct::Click, Button);
	}
	Add(ETestAct::Select, FString(), 9); // $1 coin
	Add(ETestAct::Part, FString(), 0, EKioskPart::CoinSlot);
	Add(ETestAct::ExpectUse, FString(), 1);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::WaitCash));
	Add(ETestAct::Select, FString(), 2);
	Add(ETestAct::CashUntilPaid);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Receipt));
	Add(ETestAct::Select, FString(), -1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChangeTray);
	Add(ETestAct::ExpectTray, FString(), 0);
	Add(ETestAct::Click, TEXT("No receipt · I'm done"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	// One-way paid with the bank card in the chip slot.
	for (const TCHAR* Button : { TEXT("Buy a ticket"), TEXT("Choose One-way"), TEXT("Station HarbourFront"), TEXT("Confirm selected ticket"), TEXT("Go to payment"), TEXT("Pay with chip card/NETs") })
	{
		Add(ETestAct::Click, Button);
	}
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::WaitChipCard));
	Add(ETestAct::Select, FString(), 1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChipSlot);
	Add(ETestAct::ExpectUse, FString(), 1);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Receipt));
	Add(ETestAct::ExpectTray, FString(), 1);
	Add(ETestAct::Select, FString(), -1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChangeTray);
	Add(ETestAct::ExpectTray, FString(), 0);
	Add(ETestAct::Click, TEXT("No receipt · I'm done"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	// Top-up: Metro card on the pad, an amount, paid with a card tap, then the card is lifted off the pad.
	Add(ETestAct::Click, TEXT("Top up"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::ScanCard));
	Add(ETestAct::Select, FString(), 0);
	Add(ETestAct::Part, FString(), 0, EKioskPart::CardPad);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::CardOptions));
	Add(ETestAct::Click, TEXT("Top up another amount"));
	Add(ETestAct::Click, TEXT("Amount $20"));
	Add(ETestAct::Click, TEXT("Continue"));
	Add(ETestAct::Click, TEXT("Pay with card"));
	Add(ETestAct::Select, FString(), 1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::Contactless);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::DoneLiftCard));
	Add(ETestAct::Shot, TEXT("6_topup_done"));
	Add(ETestAct::Part, FString(), 0, EKioskPart::CardPad); // lift the card
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	// Concession pass paid through the chip slot.
	Add(ETestAct::Click, TEXT("Concession"));
	Add(ETestAct::Select, FString(), 0);
	Add(ETestAct::Part, FString(), 0, EKioskPart::CardPad);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Concession));
	Add(ETestAct::Click, TEXT("Pass Bus + Train"));
	Add(ETestAct::Click, TEXT("Continue · check dates"));
	Add(ETestAct::Click, FString(TEXT("Day ")) + (FKioskSession::Today() + FTimespan::FromDays(3)).ToString(TEXT("%Y-%m-%d")));
	Add(ETestAct::Click, TEXT("Go to payment"));
	Add(ETestAct::Click, TEXT("Pay with chip card/NETs"));
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::WaitChipCard));
	Add(ETestAct::Select, FString(), 1);
	Add(ETestAct::Part, FString(), 0, EKioskPart::ChipSlot);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::DoneLiftCard));
	Add(ETestAct::Shot, TEXT("7_concession_chip_done"));
	Add(ETestAct::Part, FString(), 0, EKioskPart::CardPad);
	Add(ETestAct::ExpectScreen, FString(), static_cast<int32>(EKioskScreen::Home));

	Add(ETestAct::Unlock);
	Add(ETestAct::Collide);
}

int32 FKioskWorld::StepClickPart(EKioskPart Part)
{
	AKioskEnvironment* Env = Environment.Get();
	switch (TestSub)
	{
	case 0:
		SetSyntheticCursor(Env->GetPartLocation(Part));
		TestSub = 1;
		SubFrames = 0;
		return 0;
	case 1:
		++SubFrames;
		if (CursorPart == Part)
		{
			QueueSyntheticClick();
			TestSub = 2;
			SubFrames = 0;
		}
		else if (SubFrames > 90)
		{
			TestDetail = FString::Printf(TEXT("the cursor never reached part %d (it is over part %d)"), static_cast<int32>(Part), static_cast<int32>(CursorPart));
			return -1;
		}
		return 0;
	default:
		++SubFrames;
		if (SubFrames >= 8)
		{
			TestSub = 0;
			return 1;
		}
		return 0;
	}
}

int32 FKioskWorld::StepSelfTest()
{
	AKioskVisitorPawn* Visitor = Pawn.Get();
	AKioskEnvironment* Env = Environment.Get();
	UWorld* PlayWorld = World.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	const FTestAct& Act = TestPlan[TestStep];

	switch (Act.Type)
	{
	case ETestAct::Spawn: // wait for the kiosk screen to be drawn so the button has a position
	{
		if (TestTime <= 3.f)
		{
			return 0;
		}
		FVector2D Pixel;
		if (!FindButtonPixel(TEXT("Buy a ticket"), Pixel))
		{
			TestDetail = TEXT("the Buy a ticket button has no layout on the 3D screen");
			return -1;
		}
		TestDetail = FString::Printf(TEXT("pawn at %s, Buy a ticket at canvas px %.0f,%.0f"), *Visitor->GetActorLocation().ToCompactString(), Pixel.X, Pixel.Y);
		Visitor->SetInjectedMove(1.f, 0.f);
		return 1;
	}

	case ETestAct::Walk: // walk until the "Press E" condition holds
		if (IsNearKiosk())
		{
			Visitor->SetInjectedMove(0.f, 0.f);
			TestDetail = FString::Printf(TEXT("near the kiosk after %.1f s at %s"), TestTotal, *Visitor->GetActorLocation().ToCompactString());
			return 1;
		}
		if (TestTime > 5.f)
		{
			TestDetail = TEXT("still not near the kiosk after 5 s of forward movement");
			return -1;
		}
		return 0;

	case ETestAct::LockOn: // E through the input processor: the camera locks on the kiosk front, cursor shown, walking off
		if (TestFrames == 1)
		{
			PressKeyThroughProcessor(EKeys::E);
			if (!bKioskView)
			{
				TestDetail = TEXT("the input processor did not use E near the kiosk");
				return -1;
			}
			return 0;
		}
		if (bKioskView && Visitor->IsViewSettled() && Controller->bShowMouseCursor && !Visitor->IsWalkingEnabled())
		{
			TestDetail = TEXT("camera locked on the kiosk front, cursor shown, walking off, Root still on the 3D screen");
			return Env->ScreenContentIs(Root.ToSharedRef()) ? 1 : -1;
		}
		if (TestTime > 3.f)
		{
			TestDetail = FString::Printf(TEXT("kioskView=%d settled=%d cursor=%d"), bKioskView, Visitor->IsViewSettled(), Controller->bShowMouseCursor);
			return -1;
		}
		return 0;

	case ETestAct::Unlock: // Esc through the input processor
		if (TestFrames == 1)
		{
			bSyntheticCursor = false;
			PressKeyThroughProcessor(EKeys::Escape);
			if (bKioskView)
			{
				TestDetail = TEXT("the input processor did not use Esc in the locked view");
				return -1;
			}
			return 0;
		}
		if (!bKioskView && Visitor->IsViewFree() && Visitor->IsWalkingEnabled() && !Controller->bShowMouseCursor)
		{
			TestDetail = TEXT("camera back at the eye, walking enabled, cursor hidden");
			Visitor->ResetPointerAim();
			Controller->SetControlRotation(FRotator(0.f, 180.f, 0.f)); // face the kiosk wall straight on
			Visitor->SetInjectedMove(1.f, 0.f);
			return 1;
		}
		if (TestTime > 3.f)
		{
			TestDetail = FString::Printf(TEXT("kioskView=%d free=%d walking=%d cursor=%d"), bKioskView, Visitor->IsViewFree(), Visitor->IsWalkingEnabled(), Controller->bShowMouseCursor);
			return -1;
		}
		return 0;

	case ETestAct::Collide: // keep walking forward into the kiosk: the capsule must stop at the cabinet or a part
		if (TestTime >= 2.5f)
		{
			Visitor->SetInjectedMove(0.f, 0.f);
			const float X = Visitor->GetActorLocation().X;
			constexpr float CapsuleRadius = 34.f;
			TestDetail = FString::Printf(TEXT("stopped %.0f cm from the cabinet face (capsule radius %.0f cm)"), X, CapsuleRadius);
			return (X >= CapsuleRadius - 1.f && X < CapsuleRadius + 80.f) ? 1 : -1;
		}
		return 0;

	case ETestAct::Click: // click a button on the 3D screen by pointing the cursor at it
		switch (TestPhase)
		{
		case 0:
			if (TestTime < 0.35f)
			{
				return 0;
			}
			if (!FindButtonPixel(Act.Name, TestTarget))
			{
				if (TestTime > 4.f)
				{
					TestDetail = FString::Printf(TEXT("no button named '%s' on screen %d"), *Act.Name, static_cast<int32>(Session->Current()));
					return -1;
				}
				return 0;
			}
			SetSyntheticCursor(Env->CanvasToWorld(TestTarget, SignU, SignV));
			SerialBefore = Session->ChangeSerial();
			TestPhase = 1;
			TestFrames = 0;
			return 0;
		case 1:
		{
			if (TestFrames < 6)
			{
				return 0;
			}
			FVector2D Hit;
			if (Visitor->GetPointerHit(Hit) && FVector2D::Distance(Hit, TestTarget) < 8.0)
			{
				bSignsKnown = true;
				QueueSyntheticClick();
				TestPhase = 2;
				TestFrames = 0;
				return 0;
			}
			if (!bSignsKnown && ++TestTrial < 4)
			{
				SignU = (TestTrial & 1) ? -1.f : 1.f;
				SignV = (TestTrial & 2) ? -1.f : 1.f;
				SetSyntheticCursor(Env->CanvasToWorld(TestTarget, SignU, SignV));
				TestFrames = 0;
				return 0;
			}
			TestDetail = FString::Printf(TEXT("the cursor never reached '%s' at px %.0f,%.0f (cursor part %d)"), *Act.Name, TestTarget.X, TestTarget.Y, static_cast<int32>(CursorPart));
			return -1;
		}
		default:
			if (TestFrames >= 8 && Session->ChangeSerial() != SerialBefore)
			{
				TestDetail = FString::Printf(TEXT("clicked '%s', now screen %d"), *Act.Name, static_cast<int32>(Session->Current()));
				return 1;
			}
			if (TestFrames > 120)
			{
				TestDetail = FString::Printf(TEXT("clicking '%s' changed nothing"), *Act.Name);
				return -1;
			}
			return 0;
		}

	case ETestAct::Part:
	{
		const int32 Result = StepClickPart(Act.PartArg);
		if (Result == 1)
		{
			TestDetail = FString::Printf(TEXT("clicked part %d with item %d: accepted=%d, message '%s'"), static_cast<int32>(Act.PartArg), SelectedItem, bLastUseAccepted, *ToastText.ToString());
		}
		return Result;
	}

	case ETestAct::Select:
		if (TestFrames == 1)
		{
			if (Act.A >= 0)
			{
				PressKeyThroughProcessor(NumberKey(Act.A));
			}
			else
			{
				SelectWalletItem(INDEX_NONE);
			}
			return 0;
		}
		TestDetail = FString::Printf(TEXT("wallet item %d chosen"), SelectedItem);
		return SelectedItem == (Act.A >= 0 ? Act.A : INDEX_NONE) ? 1 : -1;

	case ETestAct::ExpectScreen:
		if (static_cast<int32>(Session->Current()) == Act.A)
		{
			TestDetail = FString::Printf(TEXT("screen %d"), Act.A);
			return 1;
		}
		if (TestTime > 8.f)
		{
			TestDetail = FString::Printf(TEXT("expected screen %d, still on %d"), Act.A, static_cast<int32>(Session->Current()));
			return -1;
		}
		return 0;

	case ETestAct::ExpectUse:
		TestDetail = FString::Printf(TEXT("accepted=%d, message '%s'"), bLastUseAccepted, *ToastText.ToString());
		return bLastUseAccepted == (Act.A != 0) ? 1 : -1;

	case ETestAct::CashUntilPaid:
		if (Session->Current() != EKioskScreen::WaitCash)
		{
			TestDetail = FString::Printf(TEXT("paid after %d note clicks, cash inserted %d c, change %d c, now screen %d"), CashClicks, Session->State.CashInsertedCents, Session->ChangeCents(), static_cast<int32>(Session->Current()));
			const bool bOk = Session->State.CashInsertedCents > 0 && bLastUseAccepted;
			CashClicks = 0;
			return bOk ? 1 : -1;
		}
		if (CashClicks >= 8)
		{
			TestDetail = TEXT("still waiting for cash after 8 notes");
			return -1;
		}
		{
			const int32 Result = StepClickPart(EKioskPart::NoteSlot);
			if (Result == 1)
			{
				++CashClicks;
			}
			return Result < 0 ? -1 : 0;
		}

	case ETestAct::ExpectTray:
		if (Act.A != 0 ? (bTrayTicket && bTrayReceipt) : !TrayHasContents())
		{
			TestDetail = FString::Printf(TEXT("ticket=%d receipt=%d change=%d c"), bTrayTicket, bTrayReceipt, TrayChangeCents);
			return 1;
		}
		if (TestTime > 2.f)
		{
			TestDetail = FString::Printf(TEXT("tray: ticket=%d receipt=%d change=%d c"), bTrayTicket, bTrayReceipt, TrayChangeCents);
			return -1;
		}
		return 0;

	case ETestAct::Shot:
		if (ShotDir.IsEmpty())
		{
			TestDetail = TEXT("skipped (no -Kiosk3DShotDir)");
			return 1;
		}
		if (TestPhase == 0)
		{
			if (TestTime < 1.5f)
			{
				return 0; // let the screen, the widget render target and the temporal AA history settle after the last change
			}
			RequestShot(FPaths::Combine(ShotDir, Act.Name + TEXT(".png")));
			TestPhase = 1;
			TestFrames = 0;
			return 0;
		}
		TestDetail = Act.Name;
		return TestFrames >= 20 ? 1 : 0;
	}
	return -1;
}

bool FKioskWorld::TickSelfTest(float DeltaSeconds)
{
	AKioskVisitorPawn* Visitor = Pawn.Get();
	AKioskEnvironment* Env = Environment.Get();
	UWorld* PlayWorld = World.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	if (!Visitor || !Env || !Controller)
	{
		SelfTestFail(TEXT("spawn"), TEXT("environment, pawn or controller missing"));
		return false;
	}
	TestTime += DeltaSeconds;
	TestTotal += DeltaSeconds;
	++TestFrames;
	if (TestTotal > 150.f)
	{
		SelfTestFail(TEXT("timeout"), FString::Printf(TEXT("stuck at step %d"), TestStep));
		return false;
	}
	if (!TestPlan.IsValidIndex(TestStep))
	{
		UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST PASS"));
		bSelfTest = false;
		FPlatformMisc::RequestExit(false);
		return false;
	}

	const FTestAct& Act = TestPlan[TestStep];
	static const TCHAR* const ActNames[] = { TEXT("spawn"), TEXT("walk up"), TEXT("E lock-on"), TEXT("Esc step back"), TEXT("collision"), TEXT("click on the 3D screen"),
		TEXT("click kiosk part"), TEXT("wallet item"), TEXT("expect screen"), TEXT("expect part result"), TEXT("notes until paid"), TEXT("expect tray"), TEXT("screenshot") };
	const FString Label = FString::Printf(TEXT("%s %s"), ActNames[static_cast<int32>(Act.Type)], *Act.Name);
	const int32 Result = StepSelfTest();
	if (Result > 0)
	{
		SelfTestOk(Label, TestDetail);
	}
	else if (Result < 0)
	{
		SelfTestFail(Label, TestDetail);
		return false;
	}
	return true;
}
