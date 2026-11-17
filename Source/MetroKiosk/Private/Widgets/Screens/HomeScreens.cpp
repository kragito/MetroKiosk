// Home and ticket-type screens.
#include "KioskScreens.h"

#include "KioskLoc.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskChrome.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> SectionLabel(const FText& Label)
	{
		return Text(Label, KioskType::Body, true, KC().Muted);
	}

	/** Right inset on tile descriptions so a two-line wrap breaks mid-sentence instead of leaving one word (e.g. "pass") alone on line two. */
	constexpr float TileDescInset = 40.f;

	/** The one action tile used for ticket / top up / concession: icon, title, description, optional info, full-width action button. */
	TSharedRef<SWidget> ActionTile(const TSharedRef<FKioskSession>& Session, const FLinearColor& Fill, const FLinearColor& Ink, bool bOutlined,
		EKioskIcon IconType, const FText& Title, const FText& Description, const TSharedRef<SWidget>& Info,
		const FText& ActionLabel, const FName& Tag, TFunction<void()> OnPick)
	{
		const FKioskColors& C = KC();
		FChoiceCardArgs A;
		A.Icon = IconType;
		A.IconSize = KioskLayout::IconLg;
		A.IconThickness = 4.f;
		A.bIconBesideTitle = true;
		A.Title = Title;
		A.TitlePx = KioskType::H1;
		A.Ink = Ink;
		A.DescColor = Ink;
		A.Description = Description;
		A.DescInset = TileDescInset;
		if (&Info.Get() != &SNullWidget::NullWidget.Get())
		{
			A.Details = Info;
		}
		A.Footer = ActionBar(ActionLabel, true);
		A.Fill = Fill;
		A.Border = bOutlined ? C.Ink : C.Transparent;
		A.BorderWidth = bOutlined ? 3.f : 0.f;
		A.OnPick = OnPick;
		A.Label = ActionLabel;
		A.Tag = Tag;
		return ChoiceCard(A);
	}
}

void SHomeScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();

	// Column 1: one-time ticket. The two price lines are plain information, not controls.
	TSharedRef<SWidget> TicketInfo = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Text(FText::Format(KT("{0} · from {1}"), KT("One-way"), FKioskSession::Money(FKioskFares::MinOneWayCents())), KioskType::Body, true, C.White)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			Text(FText::Format(KT("{0} · {1}"), KT("Day pass"), FKioskSession::Money(FKioskFares::DayPassCents)), KioskType::Body, true, C.White)
		];
	TSharedRef<SWidget> TicketCard = ActionTile(Session, C.Primary, C.White, false, EKioskIcon::Ticket, KT("One-time ticket"),
		KT("Printed ticket for one trip. Choose one-way, round trip or a day pass."), TicketInfo, KT("Buy a ticket"), TEXT("Buy a ticket"),
		[Session]() { Session->State.Flow = EKioskFlow::Ticket; Session->GoTo(EKioskScreen::TicketType); });

	TSharedRef<SWidget> TopUpCard = ActionTile(Session, C.Surface, C.Ink, true, EKioskIcon::TopUpCard, KT("Top up"),
		KT("Add value and check your balance"), SNullWidget::NullWidget, KT("Top up"), TEXT("Top up"),
		[Session]() { Session->BeginCardFlow(EKioskFlow::TopUp); });
	TSharedRef<SWidget> ConcessionCard = ActionTile(Session, C.Surface, C.Ink, true, EKioskIcon::IdCard, KT("Concession"),
		KT("Student, senior or disability card: buy or renew a pass"), SNullWidget::NullWidget, KT("Buy or renew"), TEXT("Concession"),
		[Session]() { Session->BeginCardFlow(EKioskFlow::Concession); });

	// Column 3: language + assistance
	TSharedRef<SWidget> LanguageCard = Card(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Globe, KioskLayout::IconLg, C.Ink, 3.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(22.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Language"), KioskType::H1, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					Text(FText::FromString(TEXT("语言 · Bahasa · மொழி")), KioskType::Body, false, C.Muted)
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 14.f, 0.f, 0.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 7.f, 7.f)[ MakeLanguagePill(Session, EKioskLanguage::English, true) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 0.f, 0.f, 7.f)[ MakeLanguagePill(Session, EKioskLanguage::Chinese, true) ]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 7.f, 7.f, 0.f)[ MakeLanguagePill(Session, EKioskLanguage::Malay, true) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(7.f, 7.f, 0.f, 0.f)[ MakeLanguagePill(Session, EKioskLanguage::Tamil, true) ]
			]
		],
		C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide);

	TSharedRef<SWidget> CallCard = Panel(
		SNew(SKioskButton)
		.Kind(EKioskButtonKind::Card)
		.Label(KT("Call for assistance"))
		.Padding(FMargin(0.f))
		.OnClicked_Lambda([Session]() { Session->StartCall(); return FReply::Handled(); })
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)[ Dot(C.Primary, 14.f) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(KT("Staff online"), KioskType::Caption, true, C.Ink) ],
					C.White, 20.f, C.Transparent, 0.f, FMargin(18.f, 8.f))
			]
			+ SOverlay::Slot().VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Left).VAlign(VAlign_Center)[ Icon(EKioskIcon::Headset, KioskLayout::IconHero, C.White, 5.f) ]
				+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Call for assistance"), KioskType::H1, true, C.White, KioskType::LineCompact) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ TextWrap(KT("Video call with station staff"), KioskType::Body, false, C.White, KioskType::LineBody) ]
			]
		],
		C.Help, KioskLayout::CardRadius, C.Transparent, 0.f, KioskLayout::CardPadWide);

	auto Column = [&](const FText& Label, const TSharedRef<SWidget>& Body)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 0.f, 10.f)[ SectionLabel(Label) ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ Body ];
	};

	TSharedRef<SWidget> Banner = Panel(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(60.f, 0.f, 0.f, 0.f)
		[
			Badge(EKioskIcon::Contactless, KioskLayout::IconDiscLg, C.PrimaryTint, C.Primary, 4.f)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(28.f, 14.f, 24.f, 14.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Or place your Metro card on the reader below the screen to begin"), KioskType::H2, true, C.Ink, KioskType::LineCompact) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ TextWrap(KT("We'll show your balance and what you can do with it"), KioskType::Body, false, C.Muted, KioskType::LineBody) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::DoubleChevronDown, KioskLayout::IconSm, C.Primary, 4.f) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
				[
					TextWrap(HardwareName(EHardware::MetroReader), KioskType::Caption, true, C.Ink, KioskType::LineBody)
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 28.f, 0.f)[ HardwareDiagram(Mask(EHardware::MetroReader), 0.55f) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 60.f, 0.f)
		[
			SimControl(KT("SIMULATE: tap card on reader"), FOnClicked::CreateLambda([Session]() { Session->SimulateCardTap(); return FReply::Handled(); }), TEXT("SIMULATE: tap card on reader"))
		],
		C.Surface, 0.f);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
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
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, KioskLayout::TileGap * 0.5f)[ TopUpCard ]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, KioskLayout::TileGap * 0.5f, 0.f, 0.f)[ ConcessionCard ])
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(17.f, 0.f, 0.f, 0.f)
			[
				Column(KT("HELP & SETTINGS"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, KioskLayout::TileGap * 0.5f)[ LanguageCard ]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, KioskLayout::TileGap * 0.5f, 0.f, 0.f)[ CallCard ])
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Rule(C.BorderStrong, 2.f) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox).MinDesiredHeight(166.f)[ Banner ]
			]
		]
	];
}

// --- ticket type ---------------------------------------------------------------------------

