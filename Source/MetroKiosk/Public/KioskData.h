// Static sample data: metro lines, stations, concession passes.
#pragma once

#include "CoreMinimal.h"
#include "KioskTypes.h"

struct FKioskLineInfo
{
	const TCHAR* Code;
	const TCHAR* Name;      // short name used on filter pills, e.g. "North–South"
	const TCHAR* ColorName; // e.g. "Red"
};

struct FKioskStation
{
	FString Name;
	uint8 LineMask = 0;
	FVector2f MapPos = FVector2f::ZeroVector; // schematic map space, about 40..890 x 20..580

	bool OnLine(int32 LineIndex) const { return (LineMask & (1 << LineIndex)) != 0; }
	int32 FirstLine() const;
};

/** One point on a drawn line: a station (Station is its index) or a bend between stations (Station is INDEX_NONE). */
struct FKioskPathPoint
{
	FVector2f Pos = FVector2f::ZeroVector;
	int32 Station = INDEX_NONE;
};

struct FKioskConcession
{
	const TCHAR* Name;
	const TCHAR* Description;
	int32 StudentCents;
	int32 AdultCents;
	bool bBus;
	bool bTrain;
};

/** A sample Metro card the reader can "detect". Card flows read everything card-specific from here. */
struct FKioskCardInfo
{
	EKioskCardType Type;
	const TCHAR* Last4;
	int32 BalanceCents;
	bool bLowBalance;       // shows the "Low balance" banner on card options
	bool bHasActivePass;    // false: no concession pass to renew (T-011 builds the start-date picking on this)
	int32 ActivePassIndex;  // index into Concessions(), INDEX_NONE when there is no active pass
	int32 PassEndDay;       // day of month the active pass ends, 0 when there is none
	int32 PassEndMonth = 11; // month and year of that end day (the sample pass ends 14 Nov 2026)
	int32 PassEndYear = 2026;
};

/** One note or coin the cash slots accept. Label is the face value as printed ("$5", "50¢"). */
struct FKioskCashDenom
{
	int32 Cents;
	bool bCoin;
	const TCHAR* Label;
};

class FKioskData
{
public:
	static constexpr int32 NumLines = 6;
	static const FKioskLineInfo& Line(int32 LineIndex);
	static const TArray<FKioskStation>& Stations();
	static int32 FindStation(const FString& Name);
	static int32 CityHallIndex();
	/** The station this kiosk stands in (English name, "City Hall"). The one place that names it. */
	static const FString& OriginName();
	/** Notes ($2, $5, $10, $50) then coins (10c, 20c, 50c, $1) the cash slots accept, in display order. */
	static const TArray<FKioskCashDenom>& CashDenominations();
	/** True when Cents is the value of an accepted note or coin. */
	static bool IsAcceptedCash(int32 Cents);
	/** The three quick top-up tiles on card options, in cents ($10, $20, $50). */
	static const TArray<int32>& QuickTopUpAmounts();
	/** The preset amounts on the top-up screen, in cents ($10, $20, $30, $50, $100); "More" opens the keypad. */
	static const TArray<int32>& TopUpAmounts();
	/** The fixed "popular from here" shortlist. */
	static const TArray<int32>& PopularStations();
	/** Station-only paths of a line (station indices, in running order; the Circle Line loop repeats its first station at the end). Used for routing. */
	static const TArray<TArray<int32>>& LinePaths(int32 LineIndex);
	/** The same paths as drawn on the schematic map: stations and bend points, so lines follow the system map's corners and curves. */
	static const TArray<TArray<FKioskPathPoint>>& LineShapes(int32 LineIndex);
	/** Length (map units, along the bends) of each hop of LinePaths(LineIndex)[PathIndex]; element i joins stations i and i + 1. */
	static const TArray<float>& HopLengths(int32 LineIndex, int32 PathIndex);
	/** Trains do not run through between services of one line (the East-West Changi branch needs a change at Tanah Merah). */
	static int32 PathService(int32 LineIndex, int32 PathIndex);
	static const TArray<FKioskConcession>& Concessions();
	/** The two sample cards: student (active Bus + Train pass, low balance) and adult (no active pass). */
	static const FKioskCardInfo& Card(EKioskCardType Type);
	/** Pass name and description in the active language. */
	static FText ConcessionName(int32 Index);
	static FText ConcessionDescription(int32 Index);
};
