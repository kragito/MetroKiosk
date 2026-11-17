// Card flows: scan card, card options, top-up amount, concession pass, pass dates and the "lift your card" screen.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	/** White "Metro card" with a teal stripe. */
	TSharedRef<SWidget> MetroCardGraphic(float Width, float Height)
	{
		const FKioskColors& C = KC();
		return Panel(
			SNew(SOverlay)
			+ SOverlay::Slot().VAlign(VAlign_Top).Padding(0.f, Height * 0.2f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(Height * 0.2f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(C.Brand)).BorderBackgroundColor(FLinearColor::White) ]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(Width * 0.07f, 0.f, 0.f, Height * 0.14f)
			[
				Text(KT("Metro card"), FMath::Max(KioskType::Caption, Height * 0.12f), true, C.Ink)
			],
			C.Surface, KioskLayout::InnerRadius, C.Ink, 3.f);
	}

	TSharedRef<SWidget> CardInfoLine(const FText& Label, const FText& Value)
	{
		const FKioskColors& C = KC();
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ Text(Label, KioskType::Caption, false, C.Muted) ]
			+ SHorizontalBox::Slot().AutoWidth()[ Text(Value, KioskType::Caption, false, C.Muted) ];
	}

	/** "Keep your card on the reader" with the diagram showing where the Metro card reader is. */
	TSharedRef<SWidget> KeepCardNote()
	{
		const FKioskColors& C = KC();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Contactless, KioskLayout::IconSm, C.Primary, 3.f) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)[ TextWrap(KT("Keep your card on the reader"), KioskType::Caption, true, C.Ink, KioskType::LineBody) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ HardwareHint(Mask(EHardware::MetroReader), 0.42f) ];
	}

	/** Green tick shown on a selected option tile. */
	TSharedRef<SWidget> CheckBadge()
	{
		return Badge(EKioskIcon::Check, KioskLayout::IconXl, KC().Primary, KC().White, 4.f);
	}
}

namespace
{
	FText DayMonth(const FDateTime& Date)
	{
		return FKioskSession::DayMonthText(Date);
	}
}

// --- scan card -----------------------------------------------------------------------------

void SScanCardScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	TSharedRef<SWidget> Illustration = SNew(SOverlay)
		+ SOverlay::Slot()[ Positioned(MetroCardGraphic(380.f, 240.f), FVector2D(110.f, 40.f), FVector2D(380.f, 240.f), -8.f) ]
		+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::Contactless, 150.f, C.Primary, 5.f), FVector2D(225.f, 250.f), FVector2D(150.f, 150.f), -90.f) ]
		+ SOverlay::Slot()
		[
			Positioned(Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Panel(SNew(SSpacer).Size(FVector2D(190.f, 52.f)), C.Primary, 12.f) ], C.Ink, 24.f),
				FVector2D(110.f, 410.f), FVector2D(340.f, 100.f))
		];

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 48.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 50.f, 0.f, 0.f)[ TextWrap(KT("Place your Metro card on the reader"), KioskType::H1, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, KioskLayout::TitleGap, 0.f, 0.f)
			[
				TextWrap(KT("The Metro card reader is the curved bowl below the screen, right of centre. Rest the card on it. It glows green when it's ready. Hold the card still until this screen changes."), KioskType::Body, false, C.Ink, KioskType::LineBody)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(C.Primary, 16.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ TextWrap(KT("Waiting for your card…"), KioskType::Body, true, C.Ink) ],
					C.Surface, 26.f, C.BorderStrong, 2.f, KioskLayout::CardPadTight)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)
			[
				TextWrap(KT("Not reading? Take the card out of your wallet or phone case, then try again."), KioskType::Caption, false, C.Muted, KioskType::LineBody)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				// The Student button keeps its original tag so existing journeys still find it.
				SimControl(KT("SIMULATE: tap Student card on reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateMetroCardTap(EKioskCardType::Student); return FReply::Handled(); }), TEXT("SIMULATE: tap card on reader"))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 10.f, 0.f, 0.f)
			[
				SimControl(KT("SIMULATE: tap Adult card on reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateMetroCardTap(EKioskCardType::Adult); return FReply::Handled(); }), TEXT("SIMULATE: tap Adult card on reader"))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.55f)
		[
			Card(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
					[
						SNew(SBox).WidthOverride(600.f).HeightOverride(520.f)[ Illustration ]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ HardwareHint(Mask(EHardware::MetroReader), 0.8f) ],
				C.Band, C.Transparent, 0.f, KioskLayout::CardPadWide)
		]
	];
}

// --- card options --------------------------------------------------------------------------

void SCardOptionsScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const FKioskCardInfo& Info = Session->CardInfo();

	TSharedRef<SWidget> CardPanel = Card(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ CardInfoLine(Session->CardLabel(), FText::FromString(FString(TEXT("•••• ")) + Info.Last4)) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[ Text(KT("Balance"), KioskType::Caption, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents()), KioskType::Display, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			Info.bLowBalance
				? StaticCastSharedRef<SWidget>(Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 5.f, 12.f, 0.f)[ Dot(C.AmberStrong, 12.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ TextWrap(KT("Low balance · one trip left"), KioskType::Caption, true, C.Ink) ],
					C.AmberFill, 17.f, C.AmberStroke, 2.f, FMargin(18.f, 8.f)))
				: StaticCastSharedRef<SWidget>(SNew(SSpacer))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 16.f)[ Rule(C.BorderStrong, 2.f) ]
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Concession pass"), KioskType::Caption, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			Info.bHasActivePass
				? TextWrap(FText::Format(KT("{0} · ends {1}"), KT("Bus + Train"), Session->CurrentPassEndText()), KioskType::Body, true, C.Ink, KioskType::LineBody)
				: TextWrap(KT("No active pass"), KioskType::Body, false, C.Muted, KioskType::LineBody)
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight()[ KeepCardNote() ],
		C.Band, C.BorderStrong, 2.f, KioskLayout::CardPadWide);

	TSharedRef<SHorizontalBox> Tiles = SNew(SHorizontalBox);
	const TArray<int32>& QuickAmounts = FKioskData::QuickTopUpAmounts();
	for (int32 Index = 0; Index < QuickAmounts.Num(); ++Index)
	{
		const int32 Amount = QuickAmounts[Index];
		const FText Dollars = FText::Format(KT("${0}"), FText::AsNumber(Amount / 100));
		Tiles->AddSlot().FillWidth(1.f).Padding(Index == 0 ? 0.f : KioskLayout::TileGap * 0.5f, 0.f, Index == QuickAmounts.Num() - 1 ? 0.f : KioskLayout::TileGap * 0.5f, 0.f)
		[
			SNew(SBox).MinDesiredHeight(110.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Tile)
				.Label(FText::Format(KT("{0} {1}"), KT("Top up"), Dollars))
				.ButtonTag(*FString::Printf(TEXT("Quick top up %d"), Amount / 100))
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Session, Amount]() { Session->QuickTopUp(Amount); return FReply::Handled(); })
				[
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(34.f, 0.f, 0.f, 24.f)
					[
						Text(FText::FromString(FString(TEXT("+")) + Dollars.ToString()), KioskType::Display, true, C.White)
					]
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(34.f, 0.f, 12.f, 24.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ Text(KT("New balance"), KioskType::Caption, false, C.White) ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents() + Amount), KioskType::Caption, true, C.White) ]
					]
				]
			]
		];
	}

	auto OptionCard = [&](EKioskIcon IconType, const FText& Title, const FText& Sub, const FText& Action, const FName& ButtonName, TFunction<void()> OnPick)
	{
		FChoiceCardArgs A;
		A.Icon = IconType;
		A.Title = Title;
		A.Description = Sub;
		A.DescColor = C.Muted;
		A.Footer = ActionBar(Action, true);
		A.Border = C.Ink;
		A.BorderWidth = 3.f;
		A.Padding = KioskLayout::CardPadTight; // compact card: keeps the row short enough for the quick top-up tiles above
		A.OnPick = OnPick;
		A.Tag = ButtonName;
		return ChoiceCard(A);
	};

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.58f).VAlign(VAlign_Center)[ TextWrap(KT("Card found. What would you like to do?"), KioskType::H1, true, C.Ink) ]
			+ SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)
			[
				// "Card options only ·" sits beside the link (Figma 156:29687); both wrap, so longer translations still fit.
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ TextWrap(KT("Card options only ·"), KioskType::Caption, false, C.Muted, KioskType::LineBody, ETextJustify::Right) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Link)
					.Label(KT("Buy a ticket instead"))
					.ButtonTag(TEXT("Buy a ticket instead"))
					.Padding(FMargin(8.f, 6.f))
					.MaxWidth(480.f)
					.OnClicked_Lambda([Session]()
					{
						Session->State.Flow = EKioskFlow::Ticket;
						Session->GoTo(EKioskScreen::TicketType);
						return FReply::Handled();
					})
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, KioskLayout::TitleGap, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.34f).Padding(0.f, 0.f, KioskLayout::Gutter, 0.f)[ CardPanel ]
			+ SHorizontalBox::Slot().FillWidth(0.66f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Quick top up"), KioskType::H3, true, C.Ink) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(28.f, 0.f, 0.f, 0.f)[ TextWrap(KT("One tap, then pay"), KioskType::Caption, false, C.Muted) ]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 12.f, 0.f, 0.f)[ Tiles ] // tiles absorb spare/lost height; option row below is auto-height
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 12.f)[ Text(KT("Other card options"), KioskType::H3, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					// Auto height with a minimum: both cards grow together for longer titles, so each button stays pinned at the card bottom.
					SNew(SBox).MinDesiredHeight(350.f)
					[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, KioskLayout::TileGap * 0.5f, 0.f)
					[
						OptionCard(EKioskIcon::TopUpCard, KT("Top up another amount"), KT("$30, $100 or any amount"), KT("Choose amount"), TEXT("Top up another amount"),
							[Session]() { Session->State.Flow = EKioskFlow::TopUp; Session->OpenTopUpKeypad(false); Session->GoTo(EKioskScreen::TopUpAmount); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(KioskLayout::TileGap * 0.5f, 0.f, 0.f, 0.f)
					[
						OptionCard(EKioskIcon::IdCard,
							Info.bHasActivePass ? KT("Renew concession pass") : KT("Buy a concession pass"),
							Info.bHasActivePass ? FText::Format(KT("Your {0} pass ends {1}"), KT("Bus + Train"), Session->CurrentPassEndText()) : KT("Bus, train or both for 30 days"),
							Info.bHasActivePass ? KT("Renew pass") : KT("Buy pass"),
							Info.bHasActivePass ? FName(TEXT("Renew concession pass")) : FName(TEXT("Buy concession pass")),
							[Session]() { Session->State.Flow = EKioskFlow::Concession; Session->GoTo(EKioskScreen::Concession); })
					]
					]
				]
			]
		]
	];
}

