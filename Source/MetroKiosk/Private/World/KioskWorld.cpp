// Runs the optional -Kiosk3D mode: spawns the concourse, switches between walking and the full-screen kiosk view, shows the hints.
#include "KioskWorld.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KioskEnvironment.h"
#include "KioskLoc.h"
#include "KioskStyle.h"
#include "KioskSession.h"
#include "KioskVisitorPawn.h"
#include "Widgets/Components/KioskAutomation.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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

	/** Sees E and Esc before the viewport does, so they work in both walking (game input) and kiosk (UI-only) modes. */
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
	return FParse::Param(FCommandLine::Get(), TEXT("Kiosk3D"));
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
	if (FParse::Value(FCommandLine::Get(), TEXT("-Kiosk3DShotView="), ShotView) && ShotView.Equals(TEXT("wide"), ESearchCase::IgnoreCase) && Pawn.IsValid() && Controller)
	{
		// Overview for the screenshot: back and to the side, raised, wide lens, so one frame shows the kiosk, floor, pillar, benches, lights and platform edge.
		Pawn->SetActorLocation(FVector(620.f, -720.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
		Controller->SetControlRotation(FRotator(-14.f, 96.f, 0.f));
		Pawn->SetFieldOfView(105.f);
	}
	bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("Kiosk3DSelfTest"));

	Hud = BuildHud();
	GameViewport->AddViewportWidgetContent(Hud.ToSharedRef(), 200);

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
		if (bKioskView && Root.IsValid())
		{
			GameViewport->RemoveViewportWidgetContent(Root.ToSharedRef());
		}
		if (Hud.IsValid())
		{
			GameViewport->RemoveViewportWidgetContent(Hud.ToSharedRef());
		}
	}
	Hud.Reset();
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
	UGameViewportClient* GameViewport = Viewport.Get();
	APlayerController* Controller = PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
	if (bKioskView || !GameViewport || !Controller || !Root.IsValid())
	{
		return;
	}
	// A Slate widget has one parent: take it off the 3D screen before putting it in the viewport.
	if (AKioskEnvironment* Env = Environment.Get())
	{
		Env->ClearScreenContent();
	}
	GameViewport->AddViewportWidgetContent(Root.ToSharedRef(), 100);
	bKioskView = true;
	if (AKioskVisitorPawn* Visitor = Pawn.Get())
	{
		Visitor->SetWalkingEnabled(false);
	}
	Controller->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Root);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);
}

void FKioskWorld::ExitKioskView()
{
	UGameViewportClient* GameViewport = Viewport.Get();
	if (!bKioskView || !GameViewport || !Root.IsValid())
	{
		return;
	}
	GameViewport->RemoveViewportWidgetContent(Root.ToSharedRef());
	bKioskView = false;
	if (AKioskEnvironment* Env = Environment.Get())
	{
		Env->SetScreenContent(Root.ToSharedRef());
	}
	if (AKioskVisitorPawn* Visitor = Pawn.Get())
	{
		Visitor->SetWalkingEnabled(true);
	}
	SetWalkingInput();
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
	if (bKioskView)
	{
		if (Key == EKeys::Escape)
		{
			ExitKioskView();
			return true;
		}
		return false; // every other key goes to the kiosk (search box typing, etc.)
	}
	if (Key == EKeys::E && IsNearKiosk())
	{
		EnterKioskView();
		return true;
	}
	return false;
}

