#include "KioskSession.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"

namespace
{
	bool IsWaitScreen(EKioskScreen Screen)
	{
		return Screen == EKioskScreen::WaitTapCard || Screen == EKioskScreen::WaitChipCard || Screen == EKioskScreen::WaitCash;
	}

	/** Purchase screens where a customer who walked away should be reset (not Home, not hardware waits with their own countdown). */
	bool IsIdleScreen(EKioskScreen Screen)
	{
		switch (Screen)
		{
		case EKioskScreen::TicketType:
		case EKioskScreen::Destination:
		case EKioskScreen::DestinationMap:
		case EKioskScreen::Review:
		case EKioskScreen::PayMethod:
		case EKioskScreen::ScanCard:
		case EKioskScreen::CardOptions:
		case EKioskScreen::TopUpAmount:
		case EKioskScreen::Concession:
		case EKioskScreen::PassDates:
			return true;
		default:
			return false;
		}
	}

	int32 GFixedClockMinutes = -1;  // QA sample override
	int32 GCommandClockMinutes = -2; // -2 = not parsed yet, -1 = no -KioskFakeTime
	double GCommandClockSetAt = 0.0;
	int32 GCommandCard = -2; // -2 = not parsed yet, otherwise an EKioskCardType (-KioskCard=Student|Adult)
}

EKioskCardType FKioskSession::DefaultCardType()
{
	if (GCommandCard == -2)
	{
		GCommandCard = static_cast<int32>(EKioskCardType::Student);
		FString Value;
		if (FParse::Value(FCommandLine::Get(), TEXT("-KioskCard="), Value) && Value.TrimQuotes().Equals(TEXT("Adult"), ESearchCase::IgnoreCase))
		{
			GCommandCard = static_cast<int32>(EKioskCardType::Adult);
		}
	}
	return static_cast<EKioskCardType>(GCommandCard);
}

// --- navigation ----------------------------------------------------------------------------

void FKioskSession::ResetState()
{
	State = FKioskState();
	State.CardType = DefaultCardType();
	ReturnLimit = ReturnSeconds;
	IdleSeconds = 0.f;
	bIdlePrompt = false;
	MapZoom = 1.f;
	MapPan = FVector2f::ZeroVector;
	++NavCount;
	History.Reset();
	CurrentScreen = EKioskScreen::Home;
	ScreenSeconds = 0.f;
}

void FKioskSession::NoteReturnedCash()
{
	if (State.CashInsertedCents > 0 && CurrentScreen == EKioskScreen::WaitCash)
	{
		ReturnedCash = State.CashInsertedCents;
		ReturnedAtNav = NavCount + 1;
		ReturnedAtTime = FPlatformTime::Seconds();
	}
}

void FKioskSession::EnterScreen(EKioskScreen Screen)
{
	++NavCount;
	if (Screen != EKioskScreen::Destination)
	{
		State.bKeyboardOpen = false;
	}
	if (Screen != EKioskScreen::DestinationMap)
	{
		MapPan = FVector2f::ZeroVector;
	}
	if (Screen == EKioskScreen::Concession && State.Concession == INDEX_NONE)
	{
		State.Concession = CurrentPass(); // renewing: the card holder's current pass is already chosen (an adult card has none)
	}
	CurrentScreen = Screen;
	ScreenSeconds = 0.f;
	ReturnLimit = ReturnSeconds;
	IdleSeconds = 0.f;
	bIdlePrompt = false;
	NotifyChanged();
}

void FKioskSession::GoTo(EKioskScreen Screen)
{
	History.Add(CurrentScreen);
	EnterScreen(Screen);
}

void FKioskSession::Replace(EKioskScreen Screen)
{
	EnterScreen(Screen);
}

void FKioskSession::Back()
{
	if (History.IsEmpty())
	{
		return;
	}
	if (IsWaitScreen(CurrentScreen))
	{
		NoteReturnedCash();
		State.CashInsertedCents = 0;
	}
	const EKioskScreen Previous = History.Pop();
	EnterScreen(Previous);
}

void FKioskSession::StartOver()
{
	NoteReturnedCash();
	EndCall();
	ResetState();
	NotifyChanged();
}

void FKioskSession::ReturnHome(bool bResetLanguage)
{
	NoteReturnedCash();
	EndCall();
	ResetState();
	if (bResetLanguage)
	{
		SetLanguage(EKioskLanguage::English);
	}
	NotifyChanged();
}

// --- help call -----------------------------------------------------------------------------

void FKioskSession::StartCall()
{
	bCallActive = true;
	bHelpFullscreen = true;
	CallTime = 0.f;
	bMicMuted = false;
	bStaffRequested = false;
	bCaptionsOn = true;
	NotifyChanged();
}

