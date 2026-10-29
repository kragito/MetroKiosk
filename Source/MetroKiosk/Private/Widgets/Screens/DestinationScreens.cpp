// Destination picker (list, line filter, search keyboard) and the schematic map.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskMap.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskDestination"

using namespace KioskUI;

namespace
{
	FText TripDetail(const FKioskStation& Station)
	{
		return FText::Format(LOCTEXT("TripDetail", "{0} min · {1} {1}|plural(one=change,other=changes)"),
			FText::AsNumber(Station.Minutes), FText::AsNumber(Station.Changes));
	}

	/** Station list that honours the line filter and search text; returns the heading and fills Out. */
	FText BuildStationList(const FKioskState& State, TArray<int32>& Out)
	{
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		const bool bSearching = !State.SearchText.IsEmpty();
		const bool bFiltering = State.LineFilter != INDEX_NONE;

		if (!bSearching && !bFiltering)
		{
			Out = FKioskData::PopularStations();
			return KT("Popular from City Hall");
		}

		for (int32 Index = 0; Index < Stations.Num(); ++Index)
		{
			if (Index == FKioskData::CityHallIndex())
			{
				continue;
			}
			if (bFiltering && !Stations[Index].OnLine(State.LineFilter))
			{
				continue;
			}
			if (bSearching && !Stations[Index].Name.Contains(State.SearchText, ESearchCase::IgnoreCase))
			{
				continue;
			}
			Out.Add(Index);
		}
		Out.Sort([&Stations](int32 A, int32 B) { return Stations[A].Name < Stations[B].Name; });

		if (bFiltering)
		{
			return FText::Format(LOCTEXT("FilteredBy", "Filtered by {0}"), FText::FromString(FKioskData::Line(State.LineFilter).FullName));
		}
		return FText::Format(LOCTEXT("SearchResults", "Results for “{0}”"), FText::FromString(State.SearchText));
	}

