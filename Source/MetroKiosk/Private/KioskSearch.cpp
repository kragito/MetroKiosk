#include "KioskSearch.h"

#include "KioskData.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"

namespace
{
	enum EKeyKind : int32 { KeyPrimary = 0, KeyLocal = 1, KeyAlias = 2 };

	struct FSearchKey
	{
		FString Norm;
		TArray<FString> Words;
		int32 Kind = KeyPrimary;
	};

	struct FEntry
	{
		bool bPlace = false;
		int32 Index = INDEX_NONE;
		FString Name;
		TArray<FSearchKey> Keys;
	};

	bool IsWordChar(TCHAR Ch)
	{
		return Ch > 127 || FChar::IsAlnum(Ch);
	}

	TArray<FString> SplitWords(const FString& Text)
	{
		TArray<FString> Words;
		FString Current;
		for (TCHAR Ch : Text)
		{
			if (IsWordChar(Ch))
			{
				Current.AppendChar(FChar::ToLower(Ch));
			}
			else if (!Current.IsEmpty())
			{
				Words.Add(Current);
				Current.Reset();
			}
		}
		if (!Current.IsEmpty())
		{
			Words.Add(Current);
		}
		return Words;
	}

	FSearchKey MakeKey(const FString& Text, int32 Kind)
	{
		FSearchKey Key;
		Key.Kind = Kind;
		Key.Words = SplitWords(Text);
		Key.Norm = FString::Join(Key.Words, TEXT(""));
		return Key;
	}

	void AddKey(FEntry& Entry, const FString& Text, int32 Kind)
	{
		if (Text.IsEmpty())
		{
			return;
		}
		FSearchKey Key = MakeKey(Text, Kind);
		if (!Key.Norm.IsEmpty())
		{
			Entry.Keys.Add(MoveTemp(Key));
		}
	}

	void AddNames(FEntry& Entry, const FString& Name)
	{
		AddKey(Entry, Name, KeyPrimary);
		for (int32 Language = 0; Language < 3; ++Language)
		{
			AddKey(Entry, FKioskPlaces::LocalNameIn(Name, Language), KeyLocal);
		}
		for (const FKioskAlias& Alias : FKioskPlaces::Aliases())
		{
			if (Alias.Target == Name)
			{
				AddKey(Entry, Alias.Alias, KeyAlias);
			}
		}
	}

	const TArray<FEntry>& Entries()
	{
		static TArray<FEntry> Data;
		if (!Data.IsEmpty())
		{
			return Data;
		}

		const TArray<FKioskPlace>& Places = FKioskPlaces::All();
		TSet<FString> PlaceNames;
		for (int32 Index = 0; Index < Places.Num(); ++Index)
		{
			FEntry Entry;
			Entry.bPlace = true;
			Entry.Index = Index;
			Entry.Name = Places[Index].Name;
			AddNames(Entry, Entry.Name);
			for (const FString& Keyword : Places[Index].Keywords)
			{
				AddKey(Entry, Keyword, KeyAlias);
			}
			PlaceNames.Add(FKioskSearch::Normalize(Entry.Name));
			Data.Add(MoveTemp(Entry));
		}

		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		for (int32 Index = 0; Index < Stations.Num(); ++Index)
		{
			// The kiosk's own station is not a destination; a station named exactly like a place is shown as that place.
			if (!FKioskRoutes::FromCityHall(Index).bValid || PlaceNames.Contains(FKioskSearch::Normalize(Stations[Index].Name)))
			{
				continue;
			}
			FEntry Entry;
			Entry.Index = Index;
			Entry.Name = Stations[Index].Name;
			AddNames(Entry, Entry.Name);
			Data.Add(MoveTemp(Entry));
		}
		return Data;
	}

	int32 FuzzyAllowance(int32 QueryLength)
	{
		return QueryLength <= 3 ? 0 : (QueryLength <= 5 ? 1 : 2);
	}

	/** Smallest distance between the query and a key, its words, or the key's leading characters (a half-typed name). */
	int32 NearDistance(const FString& Query, const FSearchKey& Key)
	{
		const int32 Length = Query.Len();
		int32 Best = FKioskSearch::EditDistance(Query, Key.Norm);
		for (const FString& Word : Key.Words)
		{
			Best = FMath::Min(Best, FKioskSearch::EditDistance(Query, Word));
		}
		for (int32 Extra = 0; Extra <= 1; ++Extra)
		{
			if (Key.Norm.Len() > Length + Extra)
			{
				Best = FMath::Min(Best, FKioskSearch::EditDistance(Query, Key.Norm.Left(Length + Extra)));
			}
		}
		return Best;
	}