void FKioskSession::ReturnToPurchase()
{
	bHelpFullscreen = false;
	NotifyChanged();
}

void FKioskSession::ResumeCall()
{
	if (bCallActive)
	{
		bHelpFullscreen = true;
		NotifyChanged();
	}
}

void FKioskSession::EndCall()
{
	bCallActive = false;
	bHelpFullscreen = false;
	CallTime = 0.f;
	NotifyChanged();
}

// --- language ------------------------------------------------------------------------------

EKioskLanguage FKioskSession::Language() const
{
	return Lang;
}

void FKioskSession::SetLanguage(EKioskLanguage InLanguage)
{
	Lang = InLanguage;
	FKioskLoc::SetLanguage(InLanguage);
	NotifyChanged();
}

// --- actions -------------------------------------------------------------------------------

void FKioskSession::ChooseTicket(ETicketType Type)
{
	State.Flow = EKioskFlow::Ticket;
	State.Ticket = Type;
	State.bKeyboardOpen = false;
	State.SearchText.Reset();
	State.LineFilter = INDEX_NONE;
	if (Type == ETicketType::DayPass)
	{
		State.Destination = INDEX_NONE;
		State.DestinationPlace = INDEX_NONE;
		GoTo(EKioskScreen::Review);
	}
	else
	{
		GoTo(EKioskScreen::Destination);
	}
}

void FKioskSession::OpenSearchField()
{
	if (!State.bKeyboardOpen && !State.SearchText.IsEmpty())
	{
		State.SearchText.Reset(); // leftovers from an earlier choice: start a fresh search
	}
	OpenKeyboard(true);
}

void FKioskSession::ShowDestinationList()
{
	if (CurrentScreen == EKioskScreen::DestinationMap && History.Num() > 0 && History.Last() == EKioskScreen::Destination)
	{
		Back();
	}
	else if (CurrentScreen != EKioskScreen::Destination)
	{
		GoTo(EKioskScreen::Destination);
	}
}

void FKioskSession::ShowDestinationMap()
{
	GoTo(EKioskScreen::DestinationMap);
}

void FKioskSession::SelectDestination(int32 StationIndex)
{
	State.Destination = StationIndex;
	State.DestinationPlace = INDEX_NONE;
	NotifyChanged();
}

void FKioskSession::SelectPlace(int32 PlaceIndex)
{
	const TArray<FKioskPlace>& Places = FKioskPlaces::All();
	if (Places.IsValidIndex(PlaceIndex))
	{
		State.Destination = Places[PlaceIndex].Station;
		State.DestinationPlace = PlaceIndex;
		NotifyChanged();
	}
}

void FKioskSession::ClearDestination()
{
	State.Destination = INDEX_NONE;
	State.DestinationPlace = INDEX_NONE;
	NotifyChanged();
}

void FKioskSession::SetLineFilter(int32 LineIndex)
{
	State.LineFilter = (State.LineFilter == LineIndex) ? INDEX_NONE : LineIndex;
	NotifyChanged();
}

void FKioskSession::SetSearchText(const FString& Text)
{
	State.SearchText = Text;
	NotifyChanged();
}

void FKioskSession::OpenKeyboard(bool bOpen)
{
	State.bKeyboardOpen = bOpen; // the line filter stays: it narrows typed results too, and the list heading names it
	NotifyChanged();
}

void FKioskSession::ConfirmDestination()
{
	if (State.Destination != INDEX_NONE)
	{
		State.bKeyboardOpen = false;
		GoTo(EKioskScreen::Review);
	}
}

void FKioskSession::AdjustRider(int32 Category, int32 Delta)
{
	int32* Counts[3] = { &State.Adults, &State.Children, &State.Seniors };
	int32& Count = *Counts[FMath::Clamp(Category, 0, 2)];
	const int32 NewCount = FMath::Clamp(Count + Delta, 0, 9);
	if (RiderCount() - Count + NewCount < 1)
	{
		return; // at least one rider
	}
	Count = NewCount;
	NotifyChanged();
}

void FKioskSession::ChoosePayMethod(EPayMethod Method)
{
	State.PayMethod = Method;
	State.CashInsertedCents = 0;
	switch (Method)
	{
	case EPayMethod::Tap: GoTo(EKioskScreen::WaitTapCard); break;
	case EPayMethod::Chip: GoTo(EKioskScreen::WaitChipCard); break;
	default: GoTo(EKioskScreen::WaitCash); break;
	}
}

void FKioskSession::InsertCash(int32 Cents)
{
	if (CurrentScreen != EKioskScreen::WaitCash)
	{
		return;
	}
	State.CashInsertedCents += Cents;
	if (State.CashInsertedCents >= TotalCents())
	{
		BeginProcessing();
	}
	else
	{
		NotifyChanged();
	}
}

