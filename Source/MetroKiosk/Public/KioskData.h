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
	FVector2f MapPos = FVector2f::ZeroVector; // schematic map space, 0..1000 x 0..600

	bool OnLine(int32 LineIndex) const { return (LineMask & (1 << LineIndex)) != 0; }
	int32 FirstLine() const;
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
	int32 PassEndDay;       // active pass end date (November 2026), 0 when there is none
};

class FKioskData
{
public:
	static constexpr int32 NumLines = 6;
	static const FKioskLineInfo& Line(int32 LineIndex);
	static const TArray<FKioskStation>& Stations();
	static int32 FindStation(const FString& Name);
	static int32 CityHallIndex();
	/** The fixed "popular from here" shortlist. */
	static const TArray<int32>& PopularStations();
	/** Poly-line segments (station indices) used to draw a line on the schematic map. */
	static const TArray<TArray<int32>>& LinePaths(int32 LineIndex);
	/** Trains do not run through between services of one line (the East-West Changi branch needs a change at Tanah Merah). */
	static int32 PathService(int32 LineIndex, int32 PathIndex);
	static const TArray<FKioskConcession>& Concessions();
	/** The two sample cards: student (active Bus + Train pass, low balance) and adult (no active pass). */
	static const FKioskCardInfo& Card(EKioskCardType Type);
	/** Pass name and description in the active language. */
	static FText ConcessionName(int32 Index);
	static FText ConcessionDescription(int32 Index);
};
