#include "KioskRoutes.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskSession.h"

#define LOCTEXT_NAMESPACE "KioskRoutes"

namespace
{
	struct FEdge
	{
		int32 To;
		int32 Minutes;
		float Km;
	};

	/** Ride time of one hop: about one minute per 22 map units, never under two. */
	int32 HopMinutes(float MapDistance)
	{
		return FMath::Max(2, FMath::RoundToInt(MapDistance / 22.f));
	}

	void BuildLegs(FKioskRoute& Route, const TArray<int32>& Nodes, int32 NumLines)
	{
		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			const int32 Station = Nodes[Index] / NumLines;
			const int32 Line = Nodes[Index] % NumLines;
			if (Route.Legs.Num() > 0 && Route.Legs.Last().Line == Line)
			{
				Route.Legs.Last().ToStation = Station;
			}
			else
			{
				FKioskRouteLeg Leg;
				Leg.Line = Line;
				Leg.FromStation = Station;
				Leg.ToStation = Station;
				Route.Legs.Add(Leg);
			}
		}
		// A line switch on the origin platform before riding leaves an empty first leg.
		Route.Legs.RemoveAll([](const FKioskRouteLeg& Leg) { return Leg.FromStation == Leg.ToStation; });
		Route.Changes = FMath::Max(0, Route.Legs.Num() - 1);
	}

	TArray<FKioskRoute> BuildRoutes()
	{
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		const int32 NumStations = Stations.Num();
		const int32 NumLines = FKioskData::NumLines;

		// Adjacency[Station * NumLines + Line] lists the hops along that line.
		TArray<TArray<FEdge>> Adjacency;
		Adjacency.SetNum(NumStations * NumLines);
		for (int32 Line = 0; Line < NumLines; ++Line)
		{
			for (const TArray<int32>& Path : FKioskData::LinePaths(Line))
			{
				for (int32 Index = 0; Index + 1 < Path.Num(); ++Index)
				{
					const int32 A = Path[Index];
					const int32 B = Path[Index + 1];
					const float Distance = FVector2f::Distance(Stations[A].MapPos, Stations[B].MapPos);
					const int32 Minutes = HopMinutes(Distance);
					const float Km = Distance * FKioskRoutes::KmPerMapUnit;
					Adjacency[A * NumLines + Line].Add({ B, Minutes, Km });
					Adjacency[B * NumLines + Line].Add({ A, Minutes, Km });
				}
			}
		}

		const int32 NumNodes = NumStations * NumLines;
		TArray<int32> Cost;
		TArray<float> Km;
		TArray<int32> Prev;
		TArray<bool> Done;
		Cost.Init(MAX_int32, NumNodes);
		Km.Init(0.f, NumNodes);
		Prev.Init(INDEX_NONE, NumNodes);
		Done.Init(false, NumNodes);

		const int32 Origin = FKioskData::CityHallIndex();
		for (int32 Line = 0; Line < NumLines; ++Line)
		{
			if (Stations[Origin].OnLine(Line))
			{
				Cost[Origin * NumLines + Line] = 0;
			}
		}

		// Small graph (about 50 stations x 6 lines): a linear scan for the cheapest open node is plenty.
		for (;;)
		{
			int32 Best = INDEX_NONE;
			for (int32 Node = 0; Node < NumNodes; ++Node)
			{
				if (!Done[Node] && Cost[Node] != MAX_int32 && (Best == INDEX_NONE || Cost[Node] < Cost[Best]))
				{
					Best = Node;
				}
			}
			if (Best == INDEX_NONE)
			{
				break;
			}
			Done[Best] = true;
			const int32 Station = Best / NumLines;
			const int32 Line = Best % NumLines;

			for (const FEdge& Edge : Adjacency[Best])
			{
				const int32 Next = Edge.To * NumLines + Line;
				if (Cost[Best] + Edge.Minutes < Cost[Next])
				{
					Cost[Next] = Cost[Best] + Edge.Minutes;
					Km[Next] = Km[Best] + Edge.Km;
					Prev[Next] = Best;
				}
			}
			for (int32 Other = 0; Other < NumLines; ++Other)
			{
				const int32 Next = Station * NumLines + Other;
				if (Other != Line && Stations[Station].OnLine(Other) && Cost[Best] + FKioskRoutes::TransferMinutes < Cost[Next])
				{
					Cost[Next] = Cost[Best] + FKioskRoutes::TransferMinutes;
					Km[Next] = Km[Best];
					Prev[Next] = Best;
				}
			}
		}

		TArray<FKioskRoute> Result;
		Result.SetNum(NumStations);
		for (int32 Station = 0; Station < NumStations; ++Station)
		{
			int32 Best = INDEX_NONE;
			for (int32 Line = 0; Line < NumLines; ++Line)
			{
				const int32 Node = Station * NumLines + Line;
				if (Cost[Node] != MAX_int32 && (Best == INDEX_NONE || Cost[Node] < Cost[Best]))
				{
					Best = Node;
				}
			}
			if (Station == Origin || Best == INDEX_NONE)
			{
				continue;
			}

			FKioskRoute& Route = Result[Station];
			Route.bValid = true;
			Route.Minutes = Cost[Best];
			Route.Km = Km[Best];

			TArray<int32> Nodes;
			for (int32 Node = Best; Node != INDEX_NONE; Node = Prev[Node])
			{
				Nodes.Insert(Node, 0);
			}
			BuildLegs(Route, Nodes, NumLines);
		}
		return Result;
	}
}