// --- top-up amount -------------------------------------------------------------------------

namespace
{
	TSharedRef<SWidget> KeypadKey(const TSharedRef<FKioskSession>& Session, int32 Digit)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Key)
			.Label(FText::AsNumber(Digit))
			.ButtonTag(*FString::Printf(TEXT("Keypad %d"), Digit))
			.FontPx(KioskType::H1)
			.MinHeight(0.f)
			.OnClicked_Lambda([Session, Digit]() { Session->PressTopUpDigit(Digit); return FReply::Handled(); });
	}

	TSharedRef<SWidget> KeypadRow(const TSharedRef<FKioskSession>& Session, int32 A, int32 B, int32 C)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)[ KeypadKey(Session, A) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)[ KeypadKey(Session, B) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)[ KeypadKey(Session, C) ];
	}
}

void STopUpScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const int32 Selected = Session->State.TopUpCents;
	const bool bHasSelection = Selected > 0;

	TSharedRef<SWidget> LeftPanel = Card(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(Session->CardLabel(), KioskType::Caption, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ Text(KT("Balance now"), KioskType::Caption, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents()), KioskType::Display, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 18.f)[ Rule(C.BorderStrong, 2.f) ]
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("After top-up"), KioskType::Caption, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			bHasSelection
				? Text(FKioskSession::Money(Session->NewBalanceCents()), KioskType::Display, true, C.Primary)
				: Text(FText::FromString(TEXT("—")), KioskType::Display, true, C.Muted)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bHasSelection ? 6.f : 18.f, 0.f, 0.f)
		[
			bHasSelection
				? StaticCastSharedRef<SWidget>(TextWrap(FText::Format(KT("+{0} added"), FKioskSession::Money(Selected)), KioskType::Caption, false, C.Muted, KioskType::LineBody))
				: StaticCastSharedRef<SWidget>(TextWrap(KT("Pick an amount"), KioskType::H1, false, C.Ink, KioskType::LineCompact)) // Figma 156:30545: the prompt is large
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight()[ KeepCardNote() ],
		C.Band, C.BorderStrong, 2.f, KioskLayout::CardPadWide);

	// The preset amounts come from the data layer; the last tile (Cents 0) opens the keypad.
	TArray<int32> AmountTiles = FKioskData::TopUpAmounts();
	AmountTiles.Add(0);
	constexpr float AmountTileHeight = 170.f; // Figma 156:30545: compact tiles with Continue close under the grid

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	for (int32 Row = 0; Row < 2; ++Row)
	{
		TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
		for (int32 Col = 0; Col < 3; ++Col)
		{
			const int32 Cents = AmountTiles.IsValidIndex(Row * 3 + Col) ? AmountTiles[Row * 3 + Col] : 0;
			const bool bMore = Cents == 0;
			const bool bSelected = !bMore && Selected == Cents;
			const FText Label = bMore ? KT("More") : FText::Format(KT("${0}"), FText::AsNumber(Cents / 100));
			Cells->AddSlot().FillWidth(1.f).Padding(Col == 0 ? 0.f : 8.f, 0.f, Col == 2 ? 0.f : 8.f, 0.f)
			[
				Panel(
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Card)
					.Label(Label)
					.ButtonTag(*(bMore ? FString(TEXT("Amount More")) : FString::Printf(TEXT("Amount $%d"), Cents / 100)))
					.Padding(FMargin(0.f))
					.OnClicked_Lambda([Session, Cents, bMore]()
					{
						if (bMore) { Session->OpenTopUpKeypad(true); } else { Session->SelectTopUp(Cents); }
						return FReply::Handled();
					})
					[
						SNew(SOverlay)
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(Label, KioskType::Display, true, C.Ink) ]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 14.f, 14.f, 0.f)
						[
							bSelected ? CheckBadge() : StaticCastSharedRef<SWidget>(SNew(SSpacer))
						]
					],
					bSelected ? C.PrimaryTint : C.Surface, KioskLayout::CardRadius, bSelected ? C.Ink : C.BorderStrong, bSelected ? 4.f : 2.f)
			];
		}
		Grid->AddSlot().AutoHeight().Padding(0.f, Row == 0 ? 0.f : 8.f, 0.f, Row == 0 ? 8.f : 0.f)[ SNew(SBox).HeightOverride(AmountTileHeight)[ Cells ] ];
	}

	// Custom amount: whole-dollar numeric keypad ($5 to $500).
	TSharedRef<SWidget> Keypad = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
		[
			Card(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Text(FText::Format(KT("${0}"), FText::FromString(Session->State.TopUpDigits.IsEmpty() ? TEXT("0") : *Session->State.TopUpDigits)), KioskType::Display, true, C.Ink)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 0.f, 0.f, 0.f)
				[
					TextWrap(FText::Format(KT("Enter a whole-dollar amount from ${0} to ${1}"), FText::AsNumber(FKioskSession::MinTopUpDollars), FText::AsNumber(FKioskSession::MaxTopUpDollars)),
						KioskType::Caption, false, C.Muted, KioskType::LineBody, ETextJustify::Right)
				],
				C.Surface, C.Ink, 3.f, KioskLayout::CardPadTight)
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)[ KeypadRow(Session, 1, 2, 3) ]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)[ KeypadRow(Session, 4, 5, 6) ]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)[ KeypadRow(Session, 7, 8, 9) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SKioskButton).Kind(EKioskButtonKind::KeyMuted).Label(KT("Back to amounts")).ButtonTag(TEXT("Back to amounts")).MinHeight(0.f)
					.Padding(FMargin(10.f, 4.f))
					.OnClicked_Lambda([Session]() { Session->OpenTopUpKeypad(false); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)[ KeypadKey(Session, 0) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SKioskButton).Kind(EKioskButtonKind::KeyMuted).Label(KT("Backspace")).ButtonTag(TEXT("Keypad Backspace")).MinHeight(0.f)
					.OnClicked_Lambda([Session]() { Session->BackspaceTopUp(); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Backspace, KioskLayout::IconSm, C.Ink, 3.f)
					]
				]
			]
		];

	TSharedRef<SWidget> AmountArea = Session->State.bTopUpKeypad ? Keypad : StaticCastSharedRef<SWidget>(Grid);

	// Keypad fills the column; the compact amount grid keeps its height so Continue sits right under it.
	TSharedRef<SVerticalBox> RightColumn = SNew(SVerticalBox);
	if (Session->State.bTopUpKeypad)
	{
		RightColumn->AddSlot().FillHeight(1.f)[ AmountArea ];
	}
	else
	{
		RightColumn->AddSlot().AutoHeight()[ AmountArea ];
	}
	RightColumn->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 16.f, 0.f, 0.f)
	[
		SNew(SKioskButton)
		.Kind(EKioskButtonKind::Primary)
		.Label(KT("Continue"))
		.ButtonTag(TEXT("Continue"))
		.RightIcon(EKioskIcon::ArrowRight)
		.MinWidth(KioskLayout::PrimaryMinW).MaxWidth(KioskLayout::PrimaryMaxW)
		.Enabled(bHasSelection)
		.OnClicked_Lambda([Session]() { Session->ContinueTopUp(); return FReply::Handled(); })
	];
	if (!Session->State.bTopUpKeypad)
	{
		RightColumn->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	}

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, KioskLayout::TitleGap)[ TextWrap(KT("How much would you like to add?"), KioskType::H1, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.32f).Padding(0.f, 0.f, KioskLayout::Gutter, 0.f)[ LeftPanel ]
			+ SHorizontalBox::Slot().FillWidth(0.68f)
			[
				RightColumn
			]
		]
	];
}

