// Home and ticket-type screens.
#include "KioskScreens.h"

#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskChrome.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskHome"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> SectionLabel(const FText& Label)
	{
		return Text(Label, 21.f, true, KC().Muted);
	}

	/** White outlined home card with icon, arrow, title and subtitle that acts as one big button. */
	TSharedRef<SWidget> HomeCardButton(const TSharedRef<FKioskSession>& Session, EKioskIcon IconType, const FText& Title, const FText& Subtitle, EKioskFlow Flow)
	{
		const FKioskColors& C = KC();
		return Panel(
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Card)
			.Label(Title)
			.Padding(FMargin(0.f))
			.OnClicked_Lambda([Session, Flow]() { Session->BeginCardFlow(Flow); return FReply::Handled(); })
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30.f, 30.f, 0.f, 0.f)
				[
					Icon(IconType, 168.f, C.Ink, 4.f)
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.f, 0.f, 24.f, 20.f)
				[
					Icon(EKioskIcon::ArrowRight, 70.f, C.Ink, 5.f)
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(30.f, 0.f, 20.f, 26.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Text(Title, 40.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[ TextWrap(Subtitle, 23.f, false, C.Ink) ]
				]
			],
			C.Surface, 28.f, C.Ink, 3.f);
	}
}

void SHomeScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();

	// Column 1: one-time ticket
	TSharedRef<SWidget> TicketCard = Panel(
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(34.f, 54.f, 0.f, 0.f)
		[
			Icon(EKioskIcon::Ticket, 190.f, C.White, 5.f)
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(40.f, 0.f, 36.f, 40.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("One-time ticket"), 48.f, true, C.White) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				TextWrap(KT("Printed ticket for one trip. Choose one-way, round trip or a day pass."), 24.f, false, C.White, 1.0f)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Pill(FText::FromString(TEXT("One-way · $1.68")), C.PrimaryChip, C.Ink, 22.f, true, C.Transparent, FMargin(22.f, 14.f, 54.f, 14.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				Pill(FText::FromString(TEXT("Day pass · $8.00")), C.PrimaryChip, C.Ink, 22.f, true, C.Transparent, FMargin(22.f, 14.f, 38.f, 14.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f).HAlign(HAlign_Left)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Strong)
				.Label(KT("Buy a ticket"))
				.RightIcon(EKioskIcon::ArrowRight)
				.FontPx(30.f)
				.Padding(FMargin(40.f, 0.f)).Height(106.f)
				.OnClicked_Lambda([Session]() { Session->State.Flow = EKioskFlow::Ticket; Session->GoTo(EKioskScreen::TicketType); return FReply::Handled(); })
			]
		],
		C.Primary, 28.f);

	// Column 3: language + assistance
	TSharedRef<SWidget> LanguageCard = Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Globe, 96.f, C.Ink, 3.f) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Language"), 40.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					Text(FText::FromString(TEXT("语言 · Bahasa · மொழி")), 22.f, false, C.Muted)
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 14.f, 0.f, 0.f).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 7.f, 7.f)
				[
					SNew(SKioskButton).Kind(Session->Language() == EKioskLanguage::English ? EKioskButtonKind::Strong : EKioskButtonKind::OutlineMuted)
					.Label(FKioskLoc::LanguageName(EKioskLanguage::English)).FontPx(22.f).Height(76.f)
					.Bold(Session->Language() == EKioskLanguage::English)
					.OnClicked_Lambda([Session]() { Session->SetLanguage(EKioskLanguage::English); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 0.f, 0.f, 7.f)
				[
					SNew(SKioskButton).Kind(Session->Language() == EKioskLanguage::Chinese ? EKioskButtonKind::Strong : EKioskButtonKind::OutlineMuted)
					.Label(FKioskLoc::LanguageName(EKioskLanguage::Chinese)).FontPx(22.f).Height(76.f)
					.Bold(Session->Language() == EKioskLanguage::Chinese)
					.OnClicked_Lambda([Session]() { Session->SetLanguage(EKioskLanguage::Chinese); return FReply::Handled(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 7.f, 7.f, 0.f)
				[
					SNew(SKioskButton).Kind(Session->Language() == EKioskLanguage::Malay ? EKioskButtonKind::Strong : EKioskButtonKind::OutlineMuted)
					.Label(FKioskLoc::LanguageName(EKioskLanguage::Malay)).FontPx(22.f).Height(76.f)
					.Bold(Session->Language() == EKioskLanguage::Malay)
					.OnClicked_Lambda([Session]() { Session->SetLanguage(EKioskLanguage::Malay); return FReply::Handled(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 7.f, 0.f, 0.f)
				[
					SNew(SKioskButton).Kind(Session->Language() == EKioskLanguage::Tamil ? EKioskButtonKind::Strong : EKioskButtonKind::OutlineMuted)
					.Label(FKioskLoc::LanguageName(EKioskLanguage::Tamil)).FontPx(22.f).Height(76.f)
					.Bold(Session->Language() == EKioskLanguage::Tamil)
					.OnClicked_Lambda([Session]() { Session->SetLanguage(EKioskLanguage::Tamil); return FReply::Handled(); })
				]
			]
		],
		C.Surface, 28.f, C.BorderStrong, 2.f, FMargin(30.f, 28.f));

	TSharedRef<SWidget> CallCard = Panel(
		SNew(SKioskButton)
		.Kind(EKioskButtonKind::Card)
		.Label(KT("Call for assistance"))
		.Padding(FMargin(0.f))
		.OnClicked_Lambda([Session]() { Session->StartCall(); return FReply::Handled(); })
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30.f, 36.f, 0.f, 0.f)
			[
				Icon(EKioskIcon::Headset, 150.f, C.White, 5.f)
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 22.f, 26.f, 0.f)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)[ Dot(C.Primary, 14.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Staff online"), 17.f, true, C.Ink) ],
					C.White, 20.f, C.Transparent, 0.f, FMargin(20.f, 10.f))
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(30.f, 0.f, 20.f, 30.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Call for assistance"), 34.f, true, C.White) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ Text(KT("Video call with station staff"), 22.f, false, C.White) ]
			]
		],
		C.Help, 28.f);

	auto Column = [&](const FText& Label, const TSharedRef<SWidget>& Body)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 12.f)[ SectionLabel(Label) ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ Body ];
	};

	TSharedRef<SWidget> Banner = Panel(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(60.f, 0.f, 0.f, 0.f)
		[
			SNew(SBox).WidthOverride(94.f).HeightOverride(94.f)
			[
				Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Contactless, 48.f, C.Primary, 4.f) ], C.PrimaryTint, 47.f)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(30.f, 0.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SNew(SBox).MaxDesiredWidth(1000.f)[ TextWrap(KT("Or place your card on the reader to begin"), 46.f, true, C.Ink) ] ]
			+ SVerticalBox::Slot().AutoHeight()[ Text(KT("We'll show your balance and what you can do with it"), 27.f, false, C.Muted) ]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 50.f, 0.f)
		[
			SimControl(FText::FromString(TEXT("SIMULATE: tap card on reader")), FOnClicked::CreateLambda([Session]() { Session->SimulateCardTap(); return FReply::Handled(); }))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 130.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Text(KT("Reader here"), 31.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[ Icon(EKioskIcon::DoubleChevronDown, 34.f, C.Primary, 4.f) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f, 0.f, 0.f)[ Icon(EKioskIcon::DoubleChevronDown, 34.f, C.Primary, 4.f) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f, 0.f, 0.f)[ Icon(EKioskIcon::DoubleChevronDown, 34.f, C.Primary, 4.f) ]
			]
		],
		C.Surface, 0.f);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(110.f, 24.f, 110.f, 20.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 17.f, 0.f)
			[
				Column(KT("NO CARD"), TicketCard)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(17.f, 0.f, 17.f, 0.f)
			[
				Column(KT("HAVE A CARD"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)
					[
						HomeCardButton(Session, EKioskIcon::TopUpCard, KT("Top up"), KT("Add value and check your balance"), EKioskFlow::TopUp)
					]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)
					[
						HomeCardButton(Session, EKioskIcon::IdCard, KT("Concession"), KT("Student, senior or disability card: buy or renew a pass"), EKioskFlow::Concession)
					])
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(17.f, 0.f, 0.f, 0.f)
			[
				Column(KT("HELP & SETTINGS"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)[ LanguageCard ]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)[ CallCard ])
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Rule(C.BorderStrong, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).HeightOverride(166.f)[ Banner ]
			]
		]
	];
}

