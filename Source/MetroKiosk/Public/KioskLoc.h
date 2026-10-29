// Language switching: English text is authored with NSLOCTEXT via KT(); other languages come from an in-code table.
#pragma once

#include "CoreMinimal.h"
#include "KioskTypes.h"

class FKioskLoc
{
public:
	static void SetLanguage(EKioskLanguage InLanguage);
	static EKioskLanguage GetLanguage();
	/** Returns the current-language version of an English source text (falls back to English). */
	static FText Tr(const FText& English);
	static FText LanguageName(EKioskLanguage InLanguage);
};

/** Localised text. The English literal is both the NSLOCTEXT key and the source text. */
#define KT(Text) FKioskLoc::Tr(NSLOCTEXT("MetroKiosk", Text, Text))
