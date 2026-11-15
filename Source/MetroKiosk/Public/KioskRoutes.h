// Route finding over the line graph (from the kiosk's own station, City Hall) and the fare rules derived from it.
#pragma once

#include "CoreMinimal.h"
#include "KioskTypes.h"

/** One ride on a single line between two stations (indices into FKioskData::Stations()). */
struct FKioskRouteLeg
{
	int32 Line = INDEX_NONE;
	int32 FromStation = INDEX_NONE;
	int32 ToStation = INDEX_NONE;
};

struct FKioskRoute
{
	bool bValid = false;
	int32 Minutes = 0;   // riding time plus transfer penalties
	int32 Changes = 0;   // number of line changes
	float Km = 0.f;      // distance ridden, used by the fare rule
	TArray<FKioskRouteLeg> Legs;
};

class METROKIOSK_API FKioskRoutes
{
public:
	/** Walking and waiting time added for every line change, in minutes. */
	static constexpr int32 TransferMinutes = 5;
	/** Real-world kilometres per unit of the schematic map. */
	static constexpr float KmPerMapUnit = 0.05f;

	/** Fastest route from City Hall to the station (Dijkstra over station/line nodes). Invalid for City Hall itself. */
	static const FKioskRoute& FromCityHall(int32 StationIndex);

	/** "Take East-West Line → change at Outram Park → North East Line" (translated). */
	static FText Summary(const FKioskRoute& Route);
	/** "13 min · 1 change" or "7 min · direct" (translated). */
	static FText Detail(const FKioskRoute& Route);
	/** Full line name, translated ("Circle Line"). */
	static FText LineName(int32 LineIndex);
};

/** Single source of truth for prices. All amounts are integer cents. */
class METROKIOSK_API FKioskFares
{
public:
	static constexpr int32 BaseCents = 128;       // everything up to FreeKm
	static constexpr int32 MaxCents = 257;
	static constexpr float FreeKm = 3.2f;
	static constexpr int32 CentsPerExtraKm = 6;
	static constexpr int32 DayPassCents = 800;
	static constexpr int32 DayPassReducedCents = 560;

	/** Adult one-way fare from City Hall to the station. */
	static int32 OneWayCents(int32 StationIndex);
	/** Adult fare for a ticket type (round trip = 2x one-way less 30%). Day pass ignores the station. */
	static int32 AdultCents(ETicketType Ticket, int32 StationIndex);
	/** Round trip = twice the one-way fare less 30%, rounded to the nearest cent. */
	static int32 RoundTripFromOneWay(int32 OneWayCents) { return (OneWayCents * 2 * 7 + 5) / 10; }
	/** Child and senior fare: 70% of the adult fare, rounded to the nearest cent. */
	static int32 ReducedCents(int32 AdultCents) { return (AdultCents * 7 + 5) / 10; }
	/** Lowest and highest one-way fare over every destination. */
	static int32 MinOneWayCents();
	static int32 MaxOneWayCents();
};
