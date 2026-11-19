// Review (riders) and payment-method screens.
#include "KioskScreens.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskDashedBox.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	TSharedRef<SWidget> RiderRow(const TSharedRef<FKioskSession>& Session, int32 Category, const FText& Name, const FText& Detail, int32 Count)
	{
		const FKioskColors& C = KC();
		const TCHAR* Who = Category == 0 ? TEXT("Adult") : (Category == 1 ? TEXT("Child") : TEXT("Senior"));
		return SNew(SBox).MinDesiredHeight(102.f)
		[
			Panel(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 10.f, 12.f, 10.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ TextWrap(Name, KioskType::H3, true, C.Ink, KioskType::LineCompact) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ TextWrap(Detail, KioskType::Caption, false, C.Muted, KioskType::LineBody) ]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Outline)
					.Label(FText::Format(KT("Fewer {0}"), Name))
					.ButtonTag(*FString::Printf(TEXT("Remove %s"), Who))
					.Padding(FMargin(0.f))
					.Width(82.f).Height(82.f)
					.OnClicked_Lambda([Session, Category, Count]() { if (Count > 0 && Session->RiderCount() > 1) { Session->AdjustRider(Category, -1); } return FReply::Handled(); })
					[
						Icon(EKioskIcon::Minus, KioskLayout::IconSm, C.Ink, 4.f)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(90.f).HAlign(HAlign_Center)
					[
						Text(FText::AsNumber(Count), KioskType::Figure, true, C.Ink, ETextJustify::Center)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Strong)
					.Label(FText::Format(KT("More {0}"), Name))
					.ButtonTag(*FString::Printf(TEXT("Add %s"), Who))
					.Padding(FMargin(0.f))
					.Width(82.f).Height(82.f)
					.OnClicked_Lambda([Session, Category]() { Session->AdjustRider(Category, +1); return FReply::Handled(); })
					[
						Icon(EKioskIcon::Plus, KioskLayout::IconSm, C.White, 4.f)
					]
				],
				C.Surface, KioskLayout::InnerRadius, C.BorderStrong, 2.f, FMargin(26.f, 0.f, 18.f, 0.f))
		];
	}

	/** A "From"/"To" row: the station with its line badges right after the name. */
	TSharedRef<SWidget> PlaceRow(const FText& Label, const FText& Place, uint8 Mask)
	{
		const FKioskColors& C = KC();
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(Label, KioskType::Caption, false, C.Muted) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ TextWrap(Place, KioskType::Figure, true, C.Ink, KioskType::LineCompact) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(16.f, 6.f, 0.f, 0.f)[ LineBadges(Mask) ]
			];
	}
}

void SReviewScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const bool bHasDestination = Stations.IsValidIndex(Session->State.Destination);

	TSharedRef<SVerticalBox> Note = SNew(SVerticalBox);
	Note->AddSlot().AutoHeight()[ TextWrap(FKioskSession::ValidityRule(Session->State.Ticket), KioskType::Body, true, C.Ink, KioskType::LineBody) ];
	const FKioskRoute& Route = FKioskRoutes::FromCityHall(Session->State.Destination);
	if (bHasDestination && Route.bValid && !Route.Legs.IsEmpty())
	{
		// Short trip sentence as in Figma: "About 14 min on the Blue line." / "About 13 min · 1 change."
		const FText TripText = Route.Changes == 0
			? FText::Format(KT("About {0} min on the {1}."), FText::AsNumber(Route.Minutes), FKioskRoutes::LineName(Route.Legs[0].Line))
			: FText::Format(KT("{0}."), FText::Format(KT("About {0} min · {1} {1}|plural(one=change,other=changes)"), FText::AsNumber(Route.Minutes), Route.Changes));
		Note->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ TextWrap(TripText, KioskType::Body, false, C.Ink, KioskType::LineBody) ];
	}
	else if (!bHasDestination && !FKioskSession::DayPassWarning().IsEmpty())
	{
		Note->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ TextWrap(FKioskSession::DayPassWarning(), KioskType::Body, false, C.AmberStrong, KioskType::LineBody) ];
	}

	TSharedRef<SVerticalBox> Trip = SNew(SVerticalBox);
	Trip->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Pill(FText::Format(KT("{0} ticket"), Session->TicketLabel()), C.PrimaryTint, C.Ink, KioskType::Caption, true, C.Transparent, FMargin(26.f, 12.f))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::Link)
			.Label(KT("Change"))
			.ButtonTag(TEXT("Change"))
			.Padding(FMargin(24.f, 10.f))
			.OnClicked_Lambda([Session]() { Session->Back(); return FReply::Handled(); })
		]
	];
	Trip->AddSlot().AutoHeight().Padding(0.f, 30.f, 0.f, 0.f)[ PlaceRow(KT("From"), FKioskSession::OriginLabel(), 0b000011) ];
	if (bHasDestination)
	{
		const FKioskStation& Station = Stations[Session->State.Destination];
		// Title in the active language (matching "From").
		const FString LocalStation = FKioskPlaces::LocalName(Station.Name);
		const FText Title = FText::FromString(LocalStation.IsEmpty() ? Station.Name : LocalStation);
		Trip->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ PlaceRow(KT("To"), Title, Station.LineMask) ];
	}
	else
	{
		Trip->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)[ PlaceRow(KT("To"), KT("All lines"), 0b111111) ];
	}
	Trip->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	Trip->AddSlot().AutoHeight()
	[
		SNew(SKioskDashedBox)
		.DashColor(C.BorderStrong)
		.Padding(FMargin(24.f, 22.f))
		[
			Note
		]
	];

	const FKioskState& State = Session->State;
	const int32 Riders = Session->RiderCount();

	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, KioskLayout::TitleGap)[ Text(KT("Check your trip"), KioskType::H1, true, C.Ink) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.44f).Padding(0.f, 0.f, KioskLayout::Gutter, 0.f)
			[
				Card(Trip, C.Surface, C.BorderStrong, 2.f, KioskLayout::CardPadWide)
			]
			+ SHorizontalBox::Slot().FillWidth(0.56f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[ Text(KT("Who is riding?"), KioskType::H3, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					RiderRow(Session, 0, KT("Adult"), FText::Format(KT("{0} each"), FKioskSession::Money(Session->AdultFareCents())), State.Adults)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[
					RiderRow(Session, 1, KT("Child (5–17)"),
						FText::Format(KT("{0} each · 30% off · under 5 ride free"), FKioskSession::Money(Session->ReducedFareCents())), State.Children)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					RiderRow(Session, 2, KT("Senior (65+)"),
						FText::Format(KT("{0} each · 30% off"), FKioskSession::Money(Session->ReducedFareCents())), State.Seniors)
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
							Text(FText::Format(KT("Total · {0} {0}|plural(one=rider,other=riders)"), FText::AsNumber(Riders)), KioskType::Body, false, C.Muted)
						]
						+ SVerticalBox::Slot().AutoHeight()[ Text(FKioskSession::Money(Session->TotalCents()), KioskType::Display, true, C.Ink) ]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(16.f, 0.f, 0.f, 0.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::Primary)
						.Label(KT("Go to payment"))
						.ButtonTag(TEXT("Go to payment"))
						.RightIcon(EKioskIcon::ArrowRight)
						.MinWidth(KioskLayout::PrimaryMinW).MaxWidth(KioskLayout::PrimaryMaxW)
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
	/** One payment choice. All three look the same: they are equal choices. */
	TSharedRef<SWidget> PayCard(const TSharedRef<FKioskSession>& Session, EPayMethod Method, EKioskIcon IconType, const FText& Title,
		const FText& Body, EHardware Where, const FText& ButtonLabel, const FName& Tag, bool bFastest)
	{
		const FKioskColors& C = KC();
		FChoiceCardArgs A;
		A.Icon = IconType;
		if (bFastest)
		{
			A.Badge = Pill(KT("Fastest"), C.PrimaryTint, C.Ink, KioskType::Caption, true, C.Transparent, FMargin(22.f, 8.f));
		}
		A.Title = Title;
		A.Description = Body;
		A.Details = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 6.f, 12.f, 0.f)[ Dot(C.Primary, 14.f) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ TextWrap(HardwareName(Where), KioskType::Caption, true, C.Muted, KioskType::LineBody) ];
		A.Body = SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom)[ HardwareDiagram(Mask(Where), 0.6f) ];
		A.Footer = ActionBar(ButtonLabel);
		A.OnPick = [Session, Method]() { Session->ChoosePayMethod(Method); };
		A.Label = ButtonLabel;
		A.Tag = Tag;
		return ChoiceCard(A);
	}
}

void SPayMethodScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bTicket = Session->State.Flow == EKioskFlow::Ticket;
	ChildSlot
	.Padding(KioskLayout::PageX, KioskLayout::PageTop, KioskLayout::PageX, KioskLayout::PageBottom)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Text(FText::Format(KT("{0} {1}"), KT("Pay"), FKioskSession::Money(Session->TotalCents())), KioskType::H1, true, C.Ink)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, KioskLayout::TitleGap, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				TextWrap(KT("Choose how to pay. Any of the three works."), KioskType::Body, false, C.Ink)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)
			[
				Text(Session->OrderSummary(), KioskType::Caption, false, C.Muted)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			bTicket
				? StaticCastSharedRef<SWidget>(TextWrap(FKioskSession::ValidityRule(Session->State.Ticket), KioskType::Body, true, C.Ink))
				: StaticCastSharedRef<SWidget>(SNew(SSpacer))
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 24.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 16.f, 0.f)
			[
				PayCard(Session, EPayMethod::Tap, EKioskIcon::Contactless, KT("Tap card or phone"),
					KT("Hold it flat on the reader until you hear a beep."), EHardware::Contactless, KT("Pay with card"), TEXT("Pay with card"), true)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 8.f, 0.f)
			[
				PayCard(Session, EPayMethod::Chip, EKioskIcon::ChipCard, KT("Insert chip card"),
					KT("Chip first, facing up. Leave it in until the screen says to remove it."), EHardware::ChipSlot, KT("Pay with chip card/NETs"), TEXT("Pay with chip card/NETs"), false)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(16.f, 0.f, 0.f, 0.f)
			[
				PayCard(Session, EPayMethod::Cash, EKioskIcon::Cash, KT("Pay with cash"),
					KT("Notes up to $50 and coins. Change drops into the tray."), EHardware::CashSlots, KT("Pay with cash"), TEXT("Pay with cash"), false)
			]
		]
	];
}
