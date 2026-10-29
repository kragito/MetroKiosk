#include "SKioskChrome.h"

#include "KioskLoc.h"
#include "KioskWidgets.h"
#include "SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskChrome"

// --- header --------------------------------------------------------------------------------

void SKioskHeader::Construct(const FArguments& InArgs)
{
	using namespace KioskUI;
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const bool bHelp = InArgs._bHelpMode;

	TSharedRef<SWidget> Logo = bHelp
		? Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Headset, 42.f, C.Help, 3.f) ], C.White, 14.f)
		: Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(INVTEXT("M"), 56.f, true, C.White) ], C.Logo, 14.f);

	TSharedRef<SWidget> Left = SNullWidget::NullWidget;
	if (bHelp)
	{
		Left = Text(KT("Help · In call with staff"), 22.f, true, C.White);
	}
	else
	{
		Left = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(2.f).HeightOverride(46.f)
				[
					SNew(SBorder).BorderImage(Style.Solid(C.OnBrandDivider)).BorderBackgroundColor(FLinearColor::White)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(36.f, 0.f, 0.f, 0.f)
			[
				Text(KT("You are at"), 22.f, false, C.OnBrandSoft)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
			[
				Text(KT("City Hall"), 40.f, true, C.White)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 4.f, 0.f, 0.f)
			[
				LineBadges(0b000011, 24.f, 6.f)
			];
	}

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(107.f)
		[
			SNew(SBorder)
			.BorderImage(Style.Solid(bHelp ? C.Help : C.Brand))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(58.f, 0.f, 75.f, 0.f))
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(bHelp ? 62.f : 86.f).HeightOverride(bHelp ? 62.f : 77.f)
					[
						Logo
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(bHelp ? 18.f : 40.f, 0.f, 0.f, 0.f)
				[
					Left
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					TextDyn([]() { return FKioskSession::ClockText(); }, 25.f, false, C.White)
				]
			]
		]
	];
}

// --- step bar ------------------------------------------------------------------------------

void SKioskStepBar::Construct(const FArguments& InArgs)
{
	using namespace KioskUI;
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const FKioskStepInfo& Steps = InArgs._Steps;

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Steps.Labels.Num(); ++Index)
	{
		const int32 Number = Index + 1;
		const bool bDone = Number < Steps.Current;
		const bool bCurrent = Number == Steps.Current;

		TSharedRef<SWidget> Circle = SNullWidget::NullWidget;
		if (bDone)
		{
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 22.f, C.White, 3.f) ], C.Primary, 19.f);
		}
		else if (bCurrent)
		{
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(Number), 18.f, true, C.White) ], C.Ink, 19.f);
		}
		else
		{
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(Number), 17.f, false, C.Muted) ], C.Transparent, 19.f, C.Muted, 2.f);
		}

		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(38.f).HeightOverride(38.f)[ Circle ]
		];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 0.f, 0.f)
		[
			Text(Steps.Labels[Index], 17.f, false, bDone || bCurrent ? C.Ink : C.Muted)
		];
		if (Index + 1 < Steps.Labels.Num())
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 12.f, 0.f)
			[
				SNew(SBox).WidthOverride(54.f).HeightOverride(3.f)
				[
					SNew(SBorder).BorderImage(Style.Solid(C.BorderStrong)).BorderBackgroundColor(FLinearColor::White)
				]
			];
		}
	}

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(78.f)
		[
			SNew(SBorder)
			.BorderImage(Style.Solid(C.Band))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(FMargin(75.f, 0.f, 0.f, 0.f))
			.VAlign(VAlign_Center)
			[
				Row
			]
		]
	];
}

// --- footer --------------------------------------------------------------------------------

TSharedRef<SWidget> MakeLanguagePills(const TSharedRef<FKioskSession>& Session, float Height, float FontPx, float Gap)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const EKioskLanguage Lang = static_cast<EKioskLanguage>(Index);
		const bool bSelected = Session->Language() == Lang;
		Row->AddSlot().AutoWidth().Padding(Index == 0 ? 0.f : Gap, 0.f, 0.f, 0.f)
		[
			SNew(SKioskButton)
			.Kind(bSelected ? EKioskButtonKind::PillSelected : EKioskButtonKind::PillOutline)
			.Label(FKioskLoc::LanguageName(Lang))
			.ButtonTag(FName(*FString::Printf(TEXT("Language %s"), Index == 0 ? TEXT("English") : (Index == 1 ? TEXT("Chinese") : (Index == 2 ? TEXT("Malay") : TEXT("Tamil"))))))
			.FontPx(FontPx)
			.Bold(bSelected)
			.Height(Height)
			.Padding(FMargin(30.f, 0.f))
			.OnClicked_Lambda([Session, Lang]() { Session->SetLanguage(Lang); return FReply::Handled(); })
		];
	}
	return Row;
}

