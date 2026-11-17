// Waiting-for-payment screens (tap / chip / cash / processing) and the receipt.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskDashedBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SCanvas.h"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> CardIllustrationTap()
	{
		const FKioskColors& C = KC();
		TSharedRef<SWidget> Card = Panel(
			SNew(SOverlay)
			+ SOverlay::Slot().VAlign(VAlign_Top).Padding(0.f, 44.f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(52.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(C.Help)).BorderBackgroundColor(FLinearColor::White) ]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 10.f, 0.f, 0.f)[ Text(KT("Bank card"), KioskType::Body, true, C.Ink) ]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(26.f, 0.f, 0.f, 26.f)[ Panel(SNew(SSpacer).Size(FVector2D(52.f, 40.f)), C.Chip, 6.f) ]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 30.f, 34.f)[ Panel(SNew(SSpacer).Size(FVector2D(120.f, 12.f)), C.BorderStrong, 6.f) ],
			C.Surface, 22.f, C.Ink, 4.f);

		TSharedRef<SWidget> Reader = Panel(
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				Panel(SNew(SSpacer).Size(FVector2D(190.f, 52.f)), C.Primary, 12.f)
			],
			C.Ink, 24.f);

		return SNew(SOverlay)
			+ SOverlay::Slot()[ Positioned(Card, FVector2D(110.f, 30.f), FVector2D(365.f, 235.f), -8.f) ]
			+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::Contactless, 150.f, C.Primary, 5.f), FVector2D(205.f, 235.f), FVector2D(150.f, 150.f), -90.f) ]
			+ SOverlay::Slot()[ Positioned(Reader, FVector2D(125.f, 400.f), FVector2D(340.f, 100.f)) ];
	}

	TSharedRef<SWidget> CardIllustrationChip()
	{
		const FKioskColors& C = KC();
		TSharedRef<SWidget> Card = Panel(
			SNew(SOverlay)
			+ SOverlay::Slot().VAlign(VAlign_Top)[ SNew(SBox).HeightOverride(46.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(C.Help)).BorderBackgroundColor(FLinearColor::White) ] ]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 70.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Chip first"), KioskType::H3, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ Text(KT("facing up"), KioskType::Caption, false, C.Ink) ]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 190.f, 0.f, 0.f)[ Panel(SNew(SSpacer).Size(FVector2D(62.f, 50.f)), C.Chip, 8.f) ],
			C.Surface, 22.f, C.Ink, 4.f);

		TSharedRef<SWidget> Slot = Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(30.f, 18.f, 30.f, 0.f)[ Panel(SNew(SSpacer).Size(FVector2D(1.f, 14.f)), C.DeviceDark, 7.f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(40.f, 28.f, 40.f, 0.f)[ Panel(SNew(SSpacer).Size(FVector2D(1.f, 14.f)), C.Primary, 7.f) ],
			C.Ink, 20.f);

		return SNew(SOverlay)
			+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::ArrowDown, 130.f, C.Primary, 5.f), FVector2D(40.f, 110.f), FVector2D(70.f, 150.f)) ]
			+ SOverlay::Slot()[ Positioned(Card, FVector2D(160.f, 30.f), FVector2D(205.f, 340.f)) ]
			+ SOverlay::Slot()[ Positioned(Slot, FVector2D(80.f, 350.f), FVector2D(366.f, 130.f)) ];
	}

	TSharedRef<SWidget> StatusPill(const TSharedRef<FKioskSession>& Session, bool bProcessing, bool bCash)
	{
		const FKioskColors& C = KC();
		TSharedRef<SWidget> Label = SNullWidget::NullWidget;
		if (bProcessing)
		{
			Label = TextWrap(KT("Processing..."), KioskType::Body, true, C.Ink);
		}
		else
		{
			Label = TextDyn([Session, bCash]()
			{
				const int32 Seconds = Session->PaySecondsLeft();
				return FText::Format(bCash ? KT("Waiting for cash · {0}:{1} left") : KT("Waiting for your card · {0}:{1} left"),
					FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));
			}, KioskType::Body, true, C.Ink);
		}
		return Panel(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(bProcessing ? C.Help : C.Primary, 16.f) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Label ],
			C.Surface, 26.f, C.BorderStrong, 2.f, FMargin(28.f, 12.f));
	}

	TSharedRef<SWidget> CashPanel(const TSharedRef<FKioskSession>& Session, bool bProcessing)
	{
		const FKioskColors& C = KC();
		const int32 Total = Session->TotalCents();
		const int32 Inserted = Session->State.CashInsertedCents;
		const int32 Remaining = FMath::Max(0, Total - Inserted);
		const int32 Change = Session->ChangeCents();

		auto Tile = [&](const FText& Label, const FText& Value, const FLinearColor& Fill, const FLinearColor& Ink, const FLinearColor& LabelColor)
		{
			return Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ TextWrap(Label, KioskType::Caption, false, LabelColor, KioskType::LineCompact) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ Text(Value, KioskType::Figure, true, Ink) ],
				Fill, KioskLayout::InnerRadius, C.Transparent, 0.f, KioskLayout::CardPadTight);
		};

		const bool bShowChange = bProcessing && Change > 0;
		auto Chip = [&](const FText& Label, int32 Cents, bool bCoin)
		{
			return SNew(SKioskButton)
				.Kind(bCoin ? EKioskButtonKind::PillOutline : EKioskButtonKind::Key)
				.Label(Label)
				.ButtonTag(*FString::Printf(TEXT("Insert %s"), *Label.ToString()))
				.FontPx(KioskType::BtnStd)
				.MinWidth(KioskLayout::ChipMinW).MinHeight(KioskLayout::KeyH)
				.Padding(FMargin(8.f, 4.f))
				.Enabled(!bProcessing)
				.OnClicked_Lambda([Session, Cents]() { Session->InsertCash(Cents); return FReply::Handled(); });
		};
		// The accepted notes or coins from the shared denomination list, two chips per row.
		auto ChipGrid = [&](bool bCoin)
		{
			TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
			TSharedPtr<SHorizontalBox> Row;
			int32 InRow = 0;
			for (const FKioskCashDenom& Denom : FKioskData::CashDenominations())
			{
				if (Denom.bCoin != bCoin)
				{
					continue;
				}
				if (InRow % 2 == 0)
				{
					Row = SNew(SHorizontalBox);
					Grid->AddSlot().AutoHeight().Padding(0.f, InRow == 0 ? 10.f : 12.f, 0.f, 0.f)[ Row.ToSharedRef() ];
				}
				Row->AddSlot().AutoWidth().Padding(InRow % 2 == 0 ? 0.f : 12.f, 0.f, 0.f, 0.f)[ Chip(FText::FromString(Denom.Label), Denom.Cents, bCoin) ];
				++InRow;
			}
			return Grid;
		};

		const float Progress = Total > 0 ? static_cast<float>(FMath::Min(Inserted, Total)) / Total : 0.f;

		TSharedRef<SWidget> Slots = SNew(SKioskDashedBox)
			.DashColor(C.SimInk)
			.Fill(C.SimFill)
			.Padding(FMargin(22.f, 14.f, 22.f, 16.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[
					TextWrap(KT("SIMULATE: tap a note or coin to insert it"), KioskType::Caption, true, C.SimInk)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Cash, KioskLayout::IconSm, C.Ink, 3.f) ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)[ Text(KT("Notes"), KioskType::Body, true, C.Ink) ]
						]
						+ SVerticalBox::Slot().AutoHeight()[ ChipGrid(false) ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(20.f, 0.f, 0.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(SBox).Padding(FMargin(0.f, 0.f, 12.f, 0.f))[ Icon(EKioskIcon::Coin, KioskLayout::IconSm, C.Ink, 3.f) ] ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Coins"), KioskType::Body, true, C.Ink) ]
						]
						+ SVerticalBox::Slot().AutoHeight()[ ChipGrid(true) ]
					]
				]
			];

		return Card(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
				[
					Tile(KT("To pay"), FKioskSession::Money(Total), C.Band, C.Ink, C.Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)
				[
					Tile(KT("Put in"), FKioskSession::Money(Inserted), C.Band, C.Ink, C.Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
				[
					bShowChange
						? Tile(KT("Change in the tray"), FKioskSession::Money(Change), C.Primary, C.White, C.White)
						: Tile(KT("Still to pay"), FKioskSession::Money(Remaining), C.Ink, C.White, C.White)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
			[
				ProgressBar([Progress]() { return Progress; }, 0.f, 18.f, C.BorderStrong, C.Primary)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[ Slots ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ HardwareHint(Mask(EHardware::CashSlots, EHardware::Tray), 0.5f) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
					[
						TextWrap(KT("Change and any note the kiosk can't take come out in the collection tray."), KioskType::Caption, false, C.Ink, KioskType::LineBody)
					],
					C.Band, KioskLayout::InnerRadius, C.Transparent, 0.f, KioskLayout::CardPadTight)
			],
			C.Surface, C.Transparent, 0.f, KioskLayout::CardPadWide);
	}
}

void SWaitScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bProcessing = Session->Current() == EKioskScreen::Processing;
	const EPayMethod Method = Session->State.PayMethod;
	const bool bCash = Method == EPayMethod::Cash;
	const bool bChip = Method == EPayMethod::Chip;

	FText Heading, Body;
	TArray<FText> Tips;
	uint32 Lit = 0;
	if (bCash)
	{
		Heading = KT("Insert notes or coins");
		Body = KT("The note and coin slots are in the right column, at the bottom. Their lights are flashing green. The total updates as each note or coin goes in.");
		Tips = { KT("Notes: $2, $5, $10 or $50. Coins: 10¢, 20¢, 50¢ or $1."), KT("Overpaying is fine. Change comes out in the tray.") };
		Lit = Mask(EHardware::CashSlots, EHardware::Tray);
	}
	else if (bChip)
	{
		Heading = KT("Insert your card, chip first");
		Body = KT("The chip slot is in the right column, in the middle. Its light is flashing green. Push the card in, chip facing up, until it stops.");
		Tips = { KT("Leave the card in until the screen tells you to take it."), KT("You may be asked for your PIN on the pad beside the slot."), KT("Contactless card? Tapping is faster.") };
		Lit = Mask(EHardware::ChipSlot);
	}
	else
	{
		Heading = KT("Hold your card or phone on the reader");
		Body = KT("The contactless reader is in the right column, at the top. Its ring is glowing green now. Hold your card flat on it until you hear a beep.");
		Tips = { KT("Tap one card only. Take it out of your wallet so the wrong card isn't charged."), KT("Paying by phone? Unlock it first."), KT("NETS card? Use the chip and NETS slot, below the contactless reader.") };
		Lit = Mask(EHardware::Contactless);
	}

	TSharedRef<SVerticalBox> TipBox = SNew(SVerticalBox);
	for (const FText& Tip : Tips)
	{
		TipBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ TextWrap(Tip, KioskType::Caption, false, C.Muted, KioskType::LineBody) ];
	}

	TSharedRef<SWidget> Hardware = SNullWidget::NullWidget;
	if (!bCash && !bProcessing)
	{
		Hardware = SimControl(
			bChip ? KT("SIMULATE: insert chip card") : KT("SIMULATE: tap card on reader"),
			FOnClicked::CreateLambda([Session]() { Session->SimulateCardTap(); return FReply::Handled(); }),
			bChip ? TEXT("SIMULATE: insert chip card") : TEXT("SIMULATE: tap card on reader"));
	}

	TSharedRef<SWidget> Illustration = bChip ? CardIllustrationChip() : CardIllustrationTap();

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 48.f, 0.f)
		[
			SNew(SVerticalBox)
			// Latin chips hug their content. Chinese can break between any two glyphs, so a hugging chip
			// re-wraps against its own shrinking width and collapses; it fills the column instead.
			+ SVerticalBox::Slot().AutoHeight().HAlign(FKioskLoc::GetLanguage() == EKioskLanguage::Chinese ? HAlign_Fill : HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Paying"), KioskType::H3, false, C.Ink) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->TotalCents()), KioskType::H3, true, C.Ink) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)[ TextWrap(Session->PaymentTail(), KioskType::H3, false, C.Ink, KioskType::LineCompact) ],
					C.Surface, 34.f, C.BorderStrong, 2.f, FMargin(32.f, 14.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 30.f, 0.f, 0.f)[ TextWrap(Heading, KioskType::H1, true, C.Ink, KioskType::LineCompact) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, KioskLayout::TitleGap, 0.f, 0.f)[ TextWrap(Body, KioskType::Body, false, C.Ink, KioskType::LineBody) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f).HAlign(HAlign_Left)[ StatusPill(Session, bProcessing, bCash) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ TipBox ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[ Hardware ]
		]
		+ SHorizontalBox::Slot().FillWidth(0.55f)
		[
			bCash
				? CashPanel(Session, bProcessing)
				: Card(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
						[
							SNew(SBox).WidthOverride(600.f).HeightOverride(520.f)[ Illustration ]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ HardwareHint(Lit, 0.8f) ],
					C.Band, C.Transparent, 0.f, KioskLayout::CardPadWide)
		]
	];
}

