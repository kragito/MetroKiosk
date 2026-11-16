// "Where are you going?": popular destinations on the left; trip card, search, line filter, map button and
// Confirm on the right (keyboard replaces the filter and map button while typing).
#include "KioskScreens.h"

#include "DestinationWidgets.h"
#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "KioskSearch.h"
#include "Widgets/Components/KioskAutomation.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	constexpr int32 MaxRowsWithoutScroll = 5;

	/** Scrollable result list; named "Station list" so scripted tests can scroll it. */
	TSharedRef<SWidget> ScrollableList(const TSharedRef<SWidget>& Rows)
	{
		TSharedRef<SScrollBox> Box = SNew(SScrollBox).ScrollBarVisibility(EVisibility::Collapsed) + SScrollBox::Slot()[ Rows ];
		TSharedRef<FKioskAutomationMeta> Meta = MakeShared<FKioskAutomationMeta>();
		Meta->Name = TEXT("Station list");
		Meta->Label = TEXT("Station list");
		Box->AddMetadata(Meta);
		return Box;
	}

	/** Popular destinations (or every station on the chosen line) when nothing is typed; otherwise ranked matches, or "Did you mean" suggestions. */
	TSharedRef<SWidget> ResultsPanel(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const FString& Query = Session->State.SearchText;
		const int32 Filter = Session->State.LineFilter;
		const bool bFiltered = Filter != INDEX_NONE;
		const bool bTyped = !FKioskSearch::Normalize(Query).IsEmpty();
		const FKioskSearchResult Result = FKioskSearch::Run(Query, Filter);

		FText Heading;
		TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
		int32 Count = 0;
		auto AddRow = [&Rows, &Count](const TSharedRef<SWidget>& Tile)
		{
			Rows->AddSlot().AutoHeight().Padding(0.f, Count == 0 ? 0.f : 8.f, 0.f, 0.f)[ Tile ];
			++Count;
		};

		if (!bTyped)
		{
			if (bFiltered)
			{
				// Every other station on the line, nearest first.
				TArray<int32> OnLine;
				for (int32 Index = 0; Index < FKioskData::Stations().Num(); ++Index)
				{
					if (Index != FKioskData::CityHallIndex() && FKioskData::Stations()[Index].OnLine(Filter))
					{
						OnLine.Add(Index);
					}
				}
				OnLine.StableSort([](int32 A, int32 B) { return FKioskRoutes::FromCityHall(A).Minutes < FKioskRoutes::FromCityHall(B).Minutes; });
				for (int32 Station : OnLine)
				{
					AddRow(KioskDest::StationTile(Session, Station));
				}
			}
			else
			{
				Heading = KT("Popular from City Hall");
				for (int32 Station : FKioskData::PopularStations())
				{
					AddRow(KioskDest::StationTile(Session, Station));
				}
			}
		}
		else
		{
			if (!Result.bSuggestions)
			{
				Heading = FText::Format(KT("Results for “{0}”"), FText::FromString(Query));
			}
			else if (Result.bClose)
			{
				Heading = FText::Format(KT("No exact match for “{0}”. Did you mean…"), FText::FromString(Query));
			}
			else
			{
				Heading = FText::Format(KT("No match for “{0}”. Try one of these popular places"), FText::FromString(Query));
			}
			for (const FKioskSearchHit& Hit : Result.Hits)
			{
				AddRow(Hit.bPlace ? KioskDest::PlaceTile(Session, Hit.Index) : KioskDest::StationTile(Session, Hit.Index));
			}
			if (Count == 0 && bFiltered)
			{
				Heading = FText::Format(KT("Nothing on the {0} matches “{1}”. Clear the line or change what you typed."),
					FKioskRoutes::LineName(Filter), FText::FromString(Query));
			}
		}

		TSharedRef<SVerticalBox> Top = SNew(SVerticalBox);
		if (bFiltered && !bTyped)
		{
			Heading = FText::Format(KT("Filtered by {0}"), FKioskRoutes::LineName(Filter));
		}
		if (bFiltered)
		{
			Top->AddSlot().AutoHeight().Padding(4.f, 0.f, 0.f, bTyped ? 8.f : 12.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ LineBadge(Filter) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
				[
					Text(bTyped ? FText::Format(KT("Filtered by {0}"), FKioskRoutes::LineName(Filter)) : Heading, KioskType::Body, true, C.Ink)
				]
			];
		}
		if (bTyped || !bFiltered)
		{
			Top->AddSlot().AutoHeight().Padding(4.f, 0.f, 0.f, 12.f)[ TextWrap(Heading, KioskType::Body, true, C.Ink) ];
		}

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Top ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				// Short lists need no scrolling (and stay visible to UI automation); long ones scroll.
				Count > MaxRowsWithoutScroll ? ScrollableList(Rows) : StaticCastSharedRef<SWidget>(Rows)
			];
	}
}

void SDestinationScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bKeyboard = Session->State.bKeyboardOpen;

	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
	Right->AddSlot().AutoHeight()[ KioskDest::TripCard(Session) ];
	Right->AddSlot().AutoHeight().Padding(4.f, 28.f, 0.f, 10.f)[ Text(KT("Somewhere else?"), KioskType::Body, true, C.Ink) ];
	Right->AddSlot().AutoHeight()[ KioskDest::SearchField(Session) ];
	if (bKeyboard)
	{
		Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::KeyboardWidget(Session) ];
	}
	else
	{
		Right->AddSlot().AutoHeight().Padding(4.f, 8.f, 0.f, 0.f)
		[
			TextWrap(KT("A keyboard will popup. The list narrows with every letter you type."), KioskType::Caption, false, C.Muted, 1.1f)
		];
		Right->AddSlot().AutoHeight().Padding(4.f, 26.f, 0.f, 10.f)[ Text(KT("Filter by line"), KioskType::Body, true, C.Ink) ];
		Right->AddSlot().AutoHeight()[ KioskDest::LineFilterGrid(Session) ];
		Right->AddSlot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Open metro map view"))
			.ButtonTag(TEXT("Open metro map view"))
			.LeftIcon(EKioskIcon::Map)
			.RightIcon(EKioskIcon::ArrowRight)
			.MinWidth(340.f).MaxWidth(620.f)
			.OnClicked_Lambda([Session]() { Session->ShowDestinationMap(); return FReply::Handled(); })
		];
	}
	Right->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::ConfirmButton(Session) ];

	ChildSlot
	.Padding(KioskLayout::PageX, 24.f, KioskLayout::PageX, 22.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 48.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 20.f)[ TextWrap(KT("Where are you going?"), KioskType::H1, true, C.Ink) ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ ResultsPanel(Session) ]
		]
		+ SHorizontalBox::Slot().FillWidth(0.5f)
		[
			Right
		]
	];
}
