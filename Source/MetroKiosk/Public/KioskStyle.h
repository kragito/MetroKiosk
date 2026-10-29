// Design tokens, fonts, rounded brushes and button styles. No other file defines a colour.
#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

/** Colour tokens from the Figma "Colour palettes" frame (linear, converted from sRGB hex). */
struct FKioskColors
{
	FKioskColors();

	FLinearColor Bg;            // --k-bg
	FLinearColor Surface;       // --k-surface
	FLinearColor BorderStrong;  // --k-border-strong
	FLinearColor Ink;           // --k-ink / --k-strong
	FLinearColor Muted;         // --k-muted
	FLinearColor Brand;         // --k-brand
	FLinearColor Logo;          // --k-logo
	FLinearColor Primary;       // --k-primary
	FLinearColor PrimaryTint;   // --k-primary-tint
	FLinearColor PrimaryChip;   // lighter green used on the home ticket chips
	FLinearColor Help;          // --k-help
	FLinearColor HelpTint;      // --k-help-tint
	FLinearColor Band;          // step bar band / quiet panels
	FLinearColor Disabled;      // disabled button fill
	FLinearColor DisabledInk;   // text on disabled buttons
	FLinearColor Danger;
	FLinearColor AmberFill;
	FLinearColor AmberStroke;
	FLinearColor AmberStrong;
	FLinearColor Video;         // staff video placeholder
	FLinearColor SimInk;        // simulation-affordance accent
	FLinearColor SimFill;
	FLinearColor Transparent;
	FLinearColor White;
	FLinearColor Black;
	FLinearColor Line[6];       // NS EW NE CC DT TE
	FLinearColor Chip;          // chip contact gold
};

enum class EKioskButtonKind : uint8
{
	Primary,       // green
	Strong,        // navy
	Outline,       // white, navy border
	OutlineMuted,  // white, grey-blue border
	Help,          // blue
	HelpLight,     // light blue with navy border (Back to call)
	DangerOutline, // white, red border
	Tint,          // pale blue fill
	Card,          // transparent hit area for a custom card
	Tile,          // green quick-top-up tile
	PillSelected,  // navy pill
	PillOutline,   // white pill with border
	Key,           // keyboard key
	KeyMuted,      // keyboard function key
};

class METROKIOSK_API FKioskStyle
{
public:
	static FKioskStyle& Get();
	static void Shutdown();

	const FKioskColors C;

	/** Font sized in design pixels. Falls back to a CJK / Tamil capable face per glyph. */
	FSlateFontInfo Font(float DesignPx, bool bBold = false) const;

	/** Cached rounded box brush (stable pointer for the process lifetime). */
	const FSlateBrush* Round(const FLinearColor& Fill, float Radius, const FLinearColor& Border = FLinearColor::Transparent, float BorderWidth = 0.f);
	/** Colour to pass as the MakeBox tint when drawing a brush by hand (SBorder does this for you). */
	static FLinearColor BrushTint(const FSlateBrush* Brush) { return Brush->TintColor.GetSpecifiedColor(); }
	/** Cached solid rectangle brush. */
	const FSlateBrush* Solid(const FLinearColor& Fill);

	const FButtonStyle& ButtonStyle(EKioskButtonKind Kind);
	/** Look for a button that cannot be pressed. The button itself stays enabled so Slate does not dim its text. */
	const FButtonStyle& DisabledButtonStyle(EKioskButtonKind Kind);

private:
	FKioskStyle();

	TSharedRef<const FCompositeFont> CompositeFont;
	TMap<FString, TSharedPtr<FSlateBrush>> BrushCache;
	TMap<uint8, TSharedPtr<FButtonStyle>> ButtonCache;
};

inline const FKioskColors& KC()
{
	return FKioskStyle::Get().C;
}
