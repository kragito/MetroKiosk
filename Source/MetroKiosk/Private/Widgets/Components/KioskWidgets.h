// Small reusable building blocks (text, panels, badges, pills, simulation controls).
#pragma once

#include "CoreMinimal.h"
#include "KioskStyle.h"
#include "SKioskIcon.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

class FKioskSession;

namespace KioskUI
{
	TSharedRef<STextBlock> Text(const FText& Value, float Px, bool bBold = false, const FLinearColor& Color = KC().Ink,
		ETextJustify::Type Justify = ETextJustify::Left);
	/** Wrapping text that fills the width it is given. */
	TSharedRef<STextBlock> TextWrap(const FText& Value, float Px, bool bBold = false, const FLinearColor& Color = KC().Ink,
		float LineHeight = 1.f, ETextJustify::Type Justify = ETextJustify::Left);
	TSharedRef<STextBlock> TextDyn(TFunction<FText()> Getter, float Px, bool bBold = false, const FLinearColor& Color = KC().Ink,
		ETextJustify::Type Justify = ETextJustify::Left);

	TSharedRef<SBorder> Panel(const TSharedRef<SWidget>& Child, const FLinearColor& Fill, float Radius,
		const FLinearColor& Border = FLinearColor::Transparent, float BorderWidth = 0.f, const FMargin& Padding = FMargin(0.f));

	/** Line badges are one size everywhere; their code is 18 px so it stays readable. */
	constexpr float BadgeHeight = 32.f;
	TSharedRef<SWidget> LineBadge(int32 LineIndex, float Height = BadgeHeight);
	TSharedRef<SWidget> LineBadges(uint8 LineMask, float Height = BadgeHeight, float Gap = 6.f);

	TSharedRef<SWidget> Pill(const FText& Label, const FLinearColor& Fill, const FLinearColor& TextColor, float Px, bool bBold = true,
		const FLinearColor& Border = FLinearColor::Transparent, const FMargin& Padding = FMargin(20.f, 8.f));

	TSharedRef<SWidget> Icon(EKioskIcon Which, float Size, const FLinearColor& Color, float Thickness = 0.f);
	TSharedRef<SWidget> Dot(const FLinearColor& Color, float Diameter);
	TSharedRef<SWidget> Gap(float Width, float Height = 0.f);
	/** Horizontal hairline. */
	TSharedRef<SWidget> Rule(const FLinearColor& Color, float Thickness = 2.f);

	/** White card with the standard radius and padding (the one card look used on every screen). */
	TSharedRef<SBorder> Card(const TSharedRef<SWidget>& Child, const FLinearColor& Fill, const FLinearColor& Border,
		float BorderWidth = 2.f, const FMargin& Padding = FMargin(KioskLayout::CardPad));

	/** Navy full-width bar that looks like a "choose" button, for use inside a tile that is itself the touch target. */
	TSharedRef<SWidget> ActionBar(const FText& Label, bool bArrow = false);

	/** The single "Change" control used on Review and Destination. */
	TSharedRef<SWidget> ChangeButton(FOnClicked OnClicked, const FName& Tag = TEXT("Change"));

	/** Parts of the kiosk front that the UI points at. Physical layout (single source of truth):
	 *  collection tray below the screen (left), metro card reader below the screen (right of centre, the bowl the card rests on),
	 *  right column top to bottom: contactless reader, chip/NETS slot + PIN pad, note and coin slots. */
	enum class EHardware : uint32
	{
		MetroReader = 1 << 0,
		Contactless = 1 << 1,
		ChipSlot = 1 << 2,
		CashSlots = 1 << 3,
		Tray = 1 << 4,
	};
	inline uint32 Mask(EHardware A) { return static_cast<uint32>(A); }
	inline uint32 Mask(EHardware A, EHardware B) { return static_cast<uint32>(A) | static_cast<uint32>(B); }
	/** One-line location for a device, e.g. "Collection tray · below the screen, left". */
	FText HardwareName(EHardware Which);
	/** Small kiosk-front diagram with the given parts lit green. */
	TSharedRef<SWidget> HardwareDiagram(uint32 HighlightMask, float Scale = 1.f);
	/** Diagram plus a caption naming each lit part. */
	TSharedRef<SWidget> HardwareHint(uint32 HighlightMask, float Scale = 1.f, float CaptionPx = KioskType::Caption);

	/** Clearly-marked on-screen stand-in for a hardware action (card tap, chip insert, card lift). */
	/** Tag is the English name used by the scripted harness, so journeys work in any language. */
	TSharedRef<SWidget> SimControl(const FText& Label, FOnClicked OnClicked, const FName& Tag = NAME_None);

	/** Places a fixed-size child at an offset inside an SOverlay slot (optionally rotated about its centre). */
	TSharedRef<SWidget> Positioned(const TSharedRef<SWidget>& Child, FVector2D Pos, FVector2D Size, float AngleDeg = 0.f);

	/** Rounded progress bar. Fraction is evaluated every frame. */
	TSharedRef<SWidget> ProgressBar(TFunction<float()> Fraction, float Width, float Height, const FLinearColor& Track, const FLinearColor& Fill);

	/** Print / Email receipt buttons plus the status line under them. Shared by the ticket receipt and the card-flow done screen.
	 *  bSideBySide puts the two buttons in one row (for screens short on height); otherwise they stack. */
	TSharedRef<SWidget> ReceiptActions(const TSharedRef<FKioskSession>& Session, bool bSideBySide = false);

	/** "Returning to start in N s" progress bar and countdown text, with the "I need more time" button beside it. */
	TSharedRef<SWidget> ReturnCountdown(const TSharedRef<FKioskSession>& Session);
}