void FKioskSession::BeginProcessing()
{
	Replace(EKioskScreen::Processing);
}

void FKioskSession::FinishProcessing()
{
	History.Reset();
	Replace(State.Flow == EKioskFlow::Ticket ? EKioskScreen::Receipt : EKioskScreen::DoneLiftCard);
}

void FKioskSession::SimulateCardTap()
{
	switch (CurrentScreen)
	{
	case EKioskScreen::Home:
		State.Flow = EKioskFlow::TopUp;
		GoTo(EKioskScreen::CardOptions);
		break;
	case EKioskScreen::ScanCard:
		Replace(State.Flow == EKioskFlow::Concession ? EKioskScreen::Concession : EKioskScreen::CardOptions);
		break;
	case EKioskScreen::WaitTapCard:
	case EKioskScreen::WaitChipCard:
		BeginProcessing();
		break;
	default:
		break;
	}
}

void FKioskSession::SimulateMetroCardTap(EKioskCardType Type)
{
	State.CardType = Type;
	State.Concession = INDEX_NONE; // a different card may hold a different pass; EnterScreen pre-selects the right one
	SimulateCardTap();
}

const FKioskCardInfo& FKioskSession::CardInfo() const
{
	return FKioskData::Card(State.CardType);
}

FText FKioskSession::CardLabel() const
{
	return IsStudentCard() ? KT("Student card") : KT("Adult card");
}

int32 FKioskSession::CurrentPass() const
{
	return CardInfo().bHasActivePass ? CardInfo().ActivePassIndex : INDEX_NONE;
}

int32 FKioskSession::CardBalanceCents() const
{
	return CardInfo().BalanceCents;
}

int32 FKioskSession::PassPriceCents(int32 ConcessionIndex) const
{
	const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
	if (!Passes.IsValidIndex(ConcessionIndex))
	{
		return 0;
	}
	return IsStudentCard() ? Passes[ConcessionIndex].StudentCents : Passes[ConcessionIndex].AdultCents;
}

void FKioskSession::SimulateCardLift()
{
	if (CurrentScreen == EKioskScreen::DoneLiftCard)
	{
		ReturnHome(true);
	}
}

void FKioskSession::BeginCardFlow(EKioskFlow Flow)
{
	State.Flow = Flow;
	GoTo(EKioskScreen::ScanCard);
}

void FKioskSession::SelectTopUp(int32 Cents)
{
	State.TopUpCents = (State.TopUpCents == Cents) ? 0 : Cents;
	NotifyChanged();
}

void FKioskSession::ContinueTopUp()
{
	if (State.TopUpCents > 0)
	{
		State.Flow = EKioskFlow::TopUp;
		GoTo(EKioskScreen::PayMethod);
	}
}

void FKioskSession::OpenTopUpKeypad(bool bOpen)
{
	State.bTopUpKeypad = bOpen;
	State.TopUpDigits.Reset();
	State.TopUpCents = 0;
	NotifyChanged();
}

void FKioskSession::PressTopUpDigit(int32 Digit)
{
	const FString Next = State.TopUpDigits + FString::FromInt(Digit);
	const int32 Dollars = FCString::Atoi(*Next);
	if (Dollars > MaxTopUpDollars || Next.Len() > 3)
	{
		return;
	}
	State.TopUpDigits = Dollars == 0 ? FString() : FString::FromInt(Dollars);
	State.TopUpCents = (Dollars >= MinTopUpDollars) ? Dollars * 100 : 0;
	NotifyChanged();
}

void FKioskSession::BackspaceTopUp()
{
	State.TopUpDigits.LeftChopInline(1);
	const int32 Dollars = FCString::Atoi(*State.TopUpDigits);
	State.TopUpCents = (Dollars >= MinTopUpDollars) ? Dollars * 100 : 0;
	NotifyChanged();
}

void FKioskSession::QuickTopUp(int32 Cents)
{
	State.Flow = EKioskFlow::TopUp;
	State.TopUpCents = Cents;
	GoTo(EKioskScreen::PayMethod);
}

void FKioskSession::SelectConcession(int32 Index)
{
	State.Concession = Index;
	NotifyChanged();
}

void FKioskSession::GoToPayment()
{
	GoTo(EKioskScreen::PayMethod);
}

// --- derived data --------------------------------------------------------------------------

int32 FKioskSession::AdultFareCents() const
{
	return AdultFareTo(State.Destination);
}

int32 FKioskSession::AdultFareTo(int32 StationIndex) const
{
	return FKioskFares::AdultCents(State.Ticket, StationIndex);
}

int32 FKioskSession::ReducedFareCents() const
{
	return State.Ticket == ETicketType::DayPass ? FKioskFares::DayPassReducedCents : FKioskFares::ReducedCents(AdultFareCents());
}

