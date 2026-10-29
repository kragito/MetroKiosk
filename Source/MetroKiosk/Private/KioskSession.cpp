#include "KioskSession.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Misc/DateTime.h"

#define LOCTEXT_NAMESPACE "KioskSession"

namespace
{
	bool IsWaitScreen(EKioskScreen Screen)
	{
		return Screen == EKioskScreen::WaitTapCard || Screen == EKioskScreen::WaitChipCard || Screen == EKioskScreen::WaitCash;
	}
}

// --- navigation ----------------------------------------------------------------------------

void FKioskSession::ResetState()
{
	State = FKioskState();
	MapZoom = 1.f;
	History.Reset();
	CurrentScreen = EKioskScreen::Home;
	ScreenSeconds = 0.f;
}

void FKioskSession::EnterScreen(EKioskScreen Screen)
{
	CurrentScreen = Screen;
	ScreenSeconds = 0.f;
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
		State.CashInsertedCents = 0;
	}
	const EKioskScreen Previous = History.Pop();
	EnterScreen(Previous);
}

void FKioskSession::StartOver()
{
	ResetState();
	NotifyChanged();
}

void FKioskSession::ReturnHome(bool bResetLanguage)
{
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
	if (Type == ETicketType::DayPass)
	{
		State.Destination = INDEX_NONE;
		GoTo(EKioskScreen::Review);
	}
	else
	{
		GoTo(EKioskScreen::Destination);
	}
}

void FKioskSession::SelectDestination(int32 StationIndex)
{
	State.Destination = (State.Destination == StationIndex) ? INDEX_NONE : StationIndex;
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
	State.bKeyboardOpen = bOpen;
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
		GoTo(State.Flow == EKioskFlow::Concession ? EKioskScreen::Concession : EKioskScreen::CardOptions);
		break;
	case EKioskScreen::WaitTapCard:
	case EKioskScreen::WaitChipCard:
		BeginProcessing();
		break;
	default:
		break;
	}
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

void FKioskSession::ContinueFromPayment()
{
	GoTo(EKioskScreen::PayMethod);
}

// --- derived data --------------------------------------------------------------------------

int32 FKioskSession::AdultFareCents() const
{
	if (State.Ticket == ETicketType::DayPass)
	{
		return 800;
	}
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const int32 OneWay = Stations.IsValidIndex(State.Destination) ? Stations[State.Destination].FareCents : 168;
	return State.Ticket == ETicketType::RoundTrip ? FMath::RoundToInt(OneWay * 2 * 0.7f) : OneWay;
}

int32 FKioskSession::ReducedFareCents() const
{
	if (State.Ticket == ETicketType::DayPass)
	{
		return 560;
	}
	return FMath::RoundToInt(AdultFareCents() * 0.7f);
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
		const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
		return Passes.IsValidIndex(State.Concession) ? Passes[State.Concession].StudentCents : 0;
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
	TArray<FString> Parts;
	if (State.Adults > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d adult%s"), State.Adults, State.Adults == 1 ? TEXT("") : TEXT("s")));
	}
	if (State.Children > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d child%s"), State.Children, State.Children == 1 ? TEXT("") : TEXT("ren")));
	}
	if (State.Seniors > 0)
	{
		Parts.Add(FString::Printf(TEXT("%d senior%s"), State.Seniors, State.Seniors == 1 ? TEXT("") : TEXT("s")));
	}
	return FText::FromString(FString::Join(Parts, TEXT(", ")));
}

FText FKioskSession::PaymentTail() const
{
	switch (State.Flow)
	{
	case EKioskFlow::TopUp:
		return LOCTEXT("TailTopUp", "· top-up for your Student card");
	case EKioskFlow::Concession:
	{
		const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
		return FText::Format(LOCTEXT("TailPass", "· {0} concession pass"),
			FText::FromString(Passes.IsValidIndex(State.Concession) ? Passes[State.Concession].Name : TEXT("")));
	}
	default:
		break;
	}
	const FText Lower = FText::FromString(TicketLabel().ToString().ToLower());
	if (State.Ticket == ETicketType::DayPass)
	{
		return FText::Format(LOCTEXT("TailDay", "· {0}"), Lower);
	}
	return FText::Format(LOCTEXT("TailTrip", "· {0} to {1}"), Lower, DestinationLabel());
}

FText FKioskSession::OrderSummary() const
{
	switch (State.Flow)
	{
	case EKioskFlow::TopUp:
		return LOCTEXT("OrderTopUp", "Top up · Student card •••• 4821");
	case EKioskFlow::Concession:
	{
		const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
		return FText::Format(LOCTEXT("OrderPass", "{0} · Student pass · 30 days"),
			FText::FromString(Passes.IsValidIndex(State.Concession) ? Passes[State.Concession].Name : TEXT("")));
	}
	default:
		break;
	}
	if (State.Ticket == ETicketType::DayPass)
	{
		return FText::Format(LOCTEXT("OrderDay", "{0} · All lines · {1}"), TicketLabel(), RiderSummary());
	}
	return FText::Format(LOCTEXT("OrderTrip", "{0} · City Hall → {1} · {2}"), TicketLabel(), DestinationLabel(), RiderSummary());
}

