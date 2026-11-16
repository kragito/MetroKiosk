// Development automation tests: localisation coverage (MetroKiosk.Loc.Coverage) and session rules (MetroKiosk.Session.Rules).
// Run: UnrealEditor-Cmd.exe MetroKiosk.uproject -ExecCmds="Automation RunTests MetroKiosk; Quit" -unattended -nullrhi -nosplash
#if WITH_DEV_AUTOMATION_TESTS

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "KioskSearch.h"
#include "KioskSession.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/SKioskRoot.h"

namespace
{
	/** Latin words that are allowed inside Chinese and Tamil text (brand names and things the customer types on the Latin keyboard). */
	const TCHAR* const GAllowedLatin[] = { TEXT("NETS"), TEXT("PIN"), TEXT("Gardens"), TEXT("niuchesui"), TEXT("VivoCity"), TEXT("Zoo"), TEXT("ION") };

	FString Unescape(const FString& Raw)
	{
		FString Out;
		for (int32 Index = 0; Index < Raw.Len(); ++Index)
		{
			if (Raw[Index] == TEXT('\\') && Index + 1 < Raw.Len())
			{
				++Index;
				switch (Raw[Index])
				{
				case TEXT('n'): Out.AppendChar(TEXT('\n')); break;
				case TEXT('t'): Out.AppendChar(TEXT('\t')); break;
				default: Out.AppendChar(Raw[Index]); break;
				}
			}
			else
			{
				Out.AppendChar(Raw[Index]);
			}
		}
		return Out;
	}

	/** Finds every <Macro>("literal") in a source text and returns the unescaped literals. */
	void CollectLiterals(const FString& Source, const TCHAR* Macro, TArray<FString>& Out)
	{
		const FString Needle = FString(Macro) + TEXT("(");
		int32 From = 0;
		while (true)
		{
			const int32 At = Source.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
			if (At == INDEX_NONE)
			{
				break;
			}
			From = At + Needle.Len();
			if (At > 0 && (FChar::IsAlnum(Source[At - 1]) || Source[At - 1] == TEXT('_')))
			{
				continue; // part of a longer identifier such as NSLOCTEXT
			}
			int32 Pos = From;
			while (Pos < Source.Len() && FChar::IsWhitespace(Source[Pos]))
			{
				++Pos;
			}
			if (Pos >= Source.Len() || Source[Pos] != TEXT('"'))
			{
				continue;
			}
			++Pos;
			FString Raw;
			while (Pos < Source.Len() && Source[Pos] != TEXT('"'))
			{
				if (Source[Pos] == TEXT('\\') && Pos + 1 < Source.Len())
				{
					Raw.AppendChar(Source[Pos++]);
				}
				Raw.AppendChar(Source[Pos++]);
			}
			Out.Add(Unescape(Raw));
		}
	}

