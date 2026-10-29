// Static sample data: metro lines, stations, concession passes.
#pragma once

#include "CoreMinimal.h"

struct FKioskLineInfo
{
	const TCHAR* Code;
	const TCHAR* Name;      // short name used on filter pills, e.g. "North–South"
	const TCHAR* FullName;  // e.g. "North-South Line"
	const TCHAR* ColorName; // e.g. "Red"
};

struct FKioskStation
{
	FString Name;
	uint8 LineMask = 0;
	FVector2f MapPos = FVector2f::ZeroVector; // schematic map space, 0..1000 x 0..600
	int32 Minutes = 0;
	int32 Changes = 0;
	int32 FareCents = 168;

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
	static const TArray<FKioskConcession>& Concessions();
};
