// Full-screen help call (video placeholder + captions + controls) and the screen factory.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskHelp"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> ActionCard(const TSharedRef<FKioskSession>& Session, EKioskIcon IconType, const FText& Label, bool bActive, TFunction<void()> Toggle)
	{
		const FKioskColors& C = KC();
		return SNew(SBox).HeightOverride(100.f)
		[
			Panel(
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(Label)
				.Padding(FMargin(0.f))
				.OnClicked_Lambda([Toggle]() { Toggle(); return FReply::Handled(); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(IconType, 34.f, C.Ink, 3.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(24.f, 0.f, 0.f, 0.f)[ Text(Label, 22.f, true, C.Ink) ]
				],
				bActive ? C.HelpTint : C.Surface, 18.f, bActive ? C.Ink : C.BorderStrong, bActive ? 3.f : 2.f, FMargin(28.f, 0.f))
		];
	}
}

void SHelpCallScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();

	const FText Caption = (Session->State.Flow == EKioskFlow::Ticket && Stations.IsValidIndex(Session->State.Destination))
		? FText::Format(LOCTEXT("CaptionTicket", "Hi, I can see you're buying a {0} ticket to {1}. How can I help?"),
			FText::FromString(Session->TicketLabel().ToString().ToLower()), FText::FromString(Stations[Session->State.Destination].Name))
		: LOCTEXT("CaptionGeneric", "Hi, I can see you're in the middle of a purchase. How can I help?");

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
						return FText::Format(LOCTEXT("Connected", "Connected · {0}:{1}"), FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));
					}, 21.f, true, C.Ink)
				],
				C.White, 22.f, C.Transparent, 0.f, FMargin(22.f, 10.f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Icon(EKioskIcon::Person, 90.f, C.White, 4.f) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 22.f, 0.f, 0.f)[ Text(LOCTEXT("VideoFeed", "[Staff video feed]"), 16.f, false, C.White) ]
		],
		C.Video, 28.f);

	ChildSlot
	.Padding(73.f, 50.f, 75.f, 40.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.58f).Padding(0.f, 0.f, 46.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.f)[ Video ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 24.f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(180.f)
				[
					Panel(
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(LOCTEXT("LiveCaptions", "LIVE CAPTIONS"), 14.f, true, C.Muted) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
						[
							Session->bCaptionsOn
								? StaticCastSharedRef<SWidget>(
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)[ Text(LOCTEXT("StaffSays", "Staff:"), 20.f, true, C.Help) ]
									+ SHorizontalBox::Slot().FillWidth(1.f).Padding(22.f, 0.f, 0.f, 0.f)[ TextWrap(Caption, 20.f, false, C.Ink, 1.5f) ])
								: StaticCastSharedRef<SWidget>(Text(LOCTEXT("CaptionsOff", "Captions are off."), 20.f, false, C.Muted))
						],
						C.Surface, 24.f, C.BorderStrong, 2.f, FMargin(34.f, 28.f))
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.42f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Panel(
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 6.f, 22.f, 0.f)[ Icon(EKioskIcon::Pause, 34.f, C.Help, 3.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Your purchase is paused."), 29.f, true, C.Ink) ]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
						[
							TextWrap(LOCTEXT("PausedBody", "Nothing has been charged. You can pick up where you left off."), 21.f, false, C.Ink, 1.3f)
						]
					],
					C.HelpTint, 22.f, C.Transparent, 0.f, FMargin(34.f, 30.f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Mic, Session->bMicMuted ? LOCTEXT("Unmute", "Unmute microphone") : KT("Mute microphone"), Session->bMicMuted,
					[Session]() { Session->bMicMuted = !Session->bMicMuted; Session->NotifyChanged(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Person, Session->bStaffRequested ? LOCTEXT("StaffComing", "Staff are on their way") : KT("Ask staff to come here"), Session->bStaffRequested,
					[Session]() { Session->bStaffRequested = !Session->bStaffRequested; Session->NotifyChanged(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
			[
				ActionCard(Session, EKioskIcon::Captions, Session->bCaptionsOn ? KT("Captions are on") : LOCTEXT("CaptionsOffLabel", "Captions are off"), false,
					[Session]() { Session->bCaptionsOn = !Session->bCaptionsOn; Session->NotifyChanged(); })
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				Text(LOCTEXT("Footnote", "Or visit the staffed office at Gate B, or call [HELPLINE NUMBER]."), 15.f, false, C.Muted)
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

#undef LOCTEXT_NAMESPACE