int32 FKioskSession::TicketTotalCents() const
{
	return State.Adults * AdultFareCents() + (State.Children + State.Seniors) * ReducedFareCents();
}

int32 FKioskSession::TotalCents() const
{
	switch (State.Flow)
	{
	case EKioskFlow::TopUp:
		return State.TopUpCents;
	case EKioskFlow::Concession:
	{
		return PassPriceCents(State.Concession);
	}
	default:
		return TicketTotalCents();
	}
}

int32 FKioskSession::ChangeCents() const
{
	return FMath::Max(0, State.CashInsertedCents - TotalCents());
}

FText FKioskSession::Money(int32 Cents)
{
	return FText::FromString(FString::Printf(TEXT("$%d.%02d"), Cents / 100, FMath::Abs(Cents) % 100));
}

FText FKioskSession::TicketLabel() const
{
	switch (State.Ticket)
	{
	case ETicketType::RoundTrip: return KT("Round trip");
	case ETicketType::DayPass: return KT("Day pass");
	default: return KT("One-way");
	}
}

FText FKioskSession::DestinationLabel() const
{
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	return Stations.IsValidIndex(State.Destination) ? FText::FromString(Stations[State.Destination].Name) : KT("Destination");
}

FText FKioskSession::RiderSummary() const
{
	TArray<FText> Parts;
	if (State.Adults > 0)
	{
		Parts.Add(FText::Format(State.Adults == 1 ? KT("{0} adult") : KT("{0} adults"), FText::AsNumber(State.Adults)));
	}
	if (State.Children > 0)
	{
		Parts.Add(FText::Format(State.Children == 1 ? KT("{0} child") : KT("{0} children"), FText::AsNumber(State.Children)));
	}
	if (State.Seniors > 0)
	{
		Parts.Add(FText::Format(State.Seniors == 1 ? KT("{0} senior") : KT("{0} seniors"), FText::AsNumber(State.Seniors)));
	}
	FText Joined = Parts.Num() > 0 ? Parts[0] : FText::GetEmpty();
	for (int32 Index = 1; Index < Parts.Num(); ++Index)
	{
		Joined = FText::Format(KT("{0}, {1}"), Joined, Parts[Index]);
	}
	return Joined;
}

FText FKioskSession::ReceiptKind() const
{
	return FText::Format(KT("{0} · {1}"), TicketLabel(), RiderSummary());
}

FText FKioskSession::ValidityRule(ETicketType Type)
{
	switch (Type)
	{
	case ETicketType::RoundTrip: return KT("Return valid until end of service tonight (12:00 a.m.).");
	case ETicketType::DayPass: return KT("Unlimited rides until end of service tonight (12:00 a.m.).");
	default: return KT("Valid for 2 hours after use.");
	}
}

FText FKioskSession::ReceiptValidText() const
{
	// One-way runs 2 hours from first use, so no clock time is printed for it.
	if (State.Ticket == ETicketType::OneWay)
	{
		return ValidityRule(ETicketType::OneWay);
	}
	return FText::Format(KT("Valid until {0}"), KT("12:00 a.m. tonight"));
}

FText FKioskSession::HelpPausedBody() const
{
	if (State.CashInsertedCents > 0)
	{
		return FText::Format(KT("Your {0} is safe. Nothing more will be charged. You can continue or get it back."), Money(State.CashInsertedCents));
	}
	return KT("Nothing has been charged. You can pick up where you left off.");
}

FText FKioskSession::PaymentTail() const
{
	switch (State.Flow)
	{
	case EKioskFlow::TopUp:
		return FText::Format(KT("· top-up for your {0}"), CardLabel());
	case EKioskFlow::Concession:
		return FText::Format(KT("· {0} concession pass"), FKioskData::ConcessionName(State.Concession));
	default:
		break;
	}
	const FText Lower = FText::FromString(TicketLabel().ToString().ToLower());
	if (State.Ticket == ETicketType::DayPass)
	{
		return FText::Format(KT("· {0}"), Lower);
	}
	return FText::Format(KT("· {0} to {1}"), Lower, DestinationLabel());
}

FText FKioskSession::OrderSummary() const
{
	switch (State.Flow)
	{
	case EKioskFlow::TopUp:
		return FText::Format(KT("Top up · {0} •••• {1}"), CardLabel(), FText::FromString(CardInfo().Last4));
	case EKioskFlow::Concession:
		return FText::Format(KT("{0} · {1} · 30 days"), FKioskData::ConcessionName(State.Concession), IsStudentCard() ? KT("Student pass") : KT("Adult pass"));
	default:
		break;
	}
	if (State.Ticket == ETicketType::DayPass)
	{
		return FText::Format(KT("{0} · All lines · {1}"), TicketLabel(), RiderSummary());
	}
	return FText::Format(KT("{0} · City Hall → {1} · {2}"), TicketLabel(), DestinationLabel(), RiderSummary());
}