FText FKioskSession::ClockText()
{
	static const TCHAR* const Days[] = { TEXT("Mon"), TEXT("Tue"), TEXT("Wed"), TEXT("Thu"), TEXT("Fri"), TEXT("Sat"), TEXT("Sun") };
	static const TCHAR* const Months[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	const FDateTime Now = FDateTime::Now();
	const int32 Hour12 = (Now.GetHour() % 12 == 0) ? 12 : Now.GetHour() % 12;
	return FText::FromString(FString::Printf(TEXT("%s %d %s · %d:%02d %s"),
		Days[static_cast<int32>(Now.GetDayOfWeek())], Now.GetDay(), Months[Now.GetMonth() - 1],
		Hour12, Now.GetMinute(), Now.GetHour() < 12 ? TEXT("am") : TEXT("pm")));
}

FText FKioskSession::ValidUntilText()
{
	const FDateTime Later = FDateTime::Now() + FTimespan::FromHours(2);
	const int32 Hour12 = (Later.GetHour() % 12 == 0) ? 12 : Later.GetHour() % 12;
	return FText::FromString(FString::Printf(TEXT("%d:%02d %s"), Hour12, Later.GetMinute(), Later.GetHour() < 12 ? TEXT("am") : TEXT("pm")));
}

FText FKioskSession::DateShortText()
{
	static const TCHAR* const Days[] = { TEXT("MON"), TEXT("TUE"), TEXT("WED"), TEXT("THU"), TEXT("FRI"), TEXT("SAT"), TEXT("SUN") };
	static const TCHAR* const Months[] = { TEXT("JAN"), TEXT("FEB"), TEXT("MAR"), TEXT("APR"), TEXT("MAY"), TEXT("JUN"), TEXT("JUL"), TEXT("AUG"), TEXT("SEP"), TEXT("OCT"), TEXT("NOV"), TEXT("DEC") };
	const FDateTime Now = FDateTime::Now();
	return FText::FromString(FString::Printf(TEXT("%s %d %s"), Days[static_cast<int32>(Now.GetDayOfWeek())], Now.GetDay(), Months[Now.GetMonth() - 1]));
}

FKioskStepInfo FKioskSession::StepInfo() const
{
	FKioskStepInfo Info;
	const EKioskScreen Screen = CurrentScreen;
	if (Screen == EKioskScreen::Home || Screen == EKioskScreen::Receipt)
	{
		return Info;
	}

	const bool bWait = IsWaitScreen(Screen) || Screen == EKioskScreen::Processing;

	if (State.Flow == EKioskFlow::Ticket)
	{
		switch (Screen)
		{
		case EKioskScreen::TicketType: Info.Current = 1; break;
		case EKioskScreen::Destination:
		case EKioskScreen::DestinationMap: Info.Current = 2; break;
		case EKioskScreen::Review:
		case EKioskScreen::PayMethod: Info.Current = 3; break;
		default: Info.Current = bWait ? 4 : 1; break;
		}
		Info.Labels.Add(Info.Current > 1 ? TicketLabel() : KT("Ticket"));
		if (Info.Current > 2)
		{
			Info.Labels.Add(State.Ticket == ETicketType::DayPass ? FText::FromString(TEXT("All lines")) : DestinationLabel());
		}
		else
		{
			Info.Labels.Add(KT("Destination"));
		}
	}
	else
	{
		switch (Screen)
		{
		case EKioskScreen::ScanCard: Info.Current = 1; break;
		case EKioskScreen::CardOptions: Info.Current = 2; break;
		case EKioskScreen::DoneLiftCard: Info.Current = 5; break;
		default: Info.Current = bWait ? 4 : 3; break;
		}
		Info.Labels.Add(FText::FromString(TEXT("Tap card")));
		if (Info.Current > 2)
		{
			Info.Labels.Add(State.Flow == EKioskFlow::TopUp ? FText::FromString(TEXT("Top-up")) : FText::FromString(TEXT("Concession Pass")));
		}
		else
		{
			Info.Labels.Add(FText::FromString(TEXT("What to do?")));
		}
	}
	Info.Labels.Add(KT("Review"));
	Info.Labels.Add(KT("Pay"));
	return Info;
}

// --- timers --------------------------------------------------------------------------------

int32 FKioskSession::PaySecondsLeft() const
{
	return FMath::Max(0, FMath::CeilToInt(PayTimeoutSeconds - ScreenSeconds) - 1);
}

int32 FKioskSession::ReceiptSecondsLeft() const
{
	return FMath::Max(0, FMath::CeilToInt(ReceiptReturnSeconds - ScreenSeconds));
}

void FKioskSession::Tick(float DeltaSeconds)
{
	if (bCallActive)
	{
		CallTime += DeltaSeconds;
	}
	if (bHelpFullscreen)
	{
		return; // the purchase is paused while on the call
	}
	ScreenSeconds += DeltaSeconds;
	if (bFreezeTimers)
	{
		return;
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
		if (ScreenSeconds >= ReceiptReturnSeconds)
		{
			ReturnHome(true);
		}
		break;
	case EKioskScreen::DoneLiftCard:
		if (ScreenSeconds >= DoneReturnSeconds)
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
	OnChanged.Broadcast();
}

// --- QA samples ----------------------------------------------------------------------------

void FKioskSession::SeedTicketSample()
{
	State.Flow = EKioskFlow::Ticket;
	State.Ticket = ETicketType::OneWay;
	State.Destination = FKioskData::FindStation(TEXT("HarbourFront"));
}

bool FKioskSession::ApplySample(const FString& Name)
{
	EndCall();
	ResetState();
	SetLanguage(EKioskLanguage::English);

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
	else if (Name == TEXT("Home_Chinese")) { SetLanguage(EKioskLanguage::Chinese); }
	else if (Name == TEXT("Home_Malay")) { SetLanguage(EKioskLanguage::Malay); }
	else if (Name == TEXT("Home_Tamil")) { SetLanguage(EKioskLanguage::Tamil); }
	else if (Name == TEXT("TicketType")) { Place(S::TicketType, { S::Home }); }
	else if (Name == TEXT("Destination")) { State.Ticket = ETicketType::OneWay; Place(S::Destination, { S::Home, S::TicketType }); }
	else if (Name == TEXT("DestinationSearch"))
	{
		State.SearchText = TEXT("Har");
		State.bKeyboardOpen = true;
		Place(S::Destination, { S::Home, S::TicketType });
	}
	else if (Name == TEXT("DestinationMap")) { Place(S::DestinationMap, { S::Home, S::TicketType }); }
	else if (Name == TEXT("DestinationSelected")) { SeedTicketSample(); Place(S::Destination, { S::Home, S::TicketType }); }
	else if (Name == TEXT("Review")) { SeedTicketSample(); Place(S::Review, { S::Home, S::TicketType, S::Destination }); }
	else if (Name == TEXT("PayMethod")) { SeedTicketSample(); Place(S::PayMethod, { S::Home, S::TicketType, S::Destination, S::Review }); }
	else if (Name == TEXT("WaitTapCard")) { SeedTicketSample(); State.PayMethod = EPayMethod::Tap; Place(S::WaitTapCard, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitChipCard")) { SeedTicketSample(); State.PayMethod = EPayMethod::Chip; Place(S::WaitChipCard, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitCash")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; Place(S::WaitCash, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("WaitCashPartial")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; State.CashInsertedCents = 100; Place(S::WaitCash, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("Processing")) { SeedTicketSample(); State.PayMethod = EPayMethod::Tap; Place(S::Processing, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("ProcessingCash")) { SeedTicketSample(); State.PayMethod = EPayMethod::Cash; State.CashInsertedCents = 200; Place(S::Processing, { S::Home, S::TicketType, S::Destination, S::Review, S::PayMethod }); }
	else if (Name == TEXT("Receipt")) { SeedTicketSample(); Place(S::Receipt, {}); }
	else if (Name == TEXT("HelpCall"))
	{
		SeedTicketSample();
		Place(S::Destination, { S::Home, S::TicketType });
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
	else if (Name == TEXT("ScanCard")) { State.Flow = EKioskFlow::TopUp; Place(S::ScanCard, { S::Home }); }
	else if (Name == TEXT("CardOptions")) { State.Flow = EKioskFlow::TopUp; Place(S::CardOptions, { S::Home, S::ScanCard }); }
	else if (Name == TEXT("TopUpAmount")) { State.Flow = EKioskFlow::TopUp; Place(S::TopUpAmount, { S::Home, S::ScanCard, S::CardOptions }); }
	else if (Name == TEXT("TopUpAmountSelected")) { State.Flow = EKioskFlow::TopUp; State.TopUpCents = 1000; Place(S::TopUpAmount, { S::Home, S::ScanCard, S::CardOptions }); }
	else if (Name == TEXT("Concession")) { State.Flow = EKioskFlow::Concession; Place(S::Concession, { S::Home, S::ScanCard }); }
	else if (Name == TEXT("ConcessionSelected")) { State.Flow = EKioskFlow::Concession; State.Concession = 2; Place(S::Concession, { S::Home, S::ScanCard }); }
	else if (Name == TEXT("PassDates")) { State.Flow = EKioskFlow::Concession; State.Concession = 2; Place(S::PassDates, { S::Home, S::ScanCard, S::Concession }); }
	else if (Name == TEXT("DoneLiftCard")) { State.Flow = EKioskFlow::TopUp; State.TopUpCents = 1000; Place(S::DoneLiftCard, {}); }
	else if (Name == TEXT("DoneLiftCardConcession")) { State.Flow = EKioskFlow::Concession; State.Concession = 2; Place(S::DoneLiftCard, {}); }
	else { bOk = false; }

	NotifyChanged();
	return bOk;
}

#undef LOCTEXT_NAMESPACE
