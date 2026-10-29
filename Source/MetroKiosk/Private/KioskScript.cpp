#include "KioskScript.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Input/Events.h"
#include "Input/HittestGrid.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/Components/KioskAutomation.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogKioskScript, Log, All);

namespace
{
	constexpr int32 SettleFrames = 8;
	constexpr int32 ShotSettleFrames = 10;
	constexpr int32 MaxShotWaitFrames = 900;
	constexpr float CanvasW = 1917.f;
	constexpr float CanvasH = 1105.f;

	const TCHAR* ScreenName(EKioskScreen Screen)
	{
		static const TCHAR* const Names[] = { TEXT("Home"), TEXT("TicketType"), TEXT("Destination"), TEXT("DestinationMap"), TEXT("Review"),
			TEXT("PayMethod"), TEXT("WaitTapCard"), TEXT("WaitChipCard"), TEXT("WaitCash"), TEXT("Processing"), TEXT("Receipt"),
			TEXT("HelpCall"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"), TEXT("Concession"), TEXT("PassDates"), TEXT("DoneLiftCard") };
		return Names[FMath::Clamp(static_cast<int32>(Screen), 0, static_cast<int32>(UE_ARRAY_COUNT(Names)) - 1)];
	}

	FString SafeName(const FString& In)
	{
		FString Out;
		for (TCHAR Ch : In)
		{
			Out.AppendChar((Ch < 128 && FChar::IsAlnum(Ch)) ? Ch : TEXT('_'));
		}
		return Out;
	}
}

FKioskScriptRunner::~FKioskScriptRunner()
{
	Shutdown();
}

void FKioskScriptRunner::Shutdown()
{
	if (bInputOverrideActive && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetHandleDeviceInputWhenApplicationNotActive(bPrevHandleInactive);
		bInputOverrideActive = false;
	}
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Session.Reset();
	Root.Reset();
}

bool FKioskScriptRunner::Initialise(const TSharedRef<FKioskSession>& InSession, const TSharedRef<SWidget>& InRoot)
{
	FString Script;
	if (!FParse::Value(FCommandLine::Get(), TEXT("-KioskScript="), Script))
	{
		return false;
	}
	FParse::Value(FCommandLine::Get(), TEXT("-KioskScriptOut="), OutDir);
	OutDir = OutDir.TrimQuotes();
	if (OutDir.IsEmpty())
	{
		OutDir = FPaths::ProjectSavedDir() / TEXT("Journeys/Latest");
	}
	FPaths::NormalizeDirectoryName(OutDir);
	IFileManager::Get().MakeDirectory(*OutDir, true);
	IFileManager::Get().Delete(*(OutDir / TEXT("log.txt")), false, true, true);

	Session = InSession;
	Root = InRoot;
	bKeepGoing = FParse::Param(FCommandLine::Get(), TEXT("KioskScriptKeepGoing"));
	// Slate refuses pointer capture while the application is not the active OS window, which would silently drop
	// every click when the OS declines to focus a background-launched window. Script mode accepts input regardless.
	bPrevHandleInactive = FSlateApplication::Get().GetHandleDeviceInputWhenApplicationNotActive();
	FSlateApplication::Get().SetHandleDeviceInputWhenApplicationNotActive(true);
	bInputOverrideActive = true;
	ParseScript(Script.TrimQuotes());
	Log(FString::Printf(TEXT("SCRIPT %d steps, out=%s"), Steps.Num(), *OutDir));
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FKioskScriptRunner::Tick), 0.f);
	return true;
}

