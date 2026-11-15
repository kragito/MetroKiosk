// Places (attractions, malls, hospitals, ...) that map to their nearest station, plus translated names and search aliases.
#pragma once

#include "CoreMinimal.h"

enum class EKioskPlaceCategory : uint8
{
	Attraction,
	Shopping,
	Hospital,
	Airport,
	Education,
	Food,
	Area,
};

enum class EKioskIcon : uint8;

struct FKioskPlace
{
	FString Name;
	EKioskPlaceCategory Category = EKioskPlaceCategory::Attraction;
	int32 Station = INDEX_NONE;   // nearest station (index into FKioskData::Stations())
	TArray<FString> Keywords;     // alternate names and things found there; searched like aliases
};

/** An alternate spelling or nickname for a station or place ("harbor front" -> HarbourFront). */
struct FKioskAlias
{
	FString Alias;
	FString Target; // English station or place name
};

class METROKIOSK_API FKioskPlaces
{
public:
	static const TArray<FKioskPlace>& All();
	static int32 FindPlace(const FString& Name);
	static const TArray<FKioskAlias>& Aliases();

	static FText CategoryLabel(EKioskPlaceCategory Category);
	static EKioskIcon CategoryIcon(EKioskPlaceCategory Category);

	/** The name in the active non-English language, or an empty string (English active, or no translation). */
	static FString LocalName(const FString& EnglishName);
	/** The name in a specific language (0 Chinese, 1 Malay, 2 Tamil); empty when there is none. */
	static FString LocalNameIn(const FString& EnglishName, int32 LanguageIndex);
	/** "Bayfront" in English, "Bayfront (海湾舫)" in other languages. */
	static FText StationLabel(const FString& EnglishName);
	/** What is at a popular station ("VivoCity · Sentosa", translated), or empty text. */
	static FText Blurb(int32 StationIndex);
};
