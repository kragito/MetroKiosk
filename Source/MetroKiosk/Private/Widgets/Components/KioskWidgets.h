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

	TSharedRef<SWidget> LineBadge(int32 LineIndex, float Height = 26.f);
	TSharedRef<SWidget> LineBadges(uint8 LineMask, float Height = 26.f, float Gap = 6.f);

	TSharedRef<SWidget> Pill(const FText& Label, const FLinearColor& Fill, const FLinearColor& TextColor, float Px, bool bBold = true,
		const FLinearColor& Border = FLinearColor::Transparent, const FMargin& Padding = FMargin(20.f, 8.f));

	TSharedRef<SWidget> Icon(EKioskIcon Which, float Size, const FLinearColor& Color, float Thickness = 0.f);
	TSharedRef<SWidget> Dot(const FLinearColor& Color, float Diameter);
	TSharedRef<SWidget> Gap(float Width, float Height = 0.f);
	/** Horizontal hairline. */
	TSharedRef<SWidget> Rule(const FLinearColor& Color, float Thickness = 2.f);

	/** Clearly-marked on-screen stand-in for a hardware action (card tap, chip insert, card lift). */
	TSharedRef<SWidget> SimControl(const FText& Label, FOnClicked OnClicked);

	/** Places a fixed-size child at an offset inside an SOverlay slot (optionally rotated about its centre). */
	TSharedRef<SWidget> Positioned(const TSharedRef<SWidget>& Child, FVector2D Pos, FVector2D Size, float AngleDeg = 0.f);

	/** Rounded progress bar. Fraction is evaluated every frame. */
	TSharedRef<SWidget> ProgressBar(TFunction<float()> Fraction, float Width, float Height, const FLinearColor& Track, const FLinearColor& Fill);
}
