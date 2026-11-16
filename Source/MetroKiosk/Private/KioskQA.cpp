#include "KioskQA.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "KioskScript.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogKioskQA, Log, All);

namespace
{
	constexpr int32 SettleFrames = 12;   // frames to let the UI rebuild and render before capturing
	constexpr int32 CooldownFrames = 6;  // frames after the PNG is written
	constexpr int32 MaxWaitFrames = 900;
}

FKioskQARunner::~FKioskQARunner()
{
	Shutdown();
}

void FKioskQARunner::Initialise(const TSharedRef<FKioskSession>& InSession, const TSharedRef<SWidget>& InRoot)
{
	Session = InSession;
	Script = MakeShared<FKioskScriptRunner>();
	if (Script->Initialise(InSession, InRoot))
	{
		return; // scripted run: timers stay live, nothing else to set up
	}
	Script.Reset();
	const TCHAR* CommandLine = FCommandLine::Get();

	FString ScreenName;
	if (FParse::Value(CommandLine, TEXT("-KioskScreen="), ScreenName))
	{
		if (!Session->ApplySample(ScreenName))
		{
			UE_LOG(LogKioskQA, Warning, TEXT("Unknown -KioskScreen name '%s'"), *ScreenName);
		}
	}

	if (FParse::Value(CommandLine, TEXT("-KioskCaptureAll="), CaptureDir))
	{
		CaptureDir = CaptureDir.TrimQuotes();
		FPaths::NormalizeDirectoryName(CaptureDir);
		IFileManager::Get().MakeDirectory(*CaptureDir, true);
		Session->bFreezeTimers = true;

		Steps = {
			TEXT("Home"), TEXT("TicketType"), TEXT("TicketTypeLate"), TEXT("Destination"), TEXT("DestinationSearch"), TEXT("DestinationTypo"), TEXT("DestinationNoMatch"),
			TEXT("DestinationMap"), TEXT("DestinationSelected"), TEXT("Review"), TEXT("ReviewMixed"), TEXT("ReviewDayPass"), TEXT("PayMethod"), TEXT("PayMethodTopUp"),
			TEXT("WaitTapCard"), TEXT("WaitChipCard"), TEXT("WaitCash"), TEXT("WaitCashPartial"), TEXT("Processing"), TEXT("ProcessingCash"), TEXT("Receipt"),
			TEXT("HelpCall"), TEXT("HelpCallCash"), TEXT("HelpPip"), TEXT("IdlePrompt"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"),
			TEXT("TopUpAmountSelected"), TEXT("TopUpKeypad"), TEXT("Concession"), TEXT("PassDates"), TEXT("DoneLiftCard"), TEXT("DoneLiftCardConcession"),
			TEXT("DestinationFiltered"), TEXT("DestinationSearchSelected"),
			// Adult Metro card (T-014): appended last so the numbers above stay put.
			TEXT("AdultCardOptions"), TEXT("AdultTopUpAmountSelected"), TEXT("AdultConcession"), TEXT("AdultPassDates"), TEXT("AdultDoneLiftCard"), TEXT("AdultDoneLiftCardConcession"),
			// Adult card, start day picked (T-011): today + 3 days.
			TEXT("AdultPassDatesPicked") };
		// The same screens in the other three languages (the ones where text is longest or hardware is pointed at).
		static const TCHAR* const Localised[] = {
			TEXT("Home"), TEXT("TicketType"), TEXT("TicketTypeLate"), TEXT("DestinationSelected"), TEXT("DestinationMap"), TEXT("Review"), TEXT("ReviewMixed"), TEXT("ReviewDayPass"),
			TEXT("PayMethod"), TEXT("WaitTapCard"), TEXT("WaitChipCard"), TEXT("WaitCash"), TEXT("WaitCashPartial"), TEXT("Processing"), TEXT("Receipt"), TEXT("HelpCall"),
			TEXT("HelpCallCash"), TEXT("IdlePrompt"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"), TEXT("TopUpKeypad"), TEXT("Concession"), TEXT("PassDates"),
			TEXT("DoneLiftCard"), TEXT("DoneLiftCardConcession") };
		for (const TCHAR* Suffix : { TEXT("_Chinese"), TEXT("_Tamil"), TEXT("_Malay") })
		{
			for (const TCHAR* Name : Localised)
			{
				Steps.Add(FString(Name) + Suffix);
			}
		}

		// Appended last so earlier capture numbers do not move: map with search text lit, and map with a route selected.
		for (const TCHAR* Suffix : { TEXT(""), TEXT("_Chinese"), TEXT("_Tamil"), TEXT("_Malay") })
		{
			Steps.Add(FString(TEXT("DestinationMapSearch")) + Suffix);
			Steps.Add(FString(TEXT("DestinationMapSelected")) + Suffix);
		}

		StepIndex = 0;
		FrameCounter = 0;
		Phase = EPhase::Settling;
		Session->ApplySample(Steps[0]);
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FKioskQARunner::Tick), 0.f);
		UE_LOG(LogKioskQA, Display, TEXT("Capturing %d screens to %s"), Steps.Num(), *CaptureDir);
	}
}

