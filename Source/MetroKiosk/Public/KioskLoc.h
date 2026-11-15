// Language switching: English text is authored with KT(); other languages come from an in-code table
// (Private/KioskLocTable.inl). Every user-facing string goes through KT, so the MetroKiosk.Loc.Coverage test can check it.
#pragma once

#include "CoreMinimal.h"
#include "KioskTypes.h"

struct FKioskTranslation
{
	const TCHAR* English;
	const TCHAR* Chinese;
	const TCHAR* Malay;
	const TCHAR* Tamil;
};

class METROKIOSK_API FKioskLoc
{
public:
	static void SetLanguage(EKioskLanguage InLanguage);
	static EKioskLanguage GetLanguage();
	/** Returns the current-language version of an English source text (falls back to English and records the miss). */
	static FText Tr(const FText& English);
	static FText LanguageName(EKioskLanguage InLanguage);

	/** The whole translation table (for the coverage test). */
	static TConstArrayView<FKioskTranslation> Table();
	static const FKioskTranslation* Find(const FString& English);
	static const TCHAR* Pick(const FKioskTranslation& Entry, EKioskLanguage Language);
	/** English texts that were requested in a non-English language but are missing from the table. */
	static const TSet<FString>& Missing();
	static void ClearMissing();
};

/** Localised text. The English literal is both the NSLOCTEXT key and the source text. */
#define KT(Text) FKioskLoc::Tr(NSLOCTEXT("MetroKiosk", Text, Text))