// --- receipt -------------------------------------------------------------------------------

void SReceiptScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const FKioskState& State = Session->State;

	const FText Trip = State.Ticket == ETicketType::DayPass
		? KT("All lines")
		: FText::Format(KT("{0} → {1}"), FKioskSession::OriginLabel(), Session->DestinationLabel());

	const FKioskRoute& Route = FKioskRoutes::FromCityHall(State.Destination);
	const FText RouteNote = (State.Ticket == ETicketType::DayPass || !Route.bValid)
		? FText::GetEmpty()
		: FText::Format(KT("{0} · {1}"), FKioskRoutes::Detail(Route), FKioskRoutes::Summary(Route));

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.52f).Padding(0.f, 0.f, KioskLayout::Gutter, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Badge(EKioskIcon::Check, KioskLayout::IconLg, C.Primary, C.White, 6.f)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 0.f, 0.f, 0.f)[ TextWrap(KT("Payment approved"), KioskType::Display, true, C.Ink, KioskType::LineCompact) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ TextWrap(KT("Your ticket is printing. Take it from the collection tray."), KioskType::H3, true, C.Ink, KioskType::LineBody) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ HardwareHint(Mask(EHardware::Tray), 0.42f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
			[
				Card(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[ TextWrap(Session->ReceiptKind(), KioskType::Caption, true, C.Muted, KioskType::LineCompact) ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ Text(FKioskSession::DateShortText(), KioskType::Caption, false, C.Muted) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ TextWrap(Trip, KioskType::H1, true, C.Ink, KioskType::LineCompact) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ TextWrap(RouteNote, KioskType::Caption, false, C.Muted, KioskType::LineBody) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[ TextWrap(Session->ReceiptValidText(), KioskType::Body, true, C.Ink, KioskType::LineBody) ]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[ State.Ticket == ETicketType::OneWay ? StaticCastSharedRef<SWidget>(SNew(SSpacer)) : StaticCastSharedRef<SWidget>(TextWrap(FKioskSession::ValidityRule(State.Ticket), KioskType::Caption, false, C.Muted, KioskType::LineBody)) ]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(12.f, 0.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->TicketTotalCents()), KioskType::H3, true, C.Ink) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, Session->ChangeCents() > 0 ? 10.f : 0.f, 0.f, 0.f)
					[
						Session->ChangeCents() > 0
							? StaticCastSharedRef<SWidget>(TextWrap(FText::Format(KT("Change in the tray: {0}"), FKioskSession::Money(Session->ChangeCents())), KioskType::Body, true, C.Primary))
							: StaticCastSharedRef<SWidget>(SNew(SSpacer))
					],
					C.Surface, C.Ink, 3.f, KioskLayout::CardPadWide)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ReturnCountdown(Session)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.48f)
		[
			Card(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)[ TextWrap(KT("Want a receipt?"), KioskType::H2, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight()[ ReceiptActions(Session) ]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Primary)
					.Label(KT("No receipt · I'm done"))
					.ButtonTag(TEXT("No receipt · I'm done"))
					.OnClicked_Lambda([Session]() { Session->ReturnHome(true); return FReply::Handled(); })
				],
				C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide)
		]
	];
}