// --- concession ----------------------------------------------------------------------------

void SConcessionScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	FKioskStyle& Style = FKioskStyle::Get();
	const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
	const int32 Selected = Session->State.Concession;
	const bool bStudentCard = Session->IsStudentCard();

	TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Passes.Num(); ++Index)
	{
		const FKioskConcession& Pass = Passes[Index];
		const bool bSelected = Selected == Index;
		const bool bCurrent = Index == Session->CurrentPass();

		TSharedRef<SHorizontalBox> Icons = SNew(SHorizontalBox);
		auto AddIcon = [&](EKioskIcon IconType, bool bFirst)
		{
			Icons->AddSlot().AutoWidth().Padding(bFirst ? 0.f : 10.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).WidthOverride(KioskLayout::IconDiscLg).HeightOverride(KioskLayout::IconDiscLg)
				[
					Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(IconType, KioskLayout::IconDiscLg * 0.6f, C.Ink, 3.f) ], C.Band, KioskLayout::InnerRadius)
				]
			];
		};
		if (Pass.bBus) { AddIcon(EKioskIcon::Bus, true); }
		if (Pass.bTrain) { AddIcon(EKioskIcon::Train, !Pass.bBus); }

		const FText PassName = FKioskData::ConcessionName(Index);
		Cards->AddSlot().FillWidth(1.f).Padding(Index == 0 ? 0.f : 10.f, 0.f, Index == 2 ? 0.f : 10.f, 0.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(PassName)
				.ButtonTag(*FString::Printf(TEXT("Pass %s"), Pass.Name))
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Session, Index]() { Session->SelectConcession(Index); return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ Icons ]
						+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
						[
							bSelected ? CheckBadge() : StaticCastSharedRef<SWidget>(SNew(SSpacer))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f).HAlign(HAlign_Left)
					[
						bCurrent
							? Pill(KT("Your current pass"), C.HelpTint, C.Ink, KioskType::Caption, true, C.Help, FMargin(18.f, 6.f))
							: StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(1.f, 40.f)))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[ TextWrap(PassName, KioskType::H2, true, C.Ink, KioskType::LineCompact) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ TextWrap(FKioskData::ConcessionDescription(Index), KioskType::Body, false, C.Muted, KioskType::LineBody) ]
					+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						// A student card shows the adult price struck through; an adult card just shows its own price.
							bStudentCard
								? StaticCastSharedRef<SWidget>(SNew(STextBlock)
									.Text(FText::Format(KT("Adult {0}"), FKioskSession::Money(Pass.AdultCents)))
									.Font(Style.Font(KioskType::Caption))
									.ColorAndOpacity(C.Muted)
									.StrikeBrush(Style.Solid(C.Muted)))
								: StaticCastSharedRef<SWidget>(SNew(SSpacer))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->PassPriceCents(Index)), KioskType::Display, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight()[ Text(KT("for 30 days"), KioskType::Caption, false, C.Muted) ]
				],
				bSelected ? C.PrimaryTint : C.Surface, KioskLayout::CardRadius, bSelected ? C.Ink : C.BorderStrong, bSelected ? 4.f : 2.f, KioskLayout::CardPadWide)
		];
	}

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Choose a concession pass"), KioskType::H1, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bStudentCard ? KioskLayout::TitleGap : 0.f, 0.f, 0.f)
		[
			// Only a student card gets the banner; an adult card sees plain adult prices.
			bStudentCard
				? StaticCastSharedRef<SWidget>(Card(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::IdCard, KioskLayout::IconSm, C.Primary, 3.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)[ Text(KT("Student card detected"), KioskType::H3, true, C.Ink) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)[ TextWrap(KT("· student concession prices shown"), KioskType::Body, false, C.Ink) ],
					C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadTight))
				: StaticCastSharedRef<SWidget>(SNew(SSpacer))
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 22.f, 0.f, 20.f)[ Cards ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				TextWrap(KT("Every pass lasts 30 days with unlimited rides. Next, you'll see the exact dates it covers before you pay."), KioskType::Body, false, C.Ink, KioskType::LineBody)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(24.f, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Primary)
				.Label(KT("Continue · check dates"))
				.ButtonTag(TEXT("Continue · check dates"))
				.RightIcon(EKioskIcon::ArrowRight)
				.MinWidth(KioskLayout::PrimaryMinW).MaxWidth(KioskLayout::PrimaryMaxW)
				.Enabled(Selected != INDEX_NONE)
				.OnClicked_Lambda([Session]() { Session->GoTo(EKioskScreen::PassDates); return FReply::Handled(); })
			]
		]
	];
}

