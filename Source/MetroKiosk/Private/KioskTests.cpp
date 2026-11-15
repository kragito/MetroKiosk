// Development automation tests: localisation coverage (MetroKiosk.Loc.Coverage) and session rules (MetroKiosk.Session.Rules).
// Run: UnrealEditor-Cmd.exe MetroKiosk.uproject -ExecCmds="Automation RunTests MetroKiosk; Quit" -unattended -nullrhi -nosplash
#if WITH_DEV_AUTOMATION_TESTS

#include "KioskLoc.h"
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
		for (int32 Index = 0; Index <= Text.Len(); ++Index)
		{
			const TCHAR Ch = Index < Text.Len() ? Text[Index] : TEXT(' ');
			if (Ch == TEXT('{'))
			{
				bInBraces = true;
			}
			if (!bInBraces && ((Ch >= TEXT('a') && Ch <= TEXT('z')) || (Ch >= TEXT('A') && Ch <= TEXT('Z'))))
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
			TEXT("TopUpAmountSelected"), TEXT("TopUpKeypad"), TEXT("Concession"), TEXT("PassDates"), TEXT("DoneLiftCard"), TEXT("DoneLiftCardConcession") };
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

	// Validity: the same rule everywhere, never a time after end of service, late-night day pass warning.
	FKioskSession::SetFixedClockMinutes(23 * 60 + 30);
	S->ApplySample(TEXT("Receipt"));
	FKioskSession::SetFixedClockMinutes(23 * 60 + 30);
	S->State.Ticket = ETicketType::OneWay;
	TestTrue(TEXT("one-way bought at 11:30 p.m. is valid until end of service"), S->ReceiptValidText().ToString().Replace(TEXT(" "), TEXT(" ")).Contains(TEXT("12:00 a.m.")));
	FKioskSession::SetFixedClockMinutes(10 * 60);
	TestTrue(TEXT("one-way bought at 10:00 a.m. is valid for two hours"), S->ReceiptValidText().ToString().Contains(TEXT("12:00 pm")));
	FKioskSession::SetFixedClockMinutes(20 * 60 + 59);
	TestEqual(TEXT("no late warning at 8:59 p.m."), FKioskSession::LateServiceHours(), static_cast<int32>(INDEX_NONE));
	FKioskSession::SetFixedClockMinutes(21 * 60);
	TestEqual(TEXT("3 hours left at 9:00 p.m."), FKioskSession::LateServiceHours(), 3);
	FKioskSession::SetFixedClockMinutes(-1);
	S->ApplySample(TEXT("Home"));
	return true;
}

#endif
