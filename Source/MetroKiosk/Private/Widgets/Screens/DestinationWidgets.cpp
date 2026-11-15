#include "DestinationWidgets.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskDestinationWidgets"

using namespace KioskUI;

namespace
{
	/** Single-line text that ends in an ellipsis instead of overflowing its slot. */
	TSharedRef<SWidget> Ellipsis(const FText& Value, float Px, bool bBold, const FLinearColor& Color)
	{
		return SNew(STextBlock)
			.Text(Value)
			.Font(FKioskStyle::Get().Font(Px, bBold))
			.ColorAndOpacity(Color)
			.OverflowPolicy(ETextOverflowPolicy::Ellipsis);
	}

	TSharedRef<SWidget> IconDisc(EKioskIcon Which, float Diameter, float IconPx)
	{
		const FKioskColors& C = KC();
		return SNew(SBox).WidthOverride(Diameter).HeightOverride(Diameter)
		[
			Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(Which, IconPx, C.Ink, 3.f) ], C.Band, Diameter * 0.5f)
		];
	}

	/** Everything a destination needs to be described: place if one was picked, otherwise the station. */
	struct FDescribed
	{
		FText Title;
		FString Local;
		FText Subtitle;
		EKioskIcon Icon = EKioskIcon::Train;
		uint8 LineMask = 0;
	};

	FDescribed Describe(const FKioskState& State)
	{
		FDescribed Out;
		const TArray<FKioskStation>& Stations = FKioskData::Stations();
		const TArray<FKioskPlace>& Places = FKioskPlaces::All();
		if (!Stations.IsValidIndex(State.Destination))
		{
			return Out;
		}
		const FKioskStation& Station = Stations[State.Destination];
		Out.LineMask = Station.LineMask;
		if (Places.IsValidIndex(State.DestinationPlace))
		{
			const FKioskPlace& Place = Places[State.DestinationPlace];
			Out.Title = FText::FromString(Place.Name);
			Out.Local = FKioskPlaces::LocalName(Place.Name);
			Out.Icon = FKioskPlaces::CategoryIcon(Place.Category);
			Out.Subtitle = FText::Format(FKioskLoc::Tr(LOCTEXT("NearStation", "near {0}")), FKioskPlaces::StationLabel(Station.Name));
		}
		else
		{
			Out.Title = FText::FromString(Station.Name);
			Out.Local = FKioskPlaces::LocalName(Station.Name);
			Out.Subtitle = KT("Station");
		}
		return Out;
	}

	void AppendLetter(const TSharedRef<FKioskSession>& Session, const FString& Letter)
	{
		FString Text = Session->State.SearchText;
		Text += Text.IsEmpty() ? Letter.ToUpper() : Letter.ToLower();
		Session->SetSearchText(Text);
	}
}

namespace KioskDest
{
	FText TripLine(const TSharedRef<FKioskSession>& Session, int32 StationIndex)
	{
		return FText::Format(LOCTEXT("TripLine", "{0} · {1}"),
			FKioskRoutes::Detail(FKioskRoutes::FromCityHall(StationIndex)), FKioskSession::Money(Session->AdultFareTo(StationIndex)));
	}