// --- ticket type ---------------------------------------------------------------------------

namespace
{
	TSharedRef<SWidget> TicketCardWidget(const TSharedRef<FKioskSession>& Session, ETicketType Type, EKioskIcon IconType, const FText& Title,
		const FText& Body, const FText& StrikePrice, const FText& Price, bool bBestValue)
	{
		const FKioskColors& C = KC();
		FKioskStyle& Style = FKioskStyle::Get();

		TSharedRef<SWidget> PriceBlock = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				StrikePrice.IsEmpty()
					? StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(1.f, 22.f)))
					: StaticCastSharedRef<SWidget>(
						SNew(STextBlock).Text(StrikePrice).Font(Style.Font(18.f)).ColorAndOpacity(C.Muted).StrikeBrush(Style.Solid(C.Muted)))
			]
			+ SVerticalBox::Slot().AutoHeight()[ Text(Price, 40.f, true, C.Ink) ];

		return Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(IconType, 62.f, C.Ink, 3.5f) ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[
					bBestValue
						? Pill(FText::FromString(TEXT("Best for 3+ rides")), C.PrimaryTint, C.Ink, 18.f, true, C.Transparent, FMargin(20.f, 8.f))
						: StaticCastSharedRef<SWidget>(SNew(SSpacer))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f)[ Text(Title, 35.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)[ TextWrap(Body, 22.f, false, C.Ink, 1.35f) ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)[ PriceBlock ]
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Strong)
					.Label(KT("Choose"))
					.ButtonTag(*FString::Printf(TEXT("Choose %s"), *Title.ToString()))
					.FontPx(24.f)
					.Width(176.f).Height(80.f)
					.OnClicked_Lambda([Session, Type]() { Session->ChooseTicket(Type); return FReply::Handled(); })
				]
			],
			C.Surface, 28.f, C.BorderStrong, 2.f, FMargin(40.f, 36.f, 40.f, 32.f));
	}
}

void STicketTypeScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	ChildSlot
	.Padding(75.f, 30.f, 75.f, 0.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Choose a ticket"), 42.f, true, C.Ink) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 50.f, 0.f, 0.f)
		[
			SNew(SBox).HeightOverride(515.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 18.f, 0.f)
				[
					TicketCardWidget(Session, ETicketType::OneWay, EKioskIcon::ArrowRight, KT("One-way"),
						FText::FromString(TEXT("One trip from here to one station.\nValid for 2 hours.")), FText::GetEmpty(), FText::FromString(TEXT("$1.68")), false)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(18.f, 0.f, 18.f, 0.f)
				[
					TicketCardWidget(Session, ETicketType::RoundTrip, EKioskIcon::Swap, KT("Round trip"),
						FText::FromString(TEXT("There and back between two stations.\nReturn valid until 12:00 a.m. today.\n30% discount.")),
						FText::FromString(TEXT("$2.56 - $5.14")), FText::FromString(TEXT("$1.79 - $3.60")), false)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(18.f, 0.f, 0.f, 0.f)
				[
					TicketCardWidget(Session, ETicketType::DayPass, EKioskIcon::Clock, KT("Day pass"),
						FText::FromString(TEXT("Unlimited rides on every line until 12:00 a.m. tonight. No destination needed.")),
						FText::GetEmpty(), FText::FromString(TEXT("$8.00")), true)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
		[
			Text(FText::FromString(TEXT("Prices are for one adult. Add children or seniors on the Review step.")), 17.f, false, C.Muted)
		]
	];
}

#undef LOCTEXT_NAMESPACE