namespace
{
	TSharedRef<SWidget> TicketCardWidget(const TSharedRef<FKioskSession>& Session, ETicketType Type, EKioskIcon IconType, const FText& Title,
		const FText& Description, const FText& StrikePrice, const FText& Price, const FName& Tag, bool bBestValue, const FText& Warning)
	{
		const FKioskColors& C = KC();
		FKioskStyle& Style = FKioskStyle::Get();

		// Price (with optional struck-through list price above) sits left; the compact Choose button sits right on the same row.
		TSharedRef<SWidget> PriceBlock = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				StrikePrice.IsEmpty()
					? StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(1.f, KioskType::Caption + 4.f)))
					: StaticCastSharedRef<SWidget>(
						// The text's own strike brush rendered too faint, so the line is drawn explicitly across the middle of the text.
						// Wrapped in an AutoWidth slot so the overlay (and the line) is only as wide as the text.
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[ Text(StrikePrice, KioskType::Caption, false, C.Muted) ]
							+ SOverlay::Slot().VAlign(VAlign_Center)
							[
								SNew(SBox).HeightOverride(2.f)[ SNew(SBorder).BorderImage(Style.Solid(C.Ink)) ]
							]
						])
			]
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(Price, KioskType::Figure, true, C.Ink, KioskType::LineCompact) ];

		TSharedRef<SVerticalBox> Details = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ TextWrap(FKioskSession::ValidityRule(Type), KioskType::Body, true, C.Ink, KioskType::LineBody) ];
		if (!Warning.IsEmpty())
		{
			Details->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 5.f, 12.f, 0.f)[ Dot(C.AmberStrong, 14.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)[ TextWrap(Warning, KioskType::Caption, true, C.Ink, KioskType::LineBody) ],
					C.AmberFill, KioskLayout::InnerRadius, C.AmberStroke, 2.f, FMargin(16.f, 12.f))
			];
		}

		FChoiceCardArgs A;
		A.Icon = IconType;
		A.IconSize = KioskLayout::IconMd;
		A.IconThickness = 3.5f;
		if (bBestValue)
		{
			A.Badge = Pill(KT("Best for 3+ rides"), C.PrimaryTint, C.Ink, KioskType::Caption, true, C.Transparent, FMargin(20.f, 8.f));
		}
		A.Title = Title;
		A.Description = Description;
		A.Details = Details;
		A.Footer = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 16.f, 0.f)[ PriceBlock ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Strong)
				.Label(KT("Choose"))
				.ButtonTag(Tag)
				.MinWidth(KioskLayout::CompactBtnMinW)
				.OnClicked_Lambda([Session, Type]() { Session->ChooseTicket(Type); return FReply::Handled(); })
			];
		return ChoiceCard(A);
	}
}

void STicketTypeScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const FText Late = FKioskSession::DayPassWarning();
	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Choose a ticket"), KioskType::H1, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, KioskLayout::TitleGap, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 18.f, 0.f)
			[
				TicketCardWidget(Session, ETicketType::OneWay, EKioskIcon::ArrowRight, KT("One-way"),
					KT("One trip from here to one station."), FText::GetEmpty(),
					FText::Format(KT("from {0}"), FKioskSession::Money(FKioskFares::MinOneWayCents())), TEXT("Choose One-way"), false, FText::GetEmpty())
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(9.f, 0.f, 9.f, 0.f)
			[
				TicketCardWidget(Session, ETicketType::RoundTrip, EKioskIcon::Swap, KT("Round trip"),
					KT("There and back between two stations. 30% discount."),
					FText::Format(KT("{0} - {1}"), FKioskSession::Money(FKioskFares::MinOneWayCents() * 2), FKioskSession::Money(FKioskFares::MaxOneWayCents() * 2)),
					FText::Format(KT("{0} - {1}"), FKioskSession::Money(FKioskFares::RoundTripFromOneWay(FKioskFares::MinOneWayCents())),
						FKioskSession::Money(FKioskFares::RoundTripFromOneWay(FKioskFares::MaxOneWayCents()))), TEXT("Choose Round trip"), false, FText::GetEmpty())
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(18.f, 0.f, 0.f, 0.f)
			[
				TicketCardWidget(Session, ETicketType::DayPass, EKioskIcon::Clock, KT("Day pass"),
					KT("Every line, as many trips as you like. No destination needed."),
					FText::GetEmpty(), FKioskSession::Money(FKioskFares::DayPassCents), TEXT("Choose Day pass"), Late.IsEmpty(), Late)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
		[
			Text(KT("Prices are for one adult. Add children or seniors on the Review step."), KioskType::Caption, false, C.Muted)
		]
	];
}
