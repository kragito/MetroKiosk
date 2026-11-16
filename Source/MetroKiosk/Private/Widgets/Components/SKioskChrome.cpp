#include "SKioskChrome.h"

#include "KioskLoc.h"
#include "KioskWidgets.h"
#include "SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

// --- header --------------------------------------------------------------------------------

void SKioskHeader::Construct(const FArguments& InArgs)
{
	using namespace KioskUI;
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const bool bHelp = InArgs._bHelpMode;

	TSharedRef<SWidget> Logo = bHelp
		? Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Headset, 42.f, C.Help, 3.f) ], C.White, KioskLayout::InnerRadius)
		: Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::FromString(TEXT("M")), 56.f, true, C.White) ], C.Logo, KioskLayout::InnerRadius);

	TSharedRef<SWidget> Left = SNullWidget::NullWidget;
	if (bHelp)
	{
		Left = Text(KT("Help · In call with staff"), KioskType::H3, true, C.White);
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
				Text(KT("You are at"), KioskType::Body, false, C.OnBrandSoft)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 0.f, 0.f)
			[
				Text(KT("City Hall"), KioskType::H1, true, C.White)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 4.f, 0.f, 0.f)
			[
				LineBadges(0b000011)
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
					TextDyn([]() { return FKioskSession::ClockText(); }, KioskType::H3, false, C.White)
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
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Check, 24.f, C.White, 3.f) ], C.Primary, 22.f);
		}
		else if (bCurrent)
		{
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(Number), KioskType::Body, true, C.White) ], C.Ink, 22.f);
		}
		else
		{
			Circle = Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Text(FText::AsNumber(Number), KioskType::Body, false, C.Muted) ], C.Transparent, 22.f, C.Muted, 2.f);
		}

		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(44.f).HeightOverride(44.f)[ Circle ]
		];
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
		[
			// Chosen names can be long (station names in other languages): wrap inside a capped width so later steps stay on the bar.
			SNew(SBox).MaxDesiredWidth(250.f)
			[
				TextWrap(Steps.Labels[Index], KioskType::Body, bCurrent, bDone || bCurrent ? C.Ink : C.Muted)
			]
		];
		if (Index + 1 < Steps.Labels.Num())
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(14.f, 0.f, 14.f, 0.f)
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
			.Padding(FMargin(KioskLayout::PageX, 0.f, 0.f, 0.f))
			.VAlign(VAlign_Center)
			[
				Row
			]
		]
	];
}

// --- footer --------------------------------------------------------------------------------

TSharedRef<SWidget> MakeLanguagePill(const TSharedRef<FKioskSession>& Session, EKioskLanguage Lang, bool bFill)
{
	const bool bSelected = Session->Language() == Lang;
	const int32 Index = static_cast<int32>(Lang);
	return SNew(SKioskButton)
		.Kind(bSelected ? EKioskButtonKind::PillSelected : EKioskButtonKind::PillOutline)
		.Label(FKioskLoc::LanguageName(Lang))
		.ButtonTag(FName(*FString::Printf(TEXT("Language %s"), Index == 0 ? TEXT("English") : (Index == 1 ? TEXT("Chinese") : (Index == 2 ? TEXT("Malay") : TEXT("Tamil"))))))
		.Bold(true)
		.MinWidth(bFill ? 0.f : KioskLayout::PillW)
		.Padding(FMargin(18.f, 6.f))
		.OnClicked_Lambda([Session, Lang]() { Session->SetLanguage(Lang); return FReply::Handled(); });
}

TSharedRef<SWidget> MakeLanguagePills(const TSharedRef<FKioskSession>& Session)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Row->AddSlot().AutoWidth().Padding(Index == 0 ? 0.f : 10.f, 0.f, 0.f, 0.f)
		[
			MakeLanguagePill(Session, static_cast<EKioskLanguage>(Index), false)
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
			.ButtonTag(TEXT("Back"))
			.LeftIcon(EKioskIcon::ChevronLeft)
			.MinWidth(190.f).MaxWidth(300.f)
			.Enabled(Config.bNavEnabled)
			.OnClicked_Lambda([Session]() { Session->Back(); return FReply::Handled(); })
		];
	}
	if (Config.bStartOver)
	{
		Left->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(Config.bBack ? 16.f : 0.f, 0.f, 0.f, 0.f)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Start over"))
			.ButtonTag(TEXT("Start over"))
			.LeftIcon(EKioskIcon::Restart)
			.MinWidth(210.f).MaxWidth(320.f)
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
			.ButtonTag(TEXT("+ Buy another ticket"))
			.MaxWidth(380.f)
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
			.MinWidth(300.f).MaxWidth(420.f)
			.OnClicked_Lambda([Session]() { Session->ResumeCall(); return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
				[
					Dot(C.Primary, 26.f)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Text(KT("Back to call"), KioskType::BtnStd, true, C.Ink)
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
			.ButtonTag(TEXT("Help"))
			.LeftIcon(EKioskIcon::Headset)
			.MinWidth(300.f).MaxWidth(400.f)
			.OnClicked_Lambda([Session]() { Session->StartCall(); return FReply::Handled(); });
	}

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(KioskLayout::FooterH)
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
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(KioskLayout::PageX, 0.f, 0.f, 0.f)
					[
						Left
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SSpacer)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						MakeLanguagePills(Session)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(32.f, 0.f, 73.f, 0.f)
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
		.HeightOverride(KioskLayout::FooterH)
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
						.ButtonTag(TEXT("Back to my purchase"))
						.LeftIcon(EKioskIcon::ChevronLeft)
						.MinWidth(420.f).MaxWidth(760.f)
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
						.ButtonTag(TEXT("End call"))
						.LeftIcon(EKioskIcon::Close)
						.MinWidth(280.f).MaxWidth(420.f)
						.MinHeight(KioskLayout::BtnPrimaryH)
						.OnClicked_Lambda([Session]() { Session->EndCall(); return FReply::Handled(); })
					]
				]
			]
		]
	];
}
