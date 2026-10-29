// Review (riders) and payment-method screens.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskDashedBox.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskReview"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> RiderRow(const TSharedRef<FKioskSession>& Session, int32 Category, const FText& Name, const FText& Detail, int32 Count)
	{
		const FKioskColors& C = KC();
		const bool bCanDecrease = Count > 0 && Session->RiderCount() > 1;
		return SNew(SBox).HeightOverride(102.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Text(Name, 24.f, true, C.Ink) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ Text(Detail, 17.f, false, C.Muted) ]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Outline)
					.Label(FText::Format(LOCTEXT("FewerRiders", "Fewer {0}"), Name))
					.ButtonTag(*FString::Printf(TEXT("Decrease %d"), Category))
					.Padding(FMargin(0.f))
					.Width(82.f).Height(82.f)
					.Enabled(bCanDecrease)
					.OnClicked_Lambda([Session, Category]() { Session->AdjustRider(Category, -1); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Minus, 30.f, bCanDecrease ? C.Ink : C.Muted, 4.f)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(120.f).HAlign(HAlign_Center)
					[
						Text(FText::AsNumber(Count), 34.f, true, C.Ink, ETextJustify::Center)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Strong)
					.Label(FText::Format(LOCTEXT("MoreRiders", "More {0}"), Name))
					.ButtonTag(*FString::Printf(TEXT("Increase %d"), Category))
					.Padding(FMargin(0.f))
					.Width(82.f).Height(82.f)
					.OnClicked_Lambda([Session, Category]() { Session->AdjustRider(Category, +1); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Plus, 30.f, C.White, 4.f)
					]
				],
				C.Surface, 18.f, C.BorderStrong, 2.f, FMargin(26.f, 0.f, 18.f, 0.f))
		];
	}

	TSharedRef<SWidget> PlaceRow(const FText& Label, const FText& Place, uint8 Mask)
	{
		const FKioskColors& C = KC();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(Label, 16.f, false, C.Muted) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Text(Place, 48.f, true, C.Ink) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16.f, 6.f, 0.f, 0.f)[ LineBadges(Mask, 30.f, 8.f) ]
			];
	}
}

void SReviewScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const bool bHasDestination = Stations.IsValidIndex(Session->State.Destination);

	const int32 Minutes = bHasDestination ? Stations[Session->State.Destination].Minutes + 1 : 0;
	const FText Note = bHasDestination
		? FText::Format(LOCTEXT("ValidNote", "Valid for 2 hours after purchase. About {0} min on the {1}."),
			FText::AsNumber(Minutes), FText::FromString(FKioskData::Line(Stations[Session->State.Destination].FirstLine()).FullName))
		: LOCTEXT("ValidDay", "Unlimited rides on every line until 12:00 a.m. tonight.");

	TSharedRef<SVerticalBox> Trip = SNew(SVerticalBox);
	Trip->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Pill(FText::Format(LOCTEXT("TicketTag", "{0} ticket"), Session->TicketLabel()), C.PrimaryTint, C.Ink, 18.f, true, C.Transparent, FMargin(26.f, 12.f))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Card)
			.Label(LOCTEXT("Change", "Change"))
			.FontPx(19.f).Bold(false)
			.Padding(FMargin(14.f, 10.f))
			.OnClicked_Lambda([Session]() { Session->Back(); return FReply::Handled(); })
		]
	];
	Trip->AddSlot().AutoHeight().Padding(8.f, 34.f, 0.f, 0.f)[ PlaceRow(LOCTEXT("From", "From"), KT("City Hall"), 0b000011) ];
	if (bHasDestination)
	{
		const FKioskStation& Station = Stations[Session->State.Destination];
		Trip->AddSlot().AutoHeight().Padding(8.f, 14.f, 0.f, 0.f)[ PlaceRow(LOCTEXT("To", "To"), FText::FromString(Station.Name), Station.LineMask) ];
	}
	Trip->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	Trip->AddSlot().AutoHeight()
	[
		SNew(SKioskDashedBox)
		.DashColor(C.BorderStrong)
		.Padding(FMargin(24.f, 24.f))
		[
			TextWrap(Note, 20.f, false, C.Ink, 1.6f)
		]
	];

	const FKioskState& State = Session->State;
	const int32 Riders = Session->RiderCount();

	ChildSlot
	.Padding(90.f, 28.f, 77.f, 28.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)[ Text(KT("Check your trip"), 38.f, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(0.f, 0.f, 44.f, 0.f)
			[
				Panel(Trip, C.Surface, 28.f, C.BorderStrong, 2.f, FMargin(40.f, 38.f, 40.f, 40.f))
			]
			+ SHorizontalBox::Slot().FillWidth(0.58f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)[ Text(KT("Who is riding?"), 20.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					RiderRow(Session, 0, KT("Adult"), FText::Format(LOCTEXT("EachAdult", "{0} each"), FKioskSession::Money(Session->AdultFareCents())), State.Adults)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					RiderRow(Session, 1, KT("Child (5–17)"),
						FText::Format(LOCTEXT("EachChild", "{0} each · 30% off · under 5 ride free"), FKioskSession::Money(Session->ReducedFareCents())), State.Children)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					RiderRow(Session, 2, KT("Senior (65+)"),
						FText::Format(LOCTEXT("EachSenior", "{0} each · 30% off"), FKioskSession::Money(Session->ReducedFareCents())), State.Seniors)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							Text(FText::Format(LOCTEXT("TotalRiders", "Total · {0} {0}|plural(one=rider,other=riders)"), FText::AsNumber(Riders)), 18.f, false, C.Muted)
						]
						+ SVerticalBox::Slot().AutoHeight()[ Text(FKioskSession::Money(Session->TotalCents()), 46.f, true, C.Ink) ]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::Primary)
						.Label(KT("Go to payment"))
						.RightIcon(EKioskIcon::ArrowRight)
						.FontPx(31.f)
						.Width(390.f).Height(94.f)
						.OnClicked_Lambda([Session]() { Session->GoToPayment(); return FReply::Handled(); })
					]
				]
			]
		]
	];
}

