#include "DestinationWidgets.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	/** Wrapping text for names: long names go to a second line rather than being cut off. */
	TSharedRef<SWidget> Ellipsis(const FText& Value, float Px, bool bBold, const FLinearColor& Color)
	{
		return TextWrap(Value, Px, bBold, Color, 1.f);
	}

	/** Single-line text that ends in an ellipsis (only for what the customer typed). */
	TSharedRef<SWidget> TypedText(const FText& Value, float Px, bool bBold, const FLinearColor& Color)
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
			Out.Subtitle = FKioskPlaces::NearLine(Place);
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
		return FText::Format(KT("{0} · {1}"),
			FKioskRoutes::Detail(FKioskRoutes::FromCityHall(StationIndex)), FKioskSession::Money(Session->AdultFareTo(StationIndex)));
	}

	TSharedRef<SWidget> Tile(const FTileArgs& Args)
	{
		const FKioskColors& C = KC();
		TSharedRef<SVerticalBox> Names = SNew(SVerticalBox);
		Names->AddSlot().AutoHeight()[ Ellipsis(Args.Title, KioskType::H3, true, C.Ink) ];
		if (!Args.LocalTitle.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(FText::FromString(Args.LocalTitle), KioskType::Caption, false, C.Ink) ];
		}
		if (!Args.Subtitle.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(Args.Subtitle, KioskType::Caption, false, C.Muted) ];
		}

		TSharedRef<SWidget> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ IconDisc(Args.Icon, 62.f, KioskLayout::IconSm) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 16.f, 0.f)[ Names ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[ LineBadges(Args.LineMask) ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 6.f, 0.f, 0.f)[ Text(Args.Detail, KioskType::Caption, false, C.Muted) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(Args.bSelected ? 12.f : 0.f, 0.f, 0.f, 0.f)
			[
				Args.bSelected
					? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(44.f).HeightOverride(44.f)
						[ Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 24.f, C.White, 3.5f) ], C.Primary, 22.f) ])
					: StaticCastSharedRef<SWidget>(SNew(SSpacer))
			];

		TFunction<void()> OnPick = Args.OnPick;
		return SNew(SBox).MinDesiredHeight(100.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(Args.Title)
				.ButtonTag(*Args.Tag)
				.Padding(FMargin(20.f, 8.f, 20.f, 8.f))
				.OnClicked_Lambda([OnPick]() { OnPick(); return FReply::Handled(); })
				[
					Row
				],
				Args.bSelected ? C.PrimaryTint : C.Surface, KioskLayout::InnerRadius + 2.f, Args.bSelected ? C.Ink : C.BorderStrong, Args.bSelected ? 3.f : 2.f)
		];
	}

	TSharedRef<SWidget> PlaceTile(const TSharedRef<FKioskSession>& Session, int32 PlaceIndex)
	{
		const FKioskPlace& Place = FKioskPlaces::All()[PlaceIndex];
		const FKioskStation& Station = FKioskData::Stations()[Place.Station];
		FTileArgs Args;
		Args.Title = FText::FromString(Place.Name);
		Args.LocalTitle = FKioskPlaces::LocalName(Place.Name);
		Args.Subtitle = FKioskPlaces::NearLine(Place);
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
				Card(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Select a location"), bCompact ? KioskType::H3 : KioskType::H2, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
					[
						TextWrap(KT("Pick a place or station, or type where you want to go. We show the route and fare before you confirm."),
							KioskType::Body, false, C.Muted, 1.25f)
					],
					C.HelpTint, C.BorderStrong, 2.f, FMargin(28.f, bCompact ? 22.f : 30.f))
			];
		}

		const FKioskRoute& Route = FKioskRoutes::FromCityHall(State.Destination);
		const FDescribed Info = Describe(State);

		TSharedRef<SVerticalBox> Names = SNew(SVerticalBox);
		Names->AddSlot().AutoHeight()[ Ellipsis(Info.Title, bCompact ? KioskType::H2 : KioskType::H1, true, C.Ink) ];
		if (!Info.Local.IsEmpty())
		{
			Names->AddSlot().AutoHeight()[ Ellipsis(FText::FromString(Info.Local), KioskType::Caption, false, C.Ink) ];
		}
		Names->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(Info.Subtitle, KioskType::Caption, false, C.Muted) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)[ LineBadges(Info.LineMask) ]
		];

		const FText FareText = FText::Format(KT("{0}: {1} per adult"),
			Session->TicketLabel(), FKioskSession::Money(Session->AdultFareCents()));

		return Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ IconDisc(Info.Icon, bCompact ? 60.f : 76.f, bCompact ? KioskLayout::IconSm : 42.f) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 16.f, 0.f)[ Names ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					ChangeButton(FOnClicked::CreateLambda([Session]() { Session->ClearDestination(); return FReply::Handled(); }), TEXT("Change destination"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bCompact ? 14.f : 20.f, 0.f, bCompact ? 12.f : 16.f)[ Rule(C.BorderStrong, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(FKioskRoutes::Summary(Route), bCompact ? KioskType::Body : KioskType::H3, true, C.Ink, 1.2f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bCompact ? 12.f : 18.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Pill(FKioskRoutes::Detail(Route), C.PrimaryTint, C.Ink, KioskType::Body, true, C.Transparent, FMargin(20.f, 9.f))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ TextWrap(FareText, bCompact ? KioskType::Body : KioskType::H3, true, C.Ink, 1.1f, ETextJustify::Right) ]
			],
			C.Surface, KioskLayout::CardRadius, C.Ink, 3.f, FMargin(28.f, bCompact ? 20.f : 26.f));
	}

	TSharedRef<SWidget> ConfirmButton(const TSharedRef<FKioskSession>& Session)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Primary)
			.Label(KT("Confirm destination"))
			.ButtonTag(TEXT("Confirm destination"))
			.RightIcon(EKioskIcon::ArrowRight)
			.Enabled(Session->State.Destination != INDEX_NONE)
			.OnClicked_Lambda([Session]() { Session->ConfirmDestination(); return FReply::Handled(); });
	}

	TSharedRef<SWidget> SearchField(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const bool bHasText = !Session->State.SearchText.IsEmpty();
		const FText Placeholder = KT("Type a place or station, e.g. Gardens by the Bay");
		return SNew(SBox).MinDesiredHeight(88.f)
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
					.OnClicked_Lambda([Session]() { Session->OpenSearchField(); return FReply::Handled(); })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Search, 36.f, C.Ink, 3.f) ]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
						[
							bHasText ? TypedText(FText::FromString(Session->State.SearchText), KioskType::H3, false, C.Ink) : TypedText(Placeholder, KioskType::Body, false, C.Muted)
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
								.Label(KT("Clear search"))
								.ButtonTag(TEXT("Clear search"))
								.Padding(FMargin(0.f))
								.OnClicked_Lambda([Session]() { Session->SetSearchText(FString()); Session->OpenKeyboard(false); return FReply::Handled(); })
								[
									Icon(EKioskIcon::Close, 22.f, C.Ink, 3.f)
								]
							])
						: StaticCastSharedRef<SWidget>(SNew(SSpacer))
				],
				C.Surface, KioskLayout::InnerRadius + 2.f, C.Ink, 3.f)
		];
	}

	TSharedRef<SWidget> KeyboardWidget(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		static const TCHAR* const Rows[3] = { TEXT("QWERTYUIOP"), TEXT("ASDFGHJKL"), TEXT("ZXCVBNM") };
		const float KeyW = 76.f;
		const float KeyH = KioskLayout::KeyH;
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
					.Bold(false)
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
					.Label(KT("Backspace"))
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
				.Label(KT("Clear"))
				.ButtonTag(TEXT("Key Clear"))
				.Bold(false)
				.MinWidth(136.f).Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->SetSearchText(FString()); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(Gap, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Key)
				.Label(KT("Space"))
				.ButtonTag(TEXT("Key Space"))
				.Bold(false)
				.Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->SetSearchText(Session->State.SearchText + TEXT(" ")); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(Gap, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Primary)
				.Label(KT("Done"))
				.ButtonTag(TEXT("Hide keyboard"))
				.FontPx(KioskType::BtnStd)
				.MinWidth(150.f).Height(KeyH)
				.OnClicked_Lambda([Session]() { Session->OpenKeyboard(false); return FReply::Handled(); })
			]
		];
		return Board;
	}
}

