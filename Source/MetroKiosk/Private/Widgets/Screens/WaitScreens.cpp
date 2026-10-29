// Waiting-for-payment screens (tap / chip / cash / processing) and the receipt.
#include "KioskScreens.h"

#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskDashedBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SCanvas.h"

#define LOCTEXT_NAMESPACE "KioskWait"

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
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 12.f, 0.f, 0.f)[ Text(KT("Bank card"), 20.f, true, C.Ink) ]
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
			+ SOverlay::Slot()[ Positioned(Reader, FVector2D(110.f, 400.f), FVector2D(340.f, 100.f)) ]
			+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::ArrowRight, 110.f, C.Ink, 5.f), FVector2D(480.f, 400.f), FVector2D(110.f, 100.f)) ];
	}

	TSharedRef<SWidget> CardIllustrationChip()
	{
		const FKioskColors& C = KC();
		TSharedRef<SWidget> Card = Panel(
			SNew(SOverlay)
			+ SOverlay::Slot().VAlign(VAlign_Top)[ SNew(SBox).HeightOverride(46.f)[ SNew(SBorder).BorderImage(FKioskStyle::Get().Solid(C.Help)).BorderBackgroundColor(FLinearColor::White) ] ]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 78.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Chip first"), 22.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(KT("facing up"), 16.f, false, C.Ink) ]
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
			+ SOverlay::Slot()[ Positioned(Slot, FVector2D(80.f, 350.f), FVector2D(366.f, 130.f)) ]
			+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::ArrowRight, 110.f, C.Ink, 5.f), FVector2D(480.f, 390.f), FVector2D(110.f, 100.f)) ];
	}

	TSharedRef<SWidget> StatusPill(const TSharedRef<FKioskSession>& Session, bool bProcessing, bool bCash)
	{
		const FKioskColors& C = KC();
		TSharedRef<SWidget> Label = SNullWidget::NullWidget;
		if (bProcessing)
		{
			Label = Text(KT("Processing..."), 21.f, true, C.Ink);
		}
		else if (bCash)
		{
			Label = Text(LOCTEXT("WaitingCash", "Waiting for cash"), 21.f, true, C.Ink);
		}
		else
		{
			Label = TextDyn([Session]()
			{
				const int32 Seconds = Session->PaySecondsLeft();
				return FText::Format(LOCTEXT("WaitingCard", "Waiting for your card · {0}:{1} left"),
					FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));
			}, 21.f, true, C.Ink);
		}
		return Panel(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(bProcessing ? C.Help : C.Primary, 16.f) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Label ],
			C.Surface, 26.f, C.BorderStrong, 2.f, FMargin(28.f, 14.f));
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
				+ SVerticalBox::Slot().AutoHeight()[ Text(Label, 16.f, false, LabelColor) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(Value, 40.f, true, Ink) ],
				Fill, 18.f, C.Transparent, 0.f, FMargin(24.f, 22.f));
		};

		const bool bShowChange = bProcessing && Change > 0;
		auto Chip = [&](const FText& Label, int32 Cents, float Width, bool bCoin)
		{
			return SNew(SKioskButton)
				.Kind(bCoin ? EKioskButtonKind::PillOutline : EKioskButtonKind::Key)
				.Label(Label)
				.ButtonTag(*FString::Printf(TEXT("Insert %s"), *Label.ToString()))
				.FontPx(21.f)
				.Width(Width).Height(64.f)
				.Padding(FMargin(0.f))
				.Enabled(!bProcessing)
				.OnClicked_Lambda([Session, Cents]() { Session->InsertCash(Cents); return FReply::Handled(); });
		};

		const float Progress = Total > 0 ? static_cast<float>(FMath::Min(Inserted, Total)) / Total : 0.f;

		TSharedRef<SWidget> Slots = SNew(SKioskDashedBox)
			.DashColor(C.SimInk)
			.Fill(C.SimFill)
			.Padding(FMargin(22.f, 14.f, 22.f, 18.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					Text(KT("SIMULATE: tap a note or coin to insert it"), 16.f, true, C.SimInk)
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
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Cash, 34.f, C.Ink, 3.f) ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)[ Text(LOCTEXT("Notes", "Notes"), 20.f, true, C.Ink) ]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[ Chip(FText::FromString(TEXT("$2")), 200, 92.f, false) ]
							+ SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f, 0.f, 0.f)[ Chip(FText::FromString(TEXT("$5")), 500, 92.f, false) ]
							+ SHorizontalBox::Slot().AutoWidth().Padding(14.f, 0.f, 0.f, 0.f)[ Chip(FText::FromString(TEXT("$10")), 1000, 100.f, false) ]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							TextWrap(LOCTEXT("NoteSlot", "Note slot · lower right. Insert flat, any way up."), 16.f, false, C.Muted, 1.3f)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(20.f, 0.f, 0.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(SBox).Padding(FMargin(0.f, 0.f, 12.f, 0.f))[ Icon(EKioskIcon::Coin, 34.f, C.Ink, 3.f) ] ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("Coins", "Coins"), 20.f, true, C.Ink) ]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[ Chip(FText::FromString(TEXT("10¢")), 10, 94.f, true) ]
							+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ Chip(FText::FromString(TEXT("20¢")), 20, 94.f, true) ]
							+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ Chip(FText::FromString(TEXT("50¢")), 50, 94.f, true) ]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 12.f, 0.f, 0.f)
						[
							Chip(FText::FromString(TEXT("$1")), 100, 94.f, true)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							TextWrap(LOCTEXT("CoinSlot", "Coin slot · just below the notes. Drop one at a time or a handful."), 16.f, false, C.Muted, 1.3f)
						]
					]
				]
			];

		return Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)
				[
					Tile(LOCTEXT("ToPay", "To pay"), FKioskSession::Money(Total), C.Band, C.Ink, C.Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)
				[
					Tile(LOCTEXT("PutIn", "Put in"), FKioskSession::Money(Inserted), C.Band, C.Ink, C.Muted)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
				[
					bShowChange
						? Tile(LOCTEXT("ChangeDue", "Change in the tray"), FKioskSession::Money(Change), C.Primary, C.White, C.White)
						: Tile(LOCTEXT("StillToPay", "Still to pay"), FKioskSession::Money(Remaining), C.Ink, C.White, C.White)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
			[
				ProgressBar([Progress]() { return Progress; }, 0.f, 20.f, C.BorderStrong, C.Primary)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ Slots ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Tray, 34.f, C.Ink, 3.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
					[
						TextWrap(LOCTEXT("TrayNote", "Change and any note the kiosk can't take come out in the collection tray, below the slots."), 17.f, false, C.Ink, 1.35f)
					],
					C.Band, 14.f, C.Transparent, 0.f, FMargin(22.f, 18.f))
			],
			C.Surface, 28.f, C.Transparent, 0.f, FMargin(36.f));
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
	if (bCash)
	{
		Heading = KT("Insert notes or coins");
		Body = LOCTEXT("CashBody", "The cash slots are to the right of the screen, lower down. Their lights are flashing green. The total updates as each note or coin goes in.");
		Tips = { LOCTEXT("CashTip1", "Notes up to $10. Coins 10¢ to $1."), LOCTEXT("CashTip2", "Overpaying is fine. Change comes out in the tray.") };
	}
	else if (bChip)
	{
		Heading = KT("Insert your card, chip first");
		Body = LOCTEXT("ChipBody", "The chip slot is to the right of the screen, in the middle. Its light is flashing green. Push the card in, chip facing up, until it stops.");
		Tips = { LOCTEXT("ChipTip1", "Leave the card in until the screen tells you to take it."), LOCTEXT("ChipTip2", "You may be asked for your PIN."), LOCTEXT("ChipTip3", "Contactless card? Tapping is faster.") };
	}
	else
	{
		Heading = KT("Hold your card or phone on the reader");
		Body = LOCTEXT("TapBody", "The reader is to the right of the screen, at the top. Its ring is glowing green now. Hold your card flat on it until you hear a beep.");
		Tips = { LOCTEXT("TapTip1", "Tap one card only. Take it out of your wallet so the wrong card isn't charged."), LOCTEXT("TapTip2", "Paying by phone? Unlock it first."), LOCTEXT("TapTip3", "NETS card? Use the NETS reader just below this one.") };
	}

	TSharedRef<SVerticalBox> TipBox = SNew(SVerticalBox);
	for (const FText& Tip : Tips)
	{
		TipBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ TextWrap(Tip, 17.f, false, C.Muted, 1.25f) ];
	}

	TSharedRef<SWidget> Hardware = SNullWidget::NullWidget;
	if (!bCash && !bProcessing)
	{
		Hardware = SimControl(
			FText::FromString(bChip ? TEXT("SIMULATE: insert chip card") : TEXT("SIMULATE: tap card on reader")),
			FOnClicked::CreateLambda([Session]() { Session->SimulateCardTap(); return FReply::Handled(); }));
	}

	TSharedRef<SWidget> Illustration = bCash ? CashPanel(Session, bProcessing)
		: (bChip ? CardIllustrationChip() : CardIllustrationTap());

	TSharedRef<SWidget> Caption = SNullWidget::NullWidget;
	if (!bCash)
	{
		Caption = Text(FText::FromString(bChip ? TEXT("Chip slot · middle right") : TEXT("")), 17.f, true, C.Ink);
	}

	ChildSlot
	.Padding(75.f, 40.f, 142.f, 56.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 50.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ Text(LOCTEXT("Paying", "Paying "), 29.f, false, C.Ink) ]
					+ SHorizontalBox::Slot().AutoWidth()[ Text(FKioskSession::Money(Session->TotalCents()), 29.f, true, C.Ink) ]
					+ SHorizontalBox::Slot().AutoWidth()[ Text(FText::Format(LOCTEXT("PayTail", " {0}"), Session->PaymentTail()), 29.f, false, C.Ink) ],
					C.Surface, 34.f, C.BorderStrong, 2.f, FMargin(36.f, 16.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 40.f, 0.f, 0.f)[ TextWrap(Heading, 42.f, true, C.Ink, 1.0f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 18.f, 0.f, 0.f)[ TextWrap(Body, 20.f, false, C.Ink, 1.2f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f).HAlign(HAlign_Left)[ StatusPill(Session, bProcessing, bCash) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 32.f, 0.f, 0.f)[ TipBox ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[ Hardware ]
		]
		+ SHorizontalBox::Slot().FillWidth(0.55f)
		[
			bCash
				? Illustration
				: Panel(
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(600.f).HeightOverride(520.f)[ Illustration ]
					]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 34.f, 28.f)[ Caption ],
					C.Band, 36.f)
		]
	];
}