void FKioskScriptRunner::Log(const FString& Line)
{
	UE_LOG(LogKioskScript, Display, TEXT("%s"), *Line);
	FFileHelper::SaveStringToFile(Line + TEXT("\n"), *(OutDir / TEXT("log.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

void FKioskScriptRunner::ParseScript(const FString& Script)
{
	TArray<FString> Parts;
	Script.ParseIntoArray(Parts, TEXT("|"), true);
	for (FString Part : Parts)
	{
		Part.TrimStartAndEndInline();
		FString Verb, Arg;
		if (!Part.Split(TEXT(":"), &Verb, &Arg))
		{
			Verb = Part;
		}
		Verb = Verb.TrimStartAndEnd().ToLower();
		Arg.TrimStartAndEndInline();

		FStep Step;
		Step.Source = Part;
		if (Verb == TEXT("click"))
		{
			Step.Kind = EStepKind::Click;
			Step.Text = Arg;
			Steps.Add(Step);
		}
		else if (Verb == TEXT("lang"))
		{
			Step.Kind = EStepKind::Click;
			const FString Code = Arg.ToLower();
			Step.Text = TEXT("Language ") + FString(Code == TEXT("zh") ? TEXT("Chinese") : Code == TEXT("ms") ? TEXT("Malay") : Code == TEXT("ta") ? TEXT("Tamil") : TEXT("English"));
			Steps.Add(Step);
		}
		else if (Verb == TEXT("type"))
		{
			for (int32 Index = 0; Index < Arg.Len(); ++Index)
			{
				FStep Key;
				Key.Kind = EStepKind::Click;
				const TCHAR Ch = Arg[Index];
				Key.Text = FChar::IsDigit(Ch) ? FString::Printf(TEXT("Keypad %c"), Ch)
					: Ch == TEXT(' ') ? FString(TEXT("Key Space"))
					: FString::Printf(TEXT("Key %c"), FChar::ToUpper(Ch));
				Key.Source = FString::Printf(TEXT("type:%s [%c]"), *Arg, Ch);
				Key.bAutoShot = (Index == Arg.Len() - 1);
				Key.ShotName = TEXT("after_type_") + SafeName(Arg);
				Steps.Add(Key);
			}
		}
		else if (Verb == TEXT("drag"))
		{
			Step.Kind = EStepKind::Drag;
			FString Target, Delta;
			if (!Arg.Split(TEXT(":"), &Target, &Delta))
			{
				Target = Arg;
			}
			FString Dx, Dy;
			Delta.Split(TEXT(","), &Dx, &Dy);
			Step.Text = Target;
			Step.Delta = FVector2f(FCString::Atof(*Dx), FCString::Atof(*Dy));
			Steps.Add(Step);
		}
		else if (Verb == TEXT("scroll"))
		{
			Step.Kind = EStepKind::Scroll;
			FString Target, Dy;
			if (!Arg.Split(TEXT(":"), &Target, &Dy))
			{
				Target = Arg;
			}
			Step.Text = Target;
			Step.Delta = FVector2f(0.f, FCString::Atof(*Dy));
			Steps.Add(Step);
		}
		else if (Verb == TEXT("wait"))
		{
			Step.Kind = EStepKind::Wait;
			Step.Seconds = FCString::Atof(*Arg);
			Steps.Add(Step);
		}
		else if (Verb == TEXT("shot"))
		{
			Step.Kind = EStepKind::Shot;
			Step.Text = Arg.IsEmpty() ? FString(TEXT("shot")) : Arg;
			Steps.Add(Step);
		}
		else if (Verb == TEXT("dump"))
		{
			Step.Kind = EStepKind::Dump;
			Steps.Add(Step);
		}
		else
		{
			FStep Bad;
			Bad.Kind = EStepKind::Wait;
			Bad.Source = Part;
			Bad.Text = TEXT("UNKNOWN");
			Steps.Add(Bad);
		}
	}
}

// --- widget discovery --------------------------------------------------------------------

void FKioskScriptRunner::CollectRecursive(const TSharedRef<SWidget>& Widget, TArray<FEntry>& Out) const
{
	if (!Widget->GetVisibility().IsVisible())
	{
		return;
	}
	if (TSharedPtr<FKioskAutomationMeta> Meta = Widget->GetMetaData<FKioskAutomationMeta>())
	{
		FEntry Entry;
		Entry.Widget = Widget;
		Entry.Name = Meta->Name;
		Entry.Label = Meta->Label;
		Entry.bEnabled = Meta->Enabled.Get(true);
		Out.Add(Entry);
	}
	if (FChildren* Children = Widget->GetChildren())
	{
		Children->ForEachWidget([this, &Out](SWidget& Child)
		{
			CollectRecursive(Child.AsShared(), Out);
		});
	}
}

void FKioskScriptRunner::CollectEntries(TArray<FEntry>& Out) const
{
	if (Root.IsValid())
	{
		CollectRecursive(Root.ToSharedRef(), Out);
	}
}

float FKioskScriptRunner::DesignScale(FVector2f& OutOrigin) const
{
	const FGeometry Geo = Root->GetTickSpaceGeometry();
	const FVector2f Local = Geo.GetLocalSize();
	const float Scale = FMath::Max(0.0001f, FMath::Min(Local.X / CanvasW, Local.Y / CanvasH));
	OutOrigin = (Local - FVector2f(CanvasW, CanvasH) * Scale) * 0.5f;
	return Scale;
}

bool FKioskScriptRunner::IsHittable(const FEntry& Entry, FVector2D& OutScreenPos, FString& OutWhy) const
{
	const FGeometry Geo = Entry.Widget->GetTickSpaceGeometry();
	if (Geo.GetLocalSize().X < 1.f || Geo.GetLocalSize().Y < 1.f)
	{
		OutWhy = TEXT("not laid out (scrolled out of view? try scroll:<list>:<dy>)");
		return false;
	}
	OutScreenPos = FVector2D(Geo.GetAbsolutePosition() + Geo.GetAbsoluteSize() * 0.5f);

	TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(Root.ToSharedRef());
	if (!Window.IsValid())
	{
		OutWhy = TEXT("no window");
		return false;
	}
	if (!Window->GetClientRectInScreen().ContainsPoint(FVector2f(OutScreenPos)))
	{
		OutWhy = TEXT("outside the window");
		return false;
	}
	const TArray<FWidgetAndPointer> Bubble = Window->GetHittestGrid().GetBubblePath(OutScreenPos, 0.f, false);
	bool bFound = false;
	for (const FWidgetAndPointer& Hit : Bubble)
	{
		if (Hit.Widget == Entry.Widget)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		const FString Covered = Bubble.Num() > 0 ? Bubble[0].Widget->ToString() : FString(TEXT("nothing"));
		OutWhy = FString::Printf(TEXT("covered by another widget (%s)"), *Covered.Left(60));
		return false;
	}
	return true;
}

bool FKioskScriptRunner::FindEntry(const FString& Label, FEntry& OutEntry, FString& OutWhy) const
{
	TArray<FEntry> Entries;
	CollectEntries(Entries);

	// Exact matches win. If an exact match exists but cannot be pressed, fail with its reason instead of
	// falling through to a looser "contains" match (a covered "Continue" must never click "Continue - check dates").
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		FString FirstReason;
		for (const FEntry& Entry : Entries)
		{
			const bool bMatch = (Pass == 0)
				? (Entry.Name.Equals(Label, ESearchCase::IgnoreCase) || Entry.Label.Equals(Label, ESearchCase::IgnoreCase))
				: (Entry.Name.Contains(Label, ESearchCase::IgnoreCase) || Entry.Label.Contains(Label, ESearchCase::IgnoreCase));
			if (!bMatch)
			{
				continue;
			}
			FVector2D Pos;
			FString Why;
			if (!Entry.bEnabled)
			{
				if (FirstReason.IsEmpty())
				{
					FirstReason = FString::Printf(TEXT("'%s' is disabled"), *Entry.Name);
				}
				continue;
			}
			if (IsHittable(Entry, Pos, Why))
			{
				OutEntry = Entry;
				return true;
			}
			if (FirstReason.IsEmpty())
			{
				FirstReason = FString::Printf(TEXT("'%s' not hittable: %s"), *Entry.Name, *Why);
			}
		}
		if (!FirstReason.IsEmpty())
		{
			OutWhy = FirstReason;
			return false;
		}
	}
	OutWhy = FString::Printf(TEXT("no visible widget matches '%s'"), *Label);
	return false;
}

// --- input -------------------------------------------------------------------------------

bool FKioskScriptRunner::DoClick(const FString& Label, FString& OutWhy)
{
	FEntry Entry;
	if (!FindEntry(Label, Entry, OutWhy))
	{
		return false;
	}
	FVector2D Pos;
	FString Why;
	IsHittable(Entry, Pos, Why);

	FSlateApplication& App = FSlateApplication::Get();
	// The kiosk is touch-only, so deliver a real touch press/release at the widget centre. Slate hit-tests it exactly
	// as for a user (overlap, disabled and clipped widgets behave identically) and no OS cursor is moved.
	App.ProcessTouchStartedEvent(nullptr, FPointerEvent(0, 0, Pos, Pos, 1.f, true));
	App.ProcessTouchEndedEvent(FPointerEvent(0, 0, Pos, Pos, 0.f, false));
	return true;
}

bool FKioskScriptRunner::DoDrag(const FString& Label, const FVector2f& DesignDelta, FString& OutWhy)
{
	FEntry Entry;
	if (!FindEntry(Label, Entry, OutWhy))
	{
		return false;
	}
	FVector2D Start;
	FString Why;
	IsHittable(Entry, Start, Why);

	FVector2f Origin;
	const float DesignToLocal = DesignScale(Origin);
	const float LocalToScreen = Root->GetTickSpaceGeometry().GetAccumulatedLayoutTransform().GetScale();
	const FVector2D Delta = FVector2D(DesignDelta * DesignToLocal * LocalToScreen);

	FSlateApplication& App = FSlateApplication::Get();
	App.ProcessTouchStartedEvent(nullptr, FPointerEvent(0, 0, Start, Start, 1.f, true));
	FVector2D Last = Start;
	const int32 Moves = 8;
	for (int32 Index = 1; Index <= Moves; ++Index)
	{
		const FVector2D Now = Start + Delta * (static_cast<double>(Index) / Moves);
		App.ProcessTouchMovedEvent(FPointerEvent(0, 0, Now, Last, 1.f, true));
		Last = Now;
	}
	App.ProcessTouchEndedEvent(FPointerEvent(0, 0, Last, Last, 0.f, false));
	return true;
}

bool FKioskScriptRunner::DoScroll(const FString& Label, float DesignDy, FString& OutWhy)
{
	FEntry Entry;
	if (!FindEntry(Label, Entry, OutWhy))
	{
		return false;
	}
	FVector2D Pos;
	FString Why;
	IsHittable(Entry, Pos, Why);

	// Mouse-wheel notches at the centre of the target; positive dy scrolls the content down (towards later items).
	const int32 Notches = FMath::Max(1, FMath::CeilToInt(FMath::Abs(DesignDy) / 60.f));
	const float Wheel = DesignDy > 0.f ? -1.f : 1.f;
	for (int32 Index = 0; Index < Notches; ++Index)
	{
		const FPointerEvent WheelEvent(0, Pos, Pos, TSet<FKey>(), EKeys::MouseScrollDown, Wheel, FModifierKeysState());
		FSlateApplication::Get().ProcessMouseWheelOrGestureEvent(WheelEvent, nullptr);
	}
	return true;
}

void FKioskScriptRunner::DoDump()
{
	TArray<FEntry> Entries;
	CollectEntries(Entries);
	FVector2f Origin;
	const float DesignToLocal = DesignScale(Origin);
	const FGeometry RootGeo = Root->GetTickSpaceGeometry();
	const float LocalToScreen = RootGeo.GetAccumulatedLayoutTransform().GetScale();

	FString Text = FString::Printf(TEXT("screen: %s%s\ninteractive widgets (design px, canvas 1917x1105):\n"),
		ScreenName(Session->Visible()), Session->IsHelpFullscreen() ? TEXT(" (help call)") : TEXT(""));
	for (const FEntry& Entry : Entries)
	{
		const FGeometry Geo = Entry.Widget->GetTickSpaceGeometry();
		const FVector2f Local = RootGeo.AbsoluteToLocal(Geo.GetAbsolutePosition());
		const FVector2f Design = (Local - Origin) / DesignToLocal;
		const FVector2f Size = Geo.GetAbsoluteSize() / (LocalToScreen * DesignToLocal);
		FVector2D Pos;
		FString Why;
		const bool bHit = IsHittable(Entry, Pos, Why);
		Text += FString::Printf(TEXT("%-9s %-36s | %-30s | x=%d y=%d w=%d h=%d%s\n"),
			Entry.bEnabled ? TEXT("enabled") : TEXT("DISABLED"), *Entry.Name, *Entry.Label,
			FMath::RoundToInt(Design.X), FMath::RoundToInt(Design.Y), FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y),
			bHit ? TEXT("") : TEXT("  (not hittable)"));
	}
	++FileIndex;
	FFileHelper::SaveStringToFile(Text, *(OutDir / FString::Printf(TEXT("%02d_tree.txt"), FileIndex)), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

// --- sequencing --------------------------------------------------------------------------

void FKioskScriptRunner::QueueShot(const FString& Name)
{
	PendingShotName = Name;
	FrameCounter = 0;
	Phase = EPhase::SettleBeforeShot;
}

void FKioskScriptRunner::RunStep(const FStep& Step)
{
	FString Why;
	bool bOk = true;
	switch (Step.Kind)
	{
	case EStepKind::Click:
		SerialBeforeClick = Session->ChangeSerial();
		bOk = DoClick(Step.Text, Why);
		if (bOk)
		{
			ActiveClick = Step;
			FrameCounter = 0;
			Phase = EPhase::VerifyClick;
			return;
		}
		break;
	case EStepKind::Drag:
		bOk = DoDrag(Step.Text, Step.Delta, Why);
		if (bOk)
		{
			Log(FString::Printf(TEXT("STEP %d OK: %s"), StepIndex, *Step.Source));
			QueueShot(TEXT("after_drag_") + SafeName(Step.Text));
			return;
		}
		break;
	case EStepKind::Scroll:
		bOk = DoScroll(Step.Text, Step.Delta.Y, Why);
		break;
	case EStepKind::Wait:
		if (Step.Text == TEXT("UNKNOWN"))
		{
			bOk = false;
			Why = TEXT("unknown step");
		}
		else
		{
			WaitRemaining = Step.Seconds;
			Phase = EPhase::Waiting;
			Log(FString::Printf(TEXT("STEP %d OK: %s"), StepIndex, *Step.Source));
			return;
		}
		break;
	case EStepKind::Shot:
		Log(FString::Printf(TEXT("STEP %d OK: %s"), StepIndex, *Step.Source));
		QueueShot(SafeName(Step.Text));
		return;
	case EStepKind::Dump:
		DoDump();
		break;
	}

	if (bOk)
	{
		Log(FString::Printf(TEXT("STEP %d OK: %s"), StepIndex, *Step.Source));
	}
	else
	{
		Log(FString::Printf(TEXT("STEP %d FAIL: %s - %s"), StepIndex, *Step.Source, *Why));
		FailStep(Why);
		return;
	}
	FrameCounter = 0;
	Phase = EPhase::Settle;
}

void FKioskScriptRunner::FailStep(const FString& Why)
{
	++FailCount;
	if (bKeepGoing)
	{
		FrameCounter = 0;
		Phase = EPhase::Settle;
		return;
	}
	const int32 Skipped = Steps.Num() - NextStep;
	Log(FString::Printf(TEXT("ABORT: step %d failed (%s); skipping the remaining %d step(s). Pass -KioskScriptKeepGoing to continue after failures."), StepIndex, *Why, Skipped));
	bFinishAfterShot = true;
	QueueShot(TEXT("failure"));
}

void FKioskScriptRunner::Finish()
{
	Log(FString::Printf(TEXT("DONE: %d steps, %d failed (window was %s)"), Steps.Num(), FailCount, FSlateApplication::Get().IsActive() ? TEXT("active") : TEXT("NOT active")));
	Phase = EPhase::Done;
	FPlatformMisc::RequestExit(false);
}

bool FKioskScriptRunner::Tick(float DeltaSeconds)
{
	if (!Session.IsValid() || !Root.IsValid())
	{
		return false;
	}
	++FrameCounter;

	switch (Phase)
	{
	case EPhase::Starting:
	{
		// Wait for the window to reach the 1917x1105 design size and for the layout to stay unchanged for a while.
		bool bSized = true;
		if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
		{
			const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
			bSized = (Size.X == 1917 && Size.Y == 1105) || ResizeAttempts > 900;
			if (!bSized && FrameCounter > 20 && ResizeAttempts++ % 30 == 0)
			{
				GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("r.SetRes 1917x1105w"));
			}
		}
		const FVector2f RootSize = Root->GetTickSpaceGeometry().GetAbsoluteSize();
		StableFrames = (bSized && RootSize.X > 100.f && RootSize.Equals(LastRootSize, 0.5f)) ? StableFrames + 1 : 0;
		LastRootSize = RootSize;
		StartupSeconds += DeltaSeconds;
		if (FrameCounter > 90 && StableFrames > 45 && StartupSeconds > 4.f)
		{
			FrameCounter = 0;
			Phase = EPhase::Next;
		}
		break;
	}

	case EPhase::Next:
		if (NextStep >= Steps.Num())
		{
			Finish();
			return false;
		}
		StepIndex = NextStep + 1;
		RunStep(Steps[NextStep++]);
		break;

	case EPhase::VerifyClick:
		if (FrameCounter >= SettleFrames)
		{
			const bool bActive = FSlateApplication::Get().IsActive();
			if (Session->ChangeSerial() == SerialBeforeClick)
			{
				const FString Why = FString::Printf(TEXT("click delivered but nothing changed (application active=%d)"), bActive ? 1 : 0);
				Log(FString::Printf(TEXT("STEP %d FAIL: %s - %s"), StepIndex, *ActiveClick.Source, *Why));
				FailStep(Why);
				break;
			}
			Log(FString::Printf(TEXT("STEP %d OK: %s"), StepIndex, *ActiveClick.Source));
			if (ActiveClick.bAutoShot)
			{
				QueueShot(ActiveClick.ShotName.IsEmpty() ? (TEXT("after_click_") + SafeName(ActiveClick.Text)) : ActiveClick.ShotName);
			}
			else
			{
				FrameCounter = 0;
				Phase = EPhase::Settle;
			}
		}
		break;

	case EPhase::Waiting:
		WaitRemaining -= DeltaSeconds;
		if (WaitRemaining <= 0.f)
		{
			Phase = EPhase::Next;
		}
		break;

	case EPhase::Settle:
		if (FrameCounter >= SettleFrames)
		{
			Phase = EPhase::Next;
		}
		break;

	case EPhase::SettleBeforeShot:
		if (FrameCounter >= ShotSettleFrames)
		{
			++FileIndex;
			PendingShotPath = OutDir / FString::Printf(TEXT("%02d_%s.png"), FileIndex, *PendingShotName);
			IFileManager::Get().Delete(*PendingShotPath, false, true, true);
			FScreenshotRequest::RequestScreenshot(PendingShotPath, true, false);
			FrameCounter = 0;
			Phase = EPhase::WaitingForShot;
		}
		break;

	case EPhase::WaitingForShot:
		if (!FScreenshotRequest::IsScreenshotRequested() && IFileManager::Get().FileExists(*PendingShotPath))
		{
			if (bFinishAfterShot)
			{
				Finish();
				return false;
			}
			FrameCounter = 0;
			Phase = EPhase::Settle;
		}
		else if (FrameCounter > MaxShotWaitFrames)
		{
			Log(FString::Printf(TEXT("WARN: screenshot timed out: %s"), *PendingShotPath));
			FrameCounter = 0;
			Phase = EPhase::Settle;
		}
		break;

	default:
		return false;
	}
	return true;
}