// --- pass dates ----------------------------------------------------------------------------

namespace
{
	/** One month of a Monday-first calendar. Day cells are flat (no button look): a coloured band shows the pass ranges. */
	TSharedRef<SWidget> MonthGrid(const FText& Title, int32 LeadingBlanks, int32 DaysInMonth, int32 FirstNewDay, int32 LastCurrentDay, int32 LastNewDay,
		int32 LastPastDay = 0, TFunction<TSharedRef<SWidget>(int32, const TSharedRef<SWidget>&)> MakeTappable = nullptr)
	{
		const FKioskColors& C = KC();
		TSharedRef<SVerticalBox> Calendar = SNew(SVerticalBox);
		{
			TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
			for (int32 Day = 0; Day < 7; ++Day)
			{
				FText Name;
				switch (Day)
				{
				case 0: Name = KT("Mon"); break;
				case 1: Name = KT("Tue"); break;
				case 2: Name = KT("Wed"); break;
				case 3: Name = KT("Thu"); break;
				case 4: Name = KT("Fri"); break;
				case 5: Name = KT("Sat"); break;
				default: Name = KT("Sun"); break;
				}
				Header->AddSlot().FillWidth(1.f).HAlign(HAlign_Center)[ Text(Name, KioskType::Caption, true, C.Muted) ];
			}
			Calendar->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[ Header ];
		}
		const int32 Weeks = (LeadingBlanks + DaysInMonth + 6) / 7;
		for (int32 Week = 0; Week < Weeks; ++Week)
		{
			TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
			for (int32 Col = 0; Col < 7; ++Col)
			{
				const int32 DayNumber = Week * 7 + Col - LeadingBlanks + 1;
				TSharedRef<SWidget> Cell = SNew(SSpacer);
				if (DayNumber >= 1 && DayNumber <= DaysInMonth)
				{
					const bool bCurrent = LastCurrentDay > 0 && DayNumber <= LastCurrentDay;
					const bool bNew = DayNumber >= FirstNewDay && DayNumber <= LastNewDay;
					const bool bMark = DayNumber == LastCurrentDay || DayNumber == FirstNewDay || DayNumber == LastNewDay;
					const FLinearColor Fill = bCurrent ? C.HelpTint : (bNew ? C.PrimaryTint : C.Transparent);
					const FLinearColor Mark = bCurrent ? C.Help : C.Primary;
					Cell = SNew(SOverlay)
						+ SOverlay::Slot()[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(Fill)).BorderBackgroundColor(FLinearColor::White) ]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(DayNumber), KioskType::Body, bMark, DayNumber <= LastPastDay ? C.Muted : C.Ink) ]
						+ SOverlay::Slot().VAlign(VAlign_Bottom)
						[
							bMark
								? StaticCastSharedRef<SWidget>(SNew(SBox).HeightOverride(5.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(Mark)).BorderBackgroundColor(FLinearColor::White) ])
								: StaticCastSharedRef<SWidget>(SNew(SSpacer))
						];
					if (MakeTappable && DayNumber > LastPastDay)
					{
						Cell = MakeTappable(DayNumber, Cell); // start-date picking: today and later are buttons, past days stay flat
					}
				}
				Cells->AddSlot().FillWidth(1.f)[ SNew(SBox).HeightOverride(50.f)[ Cell ] ];
			}
			Calendar->AddSlot().AutoHeight()[ Cells ];
		}
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[ Text(Title, KioskType::H3, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight()[ Calendar ];
	}
}

void SPassDatesScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const int32 PassIndex = Session->State.Concession;
	// A card with an active pass renews it (the sample runs 15 Nov to 14 Dec 2026); a card without one picks a start day.
	const bool bRenewing = Session->CurrentPass() != INDEX_NONE;
	const bool bPicked = !bRenewing && Session->HasPassStart();

	const FKioskCardInfo& Info = Session->CardInfo();
	const FDateTime Today = FKioskSession::Today();
	// Renewing: the two month grids start at the month the card's pass ends in. Otherwise they start at today's month.
	const bool bCardAnchored = bRenewing && Info.PassEndDay > 0;
	const FDateTime Anchor = bCardAnchored ? FDateTime(Info.PassEndYear, Info.PassEndMonth, Info.PassEndDay) : Today;
	const FDateTime FirstMonth(Anchor.GetYear(), Anchor.GetMonth(), 1);
	const int32 SecondYear = Anchor.GetMonth() == 12 ? Anchor.GetYear() + 1 : Anchor.GetYear();
	const int32 SecondMonthNumber = Anchor.GetMonth() == 12 ? 1 : Anchor.GetMonth() + 1;
	const FDateTime SecondMonth(SecondYear, SecondMonthNumber, 1);
	const FText FirstMonthTitle = FKioskSession::MonthYearText(FirstMonth);
	const FText SecondMonthTitle = FKioskSession::MonthYearText(SecondMonth);
	const int32 FirstLead = static_cast<int32>(FirstMonth.GetDayOfWeek()); // Monday = 0
	const int32 FirstDays = FDateTime::DaysInMonth(FirstMonth.GetYear(), FirstMonth.GetMonth());
	const int32 SecondLead = static_cast<int32>(SecondMonth.GetDayOfWeek());
	const int32 SecondDays = FDateTime::DaysInMonth(SecondMonth.GetYear(), SecondMonth.GetMonth());
	const int32 FirstLastCurrent = bCardAnchored ? Anchor.GetDay() : 0;
	const int32 FirstPast = bRenewing ? 0 : Today.GetDay() - 1;

	// The chosen range [Start, End] clipped to each month: 0/0 when it does not touch the month, and one past the last day
	// when it runs on into the next month (so no end marker is drawn there). Correct for a pass that spans three months.
	int32 FirstNewDay = 0, FirstLastNew = 0, SecondNewDay = 0, SecondLastNew = 0;
	FDateTime Start, End;
	bool bHasRange = false;
	if (bCardAnchored)
	{
		Start = Anchor + FTimespan::FromDays(1); // the new pass starts the day after the current one ends
		End = Start + FTimespan::FromDays(29);   // 30 days in all, same rule as PassEndDate()
		bHasRange = true;
	}
	else if (bPicked)
	{
		Start = Session->PassStartDate();
		End = Session->PassEndDate();
		bHasRange = true;
	}
	if (bHasRange)
	{
		auto Clip = [&Start, &End](const FDateTime& Month, int32 Days, int32& OutFirst, int32& OutLast)
		{
			const FDateTime MonthEnd(Month.GetYear(), Month.GetMonth(), Days);
			if (End < Month || Start > MonthEnd)
			{
				return;
			}
			OutFirst = Start >= Month ? Start.GetDay() : 0;
			OutLast = End <= MonthEnd + FTimespan::FromHours(23) ? End.GetDay() : Days + 1;
		};
		Clip(FirstMonth, FirstDays, FirstNewDay, FirstLastNew);
		Clip(SecondMonth, SecondDays, SecondNewDay, SecondLastNew);
	}

	TFunction<TSharedRef<SWidget>(int32, const TSharedRef<SWidget>&)> FirstTap, SecondTap;
	if (!bRenewing)
	{
		auto MakeTap = [Session](int32 Year, int32 Month)
		{
			return TFunction<TSharedRef<SWidget>(int32, const TSharedRef<SWidget>&)>([Session, Year, Month](int32 Day, const TSharedRef<SWidget>& Cell) -> TSharedRef<SWidget>
			{
				const FDateTime Date(Year, Month, Day);
				// Name "Day 2026-11-20"; label "Day +3" (days from today) so a journey can click a day whatever the real date is.
				const int32 Offset = (Date - FKioskSession::Today()).GetDays();
				return SNew(SKioskButton)
					.Kind(EKioskButtonKind::Card)
					.Label(FText::FromString(FString::Printf(TEXT("Day %+d"), Offset)))
					.ButtonTag(*FString::Printf(TEXT("Day %04d-%02d-%02d"), Year, Month, Day))
					.Padding(FMargin(0.f))
					.OnClicked_Lambda([Session, Date]() { Session->ChoosePassStart(Date); return FReply::Handled(); })
					[
						Cell
					];
			});
		};
		FirstTap = MakeTap(FirstMonth.GetYear(), FirstMonth.GetMonth());
		SecondTap = MakeTap(SecondMonth.GetYear(), SecondMonth.GetMonth());
	}

	auto Legend = [&](const FLinearColor& Swatch, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(28.f).HeightOverride(28.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(Swatch)).BorderBackgroundColor(FLinearColor::White) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 32.f, 0.f)[ Text(Label, KioskType::Caption, false, C.Ink) ];
	};

	// November 2026 starts on a Sunday and December on a Tuesday (Monday-first grid). The current pass ends 14 Nov, the new one runs 15 Nov to 14 Dec.
	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, KioskLayout::TitleGap)[ Text(KT("Check your pass dates"), KioskType::H1, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.6f).Padding(0.f, 0.f, KioskLayout::Gutter, 0.f)
			[
				Card(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 30.f, 0.f)[ MonthGrid(FirstMonthTitle, FirstLead, FirstDays, FirstNewDay, FirstLastCurrent, FirstLastNew, FirstPast, FirstTap) ]
						+ SHorizontalBox::Slot().FillWidth(1.f)[ MonthGrid(SecondMonthTitle, SecondLead, SecondDays, SecondNewDay, 0, SecondLastNew, 0, SecondTap) ]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ bRenewing ? Legend(C.HelpTint, FText::Format(KT("Current pass · ends {0}"), Session->CurrentPassEndText())) : StaticCastSharedRef<SWidget>(SNew(SSpacer)) ]
						+ SHorizontalBox::Slot().AutoWidth()[ Legend(C.PrimaryTint, KT("New pass validity")) ]
					],
					C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide)
			]
			+ SHorizontalBox::Slot().FillWidth(0.4f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Card(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ TextWrap(FText::Format(Session->IsStudentCard() ? KT("{0} – Student") : KT("{0} – Adult"), FKioskData::ConcessionName(PassIndex)), KioskType::H2, true, C.Ink, KioskType::LineCompact) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(KT("30 days · unlimited rides"), KioskType::Body, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 18.f)[ Rule(C.BorderStrong, 2.f) ]
						+ SVerticalBox::Slot().AutoHeight()[ Text(bRenewing || bPicked ? KT("New pass") : KT("No pass active"), KioskType::Caption, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
						[
							TextWrap(bRenewing ? Session->RenewalRangeText(true)
								: (bPicked ? FText::Format(KT("{0} – {1}"), DayMonth(Session->PassStartDate()), DayMonth(Session->PassEndDate())) : KT("Select a start date")),
								KioskType::H2, true, (bRenewing || bPicked) ? C.Primary : C.Ink, KioskType::LineCompact)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, bRenewing || bPicked ? 14.f : 0.f, 0.f, 0.f)
						[
							bRenewing || bPicked
								? StaticCastSharedRef<SWidget>(TextWrap(bRenewing ? FText::Format(KT("Current pass ends {0}"), Session->CurrentPassEndText(true)) : FText::Format(KT("Starts {0}"), DayMonth(Session->PassStartDate())), KioskType::Body, false, C.Ink, KioskType::LineBody))
								: StaticCastSharedRef<SWidget>(SNew(SSpacer))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
						[
							TextWrap(bRenewing ? KT("Your new pass starts the day after it ends, so you don't lose any days.")
								: (bPicked ? KT("Your pass lasts 30 days from the day you chose.") : KT("Tap a day on the calendar. Your pass lasts 30 days from that day.")),
								KioskType::Body, false, C.Muted, KioskType::LineBody)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->PassPriceCents(PassIndex)), KioskType::Display, true, C.Ink) ],
						C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Primary)
					.Label(KT("Go to payment"))
					.ButtonTag(TEXT("Go to payment"))
					.RightIcon(EKioskIcon::ArrowRight)
					.Enabled(bRenewing || bPicked)
					.OnClicked_Lambda([Session]() { Session->GoToPayment(); return FReply::Handled(); })
				]
			]
		]
	];
}