// --- clock ---------------------------------------------------------------------------------

void FKioskSession::SetFixedClockMinutes(int32 Minutes)
{
	GFixedClockMinutes = Minutes;
}

FDateTime FKioskSession::Now()
{
	const FDateTime Real = FDateTime::Now();
	if (GFixedClockMinutes >= 0)
	{
		return FDateTime(Real.GetYear(), Real.GetMonth(), Real.GetDay(), GFixedClockMinutes / 60, GFixedClockMinutes % 60);
	}
	if (GCommandClockMinutes == -2)
	{
		FString Value;
		GCommandClockMinutes = -1;
		if (FParse::Value(FCommandLine::Get(), TEXT("-KioskFakeTime="), Value))
		{
			FString Hour, Minute;
			if (Value.TrimQuotes().Split(TEXT(":"), &Hour, &Minute))
			{
				GCommandClockMinutes = FMath::Clamp(FCString::Atoi(*Hour), 0, 23) * 60 + FMath::Clamp(FCString::Atoi(*Minute), 0, 59);
				GCommandClockSetAt = FPlatformTime::Seconds();
			}
		}
	}
	if (GCommandClockMinutes >= 0)
	{
		const double Elapsed = FPlatformTime::Seconds() - GCommandClockSetAt;
		return FDateTime(Real.GetYear(), Real.GetMonth(), Real.GetDay()) + FTimespan::FromMinutes(GCommandClockMinutes) + FTimespan::FromSeconds(Elapsed);
	}
	return Real;
}

int32 FKioskSession::LateServiceHours()
{
	const FDateTime Time = Now();
	const int32 Minutes = Time.GetHour() * 60 + Time.GetMinute();
	if (Minutes < 21 * 60)
	{
		return INDEX_NONE;
	}
	return FMath::Max(1, (24 * 60 - Minutes) / 60);
}

FText FKioskSession::DayPassWarning()
{
	const int32 Hours = LateServiceHours();
	return Hours == INDEX_NONE ? FText::GetEmpty()
		: FText::Format(KT("Only {0} h of service left today. A one-way may be cheaper."), FText::AsNumber(Hours));
}

namespace
{
	FText DayName(int32 Day)
	{
		switch (Day)
		{
		case 0: return KT("Mon");
		case 1: return KT("Tue");
		case 2: return KT("Wed");
		case 3: return KT("Thu");
		case 4: return KT("Fri");
		case 5: return KT("Sat");
		default: return KT("Sun");
		}
	}

	FText MonthName(int32 Month)
	{
		switch (Month)
		{
		case 1: return KT("Jan");
		case 2: return KT("Feb");
		case 3: return KT("Mar");
		case 4: return KT("Apr");
		case 5: return KT("May");
		case 6: return KT("Jun");
		case 7: return KT("Jul");
		case 8: return KT("Aug");
		case 9: return KT("Sep");
		case 10: return KT("Oct");
		case 11: return KT("Nov");
		default: return KT("Dec");
		}
	}
}

FText FKioskSession::TimeOfDayText(const FDateTime& Time)
{
	if (FKioskLoc::GetLanguage() == EKioskLanguage::English)
	{
		const int32 Hour12 = (Time.GetHour() % 12 == 0) ? 12 : Time.GetHour() % 12;
		return FText::FromString(FString::Printf(TEXT("%d:%02d %s"), Hour12, Time.GetMinute(), Time.GetHour() < 12 ? TEXT("am") : TEXT("pm")));
	}
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Time.GetHour(), Time.GetMinute()));
}

FText FKioskSession::DateShortText()
{
	const FDateTime Time = Now();
	const FText Date = FText::Format(KT("{0} {1} {2}"), DayName(static_cast<int32>(Time.GetDayOfWeek())), FText::AsNumber(Time.GetDay()), MonthName(Time.GetMonth()));
	return FText::FromString(Date.ToString().ToUpper());
}

FText FKioskSession::ClockText()
{
	const FDateTime Time = Now();
	const FText Date = FText::Format(KT("{0} {1} {2}"), DayName(static_cast<int32>(Time.GetDayOfWeek())), FText::AsNumber(Time.GetDay()), MonthName(Time.GetMonth()));
	return FText::Format(KT("{0} · {1}"), Date, TimeOfDayText(Time));
}

// --- steps ---------------------------------------------------------------------------------

