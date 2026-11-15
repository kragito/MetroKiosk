#include "SKioskRoot.h"

#include "KioskLoc.h"
#include "KioskStyle.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskChrome.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Screens/KioskScreens.h"

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

FReply SKioskRoot::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (Session.IsValid())
	{
		Session->NoteActivity();
	}
	return FReply::Unhandled();
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
			Config.bBack = true;
			break;
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

	if (const int32 Returned = S->ReturnedCashBanner())
	{
		using namespace KioskUI;
		const FKioskColors& C = Style.C;
		Canvas->AddSlot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, Current == EKioskScreen::Home ? 190.f : 150.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 16.f, 0.f)[ Icon(EKioskIcon::Tray, 34.f, C.Ink, 3.f) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Text(FText::Format(KT("Your {0} has been returned to the collection tray."), FKioskSession::Money(Returned)), KioskType::H3, true, C.Ink)
				],
				C.AmberFill, KioskLayout::CardRadius, C.AmberStroke, 2.f, FMargin(30.f, 16.f))
		];
	}

	if (S->IsCallActive() && !bHelp)
	{
		Canvas->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 0.f, 14.f, 0.f)[ BuildCallPip() ];
	}
	if (S->IsIdlePrompt() && !bHelp)
	{
		Canvas->AddSlot()[ BuildIdlePrompt() ];
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
			.Label(KT("Return to the help call"))
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
							TextDyn([S]() { return FText::FromString(FString::Printf(TEXT("%d:%02d"), S->CallSeconds() / 60, S->CallSeconds() % 60)); }, KioskType::Caption, true, C.Ink)
						],
						C.White, 14.f, C.Transparent, 0.f, FMargin(14.f, 5.f))
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Icon(EKioskIcon::Person, 48.f, C.White, 3.f) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)[ Text(KT("[Staff video feed]"), KioskType::Caption, false, C.White) ]
				]
			],
			C.Video, KioskLayout::InnerRadius)
	];
}

TSharedRef<SWidget> SKioskRoot::BuildIdlePrompt()
{
	using namespace KioskUI;
	const FKioskColors& C = KC();
	const TSharedRef<FKioskSession> S = Session.ToSharedRef();

	return SNew(SBorder)
		.BorderImage(FKioskStyle::Get().Solid(FLinearColor(0.05f, 0.08f, 0.15f, 0.62f)))
		.BorderBackgroundColor(FLinearColor::White)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(900.f)
			[
				Card(
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SBox).WidthOverride(96.f).HeightOverride(96.f)
						[
							Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(EKioskIcon::Clock, 56.f, C.White, 5.f) ], C.AmberStrong, 48.f)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 26.f, 0.f, 0.f).HAlign(HAlign_Center)
					[
						TextWrap(KT("Are you still there?"), KioskType::H1, true, C.Ink, 1.f, ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f).HAlign(HAlign_Center)
					[
						TextDyn([S]() { return FText::Format(KT("Your purchase will be cancelled and the kiosk will reset in {0} s."), FText::AsNumber(S->IdlePromptSecondsLeft())); },
							KioskType::Body, false, C.Ink, ETextJustify::Center)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 28.f, 0.f, 0.f)
					[
						ProgressBar([S]() { return static_cast<float>(S->IdlePromptSecondsLeft()) / FKioskSession::IdlePromptSeconds; }, 0.f, 14.f, C.Disabled, C.AmberStrong)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 34.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 12.f, 0.f)
						[
							SNew(SKioskButton)
							.Kind(EKioskButtonKind::Primary)
							.Label(KT("Continue"))
							.ButtonTag(TEXT("Continue"))
							.OnClicked_Lambda([S]() { S->DismissIdlePrompt(); return FReply::Handled(); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(12.f, 0.f, 0.f, 0.f)
						[
							SNew(SKioskButton)
							.Kind(EKioskButtonKind::Outline)
							.Label(KT("Start over"))
							.LeftIcon(EKioskIcon::Restart)
							.ButtonTag(TEXT("Start over"))
							.MinHeight(KioskLayout::BtnPrimaryH)
							.OnClicked_Lambda([S]() { S->ReturnHome(true); return FReply::Handled(); })
						]
					],
					C.Surface, C.Ink, 3.f, FMargin(54.f, 44.f))
			]
		];
}

