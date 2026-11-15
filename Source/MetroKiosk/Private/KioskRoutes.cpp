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

	constexpr int32 MaxServices = 2; // separately-running services per line (East-West has the Changi branch)

	// A node is one station on one service: (Station * NumLines + Line) * MaxServices + Service.
	int32 NodeOf(int32 Station, int32 Line, int32 Service) { return (Station * FKioskData::NumLines + Line) * MaxServices + Service; }
	int32 StationOf(int32 Node) { return Node / (FKioskData::NumLines * MaxServices); }
	int32 LineOf(int32 Node) { return (Node / MaxServices) % FKioskData::NumLines; }

	void BuildLegs(FKioskRoute& Route, const TArray<int32>& Nodes)
	{
		int32 LastNode = INDEX_NONE;
		for (int32 Node : Nodes)
		{
			// Same line and service as the previous node: the ride continues.
			if (!Route.Legs.IsEmpty() && LastNode != INDEX_NONE && (LastNode % (FKioskData::NumLines * MaxServices)) == (Node % (FKioskData::NumLines * MaxServices)))
			{
				Route.Legs.Last().ToStation = StationOf(Node);
			}
			else
			{
				FKioskRouteLeg Leg;
				Leg.Line = LineOf(Node);
				Leg.FromStation = StationOf(Node);
				Leg.ToStation = StationOf(Node);
				Route.Legs.Add(Leg);
			}
			LastNode = Node;
		}
		// A switch on the origin platform before riding leaves an empty first leg.
		Route.Legs.RemoveAll([](const FKioskRouteLeg& Leg) { return Leg.FromStation == Leg.ToStation; });
		Route.Changes = FMath::Max(0, Route.Legs.Num() - 1);
	}

	TArray<FKioskRoute> BuildRoutes()
	{
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		const int32 NumStations = Stations.Num();
		const int32 NumLines = FKioskData::NumLines;
		const int32 NumNodes = NumStations * NumLines * MaxServices;

		TArray<TArray<FEdge>> Adjacency;
		Adjacency.SetNum(NumNodes);
		TArray<bool> Serves; // does the service stop at the station?
		Serves.Init(false, NumNodes);
		for (int32 Line = 0; Line < NumLines; ++Line)
		{
			const TArray<TArray<int32>>& Paths = FKioskData::LinePaths(Line);
			for (int32 PathIndex = 0; PathIndex < Paths.Num(); ++PathIndex)
			{
				const TArray<int32>& Path = Paths[PathIndex];
				const int32 Service = FKioskData::PathService(Line, PathIndex);
				for (int32 Index = 0; Index < Path.Num(); ++Index)
				{
					Serves[NodeOf(Path[Index], Line, Service)] = true;
					if (Index + 1 < Path.Num())
					{
						const int32 A = Path[Index];
						const int32 B = Path[Index + 1];
						const float Distance = FVector2f::Distance(Stations[A].MapPos, Stations[B].MapPos);
						const int32 Minutes = HopMinutes(Distance);
						const float Km = Distance * FKioskRoutes::KmPerMapUnit;
						Adjacency[NodeOf(A, Line, Service)].Add({ NodeOf(B, Line, Service), Minutes, Km });
						Adjacency[NodeOf(B, Line, Service)].Add({ NodeOf(A, Line, Service), Minutes, Km });
					}
				}
			}
		}

		TArray<int32> Cost;
		TArray<float> Km;
		TArray<int32> Prev;
		TArray<bool> Done;
		Cost.Init(MAX_int32, NumNodes);
		Km.Init(0.f, NumNodes);
		Prev.Init(INDEX_NONE, NumNodes);
		Done.Init(false, NumNodes);

		const int32 Origin = FKioskData::CityHallIndex();
		for (int32 Node = Origin * NumLines * MaxServices; Node < (Origin + 1) * NumLines * MaxServices; ++Node)
		{
			if (Serves[Node])
			{
				Cost[Node] = 0;
			}
		}

		// Small graph (about 50 stations x 6 lines x 2): a linear scan for the cheapest open node is plenty.
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

			for (const FEdge& Edge : Adjacency[Best])
			{
				if (Cost[Best] + Edge.Minutes < Cost[Edge.To])
				{
					Cost[Edge.To] = Cost[Best] + Edge.Minutes;
					Km[Edge.To] = Km[Best] + Edge.Km;
					Prev[Edge.To] = Best;
				}
			}
			// Changing to any other service at this station (another line, or another branch of the same line).
			const int32 Station = StationOf(Best);
			for (int32 Other = NodeOf(Station, 0, 0); Other < NodeOf(Station + 1, 0, 0); ++Other)
			{
				if (Other != Best && Serves[Other] && Cost[Best] + FKioskRoutes::TransferMinutes < Cost[Other])
				{
					Cost[Other] = Cost[Best] + FKioskRoutes::TransferMinutes;
					Km[Other] = Km[Best];
					Prev[Other] = Best;
				}
			}
		}

		TArray<FKioskRoute> Result;
		Result.SetNum(NumStations);
		for (int32 Station = 0; Station < NumStations; ++Station)
		{
			int32 Best = INDEX_NONE;
			for (int32 Node = NodeOf(Station, 0, 0); Node < NodeOf(Station + 1, 0, 0); ++Node)
			{
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
			BuildLegs(Route, Nodes);
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
		FText NextLine = LineName(Route.Legs[Index].Line);
		if (Route.Legs[Index].Line == Route.Legs[Index - 1].Line)
		{
			// Same line, different branch: say which way it goes ("East-West Line (to Changi Airport)").
			NextLine = FText::Format(FKioskLoc::Tr(LOCTEXT("LineTo", "{0} (to {1})")),
				NextLine, FKioskPlaces::StationLabel(Stations[Route.Legs[Index].ToStation].Name));
		}
		Text = FText::Format(FKioskLoc::Tr(LOCTEXT("ThenChange", "{0} → change at {1} → {2}")),
			Text, FKioskPlaces::StationLabel(Change.Name), NextLine);
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
