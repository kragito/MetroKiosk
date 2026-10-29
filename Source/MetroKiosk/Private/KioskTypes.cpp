#include "KioskTypes.h"

namespace KioskScreenNames
{
	static const TCHAR* const Names[] = {
		TEXT("Home"), TEXT("TicketType"), TEXT("Destination"), TEXT("DestinationMap"), TEXT("Review"),
		TEXT("PayMethod"), TEXT("WaitTapCard"), TEXT("WaitChipCard"), TEXT("WaitCash"), TEXT("Processing"),
		TEXT("Receipt"), TEXT("HelpCall"), TEXT("ScanCard"), TEXT("CardOptions"), TEXT("TopUpAmount"),
		TEXT("Concession"), TEXT("PassDates"), TEXT("DoneLiftCard") };
	static_assert(UE_ARRAY_COUNT(Names) == static_cast<int32>(EKioskScreen::Count), "Screen name table out of date");

	FString ToString(EKioskScreen Screen)
	{
		return Names[static_cast<int32>(Screen)];
	}

	bool TryParse(const FString& Name, EKioskScreen& OutScreen)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
		{
			if (Name.Equals(Names[Index], ESearchCase::IgnoreCase))
			{
				OutScreen = static_cast<EKioskScreen>(Index);
				return true;
			}
		}
		return false;
	}
}
