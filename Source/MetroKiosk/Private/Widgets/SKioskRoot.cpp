#include "SKioskRoot.h"

#include "KioskStyle.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskChrome.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Screens/KioskScreens.h"

#define LOCTEXT_NAMESPACE "KioskRoot"

void SKioskRoot::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& InSession)
{
	Session = InSession;
	ChangedHandle = Session->OnChanged.AddLambda([this]() { bDirty = true; });
	RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SKioskRoot::OnFrame));
	Rebuild();
}

SKioskRoot::~SKioskRoot()
{
	if (Session.IsValid())
	{
		Session->OnChanged.Remove(ChangedHandle);
	}
}

EActiveTimerReturnType SKioskRoot::OnFrame(double InCurrentTime, float InDeltaTime)
{
	Session->Tick(InDeltaTime);
	if (bDirty)
	{
		Rebuild();
	}
	return EActiveTimerReturnType::Continue;
}

void SKioskRoot::Rebuild()
{
	bDirty = false;
	FKioskStyle& Style = FKioskStyle::Get();

	// The outer border paints the letterbox area with the page background.
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(Style.Solid(Style.C.Bg))
		.BorderBackgroundColor(FLinearColor::White)
		.Padding(0.f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox)
				.WidthOverride(CanvasWidth)
				.HeightOverride(CanvasHeight)
				[
					BuildCanvas()
				]
			]
		]
	];
}

TSharedRef<SWidget> SKioskRoot::BuildCanvas()
{
	FKioskStyle& Style = FKioskStyle::Get();
	const TSharedRef<FKioskSession> S = Session.ToSharedRef();
	const EKioskScreen Visible = S->Visible();
	const EKioskScreen Current = S->Current();
	const bool bHelp = Visible == EKioskScreen::HelpCall;

	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);
	Column->AddSlot().AutoHeight()[ SNew(SKioskHeader).bHelpMode(bHelp) ];

	if (!bHelp)
	{
		const FKioskStepInfo Steps = S->StepInfo();
		if (Steps.Labels.Num() > 0)
		{
			Column->AddSlot().AutoHeight()[ SNew(SKioskStepBar).Steps(Steps) ];
		}
	}

	Column->AddSlot().FillHeight(1.f)[ MakeKioskScreen(Visible, S) ];

	if (bHelp)
	{
		Column->AddSlot().AutoHeight()[ SNew(SKioskHelpFooter, S) ];
	}
	else
	{
		FKioskFooterConfig Config;
		bool bShowFooter = true;
		switch (Current)
		{
		case EKioskScreen::Home:
			bShowFooter = false;
			break;
		case EKioskScreen::TicketType:
		case EKioskScreen::ScanCard:
			Config.bBack = true;
			break;
		case EKioskScreen::CardOptions:
		case EKioskScreen::DoneLiftCard:
			break;
		case EKioskScreen::Receipt:
			Config.bBuyAnother = true;
			break;
		case EKioskScreen::Processing:
			Config.bBack = true;
			Config.bStartOver = true;
			Config.bNavEnabled = false;
			break;
		default:
			Config.bBack = true;
			Config.bStartOver = true;
			break;
		}
		if (bShowFooter)
		{
			Column->AddSlot().AutoHeight()[ SNew(SKioskFooter, S).Config(Config) ];
		}
	}

	TSharedRef<SOverlay> Canvas = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(Style.Solid(Style.C.Bg))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(0.f)
			[
				Column
			]
		];

	if (S->IsCallActive() && !bHelp)
	{
		Canvas->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 0.f, 14.f, 0.f)[ BuildCallPip() ];
	}
	return Canvas;
}

TSharedRef<SWidget> SKioskRoot::BuildCallPip()
{
	using namespace KioskUI;
	const FKioskColors& C = KC();
	const TSharedRef<FKioskSession> S = Session.ToSharedRef();

	return SNew(SBox).WidthOverride(360.f).HeightOverride(194.f)
	[
		Panel(
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Card)
			.Label(LOCTEXT("PipLabel", "Return to the help call"))
			.Padding(FMargin(0.f))
			.OnClicked_Lambda([S]() { S->ResumeCall(); return FReply::Handled(); })
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(14.f, 12.f, 0.f, 0.f)
				[
					Panel(
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)[ Dot(C.Primary, 11.f) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							TextDyn([S]() { return FText::FromString(FString::Printf(TEXT("%d:%02d"), S->CallSeconds() / 60, S->CallSeconds() % 60)); }, 15.f, true, C.Ink)
						],
						C.White, 14.f, C.Transparent, 0.f, FMargin(14.f, 5.f))
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Icon(EKioskIcon::Person, 48.f, C.White, 3.f) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)[ Text(LOCTEXT("PipFeed", "[Staff video feed]"), 14.f, false, C.White) ]
				]
			],
			C.Video, 14.f)
	];
}

#undef LOCTEXT_NAMESPACE