	TSharedRef<SWidget> Tile(const FTileArgs& Args)
	{
		const FKioskColors& C = KC();
		TSharedRef<SVerticalBox> Names = SNew(SVerticalBox);
		Names->AddSlot().AutoHeight()[ Ellipsis(Args.Title, 26.f, true, C.Ink) ];
		if (!Args.LocalTitle.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(FText::FromString(Args.LocalTitle), 19.f, false, C.Ink) ];
		}
		if (!Args.Subtitle.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(Args.Subtitle, 19.f, false, C.Muted) ];
		}

		TSharedRef<SWidget> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ IconDisc(Args.Icon, 62.f, 34.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 16.f, 0.f)[ Names ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[ LineBadges(Args.LineMask, 26.f, 6.f) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 6.f, 0.f, 0.f)[ Text(Args.Detail, 18.f, false, C.Muted) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Args.bSelected ? 12.f : 0.f, 0.f, 0.f, 0.f)
			[
				Args.bSelected
					? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(44.f).HeightOverride(44.f)
						[ Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 24.f, C.White, 3.5f) ], C.Primary, 22.f) ])
					: StaticCastSharedRef<SWidget>(SNew(SSpacer))
			];

		TFunction<void()> OnPick = Args.OnPick;
		return SNew(SBox).HeightOverride(94.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(Args.Title)
				.ButtonTag(*Args.Tag)
				.Padding(FMargin(20.f, 0.f, 20.f, 0.f))
				.OnClicked_Lambda([OnPick]() { OnPick(); return FReply::Handled(); })
				[
					Row
				],
				Args.bSelected ? C.PrimaryTint : C.Surface, 18.f, Args.bSelected ? C.Ink : C.BorderStrong, Args.bSelected ? 3.f : 2.f)
		];
	}

	TSharedRef<SWidget> PlaceTile(const TSharedRef<FKioskSession>& Session, int32 PlaceIndex)
	{
		const FKioskPlace& Place = FKioskPlaces::All()[PlaceIndex];
		const FKioskStation& Station = FKioskData::Stations()[Place.Station];
		FTileArgs Args;
		Args.Title = FText::FromString(Place.Name);
		Args.LocalTitle = FKioskPlaces::LocalName(Place.Name);
		Args.Subtitle = FText::Format(FKioskLoc::Tr(LOCTEXT("NearStation", "near {0}")), FKioskPlaces::StationLabel(Station.Name));
		Args.Icon = FKioskPlaces::CategoryIcon(Place.Category);
		Args.LineMask = Station.LineMask;
		Args.Detail = TripLine(Session, Place.Station);
		Args.bSelected = Session->State.DestinationPlace == PlaceIndex;
		Args.Tag = FString::Printf(TEXT("Place %s"), *Place.Name);
		Args.OnPick = [Session, PlaceIndex]() { Session->SelectPlace(PlaceIndex); Session->OpenKeyboard(false); };
		return Tile(Args);
	}

	TSharedRef<SWidget> StationTile(const TSharedRef<FKioskSession>& Session, int32 StationIndex)
	{
		const FKioskStation& Station = FKioskData::Stations()[StationIndex];
		const FText Blurb = FKioskPlaces::Blurb(StationIndex);
		FTileArgs Args;
		Args.Title = FText::FromString(Station.Name);
		Args.LocalTitle = FKioskPlaces::LocalName(Station.Name);
		Args.Subtitle = Blurb.IsEmpty() ? KT("Station") : Blurb;
		Args.Icon = Blurb.IsEmpty() ? EKioskIcon::Train : EKioskIcon::Pin;
		Args.LineMask = Station.LineMask;
		Args.Detail = TripLine(Session, StationIndex);
		Args.bSelected = Session->State.Destination == StationIndex && Session->State.DestinationPlace == INDEX_NONE;
		Args.Tag = FString::Printf(TEXT("Station %s"), *Station.Name);
		Args.OnPick = [Session, StationIndex]() { Session->SelectDestination(StationIndex); Session->OpenKeyboard(false); };
		return Tile(Args);
	}

	TSharedRef<SWidget> SelectionCard(const TSharedRef<FKioskSession>& Session, bool bCompact)
	{
		const FKioskColors& C = KC();
		const FKioskState& State = Session->State;
		if (!FKioskData::Stations().IsValidIndex(State.Destination))
		{
			return SNew(SBox).MinDesiredHeight(bCompact ? 150.f : 240.f)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Select a location"), bCompact ? 26.f : 32.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
					[
						TextWrap(KT("Pick a place or station, or type where you want to go. We show the route and fare before you confirm."),
							bCompact ? 19.f : 22.f, false, C.Muted, 1.3f)
					],
					C.HelpTint, 22.f, C.BorderStrong, 2.f, FMargin(28.f, bCompact ? 22.f : 30.f))
			];
		}

		const FKioskRoute& Route = FKioskRoutes::FromCityHall(State.Destination);
		const FDescribed Info = Describe(State);

		TSharedRef<SVerticalBox> Names = SNew(SVerticalBox);
		Names->AddSlot().AutoHeight()[ Ellipsis(Info.Title, bCompact ? 30.f : 38.f, true, C.Ink) ];
		if (!Info.Local.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(FText::FromString(Info.Local), bCompact ? 19.f : 23.f, false, C.Ink) ];
		}
		Names->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(Info.Subtitle, bCompact ? 19.f : 22.f, false, C.Muted) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)[ LineBadges(Info.LineMask, bCompact ? 26.f : 30.f, 6.f) ]
		];

		const FText FareText = FText::Format(FKioskLoc::Tr(LOCTEXT("FarePerAdult", "{0}: {1} per adult")),
			Session->TicketLabel(), FKioskSession::Money(Session->AdultFareCents()));

		return Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ IconDisc(Info.Icon, bCompact ? 56.f : 76.f, bCompact ? 30.f : 42.f) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 16.f, 0.f)[ Names ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::OutlineMuted)
					.Label(KT("Change"))
					.ButtonTag(TEXT("Change destination"))
					.FontPx(20.f).Bold(false)
					.Width(136.f).Height(56.f)
					.OnClicked_Lambda([Session]() { Session->ClearDestination(); return FReply::Handled(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bCompact ? 14.f : 20.f, 0.f, bCompact ? 12.f : 16.f)[ Rule(C.BorderStrong, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(FKioskRoutes::Summary(Route), bCompact ? 20.f : 24.f, true, C.Ink, 1.25f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bCompact ? 12.f : 18.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Pill(FKioskRoutes::Detail(Route), C.PrimaryTint, C.Ink, bCompact ? 19.f : 22.f, true, C.Transparent, FMargin(20.f, 9.f))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(FareText, bCompact ? 20.f : 24.f, true, C.Ink) ]
			],
			C.Surface, 22.f, C.Ink, 3.f, FMargin(28.f, bCompact ? 20.f : 26.f));
	}

	TSharedRef<SWidget> ConfirmButton(const TSharedRef<FKioskSession>& Session)
	{
		return SNew(SBox).HeightOverride(110.f)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Primary)
			.Label(KT("Confirm destination"))
			.ButtonTag(TEXT("Confirm destination"))
			.RightIcon(EKioskIcon::ArrowRight)
			.FontPx(38.f)
			.Spacing(8.f)
			.Enabled(Session->State.Destination != INDEX_NONE)
			.OnClicked_Lambda([Session]() { Session->ConfirmDestination(); return FReply::Handled(); })
		];
	}

	TSharedRef<SWidget> SearchField(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const bool bHasText = !Session->State.SearchText.IsEmpty();
		const FText Placeholder = KT("Type a place or station, e.g. Gardens by the Bay");
		return SNew(SBox).HeightOverride(88.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Card)
					.Label(Placeholder)
					.ButtonTag(TEXT("Search"))
					.Padding(FMargin(26.f, 0.f))
					.OnClicked_Lambda([Session]() { Session->OpenKeyboard(true); return FReply::Handled(); })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Search, 36.f, C.Ink, 3.f) ]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
						[
							bHasText ? Ellipsis(FText::FromString(Session->State.SearchText), 28.f, false, C.Ink) : Ellipsis(Placeholder, 23.f, false, C.Muted)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)
				[
					bHasText
						? StaticCastSharedRef<SWidget>(
							SNew(SBox).WidthOverride(50.f).HeightOverride(50.f)
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

	TSharedRef<SWidget> KeyboardWidget(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		static const TCHAR* const Rows[3] = { TEXT("QWERTYUIOP"), TEXT("ASDFGHJKL"), TEXT("ZXCVBNM") };
		const float KeyW = 76.f;
		const float KeyH = 76.f;
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

		Board->AddSlot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
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
}

#undef LOCTEXT_NAMESPACE