// --- pay method ----------------------------------------------------------------------------

namespace
{
	TSharedRef<SWidget> PayCard(const TSharedRef<FKioskSession>& Session, EPayMethod Method, EKioskIcon IconType, const FText& Title,
		const FText& Body, const FText& ButtonLabel, bool bPreferred)
	{
		const FKioskColors& C = KC();
		return Panel(
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Card)
			.Label(ButtonLabel)
			.ButtonTag(*ButtonLabel.ToString())
			.Padding(FMargin(0.f))
			.OnClicked_Lambda([Session, Method]() { Session->ChoosePayMethod(Method); return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ Icon(IconType, 70.f, C.Ink, 4.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
					[
						bPreferred
							? Pill(KT("Fastest"), C.PrimaryTint, C.Ink, 17.f, true, C.Transparent, FMargin(24.f, 9.f))
							: StaticCastSharedRef<SWidget>(SNew(SSpacer))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ Text(Title, 33.f, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)[ TextWrap(Body, 20.f, false, C.Ink, 1.6f) ]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(80.f)
					[
						Panel(SNew(SBox).VAlign(VAlign_Center)[ Text(ButtonLabel, 19.f, true, bPreferred ? C.White : C.Ink) ],
							bPreferred ? C.Ink : C.Band, 14.f, C.Transparent, 0.f, FMargin(26.f, 0.f))
					]
				]
			],
			C.Surface, 28.f, bPreferred ? C.Ink : C.BorderStrong, bPreferred ? 3.f : 2.f, FMargin(38.f, 34.f, 38.f, 34.f));
	}
}

void SPayMethodScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	ChildSlot
	.Padding(70.f, 30.f, 73.f, 28.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Text(FText::Format(LOCTEXT("PayTitle", "{0} {1}"), KT("Pay"), FKioskSession::Money(Session->TotalCents())), 42.f, true, C.Ink)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Text(KT("Use any method below. The lit slot on the kiosk shows you where."), 19.f, false, C.Ink)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				Text(Session->OrderSummary(), 16.f, false, C.Muted)
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 34.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 18.f, 0.f)
			[
				PayCard(Session, EPayMethod::Tap, EKioskIcon::Contactless, KT("Tap card or phone"),
					KT("Hold it flat on the reader until you hear a beep."), KT("Pay with card"), true)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(18.f, 0.f, 18.f, 0.f)
			[
				PayCard(Session, EPayMethod::Chip, EKioskIcon::ChipCard, KT("Insert chip card"),
					KT("Chip first, facing up. Leave it in until the screen says to remove it."), KT("Pay with chip card/NETs"), false)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(18.f, 0.f, 0.f, 0.f)
			[
				PayCard(Session, EPayMethod::Cash, EKioskIcon::Cash, KT("Pay with cash"),
					KT("Bills up to $20 and coins. Change drops into the tray."), KT("Pay with cash"), false)
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