// --- receipt -------------------------------------------------------------------------------

void SReceiptScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TSharedRef<bool> bPrinted = MakeShared<bool>(false);
	const TSharedRef<bool> bEmailed = MakeShared<bool>(false);
	const FKioskState& State = Session->State;

	const FText Kind = FText::FromString(FString::Printf(TEXT("%s · %s"), *Session->TicketLabel().ToString().ToUpper(),
		State.Children + State.Seniors > 0 ? TEXT("MIXED") : TEXT("ADULT")));
	const FText Trip = State.Ticket == ETicketType::DayPass
		? KT("All lines")
		: FText::Format(LOCTEXT("TripArrow", "City Hall → {0}"), Session->DestinationLabel());

	TSharedRef<SWidget> ReceiptButtons = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)
		[
			SNew(SBox).HeightOverride(100.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Outline)
				.Label(KT("Print receipt"))
				.LeftIcon(EKioskIcon::Printer)
				.FontPx(21.f)
				.OnClicked_Lambda([Session, bPrinted]() { *bPrinted = true; Session->NotifyChanged(); return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(100.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Outline)
				.Label(KT("Email receipt"))
				.LeftIcon(EKioskIcon::Mail)
				.FontPx(21.f)
				.OnClicked_Lambda([Session, bEmailed]() { *bEmailed = true; Session->NotifyChanged(); return FReply::Handled(); })
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
		[
			TextDyn([bPrinted, bEmailed]()
			{
				return *bPrinted ? LOCTEXT("ReceiptPrinting", "Your receipt is printing in the tray.")
					: (*bEmailed ? LOCTEXT("ReceiptEmail", "Enter your email on the next screen.") : FText::GetEmpty());
			}, 18.f, false, C.Muted)
		];

	ChildSlot
	.Padding(75.f, 22.f, 75.f, 26.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 40.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(104.f).HeightOverride(104.f)
					[
						Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 58.f, C.White, 6.f) ], C.Primary, 52.f)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26.f, 0.f, 0.f, 0.f)[ Text(KT("Payment approved"), 50.f, true, C.Ink) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 34.f, 0.f, 0.f)[ Text(LOCTEXT("Printing", "Your ticket is printing."), 23.f, false, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 10.f, 0.f, 0.f)[ Text(LOCTEXT("TakeIt", "Take it from the collection tray, bottom right."), 23.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 30.f, 0.f, 0.f)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[ Text(Kind, 15.f, false, C.Muted) ]
						+ SHorizontalBox::Slot().AutoWidth()[ Text(FKioskSession::DateShortText(), 15.f, false, C.Muted) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ Text(Trip, 40.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							Text(FText::Format(LOCTEXT("ValidUntil", "Valid until {0}"), FKioskSession::ValidUntilText()), 18.f, false, C.Ink)
						]
						+ SHorizontalBox::Slot().AutoWidth()[ Text(FKioskSession::Money(Session->TicketTotalCents()), 19.f, true, C.Ink) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
					[
						Session->ChangeCents() > 0
							? Text(FText::Format(LOCTEXT("ChangeLine", "Change in the tray: {0}"), FKioskSession::Money(Session->ChangeCents())), 18.f, true, C.Primary)
							: StaticCastSharedRef<STextBlock>(Text(FText::GetEmpty(), 4.f))
					],
					C.Surface, 26.f, C.Ink, 3.f, FMargin(36.f, 32.f))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					ProgressBar([Session]() { return Session->ScreenTime() / FKioskSession::ReceiptReturnSeconds; }, 380.f, 14.f, C.Disabled, C.Ink)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26.f, 0.f, 0.f, 0.f)
				[
					TextDyn([Session]() { return FText::Format(LOCTEXT("Returning", "Returning to start in {0} s"), FText::AsNumber(Session->ReceiptSecondsLeft())); }, 17.f, false, C.Muted)
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.5f)
		[
			Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 24.f)[ Text(KT("Want a receipt?"), 24.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight()[ ReceiptButtons ]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(112.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::Primary)
						.Label(KT("No receipt · I'm done"))
						.FontPx(26.f)
						.OnClicked_Lambda([Session]() { Session->ReturnHome(true); return FReply::Handled(); })
					]
				],
				C.Surface, 28.f, C.BorderStrong, 2.f, FMargin(40.f, 34.f))
		]
	];
}

#undef LOCTEXT_NAMESPACE