void SKioskFooter::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	using namespace KioskUI;
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const FKioskFooterConfig Config = InArgs._Config;

	TSharedRef<SHorizontalBox> Left = SNew(SHorizontalBox);
	if (Config.bBack)
	{
		Left->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Back"))
			.LeftIcon(EKioskIcon::ChevronLeft)
			.FontPx(25.f)
			.Width(193.f).Height(92.f)
			.Enabled(Config.bNavEnabled)
			.OnClicked_Lambda([Session]() { Session->Back(); return FReply::Handled(); })
		];
	}
	if (Config.bStartOver)
	{
		Left->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::OutlineMuted)
			.Label(KT("Start over"))
			.LeftIcon(EKioskIcon::Restart)
			.FontPx(25.f)
			.Bold(false)
			.Width(202.f).Height(92.f)
			.Enabled(Config.bNavEnabled)
			.OnClicked_Lambda([Session]() { Session->StartOver(); return FReply::Handled(); })
		];
	}
	if (Config.bBuyAnother)
	{
		Left->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("+ Buy another ticket"))
			.FontPx(23.f)
			.Width(322.f).Height(92.f)
			.OnClicked_Lambda([Session]() { Session->StartOver(); Session->GoTo(EKioskScreen::TicketType); return FReply::Handled(); })
		];
	}

	TSharedRef<SWidget> HelpButton = SNullWidget::NullWidget;
	if (Session->IsCallActive())
	{
		HelpButton = SNew(SKioskButton)
			.Kind(EKioskButtonKind::HelpLight)
			.Label(KT("Back to call"))
			.ButtonTag(TEXT("Back to call"))
			.Width(309.f).Height(92.f)
			.OnClicked_Lambda([Session]() { Session->ResumeCall(); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
				[
					Dot(C.Primary, 30.f)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Text(KT("Back to call"), 28.f, true, C.Ink)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
				[
					Icon(EKioskIcon::ChevronRight, 28.f, C.Ink, 3.f)
				]
			];
	}
	else
	{
		HelpButton = SNew(SKioskButton)
			.Kind(EKioskButtonKind::Help)
			.Label(KT("Help · talk to staff"))
			.LeftIcon(EKioskIcon::Headset)
			.FontPx(23.f)
			.Width(309.f).Height(92.f)
			.OnClicked_Lambda([Session]() { Session->StartCall(); return FReply::Handled(); });
	}

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(128.f)
		[
			SNew(SBorder)
			.BorderImage(Style.Solid(C.Surface))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Rule(C.BorderStrong, 2.f)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(75.f, 0.f, 0.f, 0.f)
					[
						Left
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						MakeLanguagePills(Session, 71.f, 21.f, 10.f)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(40.f, 0.f, 73.f, 0.f)
					[
						HelpButton
					]
				]
			]
		]
	];
}

// --- help footer ---------------------------------------------------------------------------

void SKioskHelpFooter::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	using namespace KioskUI;
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(128.f)
		[
			SNew(SBorder)
			.BorderImage(Style.Solid(C.Surface))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					Rule(C.BorderStrong, 2.f)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(73.f, 0.f, 0.f, 0.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::Primary)
						.Label(KT("Back to my purchase"))
						.LeftIcon(EKioskIcon::ChevronLeft)
						.FontPx(33.f)
						.Width(500.f).Height(92.f)
						.OnClicked_Lambda([Session]() { Session->ReturnToPurchase(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 75.f, 0.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::DangerOutline)
						.Label(KT("End call"))
						.LeftIcon(EKioskIcon::Close)
						.FontPx(33.f)
						.Width(308.f).Height(92.f)
						.OnClicked_Lambda([Session]() { Session->EndCall(); return FReply::Handled(); })
					]
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