	TSharedRef<SWidget> StationName(const FString& Name, const FString& Highlight, bool bSelected)
	{
		const FKioskColors& C = KC();
		const float Px = 26.f;
		const int32 Where = Highlight.IsEmpty() ? INDEX_NONE : Name.Find(Highlight, ESearchCase::IgnoreCase);
		if (Where == INDEX_NONE)
		{
			return Text(FText::FromString(Name), Px, bSelected, C.Ink);
		}
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[ Text(FText::FromString(Name.Left(Where)), Px, bSelected, C.Ink) ]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Panel(Text(FText::FromString(Name.Mid(Where, Highlight.Len())), Px, bSelected, C.Ink), C.PrimaryTint, 4.f)
			]
			+ SHorizontalBox::Slot().AutoWidth()[ Text(FText::FromString(Name.Mid(Where + Highlight.Len())), Px, bSelected, C.Ink) ];
	}

	TSharedRef<SWidget> StationRow(const TSharedRef<FKioskSession>& Session, int32 StationIndex)
	{
		const FKioskColors& C = KC();
		const FKioskStation& Station = FKioskData::Stations()[StationIndex];
		const bool bSelected = Session->State.Destination == StationIndex;

		TSharedRef<SWidget> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ StationName(Station.Name, Session->State.SearchText, bSelected) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18.f, 0.f, 0.f, 0.f)[ LineBadges(Station.LineMask, 28.f, 8.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, bSelected ? 20.f : 28.f, 0.f)
			[
				Text(TripDetail(Station), 19.f, false, C.Muted)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				bSelected
					? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(46.f).HeightOverride(46.f)
						[ Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 26.f, C.White, 3.5f) ], C.Primary, 23.f) ])
					: StaticCastSharedRef<SWidget>(SNew(SSpacer))
			];

		return SNew(SBox).HeightOverride(92.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(FText::FromString(Station.Name))
				.ButtonTag(*FString::Printf(TEXT("Station %s"), *Station.Name))
				.Padding(FMargin(28.f, 0.f, 18.f, 0.f))
				.OnClicked_Lambda([Session, StationIndex]() { Session->SelectDestination(StationIndex); return FReply::Handled(); })
				[
					Row
				],
				bSelected ? C.PrimaryTint : C.Surface, 18.f, bSelected ? C.Ink : C.BorderStrong, bSelected ? 3.f : 2.f)
		];
	}

	TSharedRef<SWidget> StationListPanel(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		TArray<int32> Stations;
		const FText Heading = BuildStationList(Session->State, Stations);

		TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
		for (int32 StationIndex : Stations)
		{
			Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 11.f)[ StationRow(Session, StationIndex) ];
		}
		if (Stations.IsEmpty())
		{
			Rows->AddSlot().AutoHeight().Padding(8.f, 18.f)[ Text(LOCTEXT("NoMatch", "No stations match. Try fewer letters."), 22.f, false, C.Muted) ];
		}

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)[ Text(Heading, 19.f, true, C.Ink) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				// Short lists need no scrolling (and stay visible to UI automation); long line lists scroll.
				Stations.Num() > 6
					? StaticCastSharedRef<SWidget>(SNew(SScrollBox).ScrollBarVisibility(EVisibility::Collapsed) + SScrollBox::Slot()[ Rows ])
					: StaticCastSharedRef<SWidget>(Rows)
			];
	}

	/** Header of the right column: "Select a location" or "City Hall -> X · about N min". */
	TSharedRef<SWidget> SelectionBar(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		TSharedRef<SWidget> Body = SNullWidget::NullWidget;
		if (Stations.IsValidIndex(Session->State.Destination))
		{
			const FKioskStation& Station = Stations[Session->State.Destination];
			Body = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("City Hall"), 22.f, false, C.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.f, 0.f)[ Icon(EKioskIcon::ArrowRight, 30.f, C.Ink, 3.f) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(FText::FromString(Station.Name), 22.f, true, C.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)
				[
					Text(FText::Format(LOCTEXT("About", "· about {0} min · {1} {1}|plural(one=change,other=changes)"),
						FText::AsNumber(Station.Minutes), FText::AsNumber(Station.Changes)), 19.f, false, C.Muted)
				];
		}
		else
		{
			Body = Text(KT("Select a location"), 22.f, true, C.Ink);
		}
		return SNew(SBox).HeightOverride(82.f)
		[
			Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Body ], C.HelpTint, 18.f, C.BorderStrong, 2.f, FMargin(24.f, 0.f))
		];
	}

	TSharedRef<SWidget> ConfirmButton(const TSharedRef<FKioskSession>& Session)
	{
		return SNew(SBox).HeightOverride(118.f)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Primary)
			.Label(KT("Confirm selected ticket"))
			.RightIcon(EKioskIcon::ArrowRight)
			.FontPx(38.f)
			.Spacing(8.f)
			.Enabled(Session->State.Destination != INDEX_NONE)
			.OnClicked_Lambda([Session]() { Session->ConfirmDestination(); return FReply::Handled(); })
		];
	}

	TSharedRef<SWidget> SearchField(const TSharedRef<FKioskSession>& Session, const FText& Placeholder)
	{
		const FKioskColors& C = KC();
		const bool bHasText = !Session->State.SearchText.IsEmpty();
		return SNew(SBox).HeightOverride(80.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Card)
					.Label(Placeholder)
					.ButtonTag(TEXT("Search field"))
					.Padding(FMargin(26.f, 0.f))
					.OnClicked_Lambda([Session]() { Session->OpenKeyboard(true); return FReply::Handled(); })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Search, 34.f, C.Ink, 3.f) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
						[
							bHasText ? Text(FText::FromString(Session->State.SearchText), 24.f, false, C.Ink) : Text(Placeholder, 24.f, false, C.Muted)
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)
				[
					bHasText
						? StaticCastSharedRef<SWidget>(
							SNew(SBox).WidthOverride(46.f).HeightOverride(46.f)
							[
								SNew(SKioskButton)
								.Kind(EKioskButtonKind::KeyMuted)
								.Label(LOCTEXT("ClearSearch", "Clear search"))
								.ButtonTag(TEXT("Clear search"))
								.Padding(FMargin(0.f))
								
								.OnClicked_Lambda([Session]() { Session->State.SearchText.Reset(); Session->OpenKeyboard(false); return FReply::Handled(); })
								[
									Icon(EKioskIcon::Close, 22.f, C.Ink, 3.f)
								]
							])
						: StaticCastSharedRef<SWidget>(SNew(SSpacer))
				],
				C.Surface, 18.f, C.Ink, 3.f)
		];
	}

	void AppendLetter(const TSharedRef<FKioskSession>& Session, const FString& Letter)
	{
		FString Text = Session->State.SearchText;
		Text += Text.IsEmpty() ? Letter.ToUpper() : Letter.ToLower();
		Session->SetSearchText(Text);
	}

	TSharedRef<SWidget> KeyboardWidget(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		static const TCHAR* const Rows[3] = { TEXT("QWERTYUIOP"), TEXT("ASDFGHJKL"), TEXT("ZXCVBNM") };
		const float KeyW = 80.f;
		const float KeyH = 79.f;
		const float Gap = 10.f;

		TSharedRef<SVerticalBox> Board = SNew(SVerticalBox);
		for (int32 Row = 0; Row < 3; ++Row)
		{
			TSharedRef<SHorizontalBox> Keys = SNew(SHorizontalBox);
			const FString Letters = Rows[Row];
			for (int32 Index = 0; Index < Letters.Len(); ++Index)
			{
				const FString Letter = Letters.Mid(Index, 1);
				Keys->AddSlot().AutoWidth().Padding(Index == 0 ? 0.f : Gap, 0.f, 0.f, 0.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Key)
					.Label(FText::FromString(Letter))
					.ButtonTag(*FString::Printf(TEXT("Key %s"), *Letter))
					.FontPx(27.f).Bold(false)
					.Width(KeyW).Height(KeyH)
					.Padding(FMargin(0.f))
					.OnClicked_Lambda([Session, Letter]() { AppendLetter(Session, Letter); return FReply::Handled(); })
				];
			}
			if (Row == 2)
			{
				Keys->AddSlot().AutoWidth().Padding(Gap, 0.f, 0.f, 0.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::KeyMuted)
					.Label(LOCTEXT("Backspace", "Backspace"))
					.ButtonTag(TEXT("Key Backspace"))
					.Width(KeyW + 14.f).Height(KeyH)
					.Padding(FMargin(0.f))
					.OnClicked_Lambda([Session]() { FString T = Session->State.SearchText; T.LeftChopInline(1); Session->SetSearchText(T); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Backspace, 36.f, C.Ink, 3.f)
					]
				];
			}
			Board->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, Row == 0 ? 0.f : Gap, 0.f, 0.f)[ Keys ];
		}

		Board->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::KeyMuted)
				.Label(LOCTEXT("ClearKey", "Clear"))
				.ButtonTag(TEXT("Key Clear"))
				.FontPx(22.f).Bold(false)
				.Width(136.f).Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->State.SearchText.Reset(); Session->OpenKeyboard(false); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(Gap, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Key)
				.Label(LOCTEXT("SpaceKey", "Space"))
				.ButtonTag(TEXT("Key Space"))
				.FontPx(22.f).Bold(false)
				.Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->SetSearchText(Session->State.SearchText + TEXT(" ")); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(Gap, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Primary)
				.Label(KT("Done"))
				.ButtonTag(TEXT("Hide keyboard"))
				.FontPx(22.f)
				.Width(150.f).Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->OpenKeyboard(false); return FReply::Handled(); })
			]
		];
		return Board;
	}

	TSharedRef<SWidget> LineFilterGrid(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
		for (int32 Row = 0; Row < 2; ++Row)
		{
			TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
			for (int32 Col = 0; Col < 3; ++Col)
			{
				const int32 Line = Row * 3 + Col;
				const bool bActive = Session->State.LineFilter == Line;
				const FText Name = FText::FromString(FKioskData::Line(Line).Name);
				Cells->AddSlot().FillWidth(1.f).Padding(Col == 0 ? 0.f : 8.f, 0.f, Col == 2 ? 0.f : 8.f, 0.f)
				[
					SNew(SBox).HeightOverride(70.f)
					[
						SNew(SKioskButton)
						.Kind(bActive ? EKioskButtonKind::Tint : EKioskButtonKind::OutlineMuted)
						.Label(FText::Format(LOCTEXT("LineFilterLabel", "{0} line"), Name))
						.ButtonTag(*FString::Printf(TEXT("Filter %s"), FKioskData::Line(Line).Code))
						.Padding(FMargin(0.f))
						.OnClicked_Lambda([Session, Line]() { Session->SetLineFilter(Line); return FReply::Handled(); })
						[
							SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ LineBadge(Line, 26.f) ]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)[ Text(Name, 18.f, bActive, C.Ink) ]
							]
						]
					]
				];
			}
			Grid->AddSlot().AutoHeight().Padding(0.f, Row == 0 ? 0.f : 10.f, 0.f, 0.f)[ Cells ];
		}
		return Grid;
	}
}

void SDestinationScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bKeyboard = Session->State.bKeyboardOpen;

	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
	Right->AddSlot().AutoHeight()[ SelectionBar(Session) ];
	Right->AddSlot().AutoHeight().Padding(4.f, 22.f, 0.f, 10.f)[ Text(KT("Somewhere else?"), 19.f, true, C.Ink) ];
	Right->AddSlot().AutoHeight()[ SearchField(Session, KT("Search all stations")) ];
	if (bKeyboard)
	{
		Right->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ KeyboardWidget(Session) ];
	}
	else
	{
		Right->AddSlot().AutoHeight().Padding(4.f, 8.f, 0.f, 0.f)
		[
			Text(KT("A keyboard will popup. The list narrows with every letter you type."), 16.f, false, C.Muted)
		];
		Right->AddSlot().AutoHeight().Padding(4.f, 22.f, 0.f, 10.f)[ Text(KT("Filter by line"), 19.f, true, C.Ink) ];
		Right->AddSlot().AutoHeight()[ LineFilterGrid(Session) ];
		Right->AddSlot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::OutlineMuted)
			.Label(KT("Open metro map view"))
			.LeftIcon(EKioskIcon::Map)
			.FontPx(21.f).Bold(false)
			.Width(380.f).Height(88.f)
			.OnClicked_Lambda([Session]() { Session->Replace(EKioskScreen::DestinationMap); return FReply::Handled(); })
		];
	}
	Right->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	Right->AddSlot().AutoHeight()[ ConfirmButton(Session) ];

	ChildSlot
	.Padding(75.f, 24.f, 75.f, 22.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 56.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)[ Text(KT("Where are you going?"), 42.f, true, C.Ink) ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ StationListPanel(Session) ]
		]
		+ SHorizontalBox::Slot().FillWidth(0.55f)
		[
			Right
		]
	];
}

void SMapScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const bool bHasSelection = Stations.IsValidIndex(Session->State.Destination);

	TSharedRef<SWidget> Info = SNullWidget::NullWidget;
	if (bHasSelection)
	{
		const FKioskStation& Station = Stations[Session->State.Destination];
		Info = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(FText::FromString(Station.Name), 34.f, true, C.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)[ LineBadges(Station.LineMask, 28.f, 8.f) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				Text(FText::Format(LOCTEXT("MapAbout", "City Hall → {0} · about {1}"), FText::FromString(Station.Name), TripDetail(Station)), 20.f, false, C.Muted)
			];
	}
	else
	{
		Info = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("NoStation", "No station selected"), 24.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				TextWrap(LOCTEXT("TapStation", "Tap a station on the map to see the route, or find it by name below."), 19.f, false, C.Muted, 1.3f)
			];
	}

	auto ZoomButton = [&](EKioskIcon IconType, const FText& Label, float Delta)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Key)
			.Label(Label)
			.Padding(FMargin(0.f))
			.Width(72.f).Height(72.f)
			.OnClicked_Lambda([Session, Delta]()
			{
				Session->MapZoom = FMath::Clamp(Session->MapZoom + Delta, 1.f, 2.5f);
				if (Session->MapZoom <= 1.f)
				{
					Session->MapPan = FVector2f::ZeroVector;
				}
				else
				{
					const FVector2f Limit = FVector2f(530.f, 365.f) * (Session->MapZoom - 0.5f);
					Session->MapPan.X = FMath::Clamp(Session->MapPan.X, -Limit.X, Limit.X);
					Session->MapPan.Y = FMath::Clamp(Session->MapPan.Y, -Limit.Y, Limit.Y);
				}
				Session->NotifyChanged();
				return FReply::Handled();
			})
			[
				Icon(IconType, 34.f, C.Ink, 3.f)
			];
	};

	ChildSlot
	.Padding(75.f, 26.f, 75.f, 30.f)
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
					.OnStationTapped(FOnKioskStationTapped::CreateLambda([Session](int32 Index) { Session->SelectDestination(Index); }))
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 22.f, 22.f, 0.f)
				[
					Panel(Text(KT("Tap a station · drag to move · +/- to zoom"), 18.f, true, C.Ink), C.Band, 16.f, C.Transparent, 0.f, FMargin(24.f, 16.f))
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 22.f, 22.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ ZoomButton(EKioskIcon::Plus, LOCTEXT("ZoomIn", "Zoom in"), 0.5f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ ZoomButton(EKioskIcon::Minus, LOCTEXT("ZoomOut", "Zoom out"), -0.5f) ]
				],
				C.Surface, 22.f, C.BorderStrong, 2.f, FMargin(2.f))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(660.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Where are you going?"), 38.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
				[
					SNew(SBox).HeightOverride(78.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::OutlineMuted)
						.Label(LOCTEXT("FindOnMap", "Find station on the map"))
						.ButtonTag(TEXT("Find station on the map"))
						.Padding(FMargin(26.f, 0.f))
						.OnClicked_Lambda([Session]() { Session->OpenKeyboard(true); Session->Replace(EKioskScreen::Destination); return FReply::Handled(); })
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Search, 32.f, C.Ink, 3.f) ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)
							[
								Text(LOCTEXT("FindOnMap", "Find station on the map"), 22.f, false, C.Muted)
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 0.f, 0.f)
				[
					Text(KT("A keyboard will popup. The list narrows with every letter you type."), 16.f, false, C.Muted)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f)
				[
					SNew(SBox).MinDesiredHeight(190.f)
					[
						Panel(Info, C.Surface, 18.f, C.BorderStrong, 2.f, FMargin(26.f, 24.f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
				[
					SNew(SBox).HeightOverride(88.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::OutlineMuted)
						.Label(KT("Open station list view"))
						.LeftIcon(EKioskIcon::List)
						.FontPx(21.f).Bold(false)
						.OnClicked_Lambda([Session]() { Session->Replace(EKioskScreen::Destination); return FReply::Handled(); })
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()[ ConfirmButton(Session) ]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
