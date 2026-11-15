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
	FLinearColor OnBrandDivider; // hairline on the teal header
	FLinearColor OnBrandSoft;   // secondary text on the teal header
	FLinearColor MapLabelBg;    // translucent pill behind map labels
	FLinearColor DeviceDark;    // card-slot slit in illustrations
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
	Compact,       // small outlined control (Change)
	Link,          // underlined text link
};

/** One type scale for the whole kiosk (design pixels). Nothing readable is below Caption; map labels are the only exception. */
namespace KioskType
{
	constexpr float Display = 52.f;  // hero statements and the big balance
	constexpr float H1 = 42.f;       // screen titles
	constexpr float H2 = 32.f;       // card titles
	constexpr float H3 = 26.f;       // sub-titles, tile names
	constexpr float Body = 22.f;     // running text
	constexpr float Caption = 18.f;  // smallest readable text
	constexpr float Figure = 48.f;   // money
	constexpr float MapLabel = 16.f; // map labels only
	constexpr float BtnPrimary = 30.f;
	constexpr float BtnStd = 26.f;
	constexpr float BtnPill = 22.f;
	constexpr float BtnKey = 28.f;
}

/** Shared metrics: grid, radii, button heights per role, icon sizes. */
namespace KioskLayout
{
	constexpr float PageX = 75.f;        // left/right screen margin
	constexpr float PageTop = 28.f;
	constexpr float PageBottom = 28.f;
	constexpr float Gutter = 36.f;       // between columns
	constexpr float TileGap = 16.f;      // between stacked cards or tiles
	constexpr float CardRadius = 26.f;
	constexpr float InnerRadius = 16.f;
	constexpr float CardPad = 34.f;
	constexpr float BtnPrimaryH = 104.f; // green: proceed / pay
	constexpr float BtnStdH = 88.f;      // navy choose, outlined secondary, help
	constexpr float BtnCompactH = 60.f;  // Change, small controls
	constexpr float PillH = 72.f;        // language pills
	constexpr float PillW = 172.f;
	constexpr float KeyH = 76.f;
	constexpr float IconSm = 34.f;
	constexpr float IconMd = 62.f;
	constexpr float IconLg = 96.f;
	constexpr float FooterH = 128.f;
}

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
