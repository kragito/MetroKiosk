// Session model + navigator: owns the customer's choices, the screen history stack and all timers.
#pragma once

#include "CoreMinimal.h"
#include "KioskTypes.h"

struct FKioskStepInfo
{
	TArray<FText> Labels;
	int32 Current = 0; // 1-based index of the current step; Labels.Num() + 1 means all done
};

class METROKIOSK_API FKioskSession : public TSharedFromThis<FKioskSession>
{
public:
	// Tunables (seconds)
	static constexpr float ProcessingSeconds = 2.0f;
	static constexpr float PayTimeoutSeconds = 600.f;
	static constexpr float ReceiptReturnSeconds = 10.f;
	static constexpr float DoneReturnSeconds = 20.f;

	FKioskState State;

	/** Broadcast after every change that needs the UI rebuilt. */
	FSimpleMulticastDelegate OnChanged;

	// --- navigation -------------------------------------------------------------------------
	EKioskScreen Current() const { return CurrentScreen; }
	/** The screen actually on display (HelpCall while the full-screen call is up). */
	EKioskScreen Visible() const { return bHelpFullscreen ? EKioskScreen::HelpCall : CurrentScreen; }
	bool CanGoBack() const { return History.Num() > 0; }
	void GoTo(EKioskScreen Screen);
	void Replace(EKioskScreen Screen);
	void Back();
	void StartOver();
	void ReturnHome(bool bResetLanguage);

	// --- help call --------------------------------------------------------------------------
	void StartCall();
	void ReturnToPurchase();
	void ResumeCall();
	void EndCall();
	bool IsCallActive() const { return bCallActive; }
	bool IsHelpFullscreen() const { return bHelpFullscreen; }
	int32 CallSeconds() const { return FMath::FloorToInt(CallTime); }
	bool bMicMuted = false;
	bool bStaffRequested = false;
	bool bCaptionsOn = true;
	/** Map zoom factor (1 = whole network). */
	float MapZoom = 1.f;
	/** Map pan in widget pixels (only used while zoomed in). */
	FVector2f MapPan = FVector2f::ZeroVector;

	// --- language ---------------------------------------------------------------------------
	EKioskLanguage Language() const;
	void SetLanguage(EKioskLanguage InLanguage);

	// --- actions ----------------------------------------------------------------------------
	void ChooseTicket(ETicketType Type);
	void SelectDestination(int32 StationIndex);
	void SetLineFilter(int32 LineIndex);
	void SetSearchText(const FString& Text);
	void OpenKeyboard(bool bOpen);
	void ConfirmDestination();
	void AdjustRider(int32 Category, int32 Delta); // 0 adult, 1 child, 2 senior
	void ChoosePayMethod(EPayMethod Method);
	void InsertCash(int32 Cents);
	void SimulateCardTap();   // reader / chip slot "hardware" input
	void SimulateCardLift();
	void BeginCardFlow(EKioskFlow Flow);
	void SelectTopUp(int32 Cents);
	void ContinueTopUp();
	void QuickTopUp(int32 Cents);
	void SelectConcession(int32 Index);
	void GoToPayment();
	void OpenTopUpKeypad(bool bOpen);
	void PressTopUpDigit(int32 Digit);
	void BackspaceTopUp();
	static constexpr int32 MaxTopUpDollars = 500;
	static constexpr int32 MinTopUpDollars = 5;

	// --- derived data -----------------------------------------------------------------------
	int32 RiderCount() const { return State.Adults + State.Children + State.Seniors; }
	int32 AdultFareCents() const;
	int32 ReducedFareCents() const;
	int32 TicketTotalCents() const;
	int32 TotalCents() const;
	int32 ChangeCents() const;
	/** Cash returned after leaving the cash screen, shown as a one-off banner on the next screen (0 = none). */
	int32 ReturnedCashBanner() const { return (ReturnedCash > 0 && NavCount == ReturnedAtNav) ? ReturnedCash : 0; }
	int32 CardBalanceCents() const { return CardBalance; }
	int32 NewBalanceCents() const { return CardBalance + State.TopUpCents; }
	FText TicketLabel() const;
	FText DestinationLabel() const;
	FText PaymentTail() const;      // "· one-way to HarbourFront" (follows "Paying $1.68")
	FText RiderSummary() const;     // "1 adult, 2 children"
	FText OrderSummary() const;     // "One-way · City Hall → HarbourFront · 1 adult"
	FKioskStepInfo StepInfo() const;
	static FText Money(int32 Cents);
	static FText ClockText();
	static FText ValidUntilText();
	static FText DateShortText();

	// --- timers -----------------------------------------------------------------------------
	void Tick(float DeltaSeconds);
	float ScreenTime() const { return ScreenSeconds; }
	int32 PaySecondsLeft() const;
	int32 ReceiptSecondsLeft() const;
	bool IsProcessing() const { return CurrentScreen == EKioskScreen::Processing; }
	/** QA: stop automatic transitions so a screen can be captured. */
	bool bFreezeTimers = false;

	// --- QA ---------------------------------------------------------------------------------
	/** Jump straight to a named screen / state with sample data (see SPEC QA hooks). */
	bool ApplySample(const FString& Name);

	void NotifyChanged();

private:
	void ResetState();
	void NoteReturnedCash();
	void EnterScreen(EKioskScreen Screen);
	void BeginProcessing();
	void FinishProcessing();
	void SeedTicketSample();

	EKioskScreen CurrentScreen = EKioskScreen::Home;
	TArray<EKioskScreen> History;
	float ScreenSeconds = 0.f;
	float CallTime = 0.f;
	bool bCallActive = false;
	bool bHelpFullscreen = false;
	int32 CardBalance = 320;
	int32 ReturnedCash = 0;
	int32 ReturnedAtNav = -1;
	int32 NavCount = 0;
	EKioskLanguage Lang = EKioskLanguage::English;
};