FKioskStepInfo FKioskSession::StepInfo() const
{
	FKioskStepInfo Info;
	const EKioskScreen Screen = CurrentScreen;
	if (Screen == EKioskScreen::Home || Screen == EKioskScreen::Receipt)
	{
		return Info;
	}

	const bool bPay = IsWaitScreen(Screen) || Screen == EKioskScreen::Processing || Screen == EKioskScreen::PayMethod;

	if (State.Flow == EKioskFlow::Ticket)
	{
		switch (Screen)
		{
		case EKioskScreen::TicketType: Info.Current = 1; break;
		case EKioskScreen::Destination:
		case EKioskScreen::DestinationMap: Info.Current = 2; break;
		case EKioskScreen::Review: Info.Current = 3; break;
		default: Info.Current = bPay ? 4 : 1; break;
		}
		// Passed steps show the rider's choice; the current and later steps keep the generic label.
		Info.Labels.Add(Info.Current > 1 ? TicketLabel() : KT("Ticket"));
		// A day pass has no destination: that step reads "All lines" once it has been passed.
		FText DestinationStep = KT("Destination");
		if (Info.Current > 2)
		{
			const TArray<FKioskStation>& Stations = FKioskData::Stations();
			if (State.Ticket == ETicketType::DayPass)
			{
				DestinationStep = KT("All lines");
			}
			else if (Stations.IsValidIndex(State.Destination))
			{
				const FString& English = Stations[State.Destination].Name;
				const FString Local = FKioskPlaces::LocalName(English);
				DestinationStep = FText::FromString(Local.IsEmpty() ? English : Local);
			}
		}
		Info.Labels.Add(DestinationStep);
		Info.Labels.Add(KT("Review"));
		Info.Labels.Add(KT("Pay"));
	}
	else
	{
		switch (Screen)
		{
		case EKioskScreen::ScanCard: Info.Current = 1; break;
		case EKioskScreen::CardOptions: Info.Current = 2; break;
		case EKioskScreen::DoneLiftCard: Info.Current = 5; break;
		default: Info.Current = bPay ? 4 : 3; break;
		}
		Info.Labels.Add(KT("Tap card"));
		Info.Labels.Add(KT("What to do?"));
		Info.Labels.Add(State.Flow == EKioskFlow::TopUp ? KT("Amount") : KT("Pass"));
		Info.Labels.Add(KT("Pay"));
	}
	return Info;
}

// --- timers --------------------------------------------------------------------------------

int32 FKioskSession::PaySecondsLeft() const
{
	return FMath::Max(0, FMath::CeilToInt(PayTimeoutSeconds - ScreenSeconds) - 1);
}

int32 FKioskSession::ReturnSecondsLeft() const
{
	return FMath::Max(0, FMath::CeilToInt(ReturnLimit - ScreenSeconds));
}

float FKioskSession::ReturnFractionLeft() const
{
	return FMath::Clamp((ReturnLimit - ScreenSeconds) / FMath::Max(1.f, ReturnLimit), 0.f, 1.f);
}

void FKioskSession::ExtendReturn()
{
	ReturnLimit = FMath::Min(ReturnLimit + ExtendSeconds, ScreenSeconds + 2.f * ReturnSeconds);
	NotifyChanged();
}

void FKioskSession::NoteActivity()
{
	if (!bIdlePrompt)
	{
		IdleSeconds = 0.f;
	}
}

void FKioskSession::DismissIdlePrompt()
{
	bIdlePrompt = false;
	IdleSeconds = 0.f;
	NotifyChanged();
}

void FKioskSession::Tick(float DeltaSeconds)
{
	if (ReturnedCash > 0 && FPlatformTime::Seconds() - ReturnedAtTime > ReturnedCashSeconds)
	{
		ReturnedCash = 0; // expire the banner so the next customer never sees it
		NotifyChanged();
	}
	if (bCallActive)
	{
		CallTime += DeltaSeconds;
		// Every countdown and auto-return is paused for as long as a call is open, full screen or picture-in-picture.
		// The short "Processing" step of a payment that is already underway is not a countdown: it finishes in picture-in-picture.
		if (!(CurrentScreen == EKioskScreen::Processing && !bHelpFullscreen))
		{
			return;
		}
	}
	ScreenSeconds += DeltaSeconds;
	if (bFreezeTimers)
	{
		return;
	}

	if (IsIdleScreen(CurrentScreen))
	{
		if (bIdlePrompt)
		{
			IdlePromptLeft -= DeltaSeconds;
			if (IdlePromptLeft <= 0.f)
			{
				ReturnHome(true);
				return;
			}
		}
		else
		{
			IdleSeconds += DeltaSeconds;
			if (IdleSeconds >= IdleLimitSeconds)
			{
				bIdlePrompt = true;
				IdlePromptLeft = IdlePromptSeconds;
				NotifyChanged();
			}
		}
	}
	else
	{
		IdleSeconds = 0.f;
		bIdlePrompt = false;
	}

	switch (CurrentScreen)
	{
	case EKioskScreen::Processing:
		if (ScreenSeconds >= ProcessingSeconds)
		{
			FinishProcessing();
		}
		break;
	case EKioskScreen::WaitTapCard:
	case EKioskScreen::WaitChipCard:
	case EKioskScreen::WaitCash:
		if (PaySecondsLeft() <= 0)
		{
			Back();
		}
		break;
	case EKioskScreen::Receipt:
	case EKioskScreen::DoneLiftCard:
		if (ScreenSeconds >= ReturnLimit)
		{
			ReturnHome(true);
		}
		break;
	default:
		break;
	}
}

