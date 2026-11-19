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
	/** The one status badge / icon disc: always a circle (radius Size / 2) with the icon centred at 0.55 x Size. Thickness 0 = icon default. */
	TSharedRef<SWidget> Badge(EKioskIcon Which, float Size, const FLinearColor& Fill, const FLinearColor& IconColor = KC().White, float Thickness = 0.f);
	TSharedRef<SWidget> Dot(const FLinearColor& Color, float Diameter);
	TSharedRef<SWidget> Gap(float Width, float Height = 0.f);
	/** Horizontal hairline. */
	TSharedRef<SWidget> Rule(const FLinearColor& Color, float Thickness = 2.f);

	/** White card with the standard radius and padding (the one card look used on every screen). */
	TSharedRef<SBorder> Card(const TSharedRef<SWidget>& Child, const FLinearColor& Fill, const FLinearColor& Border,
		float BorderWidth = 2.f, const FMargin& Padding = FMargin(KioskLayout::CardPad));

	/** Navy full-width bar that looks like a "choose" button, for use inside a tile that is itself the touch target. */
	TSharedRef<SWidget> ActionBar(const FText& Label, bool bArrow = false);

	/** The one set of gaps inside a choice card: icon row to title, text to text, content to footer. */
	constexpr float ChoiceTitleGap = 16.f;
	constexpr float ChoiceTextGap = 10.f;
	constexpr float ChoiceFooterGap = 14.f;

	/** What a choice card (home tile, ticket card, pay method, card option) is made of. Only Icon and Title are required. */
	struct FChoiceCardArgs
	{
		EKioskIcon Icon = EKioskIcon::None;
		float IconSize = KioskLayout::IconLg;
		float IconThickness = 4.f;
		/** true: icon on the left with the title beside it (Badge is not shown). false: icon row (icon left, Badge right), title below. */
		bool bIconBesideTitle = false;
		TSharedPtr<SWidget> Badge;
		FText Title;
		float TitlePx = KioskType::H2;
		FText Description;
		/** Right inset on the description so a two-line wrap breaks mid-sentence. */
		float DescInset = 0.f;
		FLinearColor Ink = KC().Ink;
		FLinearColor DescColor = KC().Ink;
		/** Auto-height content under the description (info lines, rules, warnings, hardware location). */
		TSharedPtr<SWidget> Details;
		/** Fills the height left over, between Details and Footer. Wrap it in an SBox to align it (e.g. bottom centre). Empty: blank space. */
		TSharedPtr<SWidget> Body;
		/** Pinned to the bottom: an ActionBar, a price row. */
		TSharedPtr<SWidget> Footer;
		FLinearColor Fill = KC().Surface;
		FLinearColor Border = KC().BorderStrong;
		float BorderWidth = 2.f;
		FMargin Padding = KioskLayout::CardPadWide;
		/** Set: the whole card is one Card-role button named Tag (Label is its text for the harness; Title when empty). Unset: the card is plain, the Footer has its own control. */
		TFunction<void()> OnPick;
		FText Label;
		FName Tag;
	};
	/** Card surface + header (icon, optional badge, title) + description + details + fill body + footer. */
	TSharedRef<SWidget> ChoiceCard(const FChoiceCardArgs& Args);

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
