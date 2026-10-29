// Card flows: scan card, card options, top-up amount, concession pass, pass dates and the "lift your card" screen.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskCard"

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
				Text(KT("Metro card"), Height * 0.12f, true, C.Ink)
			],
			C.Surface, 16.f, C.Ink, 3.f);
	}

	TSharedRef<SWidget> CardInfoLine(const FText& Label, const FText& Value)
	{
		const FKioskColors& C = KC();
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ Text(Label, 18.f, false, C.Muted) ]
			+ SHorizontalBox::Slot().AutoWidth()[ Text(Value, 18.f, false, C.Muted) ];
	}

	TSharedRef<SWidget> KeepCardNote()
	{
		const FKioskColors& C = KC();
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Contactless, 30.f, C.Primary, 3.f) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)[ Text(LOCTEXT("KeepCard", "Keep your card on the reader"), 17.f, false, C.Muted) ];
	}
}

// --- scan card -----------------------------------------------------------------------------

void SScanCardScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	TSharedRef<SWidget> Illustration = SNew(SOverlay)
		+ SOverlay::Slot()[ Positioned(MetroCardGraphic(380.f, 240.f), FVector2D(150.f, 40.f), FVector2D(380.f, 240.f), -8.f) ]
		+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::Contactless, 150.f, C.Primary, 5.f), FVector2D(225.f, 250.f), FVector2D(150.f, 150.f), -90.f) ]
		+ SOverlay::Slot()
		[
			Positioned(Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Panel(SNew(SSpacer).Size(FVector2D(190.f, 52.f)), C.Primary, 12.f) ], C.Ink, 24.f),
				FVector2D(130.f, 410.f), FVector2D(340.f, 100.f))
		]
		+ SOverlay::Slot()[ Positioned(Icon(EKioskIcon::ArrowRight, 110.f, C.Ink, 5.f), FVector2D(500.f, 410.f), FVector2D(100.f, 100.f)) ];

	ChildSlot
	.Padding(75.f, 40.f, 142.f, 56.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(0.f, 0.f, 50.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 70.f, 0.f, 0.f)[ TextWrap(KT("Place your transit card on the reader"), 44.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 22.f, 0.f, 0.f)
			[
				TextWrap(LOCTEXT("ScanBody", "The reader is to the right of the screen, at the top. It glows green when it's ready. Hold the card still until this screen changes."), 22.f, false, C.Ink, 1.25f)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 30.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(C.Primary, 16.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("WaitingCardDots", "Waiting for your card…"), 21.f, true, C.Ink) ],
					C.Surface, 26.f, C.BorderStrong, 2.f, FMargin(28.f, 14.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 30.f, 0.f, 0.f)
			[
				TextWrap(LOCTEXT("NotReading", "Not reading? Take the card out of your wallet or phone case, then try again."), 17.f, false, C.Muted, 1.25f)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SimControl(KT("SIMULATE: tap card on reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateCardTap(); return FReply::Handled(); }))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.55f)
		[
			Panel(
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ SNew(SBox).WidthOverride(640.f).HeightOverride(540.f)[ Illustration ] ]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 34.f, 28.f)
				[
					Text(KT("Card reader · top right of the screen"), 17.f, true, C.Ink)
				],
				C.Band, 36.f)
		]
	];
}

// --- card options --------------------------------------------------------------------------

void SCardOptionsScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();

	TSharedRef<SWidget> CardPanel = Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ CardInfoLine(LOCTEXT("StudentCard", "Student card"), FText::FromString(TEXT("•••• 4821"))) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ Text(LOCTEXT("Balance", "Balance"), 18.f, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents()), 66.f, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[ Dot(C.AmberStrong, 12.f) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("LowBalance", "Low balance · one trip left"), 16.f, true, C.Ink) ],
				C.AmberFill, 17.f, C.AmberStroke, 2.f, FMargin(20.f, 8.f))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 22.f)[ Rule(C.BorderStrong, 2.f) ]
		+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("ConcessionPass", "Concession pass"), 18.f, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ Text(LOCTEXT("PassEnds", "Bus + Train · ends 14 Nov"), 21.f, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight()[ KeepCardNote() ],
		C.Band, 26.f, C.BorderStrong, 2.f, FMargin(40.f, 34.f, 40.f, 34.f));

	TSharedRef<SHorizontalBox> Tiles = SNew(SHorizontalBox);
	const int32 Amounts[3] = { 1000, 2000, 5000 };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const int32 Amount = Amounts[Index];
		Tiles->AddSlot().FillWidth(1.f).Padding(Index == 0 ? 0.f : 10.f, 0.f, Index == 2 ? 0.f : 10.f, 0.f)
		[
			SNew(SBox).HeightOverride(206.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Tile)
				.Label(FText::FromString(FString::Printf(TEXT("Top up $%d"), Amount / 100)))
				.ButtonTag(*FString::Printf(TEXT("Quick top up %d"), Amount / 100))
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Session, Amount]() { Session->QuickTopUp(Amount); return FReply::Handled(); })
				[
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(40.f, 0.f, 0.f, 24.f)
					[
						Text(FText::FromString(FString::Printf(TEXT("+$%d"), Amount / 100)), 50.f, true, C.White)
					]
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(40.f, 0.f, 0.f, 28.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ Text(LOCTEXT("NewBalance", "New balance"), 17.f, false, C.White) ]
						+ SHorizontalBox::Slot().AutoWidth().Padding(16.f, 0.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents() + Amount), 17.f, true, C.White) ]
					]
				]
			]
		];
	}

	auto OptionCard = [&](EKioskIcon IconType, const FText& Title, const FText& Sub, TFunction<void()> Action)
	{
		return Panel(
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Card)
			.Label(Title)
			.Padding(FMargin(0.f))
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 20.f, 0.f, 0.f)[ Icon(IconType, 140.f, C.Ink, 4.f) ]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 0.f, 28.f, 18.f)[ Icon(EKioskIcon::ArrowRight, 62.f, C.Ink, 5.f) ]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(26.f, 0.f, 0.f, 22.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Text(Title, 30.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(Sub, 17.f, false, C.Muted) ]
				]
			],
			C.Surface, 26.f, C.Ink, 3.f);
	};

	ChildSlot
	.Padding(56.f, 28.f, 56.f, 40.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ Text(KT("Card found. What would you like to do?"), 42.f, true, C.Ink) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("CardOptionsOnly", "Card options only · "), 16.f, false, C.Muted) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(KT("Buy a ticket instead"))
				.Padding(FMargin(8.f, 8.f))
				.OnClicked_Lambda([Session]()
				{
					Session->State.Flow = EKioskFlow::Ticket;
					Session->GoTo(EKioskScreen::TicketType);
					return FReply::Handled();
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Buy a ticket instead"), 17.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[ Rule(C.Ink, 2.f) ]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 22.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(0.f, 0.f, 36.f, 0.f)[ CardPanel ]
			+ SHorizontalBox::Slot().FillWidth(0.7f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Quick top up"), 23.f, true, C.Ink) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(34.f, 4.f, 0.f, 0.f)[ Text(LOCTEXT("OneTap", "One tap, then pay"), 16.f, false, C.Muted) ]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ Tiles ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 14.f)[ Text(KT("Other card options"), 23.f, true, C.Ink) ]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 10.f, 0.f)
					[
						OptionCard(EKioskIcon::TopUpCard, KT("Top up another amount"), LOCTEXT("OtherAmounts", "$30, $100 or any amount"),
							[Session]() { Session->State.Flow = EKioskFlow::TopUp; Session->OpenTopUpKeypad(false); Session->GoTo(EKioskScreen::TopUpAmount); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(10.f, 0.f, 0.f, 0.f)
					[
						OptionCard(EKioskIcon::IdCard, KT("Renew concession pass"), LOCTEXT("YourPassEnds", "Your Bus + Train pass ends 14 Nov"),
							[Session]() { Session->State.Flow = EKioskFlow::Concession; Session->GoTo(EKioskScreen::Concession); })
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
			.FontPx(38.f)
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

	TSharedRef<SWidget> LeftPanel = Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("StudentCard2", "Student card"), 18.f, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f)[ Text(LOCTEXT("BalanceNow", "Balance now"), 18.f, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ Text(FKioskSession::Money(Session->CardBalanceCents()), 54.f, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 22.f)[ Rule(C.BorderStrong, 2.f) ]
		+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("AfterTopUp", "After top-up"), 18.f, false, C.Muted) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)
		[
			bHasSelection
				? Text(FKioskSession::Money(Session->NewBalanceCents()), 58.f, true, C.Primary)
				: Text(FText::FromString(TEXT("—")), 58.f, true, C.Muted)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
		[
			Text(bHasSelection ? FText::Format(LOCTEXT("Added", "+{0} added"), FKioskSession::Money(Selected)) : LOCTEXT("PickAmount", "Pick an amount"), 15.f, false, C.Muted)
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight()[ KeepCardNote() ],
		C.Band, 26.f, C.BorderStrong, 2.f, FMargin(40.f, 34.f));

	struct FAmount { int32 Cents; const TCHAR* Label; };
	const FAmount Amounts[6] = { { 1000, TEXT("$10") }, { 2000, TEXT("$20") }, { 3000, TEXT("$30") }, { 5000, TEXT("$50") }, { 10000, TEXT("$100") }, { 0, TEXT("More") } };

	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);
	for (int32 Row = 0; Row < 2; ++Row)
	{
		TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
		for (int32 Col = 0; Col < 3; ++Col)
		{
			const FAmount& Amount = Amounts[Row * 3 + Col];
			const bool bMore = Amount.Cents == 0;
			const bool bSelected = !bMore && Selected == Amount.Cents;
			const FText Label = bMore ? KT("More") : FText::FromString(Amount.Label);
			const int32 Cents = Amount.Cents;
			Cells->AddSlot().FillWidth(1.f).Padding(Col == 0 ? 0.f : 10.f, 0.f, Col == 2 ? 0.f : 10.f, 0.f)
			[
				Panel(
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Card)
					.Label(Label)
					.ButtonTag(*FString::Printf(TEXT("Amount %s"), Amount.Label))
					.Padding(FMargin(0.f))
					.OnClicked_Lambda([Session, Cents, bMore]()
					{
						if (bMore) { Session->OpenTopUpKeypad(true); } else { Session->SelectTopUp(Cents); }
						return FReply::Handled();
					})
					[
						SNew(SOverlay)
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(Label, 44.f, true, C.Ink) ]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 14.f, 14.f, 0.f)
						[
							bSelected
								? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(48.f).HeightOverride(48.f)
									[ Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 28.f, C.White, 4.f) ], C.Primary, 10.f) ])
								: StaticCastSharedRef<SWidget>(SNew(SSpacer))
						]
					],
					bSelected ? C.PrimaryTint : C.Surface, 22.f, bSelected ? C.Ink : C.BorderStrong, bSelected ? 4.f : 2.f)
			];
		}
		Grid->AddSlot().FillHeight(1.f).Padding(0.f, Row == 0 ? 0.f : 10.f, 0.f, Row == 0 ? 10.f : 0.f)[ Cells ];
	}


	// Custom amount: whole-dollar numeric keypad ($5 to $500).
	TSharedRef<SWidget> Keypad = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					Text(FText::Format(LOCTEXT("KeypadAmount", "${0}"), FText::FromString(Session->State.TopUpDigits.IsEmpty() ? TEXT("0") : *Session->State.TopUpDigits)), 52.f, true, C.Ink)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Text(FText::Format(LOCTEXT("KeypadRange", "Enter a whole-dollar amount from ${0} to ${1}"), FText::AsNumber(FKioskSession::MinTopUpDollars), FText::AsNumber(FKioskSession::MaxTopUpDollars)), 17.f, false, C.Muted)
				],
				C.Surface, 18.f, C.Ink, 3.f, FMargin(30.f, 14.f))
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
					SNew(SKioskButton).Kind(EKioskButtonKind::KeyMuted).Label(KT("Back to amounts")).FontPx(20.f).ButtonTag(TEXT("Back to amounts"))
					.OnClicked_Lambda([Session]() { Session->OpenTopUpKeypad(false); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)[ KeypadKey(Session, 0) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)
				[
					SNew(SKioskButton).Kind(EKioskButtonKind::KeyMuted).Label(LOCTEXT("BackspaceKey", "Backspace")).ButtonTag(TEXT("Keypad Backspace"))
					.OnClicked_Lambda([Session]() { Session->BackspaceTopUp(); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Backspace, 40.f, C.Ink, 3.f)
					]
				]
			]
		];

	TSharedRef<SWidget> AmountArea = Session->State.bTopUpKeypad ? Keypad : StaticCastSharedRef<SWidget>(Grid);

	ChildSlot
	.Padding(80.f, 24.f, 64.f, 38.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)[ Text(KT("How much would you like to add?"), 40.f, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.3f).Padding(0.f, 0.f, 34.f, 0.f)[ LeftPanel ]
			+ SHorizontalBox::Slot().FillWidth(0.7f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(0.78f)[ AmountArea ]
				+ SVerticalBox::Slot().FillHeight(0.22f).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Primary)
					.Label(KT("Continue"))
					.RightIcon(EKioskIcon::ArrowRight)
					.FontPx(32.f)
					.Width(300.f).Height(106.f)
					.Enabled(bHasSelection)
					.OnClicked_Lambda([Session]() { Session->ContinueTopUp(); return FReply::Handled(); })
				]
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

	TSharedRef<SHorizontalBox> Cards = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Passes.Num(); ++Index)
	{
		const FKioskConcession& Pass = Passes[Index];
		const bool bSelected = Selected == Index;

		TSharedRef<SHorizontalBox> Icons = SNew(SHorizontalBox);
		auto AddIcon = [&](EKioskIcon IconType, bool bFirst)
		{
			Icons->AddSlot().AutoWidth().Padding(bFirst ? 0.f : 10.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).WidthOverride(68.f).HeightOverride(68.f)
				[
					Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(IconType, 40.f, C.Ink, 3.f) ], C.Band, 12.f)
				]
			];
		};
		if (Pass.bBus) { AddIcon(EKioskIcon::Bus, true); }
		if (Pass.bTrain) { AddIcon(EKioskIcon::Train, !Pass.bBus); }

		const FText PassName = FText::FromString(Pass.Name);
		Cards->AddSlot().FillWidth(1.f).Padding(Index == 0 ? 0.f : 12.f, 0.f, Index == 2 ? 0.f : 12.f, 0.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(PassName)
				.ButtonTag(*FString::Printf(TEXT("Pass %s"), Pass.Name))
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Session, Index]() { Session->SelectConcession(Index); return FReply::Handled(); })
				[
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)[ Icons ]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
					[
						bSelected
							? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(46.f).HeightOverride(46.f)
								[ Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 28.f, C.White, 4.f) ], C.Primary, 12.f) ])
							: StaticCastSharedRef<SWidget>(SNew(SSpacer))
					]
					+ SOverlay::Slot().VAlign(VAlign_Fill).Padding(0.f, 98.f, 0.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(PassName, 34.f, true, C.Ink) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ Text(FText::FromString(Pass.Description), 18.f, false, C.Muted) ]
						+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("AdultPrice", "Adult {0}"), FKioskSession::Money(Pass.AdultCents)))
							.Font(Style.Font(16.f))
							.ColorAndOpacity(C.Muted)
							.StrikeBrush(Style.Solid(C.Muted))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ Text(FKioskSession::Money(Pass.StudentCents), 50.f, true, C.Ink) ]
						+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("For30", "for 30 days"), 15.f, false, C.Muted) ]
					]
				],
				bSelected ? C.PrimaryTint : C.Surface, 24.f, bSelected ? C.Ink : C.BorderStrong, bSelected ? 4.f : 2.f, FMargin(34.f, 32.f, 34.f, 28.f))
		];
	}

	ChildSlot
	.Padding(56.f, 24.f, 56.f, 30.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Choose a concession pass"), 40.f, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::IdCard, 44.f, C.Primary, 3.f) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 0.f, 0.f, 0.f)[ Text(LOCTEXT("StudentDetected", "Student card detected"), 25.f, true, C.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 3.f, 0.f, 0.f)[ Text(LOCTEXT("ConcessionShown", "· student concession prices shown"), 22.f, false, C.Ink) ],
				C.Surface, 16.f, C.BorderStrong, 2.f, FMargin(20.f, 14.f))
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 26.f, 0.f, 22.f)[ Cards ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Text(LOCTEXT("PassNote", "Every pass lasts 30 days with unlimited rides. Next, you'll see the exact dates it covers before you pay."), 17.f, false, C.Ink)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Primary)
				.Label(KT("Continue · check dates"))
				.RightIcon(EKioskIcon::ArrowRight)
				.FontPx(31.f)
				.Width(500.f).Height(106.f)
				.Enabled(Selected != INDEX_NONE)
				.OnClicked_Lambda([Session]() { Session->GoTo(EKioskScreen::PassDates); return FReply::Handled(); })
			]
		]
	];
}

