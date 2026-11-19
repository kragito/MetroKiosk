#include "KioskLoc.h"

namespace
{
	EKioskLanguage GLanguage = EKioskLanguage::English;
	TSet<FString> GMissing;

	const FKioskTranslation GTable[] = {
#include "KioskLocTable.inl"
	};

	// Keys are matched case-sensitively; FString's default map compare is case-insensitive.
	struct FCaseSensitiveKeyFuncs : BaseKeyFuncs<TPair<FString, const FKioskTranslation*>, FString, false>
	{
		static const FString& GetSetKey(const TPair<FString, const FKioskTranslation*>& Element) { return Element.Key; }
		static bool Matches(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
		static uint32 GetKeyHash(const FString& Key) { return FCrc::StrCrc32(*Key); }
	};
	using FIndexMap = TMap<FString, const FKioskTranslation*, FDefaultSetAllocator, FCaseSensitiveKeyFuncs>;

	const FIndexMap& Index()
	{
		static FIndexMap Map;
		if (Map.IsEmpty())
		{
			for (const FKioskTranslation& Entry : GTable)
			{
				Map.Add(Entry.English, &Entry);
			}
		}
		return Map;
	}
}

void FKioskLoc::SetLanguage(EKioskLanguage InLanguage)
{
	GLanguage = InLanguage;
}

EKioskLanguage FKioskLoc::GetLanguage()
{
	return GLanguage;
}

TConstArrayView<FKioskTranslation> FKioskLoc::Table()
{
	return MakeArrayView(GTable);
}

const FKioskTranslation* FKioskLoc::Find(const FString& English)
{
	const FKioskTranslation* const* Found = Index().Find(English);
	return Found ? *Found : nullptr;
}

const TCHAR* FKioskLoc::Pick(const FKioskTranslation& Entry, EKioskLanguage Language)
{
	switch (Language)
	{
	case EKioskLanguage::Chinese: return Entry.Chinese;
	case EKioskLanguage::Malay: return Entry.Malay;
	case EKioskLanguage::Tamil: return Entry.Tamil;
	default: return Entry.English;
	}
}

const TSet<FString>& FKioskLoc::Missing()
{
	return GMissing;
}

void FKioskLoc::ClearMissing()
{
	GMissing.Reset();
}

FText FKioskLoc::Tr(const FText& English)
{
	if (GLanguage == EKioskLanguage::English)
	{
		return English;
	}
	const FString Key = English.ToString();
	if (const FKioskTranslation* Entry = Find(Key))
	{
		const TCHAR* Text = Pick(*Entry, GLanguage);
		if (Text && *Text)
		{
			return FText::FromString(Text);
		}
	}
	GMissing.Add(Key);
	return English;
}

FText FKioskLoc::LanguageName(EKioskLanguage InLanguage)
{
	switch (InLanguage)
	{
	case EKioskLanguage::Chinese: return FText::FromString(TEXT("中文"));
	case EKioskLanguage::Malay: return FText::FromString(TEXT("Melayu"));
	case EKioskLanguage::Tamil: return FText::FromString(TEXT("தமிழ்"));
	default: return NSLOCTEXT("MetroKiosk", "LangEnglish", "English");
	}
}
