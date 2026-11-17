// Full-screen help call (video placeholder + captions + controls) and the screen factory.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> ActionCard(const TSharedRef<FKioskSession>& Session, EKioskIcon IconType, const FText& Label, const FName& Tag, bool bActive, TFunction<void()> Toggle, bool bStatus = false)
	{
		const FKioskColors& C = KC();
		return SNew(SBox).MinDesiredHeight(100.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(Label)
				.ButtonTag(Tag)
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Toggle]() { Toggle(); return FReply::Handled(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(IconType, KioskLayout::IconSm, bStatus ? C.Muted : C.Ink, 3.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 10.f, 0.f, 10.f)[ TextWrap(Label, bStatus ? KioskType::Body : KioskType::H3, !bStatus, bStatus ? C.Muted : C.Ink, KioskType::LineCompact) ]
				],
				bActive ? C.HelpTint : C.Surface, KioskLayout::InnerRadius, bStatus ? C.Band : (bActive ? C.Ink : C.BorderStrong), bActive ? 3.f : 2.f, FMargin(28.f, 0.f))
		];
	}
}

void SHelpCallScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();

	const FText Caption = (Session->State.Flow == EKioskFlow::Ticket && Stations.IsValidIndex(Session->State.Destination))
		? FText::Format(KT("Hi, I can see you're buying a {0} ticket to {1}. How can I help?"),
			FText::FromString(Session->TicketLabel().ToString().ToLower()), Session->DestinationLabel())
		: KT("Hi, I can see you're in the middle of a purchase. How can I help?");

	TSharedRef<SWidget> Video = Panel(
		SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f, 22.f, 0.f, 0.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[ Dot(C.Primary, 15.f) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					TextDyn([Session]()
					{
						const int32 Seconds = Session->CallSeconds();
						return FText::Format(KT("Connected · {0}:{1}"), FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));
					}, KioskType::Body, true, C.Ink)
				],
				C.White, 22.f, C.Transparent, 0.f, FMargin(22.f, 10.f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Icon(EKioskIcon::Person, KioskLayout::IconLg, C.White, 4.f) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 22.f, 0.f, 0.f)[ Text(KT("[Staff video feed]"), KioskType::Caption, false, C.White) ]
		],
		C.Video, KioskLayout::CardRadius);

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.58f).Padding(0.f, 0.f, 46.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)[ Video ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
			[
				SNew(SBox).MinDesiredHeight(180.f)
				[
					Card(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(KT("LIVE CAPTIONS"), KioskType::Caption, true, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							Session->bCaptionsOn
								? StaticCastSharedRef<SWidget>(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[ Text(KT("Staff:"), KioskType::Body, true, C.Help) ]
									+ SHorizontalBox::Slot().FillWidth(1.f).Padding(20.f, 0.f, 0.f, 0.f)[ TextWrap(Caption, KioskType::Body, false, C.Ink, KioskType::LineBody) ])
								: StaticCastSharedRef<SWidget>(Text(KT("Captions are off."), KioskType::Body, false, C.Muted))
						],
						C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide)
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.42f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Card(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 6.f, 22.f, 0.f)[ Icon(EKioskIcon::Pause, KioskLayout::IconSm, C.Help, 3.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Your purchase is paused."), KioskType::H2, true, C.Ink, KioskType::LineCompact) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
						[
							TextWrap(Session->HelpPausedBody(), KioskType::Body, false, C.Ink, KioskType::LineBody)
						]
					],
					C.HelpTint, C.Transparent, 0.f, KioskLayout::CardPadWide)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Mic, Session->bMicMuted ? KT("Unmute microphone") : KT("Mute microphone"), TEXT("Mute microphone"), Session->bMicMuted,
					[Session]() { Session->bMicMuted = !Session->bMicMuted; Session->NotifyChanged(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Person, Session->bStaffRequested ? KT("Staff are on their way") : KT("Ask staff to come here"), TEXT("Ask staff to come here"), Session->bStaffRequested,
					[Session]() { Session->bStaffRequested = !Session->bStaffRequested; Session->NotifyChanged(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Captions, Session->bCaptionsOn ? KT("Captions are on") : KT("Captions are off"), TEXT("Captions"), false,
					[Session]() { Session->bCaptionsOn = !Session->bCaptionsOn; Session->NotifyChanged(); }, true)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight()
			[
				TextWrap(KT("Or visit the staffed office at Gate B, or use the Help button on the front of the kiosk."), KioskType::Caption, false, C.Muted, KioskType::LineBody, ETextJustify::Right)
			]
		]
	];
}

TSharedRef<SWidget> MakeKioskScreen(EKioskScreen Screen, const TSharedRef<FKioskSession>& Session)
{
	switch (Screen)
	{
	case EKioskScreen::Home: return SNew(SHomeScreen, Session);
	case EKioskScreen::TicketType: return SNew(STicketTypeScreen, Session);
	case EKioskScreen::Destination: return SNew(SDestinationScreen, Session);
	case EKioskScreen::DestinationMap: return SNew(SMapScreen, Session);
	case EKioskScreen::Review: return SNew(SReviewScreen, Session);
	case EKioskScreen::PayMethod: return SNew(SPayMethodScreen, Session);
	case EKioskScreen::WaitTapCard:
	case EKioskScreen::WaitChipCard:
	case EKioskScreen::WaitCash:
	case EKioskScreen::Processing: return SNew(SWaitScreen, Session);
	case EKioskScreen::Receipt: return SNew(SReceiptScreen, Session);
	case EKioskScreen::HelpCall: return SNew(SHelpCallScreen, Session);
	case EKioskScreen::ScanCard: return SNew(SScanCardScreen, Session);
	case EKioskScreen::CardOptions: return SNew(SCardOptionsScreen, Session);
	case EKioskScreen::TopUpAmount: return SNew(STopUpScreen, Session);
	case EKioskScreen::Concession: return SNew(SConcessionScreen, Session);
	case EKioskScreen::PassDates: return SNew(SPassDatesScreen, Session);
	case EKioskScreen::DoneLiftCard: return SNew(SDoneScreen, Session);
	default: return SNullWidget::NullWidget;
	}
}