bool FKioskWorld::Tick(float DeltaSeconds)
{
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
		// Hint pill at the bottom.
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
		// In the full-screen kiosk view the hint sits in the empty middle of the header, clear of the screen content.
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

// --- self-test -------------------------------------------------------------------------------

void FKioskWorld::SelfTestOk(const FString& Step, const FString& Detail)
{
	UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST step %d %s: ok (%s)"), TestStep, *Step, *Detail);
	++TestStep;
	TestFrames = 0;
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
	if (TestTotal > 60.f)
	{
		SelfTestFail(TEXT("timeout"), FString::Printf(TEXT("stuck at step %d"), TestStep));
		return false;
	}

	switch (TestStep)
	{
	case 0: // spawn: wait for the kiosk screen to be drawn so the button has a position
		if (TestTime > 3.f)
		{
			if (!FindButtonPixel(TEXT("Buy a ticket"), TestTarget))
			{
				SelfTestFail(TEXT("spawn"), TEXT("the Buy a ticket button has no layout on the 3D screen"));
				return false;
			}
			SelfTestOk(TEXT("spawn"), FString::Printf(TEXT("pawn at %s, Buy a ticket at canvas px %.0f,%.0f"), *Visitor->GetActorLocation().ToCompactString(), TestTarget.X, TestTarget.Y));
			Visitor->SetInjectedMove(1.f, 0.f);
		}
		break;

	case 1: // walk until the "Press E" condition holds
		if (IsNearKiosk())
		{
			Visitor->SetInjectedMove(0.f, 0.f);
			SelfTestOk(TEXT("walk and proximity"), FString::Printf(TEXT("near the kiosk after %.1f s at %s"), TestTotal, *Visitor->GetActorLocation().ToCompactString()));
		}
		else if (TestTime > 5.f)
		{
			SelfTestFail(TEXT("walk and proximity"), TEXT("still not near the kiosk after 5 s of forward movement"));
			return false;
		}
		break;

	case 2: // aim the pointer at the button; find the screen axis mapping by trial
	{
		const float SignU = (TestTrial & 1) ? -1.f : 1.f;
		const float SignV = (TestTrial & 2) ? -1.f : 1.f;
		if (TestFrames == 1)
		{
			Visitor->AimPointerAt(Env->CanvasToWorld(TestTarget, SignU, SignV));
		}
		else if (TestFrames >= 6)
		{
			FVector2D Hit;
			if (Visitor->GetPointerHit(Hit) && FVector2D::Distance(Hit, TestTarget) < 6.0)
			{
				SelfTestOk(TEXT("aim at Buy a ticket"), FString::Printf(TEXT("pointer hit px %.0f,%.0f for target %.0f,%.0f"), Hit.X, Hit.Y, TestTarget.X, TestTarget.Y));
				ScreenBeforeClick = Session->Current();
			}
			else if (++TestTrial >= 4)
			{
				SelfTestFail(TEXT("aim at Buy a ticket"), FString::Printf(TEXT("pointer never reached px %.0f,%.0f"), TestTarget.X, TestTarget.Y));
				return false;
			}
			else
			{
				TestFrames = 0;
			}
		}
		break;
	}

	case 3: // click
		if (TestFrames == 1)
		{
			Visitor->InjectPointer(true);
		}
		else if (TestFrames == 4)
		{
			Visitor->InjectPointer(false);
		}
		else if (TestTime > 1.0f)
		{
			if (Session->Current() != ScreenBeforeClick)
			{
				SelfTestOk(TEXT("click on the 3D screen"), FString::Printf(TEXT("screen moved off Home (now screen id %d)"), static_cast<int32>(Session->Current())));
			}
			else
			{
				SelfTestFail(TEXT("click on the 3D screen"), TEXT("the screen is still Home after press and release"));
				return false;
			}
		}
		break;

	case 4: // E through the input processor
		if (TestFrames == 1)
		{
			const FKeyEvent Event(EKeys::E, FModifierKeysState(), 0, false, 0, 0);
			if (!InputProcessor.IsValid() || !InputProcessor->HandleKeyDownEvent(FSlateApplication::Get(), Event))
			{
				SelfTestFail(TEXT("E key"), TEXT("the input processor did not use E near the kiosk"));
				return false;
			}
		}
		else if (TestFrames >= 8)
		{
			UGameViewportClient* GameViewport = Viewport.Get();
			const TSharedPtr<SWindow> RootWindow = FSlateApplication::Get().FindWidgetWindow(Root.ToSharedRef());
			const bool bInViewportWindow = RootWindow.IsValid() && GameViewport && RootWindow == GameViewport->GetWindow();
			if (bKioskView && Controller->bShowMouseCursor && bInViewportWindow && !Env->ScreenContentIs(Root.ToSharedRef()))
			{
				SelfTestOk(TEXT("E key to full-screen kiosk"), TEXT("Root is in the game viewport window, UI-only input with cursor shown, 3D screen released it"));
			}
			else
			{
				SelfTestFail(TEXT("E key to full-screen kiosk"), FString::Printf(TEXT("kioskView=%d cursor=%d inViewportWindow=%d"), bKioskView, Controller->bShowMouseCursor, bInViewportWindow));
				return false;
			}
		}
		break;

	case 5: // Esc through the input processor
		if (TestFrames == 1)
		{
			const FKeyEvent Event(EKeys::Escape, FModifierKeysState(), 0, false, 27, 27);
			if (!InputProcessor.IsValid() || !InputProcessor->HandleKeyDownEvent(FSlateApplication::Get(), Event))
			{
				SelfTestFail(TEXT("Esc key"), TEXT("the input processor did not use Esc in the kiosk view"));
				return false;
			}
		}
		else if (TestFrames >= 8)
		{
			if (!bKioskView && Visitor->IsWalkingEnabled() && !Controller->bShowMouseCursor && Env->ScreenContentIs(Root.ToSharedRef()))
			{
				SelfTestOk(TEXT("Esc key back to walking"), TEXT("Root is back on the 3D screen, walking enabled, cursor hidden"));
				Visitor->ResetPointerAim();
				Controller->SetControlRotation(FRotator(0.f, 180.f, 0.f)); // face the kiosk wall straight on
				Visitor->SetInjectedMove(1.f, 0.f);
			}
			else
			{
				SelfTestFail(TEXT("Esc key back to walking"), FString::Printf(TEXT("kioskView=%d walking=%d cursor=%d onScreen=%d"), bKioskView, Visitor->IsWalkingEnabled(), Controller->bShowMouseCursor, Env->ScreenContentIs(Root.ToSharedRef())));
				return false;
			}
		}
		break;

	case 6: // keep walking forward into the kiosk for 2 s: the capsule must stop at the cabinet
		if (TestTime >= 2.f)
		{
			Visitor->SetInjectedMove(0.f, 0.f);
			const float X = Visitor->GetActorLocation().X;
			constexpr float CapsuleRadius = 34.f;
			if (X >= CapsuleRadius - 1.f && X < CapsuleRadius + 60.f)
			{
				SelfTestOk(TEXT("collision with the kiosk"), FString::Printf(TEXT("stopped %.0f cm from the cabinet face (capsule radius %.0f cm)"), X, CapsuleRadius));
			}
			else
			{
				SelfTestFail(TEXT("collision with the kiosk"), FString::Printf(TEXT("pawn X is %.0f cm; expected just over %.0f"), X, CapsuleRadius));
				return false;
			}
		}
		break;

	default:
		UE_LOG(LogKiosk3D, Display, TEXT("KIOSK3D SELFTEST PASS"));
		bSelfTest = false;
		FPlatformMisc::RequestExit(false);
		return false;
	}
	return true;
}
