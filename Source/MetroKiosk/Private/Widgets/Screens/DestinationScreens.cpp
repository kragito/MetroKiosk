// "Where are you going?": one search box for places and stations, popular destinations, and a summary card
// with the route and fare before the customer confirms.
#include "KioskScreens.h"

#include "DestinationWidgets.h"
#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskSearch.h"
#include "Widgets/Components/KioskAutomation.h"
#include "Widgets/Components/KioskWidgets.h"
#include "Widgets/Components/SKioskButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"

#define LOCTEXT_NAMESPACE "KioskDestination"

using namespace KioskUI;

namespace
{
	constexpr int32 MaxRowsWithoutScroll = 5;

	/** Scrollable result list; named "Station list" so scripted tests can scroll it. */
	TSharedRef<SWidget> ScrollableList(const TSharedRef<SWidget>& Rows)
	{
		TSharedRef<SScrollBox> Box = SNew(SScrollBox).ScrollBarVisibility(EVisibility::Collapsed) + SScrollBox::Slot()[ Rows ];
		TSharedRef<FKioskAutomationMeta> Meta = MakeShared<FKioskAutomationMeta>();
		Meta->Name = TEXT("Station list");
		Meta->Label = TEXT("Station list");
		Box->AddMetadata(Meta);
		return Box;
	}

	/** Popular destinations when nothing is typed; otherwise ranked matches, or "Did you mean" suggestions. */
	TSharedRef<SWidget> ResultsPanel(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		const FString& Query = Session->State.SearchText;
		const FKioskSearchResult Result = FKioskSearch::Run(Query);

		FText Heading;
		TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
		int32 Count = 0;
		auto AddRow = [&Rows, &Count](const TSharedRef<SWidget>& Tile)
		{
			Rows->AddSlot().AutoHeight().Padding(0.f, Count == 0 ? 0.f : 8.f, 0.f, 0.f)[ Tile ];
			++Count;
		};

		if (FKioskSearch::Normalize(Query).IsEmpty())
		{
			Heading = KT("Popular from City Hall");
			for (int32 Station : FKioskData::PopularStations())
			{
				AddRow(KioskDest::StationTile(Session, Station));
			}
		}
		else
		{
			if (!Result.bSuggestions)
			{
				Heading = FText::Format(FKioskLoc::Tr(LOCTEXT("ResultsFor", "Results for “{0}”")), FText::FromString(Query));
			}
			else if (Result.bClose)
			{
				Heading = FText::Format(FKioskLoc::Tr(LOCTEXT("DidYouMean", "No exact match for “{0}”. Did you mean…")), FText::FromString(Query));
			}
			else
			{
				Heading = FText::Format(FKioskLoc::Tr(LOCTEXT("TryThese", "No match for “{0}”. Try one of these popular places")), FText::FromString(Query));
			}
			for (const FKioskSearchHit& Hit : Result.Hits)
			{
				AddRow(Hit.bPlace ? KioskDest::PlaceTile(Session, Hit.Index) : KioskDest::StationTile(Session, Hit.Index));
			}
		}

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 0.f, 12.f)[ TextWrap(Heading, 20.f, true, C.Ink) ]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				// Short lists need no scrolling (and stay visible to UI automation); long ones scroll.
				Count > MaxRowsWithoutScroll ? ScrollableList(Rows) : StaticCastSharedRef<SWidget>(Rows)
			];
	}
}

void SDestinationScreen::Construct(const FArguments& InArgs, const TSharedRef<FKioskSession>& Session)
{
	const FKioskColors& C = KC();
	const bool bKeyboard = Session->State.bKeyboardOpen;

	TSharedRef<SVerticalBox> Right = SNew(SVerticalBox);
	Right->AddSlot().AutoHeight()[ KioskDest::SelectionCard(Session, bKeyboard) ];
	if (bKeyboard)
	{
		Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::KeyboardWidget(Session) ];
	}
	else
	{
		Right->AddSlot().AutoHeight().Padding(0.f, 22.f, 0.f, 0.f).HAlign(HAlign_Left)
		[
			SNew(SKioskButton)
			.Kind(EKioskButtonKind::OutlineMuted)
			.Label(KT("See it on the map"))
			.ButtonTag(TEXT("See it on the map"))
			.LeftIcon(EKioskIcon::Map)
			.FontPx(21.f).Bold(false)
			.Width(340.f).Height(72.f)
			.OnClicked_Lambda([Session]() { Session->Replace(EKioskScreen::DestinationMap); return FReply::Handled(); })
		];
	}
	Right->AddSlot().FillHeight(1.f)[ SNew(SSpacer) ];
	Right->AddSlot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)[ KioskDest::ConfirmButton(Session) ];

	ChildSlot
	.Padding(75.f, 24.f, 75.f, 22.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 48.f, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)[ Text(KT("Where are you going?"), 42.f, true, C.Ink) ]
			+ SVerticalBox::Slot().AutoHeight()[ KioskDest::SearchField(Session) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f, 0.f, 16.f)
			[
				Text(KT("Spelling does not need to be perfect. Places like VivoCity or the Zoo work too."), 17.f, false, C.Muted)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)[ ResultsPanel(Session) ]
		]
		+ SHorizontalBox::Slot().FillWidth(0.5f)
		[
			Right
		]
	];
}

#undef LOCTEXT_NAMESPACE
