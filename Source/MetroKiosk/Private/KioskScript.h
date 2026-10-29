// Scriptable, non-intrusive test harness: -KioskScript="click:Buy a ticket|wait:2|shot:home" -KioskScriptOut=<dir>
// Runs inside the standalone -game process and delivers real Slate pointer events (no OS cursor movement).
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "KioskSession.h"

class SWidget;

class FKioskScriptRunner
{
public:
	~FKioskScriptRunner();

	/** Returns true when -KioskScript was present and the script was started. */
	bool Initialise(const TSharedRef<FKioskSession>& InSession, const TSharedRef<SWidget>& InRoot);
	void Shutdown();

private:
	enum class EStepKind : uint8 { Click, Drag, Wait, Shot, Dump };
	struct FStep
	{
		EStepKind Kind = EStepKind::Click;
		FString Text;       // label / shot name
		FString Source;     // original step text for the log
		FVector2f Delta = FVector2f::ZeroVector;
		float Seconds = 0.f;
		bool bAutoShot = true;
		FString ShotName;
	};
	struct FEntry
	{
		TSharedPtr<SWidget> Widget;
		FString Name;
		FString Label;
		bool bEnabled = true;
	};
	enum class EPhase : uint8 { Starting, Next, Waiting, VerifyClick, SettleBeforeShot, WaitingForShot, Settle, Done };

	bool Tick(float DeltaSeconds);
	void ParseScript(const FString& Script);
	void RunStep(const FStep& Step);
	void CollectEntries(TArray<FEntry>& Out) const;
	void CollectRecursive(const TSharedRef<SWidget>& Widget, TArray<FEntry>& Out) const;
	bool FindEntry(const FString& Label, FEntry& OutEntry, FString& OutWhy) const;
	bool IsHittable(const FEntry& Entry, FVector2D& OutScreenPos, FString& OutWhy) const;
	bool DoClick(const FString& Label, FString& OutWhy);
	bool DoDrag(const FString& Label, const FVector2f& DesignDelta, FString& OutWhy);
	void DoDump();
	void QueueShot(const FString& Name);
	void Log(const FString& Line);
	void Finish();
	float DesignScale(FVector2f& OutOrigin) const;

	TSharedPtr<FKioskSession> Session;
	TSharedPtr<SWidget> Root;
	FTSTicker::FDelegateHandle TickerHandle;
	TArray<FStep> Steps;
	FString OutDir;
	int32 StepIndex = 0;   // 1-based number of the step being run
	int32 NextStep = 0;
	int32 FileIndex = 0;
	int32 FrameCounter = 0;
	int32 ResizeAttempts = 0;
	float WaitRemaining = 0.f;
	FString PendingShotName;
	FString PendingShotPath;
	EPhase Phase = EPhase::Starting;
	int32 FailCount = 0;
	// click verification: a click that changes nothing is retried (the first events after window start can be dropped)
	FStep ActiveClick;
	uint32 SerialBeforeClick = 0;
	int32 ClickAttempt = 0;
	int32 StableFrames = 0;
	float StartupSeconds = 0.f;
	FVector2f LastRootSize = FVector2f::ZeroVector;
};
