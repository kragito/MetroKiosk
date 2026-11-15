// Schematic metro map: tap a station to choose it. The line highlighter here is an optional tool.
#include "KioskScreens.h"

#include "DestinationWidgets.h"
#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskRoutes.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Components/SKioskMap.h"
#include "Widgets/Layout/SSpacer.h"

using namespace KioskUI;

namespace
{
	/** Optional tool: tap a line to highlight it on the map; tap it again to show every line. */
	TSharedRef<SWidget> LineHighlighter(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		TSharedRef<SHorizontalBox> Badges = SNew(SHorizontalBox);
		for (int32 Line = 0; Line < FKioskData::NumLines; ++Line)
		{
			const bool bActive = Session->State.LineFilter == Line;
			Badges->AddSlot().AutoWidth().Padding(Line == 0 ? 0.f : 10.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(60.f)
				[
					SNew(SKioskButton)
					.Kind(bActive ? EKioskButtonKind::Tint : EKioskButtonKind::OutlineMuted)
					.Label(FKioskRoutes::LineName(Line))
					.ButtonTag(*FString::Printf(TEXT("Filter %s"), FKioskData::Line(Line).Code))
					.Padding(FMargin(12.f, 0.f))
					.OnClicked_Lambda([Session, Line]() { Session->SetLineFilter(Line); return FReply::Handled(); })
					[
						SNew(SBox).VAlign(VAlign_Center)[ LineBadge(Line) ]
					]
				]
			];
		}
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Text(KT("Highlight one line (optional)"), KioskType::Caption, true, C.Muted) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[ Badges ];
	}
}

void SMapScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();

	auto ZoomButton = [&](EKioskIcon IconType, const FText& Label, float Delta)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Key)
			.Label(Label)
			.ButtonTag(Delta > 0.f ? TEXT("Zoom in") : TEXT("Zoom out"))
			.Padding(FMargin(0.f))
			.Width(72.f).Height(72.f)
			.OnClicked_Lambda([Session, Delta]()
			{
				Session->MapZoom = FMath::Clamp(Session->MapZoom + Delta, 1.f, 2.5f);
				if (Session->MapZoom <= 1.f)
				{
					Session->MapPan = FVector2f::ZeroVector;
				}
				Session->NotifyChanged();
				return FReply::Handled();
			})
			[
				Icon(IconType, 34.f, C.Ink, 3.f)
			];
	};

	ChildSlot
	.Padding(KioskLayout::PageX, 26.f, KioskLayout::PageX, 30.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 40.f, 0.f)
		[
			Panel(
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SKioskMap)
					.SelectedStation(Session->State.Destination)
					.Zoom(Session->MapZoom)
					.Session(Session)
					.OnStationTapped(FOnKioskStationTapped::CreateLambda([Session](int32 Index) { Session->SelectDestination(Index); }))
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.f, 22.f, 22.f, 0.f)
				[
					Panel(Text(KT("Tap a station · drag to move · +/- to zoom"), KioskType::Caption, true, C.Ink), C.Band, KioskLayout::InnerRadius, C.Transparent, 0.f, FMargin(24.f, 16.f))
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 22.f, 22.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[ ZoomButton(EKioskIcon::Plus, KT("Zoom in"), 0.5f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 0.f, 0.f, 0.f)[ ZoomButton(EKioskIcon::Minus, KT("Zoom out"), -0.5f) ]
				],
				C.Surface, KioskLayout::CardRadius, C.BorderStrong, 2.f, FMargin(2.f))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(680.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ TextWrap(KT("Where are you going?"), KioskType::H1, true, C.Ink) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f)
				[
					SNew(SBox).MinDesiredHeight(78.f)
					[
						SNew(SKioskButton)
						.Kind(EKioskButtonKind::OutlineMuted)
						.Label(KT("Search for a place or station"))
						.ButtonTag(TEXT("Search"))
						.Padding(FMargin(26.f, 0.f))
						.OnClicked_Lambda([Session]() { Session->OpenSearchField(); Session->ShowDestinationList(); return FReply::Handled(); })
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Icon(EKioskIcon::Search, 32.f, C.Ink, 3.f) ]
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)
							[
								TextWrap(KT("Search for a place or station"), KioskType::Body, false, C.Muted)
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f)[ KioskDest::SelectionCard(Session, true) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 20.f, 0.f, 0.f)[ LineHighlighter(Session) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
				[
					SNew(SKioskButton)
					.Kind(EKioskButtonKind::Outline)
					.Label(KT("Open station list view"))
					.ButtonTag(TEXT("Open station list view"))
					.LeftIcon(EKioskIcon::List)
					.OnClicked_Lambda([Session]() { Session->ShowDestinationList(); return FReply::Handled(); })
				]
				+ SVerticalBox::Slot().FillHeight(1.f)[ SNew(SSpacer) ]
				+ SVerticalBox::Slot().AutoHeight()[ KioskDest::ConfirmButton(Session) ]
			]
		]
	];
}

