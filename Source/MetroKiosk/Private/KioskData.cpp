#include "KioskData.h"
#include "KioskLoc.h"

int32 FKioskStation::FirstLine() const
{
	for (int32 Index = 0; Index < FKioskData::NumLines; ++Index)
	{
		if (OnLine(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const FKioskLineInfo& FKioskData::Line(int32 LineIndex)
{
	static const FKioskLineInfo Lines[NumLines] = {
		{ TEXT("NS"), TEXT("North–South"), TEXT("Red") },
		{ TEXT("EW"), TEXT("East–West"), TEXT("Green") },
		{ TEXT("NE"), TEXT("North East"), TEXT("Purple") },
		{ TEXT("CC"), TEXT("Circle"), TEXT("Orange") },
		{ TEXT("DT"), TEXT("Downtown"), TEXT("Blue") },
		{ TEXT("TE"), TEXT("Thomson"), TEXT("Brown") },
	};
	return Lines[FMath::Clamp(LineIndex, 0, NumLines - 1)];
}

namespace
{
	enum : uint8 { NS = 1, EW = 2, NE = 4, CC = 8, DT = 16, TE = 32 };

	struct FStationRow
	{
		const TCHAR* Name;
		uint8 Lines;
		float X;
		float Y;
	};

	const FStationRow GRows[] = {
		{ TEXT("Woodlands North"), TE, 250, 30 },
		{ TEXT("Woodlands"), NS | TE, 300, 70 },
		{ TEXT("Marsiling"), NS, 240, 70 },
		{ TEXT("Yishun"), NS, 330, 130 },
		{ TEXT("Ang Mo Kio"), NS, 350, 200 },
		{ TEXT("Bishan"), NS | CC, 350, 260 },
		{ TEXT("Marymount"), CC, 330, 292 },
		{ TEXT("Caldecott"), CC | TE, 295, 322 },
		{ TEXT("Botanic Gardens"), CC | DT, 265, 372 },
		{ TEXT("Newton"), NS | DT, 385, 345 },
		{ TEXT("Orchard"), NS | TE, 400, 392 },
		{ TEXT("Dhoby Ghaut"), NS | NE | CC, 440, 425 },
		{ TEXT("Little India"), NE | DT, 470, 380 },
		{ TEXT("Bugis"), EW | DT, 560, 420 },
		{ TEXT("City Hall"), NS | EW, 485, 465 },
		{ TEXT("Raffles Place"), NS | EW, 520, 495 },
		{ TEXT("Marina Bay"), NS | CC | TE, 560, 525 },
		{ TEXT("Marina South Pier"), NS, 595, 555 },
		{ TEXT("Promenade"), CC | DT, 562, 458 },
		{ TEXT("Outram Park"), EW | NE, 400, 512 },
		{ TEXT("Chinatown"), NE | DT, 450, 488 },
		{ TEXT("HarbourFront"), NE | CC, 330, 538 },
		{ TEXT("Hume"), DT, 215, 205 },
		{ TEXT("Beauty World"), DT, 235, 262 },
		{ TEXT("Bukit Panjang"), DT, 205, 150 },
		{ TEXT("Buona Vista"), EW | CC, 250, 442 },
		{ TEXT("Jurong East"), NS | EW, 150, 412 },
		{ TEXT("Boon Lay"), EW, 60, 412 },
		{ TEXT("Serangoon"), NE | CC, 600, 300 },
		{ TEXT("Sengkang"), NE, 700, 215 },
		{ TEXT("Punggol"), NE, 750, 150 },
		{ TEXT("Paya Lebar"), EW | CC, 670, 360 },
		{ TEXT("MacPherson"), CC, 640, 330 },
		{ TEXT("Tanah Merah"), EW, 780, 290 },
		{ TEXT("Tampines"), EW, 820, 235 },
		{ TEXT("Pasir Ris"), EW, 860, 175 },
		{ TEXT("Expo"), EW, 820, 335 },
		{ TEXT("Changi Airport"), EW, 895, 355 },
		{ TEXT("Gardens by the Bay"), TE, 625, 545 },
		{ TEXT("Marine Parade"), TE, 740, 497 },
		{ TEXT("Marine Terrace"), TE, 810, 462 },
		{ TEXT("Bayshore"), TE, 880, 430 },
		{ TEXT("Bayfront"), CC | DT, 628, 490 },
		{ TEXT("Kent Ridge"), CC, 270, 490 },
		{ TEXT("Khatib"), NS, 338, 165 },
		{ TEXT("Novena"), NS, 372, 305 },
		{ TEXT("Clarke Quay"), NE, 452, 455 },
		{ TEXT("Esplanade"), CC, 515, 435 },
		{ TEXT("Simei"), EW, 800, 262 },
		{ TEXT("Bukit Batok"), NS, 125, 335 },
		{ TEXT("Choa Chu Kang"), NS, 128, 225 },
	};

	TArray<FKioskStation> BuildStations()
	{
		TArray<FKioskStation> Result;
		for (const FStationRow& Row : GRows)
		{
			FKioskStation Station;
			Station.Name = Row.Name;
			Station.LineMask = Row.Lines;
			Station.MapPos = FVector2f(Row.X, Row.Y);
			Result.Add(Station);
		}
		return Result;
	}

	TArray<TArray<int32>> BuildPaths(int32 LineIndex)
	{
		auto Path = [](std::initializer_list<const TCHAR*> Names)
		{
			TArray<int32> Out;
			for (const TCHAR* Name : Names)
			{
				const int32 Index = FKioskData::FindStation(Name);
				check(Index != INDEX_NONE);
				Out.Add(Index);
			}
			return Out;
		};

		TArray<TArray<int32>> Result;
		switch (LineIndex)
		{
		case 0:
			Result.Add(Path({ TEXT("Jurong East"), TEXT("Bukit Batok"), TEXT("Choa Chu Kang"), TEXT("Marsiling"), TEXT("Woodlands"), TEXT("Yishun"), TEXT("Khatib"), TEXT("Ang Mo Kio"), TEXT("Bishan"), TEXT("Novena"), TEXT("Newton"), TEXT("Orchard"), TEXT("Dhoby Ghaut"), TEXT("City Hall"), TEXT("Raffles Place"), TEXT("Marina Bay"), TEXT("Marina South Pier") }));
			break;
		case 1:
			Result.Add(Path({ TEXT("Boon Lay"), TEXT("Jurong East"), TEXT("Buona Vista"), TEXT("Outram Park"), TEXT("Raffles Place"), TEXT("City Hall"), TEXT("Bugis"), TEXT("Paya Lebar"), TEXT("Tanah Merah"), TEXT("Simei"), TEXT("Tampines"), TEXT("Pasir Ris") }));
			Result.Add(Path({ TEXT("Tanah Merah"), TEXT("Expo"), TEXT("Changi Airport") }));
			break;
		case 2:
			Result.Add(Path({ TEXT("HarbourFront"), TEXT("Outram Park"), TEXT("Chinatown"), TEXT("Clarke Quay"), TEXT("Dhoby Ghaut"), TEXT("Little India"), TEXT("Serangoon"), TEXT("Sengkang"), TEXT("Punggol") }));
			break;
		case 3:
			Result.Add(Path({ TEXT("Marina Bay"), TEXT("Bayfront"), TEXT("Promenade"), TEXT("Paya Lebar"), TEXT("MacPherson"), TEXT("Serangoon"), TEXT("Bishan"), TEXT("Marymount"), TEXT("Caldecott"), TEXT("Botanic Gardens"), TEXT("Buona Vista"), TEXT("Kent Ridge"), TEXT("HarbourFront"), TEXT("Marina Bay") }));
			Result.Add(Path({ TEXT("Promenade"), TEXT("Esplanade"), TEXT("Dhoby Ghaut") }));
			break;
		case 4:
			Result.Add(Path({ TEXT("Bukit Panjang"), TEXT("Hume"), TEXT("Beauty World"), TEXT("Botanic Gardens"), TEXT("Newton"), TEXT("Little India"), TEXT("Bugis"), TEXT("Promenade"), TEXT("Bayfront"), TEXT("Chinatown") }));
			break;
		default:
			Result.Add(Path({ TEXT("Woodlands North"), TEXT("Woodlands"), TEXT("Caldecott"), TEXT("Orchard"), TEXT("Marina Bay"), TEXT("Gardens by the Bay"), TEXT("Marine Parade"), TEXT("Marine Terrace"), TEXT("Bayshore") }));
			break;
		}
		return Result;
	}
}

const TArray<FKioskStation>& FKioskData::Stations()
{
	static const TArray<FKioskStation> Data = BuildStations();
	return Data;
}

int32 FKioskData::FindStation(const FString& Name)
{
	static TMap<FString, int32> Lookup;
	if (Lookup.IsEmpty())
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(GRows); ++Index)
		{
			Lookup.Add(GRows[Index].Name, Index);
		}
	}
	const int32* Found = Lookup.Find(Name);
	return Found ? *Found : INDEX_NONE;
}

int32 FKioskData::CityHallIndex()
{
	static const int32 Index = FindStation(TEXT("City Hall"));
	return Index;
}

const TArray<int32>& FKioskData::PopularStations()
{
	static const TArray<int32> Data = {
		FindStation(TEXT("HarbourFront")), FindStation(TEXT("Bayfront")), FindStation(TEXT("Changi Airport")),
		FindStation(TEXT("Orchard")), FindStation(TEXT("Chinatown")) };
	return Data;
}

int32 FKioskData::PathService(int32 LineIndex, int32 PathIndex)
{
	// The Circle Line's Promenade - Esplanade - Dhoby Ghaut spur is part of the same service as the loop.
	return (LineIndex == 3) ? 0 : PathIndex;
}

const TArray<TArray<int32>>& FKioskData::LinePaths(int32 LineIndex)
{
	static TArray<TArray<TArray<int32>>> Cache;
	if (Cache.IsEmpty())
	{
		for (int32 Index = 0; Index < NumLines; ++Index)
		{
			Cache.Add(BuildPaths(Index));
		}
	}
	return Cache[FMath::Clamp(LineIndex, 0, NumLines - 1)];
}

const TArray<FKioskConcession>& FKioskData::Concessions()
{
	static const TArray<FKioskConcession> Data = {
		{ TEXT("Bus"), TEXT("All bus services"), 2900, 9000, true, false },
		{ TEXT("Train"), TEXT("All MRT and LRT lines"), 2900, 9000, false, true },
		{ TEXT("Bus + Train"), TEXT("Every bus and train"), 5500, 12800, true, true },
	};
	return Data;
}

FText FKioskData::ConcessionName(int32 Index)
{
	switch (Index)
	{
	case 0: return KT("Bus");
	case 1: return KT("Train");
	default: return KT("Bus + Train");
	}
}

FText FKioskData::ConcessionDescription(int32 Index)
{
	switch (Index)
	{
	case 0: return KT("All bus services");
	case 1: return KT("All MRT and LRT lines");
	default: return KT("Every bus and train");
	}
}