void FKioskQARunner::Shutdown()
{
	Script.Reset();
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Phase = EPhase::Idle;
	Session.Reset();
}

bool FKioskQARunner::EnsureCaptureResolution()
{
	// Captures are always the 1917x1105 design size regardless of how the OS sized the window.
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		return true;
	}
	const FIntPoint Size = GEngine->GameViewport->Viewport->GetSizeXY();
	if (Size.X == 1917 && Size.Y == 1105)
	{
		return true;
	}
	if (++ResizeAttempts > 60)
	{
		UE_LOG(LogKioskQA, Warning, TEXT("Could not resize viewport to 1917x1105 (now %dx%d); capturing anyway"), Size.X, Size.Y);
		return true;
	}
	if (ResizeAttempts % 10 == 1)
	{
		GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("r.SetRes 1917x1105w"));
	}
	return false;
}

FString FKioskQARunner::CurrentFileName() const
{
	return CaptureDir / FString::Printf(TEXT("%02d_%s.png"), StepIndex + 1, *Steps[StepIndex]);
}

bool FKioskQARunner::Tick(float DeltaSeconds)
{
	if (!Session.IsValid() || Steps.IsEmpty())
	{
		return false;
	}
	++FrameCounter;

	switch (Phase)
	{
	case EPhase::Settling:
		if (FrameCounter >= SettleFrames && !EnsureCaptureResolution())
		{
			if (FrameCounter % 20 == 0)
			{
				FrameCounter = SettleFrames;
			}
		}
		else if (FrameCounter >= SettleFrames)
		{
			const FString File = CurrentFileName();
			IFileManager::Get().Delete(*File, false, true, true);
			FScreenshotRequest::RequestScreenshot(File, true, false);
			Phase = EPhase::WaitingForCapture;
			FrameCounter = 0;
		}
		break;

	case EPhase::WaitingForCapture:
		if (!FScreenshotRequest::IsScreenshotRequested() && IFileManager::Get().FileExists(*CurrentFileName()))
		{
			Phase = EPhase::Cooldown;
			FrameCounter = 0;
		}
		else if (FrameCounter > MaxWaitFrames)
		{
			UE_LOG(LogKioskQA, Warning, TEXT("Timed out waiting for %s"), *CurrentFileName());
			Phase = EPhase::Cooldown;
			FrameCounter = 0;
		}
		break;

	case EPhase::Cooldown:
		if (FrameCounter >= CooldownFrames)
		{
			++StepIndex;
			FrameCounter = 0;
			if (StepIndex >= Steps.Num())
			{
				UE_LOG(LogKioskQA, Display, TEXT("Capture complete: %d screens"), Steps.Num());
				Phase = EPhase::Idle;
				FPlatformMisc::RequestExit(false);
				return false;
			}
			Session->ApplySample(Steps[StepIndex]);
			Phase = EPhase::Settling;
		}
		break;

	default:
		break;
	}
	return true;
}