void FKioskSession::NotifyChanged()
{
	++Serial;
	OnChanged.Broadcast();
}

// --- QA samples ----------------------------------------------------------------------------

void FKioskSession::SeedTicketSample()
{
	State.Flow = EKioskFlow::Ticket;
	State.Ticket = ETicketType::OneWay;
	SelectPlace(FKioskPlaces::FindPlace(TEXT("Gardens by the Bay")));
}

bool FKioskSession::ApplySample(const FString& RawName)
{
	EndCall();
	ResetState();
	SetFixedClockMinutes(-1);
	SetLanguage(EKioskLanguage::English);

	// "<Screen>_Chinese|_Malay|_Tamil" shows the same state in another language.
	FString Name = RawName;
	EKioskLanguage Wanted = EKioskLanguage::English;
	if (Name.EndsWith(TEXT("_Chinese"))) { Wanted = EKioskLanguage::Chinese; Name.LeftChopInline(8); }
	else if (Name.EndsWith(TEXT("_Malay"))) { Wanted = EKioskLanguage::Malay; Name.LeftChopInline(6); }
	else if (Name.EndsWith(TEXT("_Tamil"))) { Wanted = EKioskLanguage::Tamil; Name.LeftChopInline(6); }
	SetLanguage(Wanted);
	// "Adult<Screen>" / "Student<Screen>" shows the same state for that Metro card (the default card follows -KioskCard).
	if (Name.StartsWith(TEXT("Adult"))) { State.CardType = EKioskCardType::Adult; Name.RightChopInline(5); }
	else if (Name.StartsWith(TEXT("Student"))) { State.CardType = EKioskCardType::Student; Name.RightChopInline(7); }
	// Pass already chosen on the pass screens: the card's own pass, or Bus + Train for a card with none.
	const int32 SamplePass = CurrentPass() != INDEX_NONE ? CurrentPass() : CurrentPassIndex;

	auto Place = [this](EKioskScreen Screen, std::initializer_list<EKioskScreen> Path)
	{
		History = TArray<EKioskScreen>(Path);
		CurrentScreen = Screen;
		ScreenSeconds = 0.f;
	};
	using S = EKioskScreen;

	bool bOk = true;
	if (Name == TEXT("Home"))
	{
	}
	else if (Name == TEXT("TicketType")) { Place(S::TicketType, { S::Home }); }
	else if (Name == TEXT("TicketTypeLate")) { SetFixedClockMinutes(22 * 60 + 10); Place(S::TicketType, { S::Home }); }
	else if (Name == TEXT("Destination")) { State.Ticket = ETicketType::OneWay; Place(S::Destination, { S::Home, S::TicketType }); }
	else if (Name == TEXT("DestinationSearch") || Name == TEXT("DestinationTypo") || Name == TEXT("DestinationNoMatch"))
	{
		State.SearchText = Name == TEXT("DestinationSearch") ? TEXT("garden") : (Name == TEXT("DestinationTypo") ? TEXT("harbor frnt") : TEXT("hrbrfrnt"));
		State.bKeyboardOpen = true;
		Place(S::Destination, { S::Home, S::TicketType });
	}
	else if (Name == TEXT("DestinationFiltered")) { State.Ticket = ETicketType::OneWay; State.LineFilter = 0; Place(S::Destination, { S::Home, S::TicketType }); }
	else if (Name == TEXT("DestinationSearchSelected"))
	{
		SeedTicketSample();
		State.SearchText = TEXT("har");
		State.bKeyboardOpen = true;
		Place(S::Destination, { S::Home, S::TicketType });
	}
	else if (Name == TEXT("DestinationMap")) { Place(S::DestinationMap, { S::Home, S::TicketType, S::Destination }); }
	else if (Name == TEXT("DestinationMapSearch"))
	{
		State.Ticket = ETicketType::OneWay;
		State.SearchText = TEXT("Mar");
		State.bKeyboardOpen = true;
		Place(S::DestinationMap, { S::Home, S::TicketType, S::Destination });
	}
	else if (Name == TEXT("DestinationMapSelected"))
	{
		State.Ticket = ETicketType::OneWay;
		State.Destination = FKioskData::FindStation(TEXT("HarbourFront"));
		Place(S::DestinationMap, { S::Home, S::TicketType, S::Destination });
	}
	else if (Name == TEXT("DestinationSelected")) { SeedTicketSample(); Place(S::Destination, { S::Home, S::TicketType }); }
	else if (Name == TEXT("Review")) { SeedTicketSample(); Place(S::Review, { S::Home, S::TicketType, S::Destination }); }
	else if (Name == TEXT("ReviewMixed")) { SeedTicketSample(); State.Children = 1; Place(S::Review, { S::Home, S::TicketType, S::Destination }); }
	else if (Name == TEXT("ReviewDayPass")) { State.Ticket = ETicketType::DayPass; Place(S::Review, { S::Home, S::TicketType }); }
	else if (Name == TEXT("PayMethod")) { SeedTicketSample(); Place(S::PayMethod, { S::Home, S::TicketType, S::Destination, S::Review }); }
	else if (Name == TEXT("WaitTapCard")) { SeedTicketSample(); State.PayMethod = EPayMethod::Tap; Place(S::WaitTapCard, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitChipCard")) { SeedTicketSample(); State.PayMethod = EPayMethod::Chip; Place(S::WaitChipCard, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitCash")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; Place(S::WaitCash, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitCashPartial")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; State.CashInsertedCents = 100; Place(S::WaitCash, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("Processing")) { SeedTicketSample(); State.PayMethod = EPayMethod::Tap; Place(S::Processing, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("ProcessingCash")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; State.CashInsertedCents = 200; Place(S::Processing, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("Receipt")) { SeedTicketSample(); State.Children = 1; State.PayMethod = EPayMethod::Cash; State.CashInsertedCents = 500; Place(S::Receipt, {}); }
	else if (Name == TEXT("HelpCall"))
	{
		SeedTicketSample();
		Place(S::Destination, { S::Home, S::TicketType });
		StartCall();
		CallTime = 24.f;
	}
	else if (Name == TEXT("HelpCallCash"))
	{
		SeedTicketSample();
		State.PayMethod = EPayMethod::Cash;
		State.CashInsertedCents = 200;
		Place(S::WaitCash, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod });
		StartCall();
		CallTime = 24.f;
	}
	else if (Name == TEXT("HelpPip"))
	{
		SeedTicketSample();
		Place(S::Destination, { S::Home, S::TicketType });
		StartCall();
		CallTime = 24.f;
		ReturnToPurchase();
	}
	else if (Name == TEXT("IdlePrompt"))
	{
		SeedTicketSample();
		Place(S::Review, { S::Home, S::TicketType, S::Destination });
		bIdlePrompt = true;
		IdlePromptLeft = 14.f;
	}
	else if (Name == TEXT("ScanCard")) { State.Flow = EKioskFlow::TopUp; Place(S::ScanCard, { S::Home }); }
	else if (Name == TEXT("CardOptions")) { State.Flow = EKioskFlow::TopUp; Place(S::CardOptions, { S::Home }); }
	else if (Name == TEXT("TopUpAmount")) { State.Flow = EKioskFlow::TopUp; Place(S::TopUpAmount, { S::Home, S::CardOptions }); }
	else if (Name == TEXT("TopUpAmountSelected")) { State.Flow = EKioskFlow::TopUp; State.TopUpCents = 1000; Place(S::TopUpAmount, { S::Home, S::CardOptions }); }
	else if (Name == TEXT("TopUpKeypad")) { State.Flow = EKioskFlow::TopUp; State.bTopUpKeypad = true; State.TopUpDigits = TEXT("25"); State.TopUpCents = 2500; Place(S::TopUpAmount, { S::Home, S::CardOptions }); }
	else if (Name == TEXT("Concession")) { State.Flow = EKioskFlow::Concession; State.Concession = CurrentPass(); Place(S::Concession, { S::Home }); }
	else if (Name == TEXT("PassDates")) { State.Flow = EKioskFlow::Concession; State.Concession = SamplePass; Place(S::PassDates, { S::Home, S::Concession }); }
	else if (Name == TEXT("PayMethodTopUp")) { State.Flow = EKioskFlow::TopUp; State.TopUpCents = 1000; Place(S::PayMethod, { S::Home, S::CardOptions }); }
	else if (Name == TEXT("DoneLiftCard")) { State.Flow = EKioskFlow::TopUp; State.TopUpCents = 1000; Place(S::DoneLiftCard, {}); }
	else if (Name == TEXT("DoneLiftCardConcession")) { State.Flow = EKioskFlow::Concession; State.Concession = SamplePass; Place(S::DoneLiftCard, {}); }
	else { bOk = false; }

	NotifyChanged();
	return bOk;
}