	/** Placeholder tokens such as {0} in order of first appearance, as a sorted unique list. */
	TArray<FString> Placeholders(const FString& Text)
	{
		TSet<FString> Found;
		int32 Open = INDEX_NONE;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			if (Text[Index] == TEXT('{'))
			{
				Open = Index;
			}
			else if (Text[Index] == TEXT('}') && Open != INDEX_NONE)
			{
				const FString Token = Text.Mid(Open + 1, Index - Open - 1);
				if (!Token.IsEmpty() && FChar::IsDigit(Token[0]))
				{
					Found.Add(Token.Left(1));
				}
				Open = INDEX_NONE;
			}
		}
		TArray<FString> Out = Found.Array();
		Out.Sort();
		return Out;
	}

	/** Returns the first run of 3+ Latin letters that is not an allowed word, outside placeholders. */
	FString FirstStrayLatin(const FString& Text)
	{
		FString Run;
		bool bInBraces = false;
		// Format directives such as {0}|plural(one=x,other=y): the directive name and its keys are syntax, not text.
		bool bInDirective = false;
		bool bDirectiveKey = false;
		for (int32 Index = 0; Index <= Text.Len(); ++Index)
		{
			const TCHAR Ch = Index < Text.Len() ? Text[Index] : TEXT(' ');
			if (Ch == TEXT('|') && Text.Mid(Index, 8) == TEXT("|plural(")) { bInDirective = true; bDirectiveKey = true; }
			else if (bInDirective && Ch == TEXT('=')) { bDirectiveKey = false; }
			else if (bInDirective && Ch == TEXT(',')) { bDirectiveKey = true; }
			else if (bInDirective && Ch == TEXT(')')) { bInDirective = false; bDirectiveKey = false; }
			if (Ch == TEXT('{'))
			{
				bInBraces = true;
			}
			if (!bInBraces && !bDirectiveKey && ((Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('A') && Ch <= TEXT('Z'))))
			{
				Run.AppendChar(Ch);
			}
			else
			{
				if (Run.Len() >= 3)
				{
					bool bAllowed = false;
					for (const TCHAR* Word : GAllowedLatin)
					{
						bAllowed |= Run.Equals(Word, ESearchCase::CaseSensitive);
					}
					if (!bAllowed)
					{
						return Run;
					}
				}
				Run.Reset();
			}
			if (Ch == TEXT('}'))
			{
				bInBraces = false;
			}
		}
		return FString();
	}

	const TCHAR* LanguageLabel(EKioskLanguage Language)
	{
		switch (Language)
		{
		case EKioskLanguage::Chinese: return TEXT("Chinese");
		case EKioskLanguage::Malay: return TEXT("Malay");
		case EKioskLanguage::Tamil: return TEXT("Tamil");
		default: return TEXT("English");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKioskLocCoverageTest, "MetroKiosk.Loc.Coverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FKioskLocCoverageTest::RunTest(const FString& Parameters)
{
	const EKioskLanguage Languages[3] = { EKioskLanguage::Chinese, EKioskLanguage::Malay, EKioskLanguage::Tamil };

	// 1. The table itself: every entry complete, placeholders agree with English, no mixed-language Chinese or Tamil.
	for (const FKioskTranslation& Entry : FKioskLoc::Table())
	{
		const FString English = Entry.English;
		const TArray<FString> Expected = Placeholders(English);
		for (EKioskLanguage Language : Languages)
		{
			const FString Text = FKioskLoc::Pick(Entry, Language);
			if (Text.IsEmpty())
			{
				AddError(FString::Printf(TEXT("%s translation is empty for \"%s\""), LanguageLabel(Language), *English));
				continue;
			}
			if (Placeholders(Text) != Expected)
			{
				AddError(FString::Printf(TEXT("%s translation of \"%s\" has different {n} placeholders: \"%s\""), LanguageLabel(Language), *English, *Text));
			}
			if (Language != EKioskLanguage::Malay)
			{
				const FString Stray = FirstStrayLatin(Text);
				if (!Stray.IsEmpty())
				{
					AddError(FString::Printf(TEXT("%s translation of \"%s\" mixes in English (\"%s\"): \"%s\""), LanguageLabel(Language), *English, *Stray, *Text));
				}
			}
		}
	}

	// 2. Every KT("...") literal in the sources has a translation in all three languages, and nothing bypasses KT.
	const FString SourceDir = FPaths::ProjectDir() / TEXT("Source/MetroKiosk");
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *SourceDir, TEXT("*.cpp"), true, false);
	IFileManager::Get().FindFilesRecursive(Files, *SourceDir, TEXT("*.h"), true, false, false);
	if (Files.IsEmpty())
	{
		AddWarning(TEXT("Source folder not found; skipped the source scan"));
	}
	int32 Checked = 0;
	for (const FString& File : Files)
	{
		const FString Name = FPaths::GetCleanFilename(File);
		if (Name == TEXT("KioskLoc.h") || Name == TEXT("KioskLoc.cpp") || Name == TEXT("KioskTests.cpp"))
		{
			continue;
		}
		FString Source;
		if (!FFileHelper::LoadFileToString(Source, *File))
		{
			continue;
		}
		TArray<FString> Used;
		CollectLiterals(Source, TEXT("KT"), Used);
		for (const FString& English : Used)
		{
			++Checked;
			const FKioskTranslation* Entry = FKioskLoc::Find(English);
			if (!Entry)
			{
				AddError(FString::Printf(TEXT("%s: KT(\"%s\") has no translation in KioskLocTable.inl"), *Name, *English));
				continue;
			}
			for (EKioskLanguage Language : Languages)
			{
				if (FString(FKioskLoc::Pick(*Entry, Language)).IsEmpty())
				{
					AddError(FString::Printf(TEXT("%s: KT(\"%s\") is missing the %s translation"), *Name, *English, LanguageLabel(Language)));
				}
			}
		}
		if (Source.Contains(TEXT("LOCTEXT(")))
		{
			AddError(FString::Printf(TEXT("%s uses LOCTEXT/NSLOCTEXT; user-facing text must go through KT()"), *Name));
		}
		TArray<FString> Invariant;
		CollectLiterals(Source, TEXT("INVTEXT"), Invariant);
		for (const FString& Text : Invariant)
		{
			if (!FirstStrayLatin(Text).IsEmpty())
			{
				AddError(FString::Printf(TEXT("%s: INVTEXT(\"%s\") is untranslated English"), *Name, *Text));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("Checked %d KT literals against %d table entries"), Checked, FKioskLoc::Table().Num()));

	// 3. Runtime sweep: build the whole UI for every sample screen in every language; no lookup may miss the table.
	if (FSlateApplication::IsInitialized())
	{
		static const TCHAR* const Samples[] = {
			TEXT("Home"), TEXT("TicketType"), TEXT("TicketTypeLate"), TEXT("Destination"), TEXT("DestinationSearch"), TEXT("DestinationTypo"), TEXT("DestinationNoMatch"),
			TEXT("DestinationMap"), TEXT("DestinationSelected"), TEXT("Review"), TEXT("ReviewMixed"), TEXT("ReviewDayPass"), TEXT("PayMethod"), TEXT("PayMethodTopUp"),
			TEXT("WaitTapCard"), TEXT("WaitChipCard"), TEXT("WaitCash"), TEXT("WaitCashPartial"), TEXT("Processing"), TEXT("ProcessingCash"), TEXT("Receipt"),
			TEXT("HelpCall"), TEXT("HelpCallCash"), TEXT("HelpPip"), TEXT("IdlePrompt"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"),
			TEXT("TopUpAmountSelected"), TEXT("TopUpKeypad"), TEXT("Concession"), TEXT("PassDates"), TEXT("DoneLiftCard"), TEXT("DoneLiftCardConcession"),
			TEXT("AdultCardOptions"), TEXT("AdultTopUpAmountSelected"), TEXT("AdultTopUpKeypad"), TEXT("AdultConcession"), TEXT("AdultPassDates"), TEXT("AdultDoneLiftCard"), TEXT("AdultDoneLiftCardConcession"), TEXT("AdultPassDatesPicked"),
			TEXT("DestinationMapSearch"), TEXT("DestinationMapSelected") };
		static const TCHAR* const Suffixes[] = { TEXT("_Chinese"), TEXT("_Malay"), TEXT("_Tamil") };

		TSharedRef<FKioskSession> Session = MakeShared<FKioskSession>();
		TSet<FString> Missing;
		int32 Built = 0;
		for (const TCHAR* Suffix : Suffixes)
		{
			for (const TCHAR* Sample : Samples)
			{
				FKioskLoc::ClearMissing();
				if (!Session->ApplySample(FString(Sample) + Suffix))
				{
					AddError(FString::Printf(TEXT("Unknown sample %s"), Sample));
					continue;
				}
				TSharedRef<SKioskRoot> Root = SNew(SKioskRoot, Session);
				++Built;
				for (const FString& English : FKioskLoc::Missing())
				{
					Missing.Add(FString::Printf(TEXT("%s (%s%s)"), *English, Sample, Suffix));
				}
			}
		}
		for (const FString& Entry : Missing)
		{
			AddError(FString::Printf(TEXT("Untranslated at runtime: %s"), *Entry));
		}
		AddInfo(FString::Printf(TEXT("Runtime sweep built %d screens"), Built));
		Session->ApplySample(TEXT("Home"));
	}
	else
	{
		AddWarning(TEXT("Slate is not initialised; skipped the runtime sweep"));
	}
	FKioskLoc::SetLanguage(EKioskLanguage::English);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKioskSessionRulesTest, "MetroKiosk.Session.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FKioskSessionRulesTest::RunTest(const FString& Parameters)
{
	TSharedRef<FKioskSession> S = MakeShared<FKioskSession>();
	using Screen = EKioskScreen;

	// Back from the map returns to the list, not to step 1.
	S->ApplySample(TEXT("Destination"));
	S->ShowDestinationMap();
	TestTrue(TEXT("map opened"), S->Current() == Screen::DestinationMap);
	S->Back();
	TestTrue(TEXT("Back from the map lands on the destination list"), S->Current() == Screen::Destination);

	// Tapping the search field with leftover text starts a fresh search.
	S->State.SearchText = TEXT("garden");
	S->State.bKeyboardOpen = false;
	S->OpenSearchField();
	TestTrue(TEXT("leftover search text cleared"), S->State.SearchText.IsEmpty());
	TestTrue(TEXT("keyboard open"), S->State.bKeyboardOpen);

	// Line filter: toggles, survives opening the keyboard, and narrows typed results to that line.
	S->ApplySample(TEXT("Destination"));
	S->SetLineFilter(0);
	TestEqual(TEXT("line filter set"), S->State.LineFilter, 0);
	S->SetLineFilter(0);
	TestEqual(TEXT("tapping the active line clears the filter"), S->State.LineFilter, INDEX_NONE);
	S->SetLineFilter(0);
	S->OpenSearchField();
	TestEqual(TEXT("opening the keyboard keeps the line filter"), S->State.LineFilter, 0);
	{
		const FKioskSearchResult Filtered = FKioskSearch::Run(TEXT("har"), 0);
		TestFalse(TEXT("filtered search returns something"), Filtered.Hits.IsEmpty());
		for (const FKioskSearchHit& Hit : Filtered.Hits)
		{
			const int32 Station = Hit.bPlace ? FKioskPlaces::All()[Hit.Index].Station : Hit.Index;
			TestTrue(TEXT("every filtered hit is on the North-South line"), FKioskData::Stations()[Station].OnLine(0));
		}
		TestTrue(TEXT("unfiltered search still finds HarbourFront"), FKioskSearch::Run(TEXT("harbour")).Hits.ContainsByPredicate(
			[](const FKioskSearchHit& Hit) { return !Hit.bPlace && FKioskData::Stations()[Hit.Index].Name == TEXT("HarbourFront"); }));
	}
	S->ApplySample(TEXT("DestinationFiltered"));
	TestEqual(TEXT("filtered sample has the NS filter"), S->State.LineFilter, 0);

	// The step bar always matches the screen.
	S->ApplySample(TEXT("PayMethod"));
	TestEqual(TEXT("PayMethod is step 4"), S->StepInfo().Current, 4);
	TestEqual(TEXT("ticket flow has 4 steps"), S->StepInfo().Labels.Num(), 4);
	S->ApplySample(TEXT("Review"));
	TestEqual(TEXT("Review is step 3"), S->StepInfo().Current, 3);
	S->ApplySample(TEXT("WaitCash"));
	TestEqual(TEXT("WaitCash is step 4"), S->StepInfo().Current, 4);
	S->ApplySample(TEXT("PayMethodTopUp"));
	TestEqual(TEXT("card flow PayMethod is step 4"), S->StepInfo().Current, 4);
	S->ApplySample(TEXT("TopUpAmount"));
	TestEqual(TEXT("TopUpAmount is step 3"), S->StepInfo().Current, 3);
	TestEqual(TEXT("TopUpAmount step label"), S->StepInfo().Labels[2].ToString(), FString(TEXT("Amount")));
	S->ApplySample(TEXT("PassDates"));
	TestEqual(TEXT("PassDates step label"), S->StepInfo().Labels[2].ToString(), FString(TEXT("Pass")));

	// Passed steps show the rider's choice; Back restores the generic label.
	S->ApplySample(TEXT("Destination"));
	TestEqual(TEXT("Destination screen: step 1 shows the ticket"), S->StepInfo().Labels[0].ToString(), FString(TEXT("One-way")));
	TestEqual(TEXT("Destination screen: step 2 is still generic"), S->StepInfo().Labels[1].ToString(), FString(TEXT("Destination")));
	S->ApplySample(TEXT("Review"));
	{
		const FKioskStepInfo Info = S->StepInfo();
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		TestTrue(TEXT("sample has a destination"), Stations.IsValidIndex(S->State.Destination));
		TestEqual(TEXT("Review: step 1 shows the ticket"), Info.Labels[0].ToString(), FString(TEXT("One-way")));
		TestEqual(TEXT("Review: step 2 shows the station"), Info.Labels[1].ToString(), Stations[S->State.Destination].Name);
		FKioskLoc::SetLanguage(EKioskLanguage::Chinese);
		const FString Local = FKioskPlaces::LocalName(Stations[S->State.Destination].Name);
		TestEqual(TEXT("Review: step 2 uses the active language"), S->StepInfo().Labels[1].ToString(),
			Local.IsEmpty() ? Stations[S->State.Destination].Name : Local);
		FKioskLoc::SetLanguage(EKioskLanguage::English);
	}
	S->Back();
	TestEqual(TEXT("Back to the destination list restores the generic label"), S->StepInfo().Labels[1].ToString(), FString(TEXT("Destination")));
	S->ApplySample(TEXT("Review"));
	S->State.Ticket = ETicketType::DayPass;
	TestEqual(TEXT("day pass: step 1 shows Day pass"), S->StepInfo().Labels[0].ToString(), FString(TEXT("Day pass")));
	TestEqual(TEXT("day pass: step 2 reads All lines"), S->StepInfo().Labels[1].ToString(), FString(TEXT("All lines")));

	// Idle prompt: 90 s without touches, then 20 s to answer; it never runs during a call.
	S->ApplySample(TEXT("Review"));
	for (int32 Second = 0; Second < 89; ++Second) { S->Tick(1.f); }
	TestFalse(TEXT("no prompt before 90 s"), S->IsIdlePrompt());
	S->NoteActivity();
	for (int32 Second = 0; Second < 89; ++Second) { S->Tick(1.f); }
	TestFalse(TEXT("a touch restarts the idle timer"), S->IsIdlePrompt());
	for (int32 Second = 0; Second < 2; ++Second) { S->Tick(1.f); }
	TestTrue(TEXT("prompt after 90 s idle"), S->IsIdlePrompt());
	S->DismissIdlePrompt();
	TestFalse(TEXT("Continue dismisses it"), S->IsIdlePrompt());
	for (int32 Second = 0; Second < 91; ++Second) { S->Tick(1.f); }
	TestTrue(TEXT("prompt again"), S->IsIdlePrompt());
	for (int32 Second = 0; Second < 21; ++Second) { S->Tick(1.f); }
	TestTrue(TEXT("unanswered prompt resets to Home"), S->Current() == Screen::Home);

	S->ApplySample(TEXT("Review"));
	S->StartCall();
	S->Tick(500.f);
	TestFalse(TEXT("no idle prompt during a call"), S->IsIdlePrompt());
	S->ReturnToPurchase(); // picture-in-picture: the call is still open
	S->Tick(500.f);
	TestFalse(TEXT("no idle prompt during the picture-in-picture call"), S->IsIdlePrompt());
	S->EndCall();

	// Receipt: 30 s, visible countdown, "I need more time" adds 30 s, a call pauses it.
	S->ApplySample(TEXT("Receipt"));
	S->Tick(10.f);
	TestEqual(TEXT("receipt countdown"), S->ReturnSecondsLeft(), 20);
	S->ExtendReturn();
	TestEqual(TEXT("+30 s"), S->ReturnSecondsLeft(), 50);
	S->Tick(35.f);
	TestTrue(TEXT("still on the receipt after the original 30 s"), S->Current() == Screen::Receipt);
	S->StartCall();
	S->Tick(500.f);
	TestTrue(TEXT("the receipt waits while a call is open"), S->Current() == Screen::Receipt);
	S->EndCall();
	S->Tick(20.f);
	TestTrue(TEXT("receipt returns Home when time runs out"), S->Current() == Screen::Home);

	// Concession renewal pre-selects the card holder's current pass.
	S->ApplySample(TEXT("Home"));
	S->BeginCardFlow(EKioskFlow::Concession);
	S->SimulateCardTap();
	TestTrue(TEXT("concession screen"), S->Current() == Screen::Concession);
	TestEqual(TEXT("current pass pre-selected"), S->State.Concession, FKioskSession::CurrentPassIndex);
	S->Back();
	TestTrue(TEXT("Back from the concession screen leaves the flow"), S->Current() == Screen::Home);

	// Card type (T-014): the tapped card decides the label, the pass and the price.
	S->ApplySample(TEXT("Home"));
	S->BeginCardFlow(EKioskFlow::Concession);
	S->SimulateMetroCardTap(EKioskCardType::Student);
	TestTrue(TEXT("student card"), S->IsStudentCard());
	TestEqual(TEXT("student card label"), S->CardLabel().ToString(), FString(TEXT("Student card")));
	TestEqual(TEXT("student card keeps its pass pre-selected"), S->State.Concession, FKioskSession::CurrentPassIndex);
	TestEqual(TEXT("student pays the student concession price"), S->TotalCents(), FKioskData::Concessions()[FKioskSession::CurrentPassIndex].StudentCents);
	TestEqual(TEXT("student card balance"), S->CardBalanceCents(), FKioskData::Card(EKioskCardType::Student).BalanceCents);
	S->ApplySample(TEXT("Home"));
	S->BeginCardFlow(EKioskFlow::Concession);
	S->SimulateMetroCardTap(EKioskCardType::Adult);
	TestTrue(TEXT("adult concession screen"), S->Current() == Screen::Concession);
	TestFalse(TEXT("adult card is not a student card"), S->IsStudentCard());
	TestEqual(TEXT("adult card label"), S->CardLabel().ToString(), FString(TEXT("Adult card")));
	TestEqual(TEXT("adult card has no pass to pre-select"), S->State.Concession, static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("adult card has no current pass"), S->CurrentPass(), static_cast<int32>(INDEX_NONE));
	S->SelectConcession(FKioskSession::CurrentPassIndex);
	TestEqual(TEXT("adult pays the adult pass price"), S->TotalCents(), FKioskData::Concessions()[FKioskSession::CurrentPassIndex].AdultCents);
	TestTrue(TEXT("adult pass price differs from the student price"), S->TotalCents() != FKioskData::Concessions()[FKioskSession::CurrentPassIndex].StudentCents);
	TestEqual(TEXT("adult card balance"), S->CardBalanceCents(), FKioskData::Card(EKioskCardType::Adult).BalanceCents);
	TestTrue(TEXT("adult order summary names the adult card"), S->OrderSummary().ToString().Contains(TEXT("Adult")));
	S->ApplySample(TEXT("AdultTopUpAmountSelected"));
	TestEqual(TEXT("adult top-up sample label"), S->CardLabel().ToString(), FString(TEXT("Adult card")));
	TestEqual(TEXT("adult top-up new balance"), S->NewBalanceCents(), FKioskData::Card(EKioskCardType::Adult).BalanceCents + 1000);
	TestTrue(TEXT("adult top-up summary names the adult card"), S->OrderSummary().ToString().Contains(TEXT("Adult card")));

	// Start-date picking (T-011): a card with no active pass chooses the day its 30-day pass starts.
	S->ApplySample(TEXT("AdultPassDates"));
	TestTrue(TEXT("adult pass dates screen"), S->Current() == Screen::PassDates);
	TestTrue(TEXT("adult card needs a start date"), S->NeedsPassStart());
	TestFalse(TEXT("no start date chosen at first"), S->HasPassStart());
	S->GoToPayment();
	TestTrue(TEXT("payment is blocked until a day is chosen"), S->Current() == Screen::PassDates);
	const FDateTime Today = FKioskSession::Today();
	TestFalse(TEXT("yesterday cannot be chosen"), S->ChoosePassStart(Today - FTimespan::FromDays(1)));
	TestFalse(TEXT("a rejected day chooses nothing"), S->HasPassStart());
	TestTrue(TEXT("today can be chosen"), S->ChoosePassStart(Today));
	TestTrue(TEXT("pass ends 29 days after the start"), S->PassEndDate() == (Today + FTimespan::FromDays(29)));
	const FDateTime Later = Today + FTimespan::FromDays(40);
	TestTrue(TEXT("a later day can be chosen"), S->ChoosePassStart(Later + FTimespan::FromHours(13)));
	TestTrue(TEXT("the start keeps only the date"), S->PassStartDate() == (Later));
	TestTrue(TEXT("end is start + 29 days"), S->PassEndDate() == (Later + FTimespan::FromDays(29)));
	S->GoToPayment();
	TestTrue(TEXT("payment opens once a day is chosen"), S->Current() == Screen::PayMethod);
	S->ApplySample(TEXT("Home"));
	TestFalse(TEXT("the chosen day is cleared with the session"), S->HasPassStart());
	S->ApplySample(TEXT("AdultPassDatesPicked"));
	TestTrue(TEXT("picked sample has a start"), S->HasPassStart());
	TestTrue(TEXT("picked sample starts three days from today"), S->PassStartDate() == (FKioskSession::Today() + FTimespan::FromDays(3)));
	S->ApplySample(TEXT("PassDates"));
	TestFalse(TEXT("student renewal needs no start date"), S->NeedsPassStart());
	S->GoToPayment();
	TestTrue(TEXT("student renewal goes straight to payment"), S->Current() == Screen::PayMethod);

	S->ApplySample(TEXT("TopUpAmountSelected"));
	TestTrue(TEXT("existing samples stay on the student card"), S->State.CardType == FKioskSession::DefaultCardType());
	S->ApplySample(TEXT("Home"));

	// Validity: the same rule everywhere, never a time after end of service, late-night day pass warning.
	FKioskSession::SetFixedClockMinutes(23 * 60 + 30);
	S->ApplySample(TEXT("Receipt"));
	FKioskSession::SetFixedClockMinutes(23 * 60 + 30);
	S->State.Ticket = ETicketType::OneWay;
	TestEqual(TEXT("one-way receipt states 2 hours after use, no clock time (late)"), S->ReceiptValidText().ToString(), FString(TEXT("Valid for 2 hours after use.")));
	FKioskSession::SetFixedClockMinutes(10 * 60);
	TestEqual(TEXT("one-way receipt states 2 hours after use, no clock time (day)"), S->ReceiptValidText().ToString(), FString(TEXT("Valid for 2 hours after use.")));
	TestEqual(TEXT("one-way rule is identical on every screen"), FKioskSession::ValidityRule(ETicketType::OneWay).ToString(), S->ReceiptValidText().ToString());
	S->State.Ticket = ETicketType::RoundTrip;
	TestTrue(TEXT("round trip receipt is valid until end of service"), S->ReceiptValidText().ToString().Contains(TEXT("12:00")));
	S->State.Ticket = ETicketType::OneWay;
	FKioskSession::SetFixedClockMinutes(20 * 60 + 59);
	TestEqual(TEXT("no late warning at 8:59 p.m."), FKioskSession::LateServiceHours(), static_cast<int32>(INDEX_NONE));
	FKioskSession::SetFixedClockMinutes(21 * 60);
	TestEqual(TEXT("3 hours left at 9:00 p.m."), FKioskSession::LateServiceHours(), 3);
	FKioskSession::SetFixedClockMinutes(-1);
	S->ApplySample(TEXT("Home"));
	return true;
}

// Route legs: stops are counted, legs chain end to start, and the last leg ends at the destination.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKioskRouteLegsTest, "MetroKiosk.Routes.Legs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FKioskRouteLegsTest::RunTest(const FString& Parameters)
{
	int32 Checked = 0;
	int32 WithChange = 0;
	for (int32 Station = 0; Station < FKioskData::Stations().Num(); ++Station)
	{
		const FKioskRoute& Route = FKioskRoutes::FromCityHall(Station);
		if (!Route.bValid)
		{
			continue;
		}
		++Checked;
		WithChange += Route.Changes > 0 ? 1 : 0;
		const FString Name = FKioskData::Stations()[Station].Name;
		TestFalse(*FString::Printf(TEXT("%s has legs"), *Name), Route.Legs.IsEmpty());
		TestEqual(*FString::Printf(TEXT("%s: changes = legs - 1"), *Name), Route.Changes, Route.Legs.Num() - 1);
		if (Route.Legs.IsEmpty())
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s: first leg starts at City Hall"), *Name), Route.Legs[0].FromStation, FKioskData::CityHallIndex());
		TestEqual(*FString::Printf(TEXT("%s: last leg ends at the destination"), *Name), Route.Legs.Last().ToStation, Station);
		for (int32 Index = 0; Index < Route.Legs.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("%s: leg %d has at least one stop"), *Name, Index), Route.Legs[Index].Stops >= 1);
			if (Index > 0)
			{
				TestEqual(*FString::Printf(TEXT("%s: leg %d starts where the previous one ended"), *Name, Index), Route.Legs[Index].FromStation, Route.Legs[Index - 1].ToStation);
			}
			TestFalse(*FString::Printf(TEXT("%s: leg %d has text"), *Name, Index), FKioskRoutes::LegText(Route, Index).IsEmpty());
		}
	}
	TestTrue(TEXT("some destinations were checked"), Checked > 0);
	TestTrue(TEXT("both direct and one-change routes exist"), WithChange > 0 && WithChange < Checked);
	return true;
}

#endif
