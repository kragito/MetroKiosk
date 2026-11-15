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
	static constexpr float ReturnSeconds = 30.f;        // receipt / done screens go Home after this long
	static constexpr float ExtendSeconds = 30.f;        // "I need more time" adds this much
	static constexpr float IdleLimitSeconds = 90.f;     // no touches on a purchase screen for this long -> "Are you still there?"
	static constexpr float IdlePromptSeconds = 20.f;    // ...and this long to answer before the kiosk resets

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
	/** Tapping the search field: text left over from an earlier choice starts a fresh search. */
	void OpenSearchField();
	/** Back from the map lands on the list; from anywhere else it navigates to it. */
	void ShowDestinationList();
	void ShowDestinationMap();
	void SelectDestination(int32 StationIndex);
	void SelectPlace(int32 PlaceIndex);
	void ClearDestination();
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
	/** Adult fare for the current ticket type to any station (what a destination tile shows). */
	int32 AdultFareTo(int32 StationIndex) const;
	int32 ReducedFareCents() const;
	int32 TicketTotalCents() const;
	int32 TotalCents() const;
	int32 ChangeCents() const;
	/** Cash returned after leaving the cash screen, shown as a one-off banner on the next screen (0 = none). */
	int32 ReturnedCashBanner() const { return (ReturnedCash > 0 && NavCount == ReturnedAtNav) ? ReturnedCash : 0; }
	static constexpr double ReturnedCashSeconds = 10.0;
	int32 CardBalanceCents() const { return CardBalance; }
	int32 NewBalanceCents() const { return CardBalance + State.TopUpCents; }
	/** Index of the pass the card holder already has (pre-selected when renewing). */
	static constexpr int32 CurrentPassIndex = 2;
	FText TicketLabel() const;
	FText DestinationLabel() const;
	FText PaymentTail() const;      // "· one-way to HarbourFront" (follows "Paying $1.50")
	FText RiderSummary() const;     // "1 adult, 2 children"
	FText OrderSummary() const;     // "One-way · City Hall → HarbourFront · 1 adult"
	FKioskStepInfo StepInfo() const;
	/** Receipt headline, e.g. "One-way · 1 adult, 1 child" (never "mixed"). */
	FText ReceiptKind() const;
	/** The validity rule for a ticket type, worded the same on every screen. */
	static FText ValidityRule(ETicketType Type);
	/** Ticket type the receipt's "Valid until" line is computed for. */
	FText ReceiptValidText() const;
	/** The kiosk clock (real time, or the -KioskFakeTime QA time). */
	static FDateTime Now();
	/** QA only: freeze the clock at a time of day (minutes after midnight); a negative value releases it. */
	static void SetFixedClockMinutes(int32 Minutes);
	/** Whole hours of service left today, or INDEX_NONE before 9:00 p.m. (day pass warning). */
	static int32 LateServiceHours();
	static FText DayPassWarning();
	/** Help call body: says whether cash is safe or nothing has been charged. */
	FText HelpPausedBody() const;
	static FText Money(int32 Cents);
	static FText ClockText();
	static FText DateShortText();
	static FText TimeOfDayText(const FDateTime& Time);

	// --- timers -----------------------------------------------------------------------------
	void Tick(float DeltaSeconds);
	float ScreenTime() const { return ScreenSeconds; }
	int32 PaySecondsLeft() const;
	/** Seconds until the receipt / done screen returns Home, and the fraction of the allowance still left. */
	int32 ReturnSecondsLeft() const;
	float ReturnFractionLeft() const;
	void ExtendReturn();
	bool IsIdlePrompt() const { return bIdlePrompt; }
	int32 IdlePromptSecondsLeft() const { return FMath::Max(0, FMath::CeilToInt(IdlePromptLeft)); }
	void NoteActivity();
	void DismissIdlePrompt();
	bool IsProcessing() const { return CurrentScreen == EKioskScreen::Processing; }
	/** QA: stop automatic transitions so a screen can be captured. */
	bool bFreezeTimers = false;

	// --- QA ---------------------------------------------------------------------------------
	/** Jump straight to a named screen / state with sample data (see SPEC QA hooks). */
	bool ApplySample(const FString& Name);

	void NotifyChanged();
	/** Increments on every UI-visible change; used by the scripted harness to confirm a click had an effect. */
	uint32 ChangeSerial() const { return Serial; }

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
	float ReturnLimit = ReturnSeconds;
	float IdleSeconds = 0.f;
	float IdlePromptLeft = 0.f;
	bool bIdlePrompt = false;
	float CallTime = 0.f;
	bool bCallActive = false;
	bool bHelpFullscreen = false;
	int32 CardBalance = 320;
	int32 ReturnedCash = 0;
	int32 ReturnedAtNav = -1;
	double ReturnedAtTime = 0.0;
	int32 NavCount = 0;
	uint32 Serial = 0;
	EKioskLanguage Lang = EKioskLanguage::English;
};