// --- done ----------------------------------------------------------------------------------

void SDoneScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bTopUp = Session->State.Flow == EKioskFlow::TopUp;

	const FText Body = bTopUp
		? KT("Your top-up is done. Lift your Metro card off the reader below the screen before you go.")
		: KT("Your purchase is done. Lift your Metro card off the reader below the screen before you go.");

	// Renewal range: starts the day after the card's pass ends and runs 30 days (same rule as FKioskSession::RenewalRangeText).
	FDateTime RenewalStart, RenewalEnd;
	const bool bHasRenewal = Session->RenewalRange(RenewalStart, RenewalEnd);

	TSharedRef<SWidget> BalanceDetail = SNullWidget::NullWidget;
	if (bTopUp)
	{
		BalanceDetail = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				TextWrap(FText::Format(KT("+{0} added · was {1}"), FKioskSession::Money(Session->State.TopUpCents), FKioskSession::Money(Session->CardBalanceCents())), KioskType::Body, true, C.Ink, KioskType::LineBody)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ Text(KT("Ready to use now"), KioskType::Body, false, C.Muted) ];
	}
	else
	{
		BalanceDetail = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				TextWrap(bHasRenewal ? FText::Format(KT("New pass validity · {0} – {1}"), DayMonth(RenewalStart), DayMonth(RenewalEnd)) : FText::Format(KT("New pass validity · {0} – {1}"), DayMonth(Session->PassStartDate()), DayMonth(Session->PassEndDate())),
					KioskType::Body, true, C.Ink, KioskType::LineBody)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				TextWrap(bHasRenewal ? FText::Format(KT("Current pass · ends {0}"), Session->CurrentPassEndText()) : FText::Format(KT("Starts {0}"), DayMonth(Session->PassStartDate())), KioskType::Body, false, C.Muted, KioskType::LineBody)
			];
	}

	const int32 Balance = bTopUp ? Session->NewBalanceCents() : Session->CardBalanceCents();

	TSharedRef<SWidget> Illustration = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			Positioned(Panel(SNew(SBox).Padding(20.f)[ Panel(SNew(SSpacer), C.Brand, 8.f) ], C.Ink, 18.f),
				FVector2D(90.f, 10.f), FVector2D(480.f, 260.f))
		]
		+ SOverlay::Slot()[ Positioned(Panel(SNew(SSpacer), C.Primary, 6.f), FVector2D(300.f, 290.f), FVector2D(60.f, 12.f)) ]
		+ SOverlay::Slot()[ Positioned(MetroCardGraphic(220.f, 140.f), FVector2D(255.f, 300.f), FVector2D(220.f, 140.f), -3.f) ]
		+ SOverlay::Slot()
		[
			Positioned(
				SNew(SBox).Clipping(EWidgetClipping::ClipToBounds)
				[
					SNew(SOverlay)
					+ SOverlay::Slot().VAlign(VAlign_Top)[ Positioned(Panel(SNew(SSpacer), C.BorderStrong, 165.f, C.Ink, 4.f), FVector2D(0.f, -165.f), FVector2D(330.f, 330.f)) ]
					+ SOverlay::Slot().VAlign(VAlign_Top)[ SNew(SBox).HeightOverride(4.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(C.Ink)).BorderBackgroundColor(FLinearColor::White) ] ]
				],
				FVector2D(150.f, 400.f), FVector2D(330.f, 165.f))
		]
		+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::ArrowUp, 140.f, C.Primary, 6.f), FVector2D(500.f, 330.f), FVector2D(70.f, 200.f)) ];

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.54f).Padding(0.f, 0.f, 44.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(KioskLayout::IconDiscLg).HeightOverride(KioskLayout::IconDiscLg)
					[
						Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::FromString(TEXT("!")), KioskType::Display, true, C.White) ], C.AmberStrong, KioskLayout::IconDiscLg * 0.5f)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 0.f, 0.f, 0.f)[ TextWrap(KT("Don't forget your card"), KioskType::Display, true, C.Ink, KioskType::LineCompact) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ TextWrap(Body, KioskType::Body, false, C.Ink, KioskType::LineBody) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(C.AmberStrong, 16.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ TextWrap(KT("Card still on the reader"), KioskType::Body, true, C.Ink) ],
					C.AmberFill, 26.f, C.AmberStroke, 2.f, FMargin(26.f, 10.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				Card(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(FText::Format(KT("{0} · {1}"), Session->CardLabel(), FText::FromString(FString(TEXT("•••• ")) + Session->CardInfo().Last4)), KioskType::Caption, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(KT("Balance"), KioskType::Caption, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ Text(FKioskSession::Money(Balance), KioskType::Figure, true, C.Ink) ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(40.f, 0.f, 0.f, 0.f)[ BalanceDetail ],
					C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadTight)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, Session->ChangeCents() > 0 ? 10.f : 0.f, 0.f, 0.f)
			[
				Session->ChangeCents() > 0
					? StaticCastSharedRef<SWidget>(TextWrap(FText::Format(KT("Take your change from the tray: {0}"), FKioskSession::Money(Session->ChangeCents())), KioskType::Body, true, C.Primary))
					: StaticCastSharedRef<SWidget>(SNew(SSpacer))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				ReceiptActions(Session, true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ReturnCountdown(Session)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SimControl(KT("SIMULATE: lift card off reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateCardLift(); return FReply::Handled(); }), TEXT("SIMULATE: lift card off reader"))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.46f)
		[
			Card(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
					[
						SNew(SBox).WidthOverride(640.f).HeightOverride(580.f)[ Illustration ]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ HardwareHint(Mask(EHardware::MetroReader), 0.7f) ],
				C.Band, C.Transparent, 0.f, KioskLayout::CardPadWide)
		]
	];
}
