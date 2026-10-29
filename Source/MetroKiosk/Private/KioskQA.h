// Review/QA hooks: -KioskScreen=<Name> and -KioskCaptureAll=<dir>.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "KioskSession.h"

class FKioskQARunner
{
public:
	~FKioskQARunner();

	/** Reads the command line; does nothing when no QA switch is present. */
	void Initialise(const TSharedRef<FKioskSession>& InSession);
	void Shutdown();

private:
	bool Tick(float DeltaSeconds);
	FString CurrentFileName() const;
	bool EnsureCaptureResolution();

	enum class EPhase : uint8 { Idle, Settling, WaitingForCapture, Cooldown };

	TSharedPtr<FKioskSession> Session;
	FTSTicker::FDelegateHandle TickerHandle;
	TArray<FString> Steps;
	FString CaptureDir;
	int32 StepIndex = 0;
	int32 FrameCounter = 0;
	int32 ResizeAttempts = 0;
	EPhase Phase = EPhase::Idle;
};
