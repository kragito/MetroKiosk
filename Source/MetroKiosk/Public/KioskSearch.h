// Forgiving destination search over places and stations: case, spaces and punctuation are ignored,
// prefixes and substrings match, aliases and translated names match, and small typos are tolerated.
#pragma once

#include "CoreMinimal.h"

struct FKioskSearchHit
{
	bool bPlace = false;  // true: index into FKioskPlaces::All(); false: index into FKioskData::Stations()
	int32 Index = INDEX_NONE;
	int32 Score = 0;
};

struct FKioskSearchResult
{
	TArray<FKioskSearchHit> Hits;
	/** True when nothing matched well and Hits holds the closest "Did you mean" suggestions instead. */
	bool bSuggestions = false;
	/** With bSuggestions: true when the suggestions are genuinely close to the text, false when they are just popular places. */
	bool bClose = false;
};

class METROKIOSK_API FKioskSearch
{
public:
	/** Ranked results for what the user has typed. Empty query gives an empty result. Never returns nothing for a non-empty query. */
	static FKioskSearchResult Run(const FString& Query);
	/** Lower-case, drops spaces and punctuation, keeps non-ASCII letters. */
	static FString Normalize(const FString& Text);
	/** Optimal string alignment (Damerau-Levenshtein with adjacent swaps). */
	static int32 EditDistance(const FString& A, const FString& B);

	static constexpr int32 MaxResults = 12;
	static constexpr int32 MaxSuggestions = 4;
};