	/** Score for one key; 0 means no match. Exact > prefix > word prefix > substring > fuzzy, then primary > translated > alias. */
	int32 ScoreKey(const FString& Query, const TArray<FString>& QueryTokens, const FSearchKey& Key)
	{
		const int32 Penalty = Key.Kind == KeyPrimary ? 0 : (Key.Kind == KeyLocal ? 50 : 150);
		const int32 Length = Query.Len();
		const bool bHasWide = Query.Len() > 0 && Query[0] > 127;
		int32 Base = 0;

		if (Key.Norm == Query)
		{
			Base = 1000;
		}
		else if (Key.Norm.StartsWith(Query))
		{
			Base = 900 - FMath::Min(Key.Norm.Len() - Length, 20) / 2;
		}
		else
		{
			bool bWordPrefix = false;
			for (const FString& Word : Key.Words)
			{
				bWordPrefix |= Word.StartsWith(Query);
			}
			if (bWordPrefix)
			{
				Base = 700;
			}
			else if (QueryTokens.Num() > 1)
			{
				// "gardens bay": every typed word starts some word of the name
				bool bAll = true;
				for (const FString& Token : QueryTokens)
				{
					bool bFound = false;
					for (const FString& Word : Key.Words)
					{
						bFound |= Word.StartsWith(Token);
					}
					bAll &= bFound;
				}
				Base = bAll ? 650 : 0;
			}
			if (Base == 0 && (Length >= 2 || bHasWide) && Key.Norm.Contains(Query))
			{
				Base = 600;
			}
		}
		if (Base == 0)
		{
			const int32 Allowed = FuzzyAllowance(Length);
			if (Allowed > 0)
			{
				const int32 Distance = NearDistance(Query, Key);
				if (Distance <= Allowed)
				{
					Base = 400 - Distance * 50;
				}
			}
		}
		return Base > 0 ? Base - Penalty : 0;
	}

	bool IsPopular(const FEntry& Entry)
	{
		return !Entry.bPlace && FKioskData::PopularStations().Contains(Entry.Index);
	}
}

FString FKioskSearch::Normalize(const FString& Text)
{
	return FString::Join(SplitWords(Text), TEXT(""));
}

int32 FKioskSearch::EditDistance(const FString& A, const FString& B)
{
	const int32 N = A.Len();
	const int32 M = B.Len();
	// Three rolling rows (current, previous, and the one before for adjacent swaps) instead of a full table.
	TArray<int32, TInlineAllocator<64>> Rows[3];
	for (TArray<int32, TInlineAllocator<64>>& Row : Rows)
	{
		Row.SetNumZeroed(M + 1);
	}
	for (int32 J = 0; J <= M; ++J) { Rows[0][J] = J; } // row 0: distance from the empty prefix of A
	for (int32 I = 1; I <= N; ++I)
	{
		TArray<int32, TInlineAllocator<64>>& Cur = Rows[I % 3];
		const TArray<int32, TInlineAllocator<64>>& Prev = Rows[(I + 2) % 3];
		const TArray<int32, TInlineAllocator<64>>& Prev2 = Rows[(I + 1) % 3];
		Cur[0] = I;
		for (int32 J = 1; J <= M; ++J)
		{
			const int32 Cost = A[I - 1] == B[J - 1] ? 0 : 1;
			int32 Best = FMath::Min3(Prev[J] + 1, Cur[J - 1] + 1, Prev[J - 1] + Cost);
			if (I > 1 && J > 1 && A[I - 1] == B[J - 2] && A[I - 2] == B[J - 1])
			{
				Best = FMath::Min(Best, Prev2[J - 2] + 1);
			}
			Cur[J] = Best;
		}
	}
	return Rows[N % 3][M];
}

