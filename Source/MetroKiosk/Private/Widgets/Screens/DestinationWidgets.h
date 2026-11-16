// Pieces shared by the destination list screen and the map screen: result tiles, the selected-destination card,
// the search field with its on-screen keyboard, and the Confirm button.
#pragma once

#include "CoreMinimal.h"
#include "KioskSession.h"
#include "Widgets/SWidget.h"

class SKioskButton;
enum class EKioskIcon : uint8;

namespace KioskDest
{
	struct FTileArgs
	{
		FText Title;               // English place or station name
		FString LocalTitle;        // name in the active language, shown beneath the English one
		FText Subtitle;            // "near Bayfront", "VivoCity · Sentosa"
		EKioskIcon Icon = static_cast<EKioskIcon>(0); // EKioskIcon::None
		uint8 LineMask = 0;
		FText Detail;              // "13 min · 1 change · $1.50"
		bool bSelected = false;
		FString Tag;               // automation name, e.g. "Place Gardens by the Bay"
		TFunction<void()> OnPick;
	};

	/** One tappable destination tile (used for popular destinations and search results alike). */
	TSharedRef<SWidget> Tile(const FTileArgs& Args);

	/** Tile for a place ("Gardens by the Bay: near Bayfront"). */
	TSharedRef<SWidget> PlaceTile(const TSharedRef<FKioskSession>& Session, int32 PlaceIndex);
	/** Tile for a station; the subtitle says what is there when that is known. */
	TSharedRef<SWidget> StationTile(const TSharedRef<FKioskSession>& Session, int32 StationIndex);

	/** Route, time and fare for the chosen destination, or a prompt when nothing is chosen yet. */
	TSharedRef<SWidget> SelectionCard(const TSharedRef<FKioskSession>& Session, bool bCompact);

	/** Slim card at the top of the right column: "Select a location", or "City Hall -> X · about N min · N change". */
	TSharedRef<SWidget> TripCard(const TSharedRef<FKioskSession>& Session);
	/** "Filter by line": six line buttons (NS, EW, NE, CC, DT, TE); tapping the active one clears the filter. */
	TSharedRef<SWidget> LineFilterGrid(const TSharedRef<FKioskSession>& Session);
	/** Short line name in the active language ("North–South"). */
	FText LineShortName(int32 LineIndex);

	TSharedRef<SWidget> ConfirmButton(const TSharedRef<FKioskSession>& Session);
	/** Placeholder defaults to "Search all stations"; the map screen passes "Find station on the map". */
	TSharedRef<SWidget> SearchField(const TSharedRef<FKioskSession>& Session, const FText& PlaceholderText = FText::GetEmpty());
	TSharedRef<SWidget> KeyboardWidget(const TSharedRef<FKioskSession>& Session);

	/** "13 min · 1 change · $1.50" for a station under the current ticket type. */
	FText TripLine(const TSharedRef<FKioskSession>& Session, int32 StationIndex);
}
