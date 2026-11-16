// Schematic metro map: tap a station to choose it, or type a name to light up the matches. Right column shows the
// step-by-step route; the line highlighter here is an optional tool.
#include "KioskScreens.h"

#include "DestinationWidgets.h"
#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "KioskSearch.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskMap.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	/** Optional tool: tap a line to highlight it on the map; tap it again to show every line. */
	TSharedRef<SWidget> LineHighlighter(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		TSharedRef<SHorizontalBox> Badges = SNew(SHorizontalBox);
		for (int32 Line = 0; Line < FKioskData::NumLines; ++Line)
		{
			const bool bActive = Session->State.LineFilter == Line;
			Badges->AddSlot().AutoWidth().Padding(Line == 0 ? 0.f : 10.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(60.f)
				[
					SNew(SKioskButton)
					.Kind(bActive ? EKioskButtonKind::Tint : EKioskButtonKind::OutlineMuted)
					.Label(FKioskRoutes::LineName(Line))
					.ButtonTag(*FString::Printf(TEXT("Filter %s"), FKioskData::Line(Line).Code))
					.Padding(FMargin(12.f, 0.f))
					.OnClicked_Lambda([Session, Line]() { Session->SetLineFilter(Line); return FReply::Handled(); })
					[
						SNew(SBox).VAlign(VAlign_Center)[ LineBadge(Line) ]
					]
				]
			];
		}
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Highlight one line (optional)"), KioskType::Caption, true, C.Muted) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ Badges ];
	}

	/** Stations that match the typed text (a place counts for the station it is at). Empty when nothing is typed or nothing matches. */
	TSet<int32> MatchingStations(const FKioskState& State)
	{
		TSet<int32> Lit;
		if (FKioskSearch::Normalize(State.SearchText).IsEmpty())
		{
			return Lit;
		}
		const FKioskSearchResult Result = FKioskSearch::Run(State.SearchText, State.LineFilter, MAX_int32);
		if (Result.bSuggestions)
		{
			return Lit; // nothing really matched: only "did you mean" guesses
		}
		for (const FKioskSearchHit& Hit : Result.Hits)
		{
			const int32 Station = Hit.bPlace ? FKioskPlaces::All()[Hit.Index].Station : Hit.Index;
			if (FKioskData::Stations().IsValidIndex(Station))
			{
				Lit.Add(Station);
			}
		}
		return Lit;
	}

	/** "No station selected" prompt, or From / To, time and one line per leg of the route. */
	TSharedRef<SWidget> RouteCard(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const FKioskState& State = Session->State;
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		if (!Stations.IsValidIndex(State.Destination))
		{
			return Card(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("No station selected"), KioskType::H3, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
				[
					TextWrap(KT("Tap a station on the map to see the route, or find it by name below."), KioskType::Body, false, C.Muted, 1.25f)
				],
				C.HelpTint, C.BorderStrong, 2.f, FMargin(28.f, 24.f));
		}

		const FKioskRoute& Route = FKioskRoutes::FromCityHall(State.Destination);
		const FKioskStation& Here = Stations[FKioskData::CityHallIndex()];
		const FKioskStation& There = Stations[State.Destination];

		auto EndRow = [&C](const FText& Label, const FKioskStation& Station) -> TSharedRef<SWidget>
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(Label, KioskType::Caption, false, C.Muted) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14.f, 0.f, 12.f, 0.f)
				[
					TextWrap(FKioskPlaces::StationLabel(Station.Name), KioskType::H3, true, C.Ink)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ LineBadges(Station.LineMask) ];
		};

		const FText About = Route.Changes == 0
			? FText::Format(KT("About {0} min · direct"), FText::AsNumber(Route.Minutes))
			: FText::Format(KT("About {0} min · {1} {1}|plural(one=change,other=changes)"), FText::AsNumber(Route.Minutes), Route.Changes);

		TSharedRef<SVerticalBox> Legs = SNew(SVerticalBox);
		for (int32 Index = 0; Index < Route.Legs.Num(); ++Index)
		{
			Legs->AddSlot().AutoHeight().Padding(0.f, Index == 0 ? 0.f : 12.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 2.f, 14.f, 0.f)[ LineBadge(Route.Legs[Index].Line) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					TextWrap(FKioskRoutes::LegText(Route, Index), KioskType::Body, true, C.Ink, 1.2f)
				]
			];
		}

		return Card(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ EndRow(KT("From"), Here) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ EndRow(KT("To"), There) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 12.f)[ Rule(C.BorderStrong, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ TextWrap(About, KioskType::Body, false, C.Muted) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
				[
					Text(FKioskSession::Money(Session->AdultFareCents()), KioskType::Body, true, C.Ink)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ Legs ],
			C.Surface, C.Ink, 3.f, FMargin(26.f, 20.f));
	}
}

void SMapScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bKeyboard = Session->State.bKeyboardOpen;
	const TSet<int32> Lit = MatchingStations(Session->State);
	const bool bTyped = !FKioskSearch::Normalize(Session->State.SearchText).IsEmpty();

	auto ZoomButton = [&](EKioskIcon IconType, const FText& Label, float Delta)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Key)
			.Label(Label)
			.ButtonTag(Delta > 0.f ? TEXT("Zoom in") : TEXT("Zoom out"))
			.Padding(FMargin(0.f))
			.Width(72.f).Height(72.f)
			.OnClicked_Lambda([Session, Delta]()
			{
				Session->MapZoom = FMath::Clamp(Session->MapZoom + Delta, 1.f, 2.5f);
				if (Session->MapZoom <= 1.f)
				{
					Session->MapPan = FVector2f::ZeroVector;
				}
				Session->NotifyChanged();
				return FReply::Handled();
			})
			[
				Icon(IconType, 34.f, C.Ink, 3.f)
			];
	};

	// Typed text: "6 stations lit up · others greyed out" (or a note when nothing matches and the map stays as it is).
	TSharedRef<SWidget> CountNote = SNew(SSpacer);
	if (bTyped)
	{
		const FText CountText = Lit.IsEmpty()
			? KT("No station matches · try fewer letters")
			: FText::Format(KT("{0} {0}|plural(one=station,other=stations) lit up · others greyed out"), Lit.Num());
		CountNote = Panel(TextWrap(CountText, KioskType::Caption, true, C.Ink), C.PrimaryTint, KioskLayout::InnerRadius, C.Transparent, 0.f, FMargin(24.f, 16.f));
	}

	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
	if (bKeyboard)
	{
		Right->AddSlot().AutoHeight()[ KioskDest::TripCard(Session) ];
		Right->AddSlot().AutoHeight().Padding(4.f, 28.f, 0.f, 10.f)[ Text(KT("Somewhere else?"), KioskType::Body, true, C.Ink) ];
		Right->AddSlot().AutoHeight()[ KioskDest::SearchField(Session, KT("Find station on the map")) ];
		Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::KeyboardWidget(Session) ];
		Right->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	}
	else
	{
		Right->AddSlot().AutoHeight()[ TextWrap(KT("Where are you going?"), KioskType::H1, true, C.Ink) ];
		Right->AddSlot().AutoHeight().Padding(4.f, 8.f, 0.f, 0.f)
		[
			TextWrap(KT("A keyboard will popup. The list narrows with every letter you type."), KioskType::Caption, false, C.Muted, 1.1f)
		];
		Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::SearchField(Session, KT("Find station on the map")) ];
		Right->AddSlot().FillHeight(1.f).Padding(0.f, 20.f, 0.f, 0.f)
		[
			SNew(SScrollBox).ScrollBarVisibility(EVisibility::Collapsed)
			+ SScrollBox::Slot()[ RouteCard(Session) ]
		];
		Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Open station list view"))
			.ButtonTag(TEXT("Open station list view"))
			.LeftIcon(EKioskIcon::List)
			.RightIcon(EKioskIcon::ArrowRight)
			.MinWidth(340.f).MaxWidth(620.f)
			.OnClicked_Lambda([Session]() { Session->ShowDestinationList(); return FReply::Handled(); })
		];
	}
	Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::ConfirmButton(Session) ];

	ChildSlot
	.Padding(KioskLayout::PageX, 26.f, KioskLayout::PageX, 30.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 40.f, 0.f)
		[
			Panel(
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SKioskMap)
					.SelectedStation(Session->State.Destination)
					.Zoom(Session->MapZoom)
					.Session(Session)
					.LitStations(Lit)
					.bDimOthers(!Lit.IsEmpty())
					.OnStationTapped(FOnKioskStationTapped::CreateLambda([Session](int32 Index) { Session->SelectDestination(Index); }))
				]
				+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top).Padding(22.f, 22.f, 22.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Left).Padding(0.f, 0.f, 16.f, 0.f)[ CountNote ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
					[
						Panel(Text(KT("Tap a station · drag to move · +/- to zoom"), KioskType::Caption, true, C.Ink), C.Band, KioskLayout::InnerRadius, C.Transparent, 0.f, FMargin(24.f, 16.f))
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(22.f, 0.f, 0.f, 22.f)
				[
					Panel(LineHighlighter(Session), C.Band, KioskLayout::InnerRadius, C.Transparent, 0.f, FMargin(18.f, 14.f))
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 22.f, 22.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ ZoomButton(EKioskIcon::Plus, KT("Zoom in"), 0.5f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ ZoomButton(EKioskIcon::Minus, KT("Zoom out"), -0.5f) ]
				],
				C.Surface, KioskLayout::CardRadius, C.BorderStrong, 2.f, FMargin(2.f))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(680.f)
			[
				Right
			]
		]
	];
}