const FKioskRoute& FKioskRoutes::FromCityHall(int32 StationIndex)
{
	static const TArray<FKioskRoute> Routes = BuildRoutes();
	static const FKioskRoute Invalid;
	return Routes.IsValidIndex(StationIndex) ? Routes[StationIndex] : Invalid;
}

FText FKioskRoutes::LineName(int32 LineIndex)
{
	switch (LineIndex)
	{
	case 0: return KT("North-South Line");
	case 1: return KT("East-West Line");
	case 2: return KT("North East Line");
	case 3: return KT("Circle Line");
	case 4: return KT("Downtown Line");
	default: return KT("Thomson-East Coast Line");
	}
}

FText FKioskRoutes::Summary(const FKioskRoute& Route)
{
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	if (!Route.bValid || Route.Legs.IsEmpty())
	{
		return FText::GetEmpty();
	}
	FText Text = FText::Format(FKioskLoc::Tr(LOCTEXT("TakeLine", "Take {0}")), LineName(Route.Legs[0].Line));
	for (int32 Index = 1; Index < Route.Legs.Num(); ++Index)
	{
		const FKioskStation& Change = Stations[Route.Legs[Index].FromStation];
		Text = FText::Format(FKioskLoc::Tr(LOCTEXT("ThenChange", "{0} → change at {1} → {2}")),
			Text, FKioskPlaces::StationLabel(Change.Name), LineName(Route.Legs[Index].Line));
	}
	return Text;
}

FText FKioskRoutes::Detail(const FKioskRoute& Route)
{
	if (Route.Changes == 0)
	{
		return FText::Format(FKioskLoc::Tr(LOCTEXT("DetailDirect", "{0} min · direct")), FText::AsNumber(Route.Minutes));
	}
	return FText::Format(FKioskLoc::Tr(LOCTEXT("DetailChanges", "{0} min · {1} {1}|plural(one=change,other=changes)")),
		FText::AsNumber(Route.Minutes), FText::AsNumber(Route.Changes));
}

// --- fares ---------------------------------------------------------------------------------

int32 FKioskFares::OneWayCents(int32 StationIndex)
{
	const FKioskRoute& Route = FKioskRoutes::FromCityHall(StationIndex);
	if (!Route.bValid)
	{
		return BaseCents;
	}
	const int32 ExtraKm = FMath::Max(0, FMath::CeilToInt(Route.Km - FreeKm));
	return FMath::Min(MaxCents, BaseCents + ExtraKm * CentsPerExtraKm);
}

int32 FKioskFares::AdultCents(ETicketType Ticket, int32 StationIndex)
{
	switch (Ticket)
	{
	case ETicketType::DayPass: return DayPassCents;
	case ETicketType::RoundTrip: return RoundTripFromOneWay(OneWayCents(StationIndex));
	default: return OneWayCents(StationIndex);
	}
}

int32 FKioskFares::MinOneWayCents()
{
	static const int32 Value = []()
	{
		int32 Min = MAX_int32;
		for (int32 Index = 0; Index < FKioskData::Stations().Num(); ++Index)
		{
			if (FKioskRoutes::FromCityHall(Index).bValid)
			{
				Min = FMath::Min(Min, OneWayCents(Index));
			}
		}
		return Min;
	}();
	return Value;
}

int32 FKioskFares::MaxOneWayCents()
{
	static const int32 Value = []()
	{
		int32 Max = 0;
		for (int32 Index = 0; Index < FKioskData::Stations().Num(); ++Index)
		{
			if (FKioskRoutes::FromCityHall(Index).bValid)
			{
				Max = FMath::Max(Max, OneWayCents(Index));
			}
		}
		return Max;
	}();
	return Value;
}

#undef LOCTEXT_NAMESPACE
