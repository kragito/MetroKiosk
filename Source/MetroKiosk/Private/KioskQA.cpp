#include "KioskQA.h"

#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
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

void FKioskQARunner::Initialise(const TSharedRef<FKioskSession>& InSession)
{
	Session = InSession;
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
			TEXT("Home"), TEXT("TicketType"), TEXT("Destination"), TEXT("DestinationSearch"), TEXT("DestinationMap"),
			TEXT("DestinationSelected"), TEXT("Review"), TEXT("PayMethod"), TEXT("WaitTapCard"), TEXT("WaitChipCard"),
			TEXT("WaitCash"), TEXT("WaitCashPartial"), TEXT("Processing"), TEXT("ProcessingCash"), TEXT("Receipt"),
			TEXT("HelpCall"), TEXT("HelpPip"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"), TEXT("TopUpAmountSelected"), TEXT("TopUpKeypad"),
			TEXT("Concession"), TEXT("ConcessionSelected"), TEXT("PassDates"), TEXT("DoneLiftCard"), TEXT("DoneLiftCardConcession"),
			TEXT("Home_Chinese"), TEXT("Home_Tamil"), TEXT("Home_Malay") };

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
	if (FSlateApplication::IsInitialized())
	{
		// Keep the OS cursor off the canvas so no hover state leaks into captures.
		FSlateApplication::Get().SetCursorPos(FVector2D(-4000.0, -4000.0));
	}

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