// --- pass dates ----------------------------------------------------------------------------

void SPassDatesScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskConcession>& Passes = FKioskData::Concessions();
	const FKioskConcession& Pass = Passes.IsValidIndex(Session->State.Concession) ? Passes[Session->State.Concession] : Passes.Last();

	// November 2026 starts on a Sunday; Monday-first grid => six leading blanks.
	const int32 LeadingBlanks = 6;
	const int32 DaysInMonth = 30;
	const int32 CurrentPassEnd = 14;

	static const TCHAR* const Weekdays[7] = { TEXT("Mon"), TEXT("Tue"), TEXT("Wed"), TEXT("Thu"), TEXT("Fri"), TEXT("Sat"), TEXT("Sun") };
	TSharedRef<SVerticalBox> Calendar = SNew(SVerticalBox);
	{
		TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);
		for (int32 Day = 0; Day < 7; ++Day)
		{
			Header->AddSlot().FillWidth(1.f).HAlign(HAlign_Center)[ Text(FText::FromString(Weekdays[Day]), 17.f, true, C.Muted) ];
		}
		Calendar->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)[ Header ];
	}
	for (int32 Week = 0; Week < 6; ++Week)
	{
		TSharedRef<SHorizontalBox> Cells = SNew(SHorizontalBox);
		for (int32 Col = 0; Col < 7; ++Col)
		{
			const int32 DayNumber = Week * 7 + Col - LeadingBlanks + 1;
			TSharedRef<SWidget> Cell = SNew(SSpacer);
			if (DayNumber >= 1 && DayNumber <= DaysInMonth)
			{
				const bool bCurrent = DayNumber <= CurrentPassEnd;
				const FLinearColor Fill = bCurrent ? C.HelpTint : C.PrimaryTint;
				const FLinearColor Border = bCurrent ? C.Help : C.Primary;
				Cell = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(DayNumber), 24.f, DayNumber == 14 || DayNumber == 15, C.Ink) ],
					Fill, 12.f, DayNumber == 14 || DayNumber == 15 ? Border : C.Transparent, 3.f);
			}
			Cells->AddSlot().FillWidth(1.f).Padding(4.f, 0.f)[ SNew(SBox).HeightOverride(66.f)[ Cell ] ];
		}
		Calendar->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)[ Cells ];
	}

	auto Legend = [&](const FLinearColor& Swatch, const FLinearColor& Border, const FText& Label)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)[ Panel(SNew(SSpacer), Swatch, 8.f, Border, 3.f) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 36.f, 0.f)[ Text(Label, 19.f, false, C.Ink) ];
	};

	ChildSlot
	.Padding(75.f, 24.f, 75.f, 30.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 18.f)[ Text(KT("Check your pass dates"), 40.f, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.58f).Padding(0.f, 0.f, 40.f, 0.f)
			[
				Panel(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)[ Text(LOCTEXT("MonthName", "November 2026"), 30.f, true, C.Ink) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("ContinuesTo", "New pass continues to 14 Dec"), 17.f, false, C.Muted) ]
					]
					+ SVerticalBox::Slot().AutoHeight()[ Calendar ]
					+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ Legend(C.HelpTint, C.Help, LOCTEXT("LegendCurrent", "Current pass · ends 14 Nov")) ]
						+ SHorizontalBox::Slot().AutoWidth()[ Legend(C.PrimaryTint, C.Primary, LOCTEXT("LegendNew", "New pass validity")) ]
					],
					C.Surface, 26.f, C.BorderStrong, 2.f, FMargin(36.f, 28.f))
			]
			+ SHorizontalBox::Slot().FillWidth(0.42f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(FText::Format(LOCTEXT("PassSummary", "{0} – Student"), FText::FromString(Pass.Name)), 30.f, true, C.Ink) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ Text(LOCTEXT("PassTerms", "30 days · unlimited rides"), 20.f, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 22.f)[ Rule(C.BorderStrong, 2.f) ]
						+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("NewPassLabel", "New pass"), 17.f, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(LOCTEXT("NewPassDates", "15 Nov – 14 Dec 2026"), 32.f, true, C.Primary) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ Text(LOCTEXT("CurrentEnds", "Current pass ends 14 Nov 2026"), 19.f, false, C.Ink) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
						[
							TextWrap(LOCTEXT("NoLostDays", "Your new pass starts the day after it ends, so you don't lose any days."), 19.f, false, C.Muted, 1.3f)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)[ Text(FKioskSession::Money(Pass.StudentCents), 56.f, true, C.Ink) ],
						C.Surface, 26.f, C.BorderStrong, 2.f, FMargin(36.f, 32.f))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(106.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::Primary)
						.Label(KT("Go to payment"))
						.RightIcon(EKioskIcon::ArrowRight)
						.FontPx(32.f)
						.OnClicked_Lambda([Session]() { Session->GoToPayment(); return FReply::Handled(); })
					]
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
		? LOCTEXT("TopUpDone", "Your top-up is done. Lift your Metro card off the reader below the screen before you go.")
		: LOCTEXT("PurchaseDone", "Your purchase is done. Lift your Metro card off the reader below the screen before you go.");

	TSharedRef<SWidget> BalanceDetail = SNullWidget::NullWidget;
	if (bTopUp)
	{
		BalanceDetail = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Text(FText::Format(LOCTEXT("AddedWas", "+{0} added · was {1}"), FKioskSession::Money(Session->State.TopUpCents), FKioskSession::Money(Session->CardBalanceCents())), 19.f, true, C.Ink)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(LOCTEXT("UseNow", "Ready to use now"), 19.f, false, C.Muted) ];
	}
	else
	{
		BalanceDetail = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("NewValidity", "New pass validity · 15 Nov – 14 Dec"), 19.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(LOCTEXT("CurrentEndsShort", "Current pass · ends 14 Nov"), 19.f, false, C.Muted) ];
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
	.Padding(75.f, 40.f, 80.f, 40.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 50.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				SNew(SBox).WidthOverride(104.f).HeightOverride(104.f)
				[
					Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::FromString(TEXT("!")), 62.f, true, C.White) ], C.AmberStrong, 24.f)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 36.f, 0.f, 0.f)[ Text(KT("Don't forget your card"), 52.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)[ TextWrap(Body, 23.f, false, C.Ink, 1.5f) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Dot(C.AmberStrong, 16.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(LOCTEXT("StillOnReader", "Card still on the reader"), 20.f, true, C.Ink) ],
					C.AmberFill, 26.f, C.AmberStroke, 2.f, FMargin(28.f, 14.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("CurrentBalance", "Current Balance"), 16.f, false, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ Text(FKioskSession::Money(Balance), 38.f, true, C.Ink) ]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(70.f, 0.f, 0.f, 0.f)[ BalanceDetail ],
					C.Surface, 20.f, C.BorderStrong, 2.f, FMargin(32.f, 22.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
			[
				Session->ChangeCents() > 0
					? Text(FText::Format(LOCTEXT("DoneChange", "Take your change from the tray: {0}"), FKioskSession::Money(Session->ChangeCents())), 21.f, true, C.Primary)
					: StaticCastSharedRef<STextBlock>(Text(FText::GetEmpty(), 4.f))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SimControl(KT("SIMULATE: lift card off reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateCardLift(); return FReply::Handled(); }))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.5f)
		[
			Panel(
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ SNew(SBox).WidthOverride(640.f).HeightOverride(620.f)[ Illustration ] ]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 30.f, 24.f)
				[
					Text(KT("Metro card reader · below the screen"), 16.f, true, C.Ink)
				],
				C.Band, 30.f)
		]
	];
}

#undef LOCTEXT_NAMESPACE