FKioskSearchResult FKioskSearch::Run(const FString& Query)
{
	FKioskSearchResult Result;
	const FString Normalized = Normalize(Query);
	if (Normalized.IsEmpty())
	{
		return Result;
	}
	const TArray<FString> Tokens = SplitWords(Query);
	const TArray<FEntry>& All = Entries();

	for (const FEntry& Entry : All)
	{
		int32 Best = 0;
		for (const FSearchKey& Key : Entry.Keys)
		{
			Best = FMath::Max(Best, ScoreKey(Normalized, Tokens, Key));
		}
		if (Best > 0)
		{
			Result.Hits.Add({ Entry.bPlace, Entry.Index, Best });
		}
	}

	auto NameOf = [](const FKioskSearchHit& Hit)
	{
		return Hit.bPlace ? FKioskPlaces::All()[Hit.Index].Name : FKioskData::Stations()[Hit.Index].Name;
	};
	if (!Result.Hits.IsEmpty())
	{
		Result.Hits.Sort([&NameOf](const FKioskSearchHit& A, const FKioskSearchHit& B)
		{
			if (A.Score != B.Score)
			{
				return A.Score > B.Score;
			}
			if (A.bPlace != B.bPlace)
			{
				return !A.bPlace; // a station beats a place at equal strength
			}
			return NameOf(A) < NameOf(B);
		});
		if (Result.Hits.Num() > MaxResults)
		{
			Result.Hits.SetNum(MaxResults);
		}
		return Result;
	}

	// Nothing matched: offer the closest names, or the popular places when nothing is even close.
	Result.bSuggestions = true;
	struct FCandidate { const FEntry* Entry; float Ratio; };
	TArray<FCandidate> Candidates;
	for (const FEntry& Entry : All)
	{
		int32 Best = MAX_int32;
		for (const FSearchKey& Key : Entry.Keys)
		{
			Best = FMath::Min(Best, NearDistance(Normalized, Key));
		}
		Candidates.Add({ &Entry, static_cast<float>(Best) / FMath::Max(Normalized.Len(), 3) });
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (!FMath::IsNearlyEqual(A.Ratio, B.Ratio, 0.05f))
		{
			return A.Ratio < B.Ratio;
		}
		if (IsPopular(*A.Entry) != IsPopular(*B.Entry))
		{
			return IsPopular(*A.Entry);
		}
		return A.Entry->Name < B.Entry->Name;
	});

	Result.bClose = Normalized.Len() >= 3 && !Candidates.IsEmpty() && Candidates[0].Ratio <= 0.5f;
	if (Result.bClose)
	{
		for (int32 Index = 0; Index < FMath::Min(MaxSuggestions, Candidates.Num()) && Candidates[Index].Ratio <= 0.55f; ++Index)
		{
			Result.Hits.Add({ Candidates[Index].Entry->bPlace, Candidates[Index].Entry->Index, 0 });
		}
	}
	if (Result.Hits.IsEmpty())
	{
		Result.bClose = false;
		for (int32 Station : FKioskData::PopularStations())
		{
			Result.Hits.Add({ false, Station, 0 });
		}
	}
	return Result;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKioskSearchTest, "MetroKiosk.Search.EditDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FKioskSearchTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("kitten/sitting"), FKioskSearch::EditDistance(TEXT("kitten"), TEXT("sitting")), 3);
	TestEqual(TEXT("ab/ba swap"), FKioskSearch::EditDistance(TEXT("ab"), TEXT("ba")), 1);
	TestEqual(TEXT("bay/marinabay"), FKioskSearch::EditDistance(TEXT("bay"), TEXT("marinabay")), 6);
	TestEqual(TEXT("empty/abc"), FKioskSearch::EditDistance(TEXT(""), TEXT("abc")), 3);
	TestEqual(TEXT("abc/empty"), FKioskSearch::EditDistance(TEXT("abc"), TEXT("")), 3);
	TestEqual(TEXT("same"), FKioskSearch::EditDistance(TEXT("front"), TEXT("front")), 0);

	auto Names = [](const FString& Query)
	{
		TSet<FString> Out;
		for (const FKioskSearchHit& Hit : FKioskSearch::Run(Query).Hits)
		{
			Out.Add(Hit.bPlace ? FKioskPlaces::All()[Hit.Index].Name : FKioskData::Stations()[Hit.Index].Name);
		}
		return Out;
	};
	const TSet<FString> Island = Names(TEXT("island"));
	TestTrue(TEXT("island finds Sentosa"), Island.Contains(TEXT("Sentosa")));
	TestFalse(TEXT("island not Bishan"), Island.Contains(TEXT("Bishan")));
	TestFalse(TEXT("island not Woodlands"), Island.Contains(TEXT("Woodlands")));
	TestFalse(TEXT("island not Junction 8"), Island.Contains(TEXT("Junction 8")));
	const TSet<FString> City = Names(TEXT("city"));
	TestFalse(TEXT("city not Science Centre"), City.Contains(TEXT("Science Centre Singapore")));
	TestFalse(TEXT("city not NTU"), City.Contains(TEXT("Nanyang Technological University")));
	TestFalse(TEXT("sands not Woodlands"), Names(TEXT("sands")).Contains(TEXT("Woodlands")));
	TestTrue(TEXT("sands finds Marina Bay Sands"), Names(TEXT("sands")).Contains(TEXT("Marina Bay Sands")));
	TestTrue(TEXT("typo finds HarbourFront"), Names(TEXT("harbor frnt")).Contains(TEXT("HarbourFront")));
	{
		// Ordering: the best match must come first, not merely be somewhere in the list.
		const FKioskSearchResult Typo = FKioskSearch::Run(TEXT("harbor frnt"));
		TestTrue(TEXT("harbor frnt has hits"), Typo.Hits.Num() > 0);
		if (Typo.Hits.Num() > 0)
		{
			const FKioskSearchHit& First = Typo.Hits[0];
			const FString FirstName = First.bPlace ? FKioskPlaces::All()[First.Index].Name : FKioskData::Stations()[First.Index].Name;
			TestEqual(TEXT("first hit for \"harbor frnt\" is HarbourFront"), FirstName, FString(TEXT("HarbourFront")));
		}
	}
	return true;
}
#endif
