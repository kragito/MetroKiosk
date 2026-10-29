#include "KioskWidgets.h"

#include "KioskData.h"
#include "SKioskButton.h"
#include "SKioskDashedBox.h"
#include "Widgets/Layout/SSpacer.h"

namespace KioskUI
{
	TSharedRef<STextBlock> Text(const FText& Value, float Px, bool bBold, const FLinearColor& Color, ETextJustify::Type Justify)
	{
		return SNew(STextBlock)
			.Text(Value)
			.Font(FKioskStyle::Get().Font(Px, bBold))
			.ColorAndOpacity(Color)
			.Justification(Justify);
	}

	TSharedRef<STextBlock> TextWrap(const FText& Value, float Px, bool bBold, const FLinearColor& Color, float LineHeight, ETextJustify::Type Justify)
	{
		return SNew(STextBlock)
			.Text(Value)
			.Font(FKioskStyle::Get().Font(Px, bBold))
			.ColorAndOpacity(Color)
			.AutoWrapText(true)
			.LineHeightPercentage(LineHeight)
			.Justification(Justify);
	}

	TSharedRef<STextBlock> TextDyn(TFunction<FText()> Getter, float Px, bool bBold, const FLinearColor& Color, ETextJustify::Type Justify)
	{
		return SNew(STextBlock)
			.Text_Lambda(MoveTemp(Getter))
			.Font(FKioskStyle::Get().Font(Px, bBold))
			.ColorAndOpacity(Color)
			.Justification(Justify);
	}

	TSharedRef<SBorder> Panel(const TSharedRef<SWidget>& Child, const FLinearColor& Fill, float Radius,
		const FLinearColor& Border, float BorderWidth, const FMargin& Padding)
	{
		return SNew(SBorder)
			.BorderImage(FKioskStyle::Get().Round(Fill, Radius, Border, BorderWidth))
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(Padding)
			[
				Child
			];
	}

	TSharedRef<SWidget> LineBadge(int32 LineIndex, float Height)
	{
		FKioskStyle& Style = FKioskStyle::Get();
		const FLinearColor Fill = Style.C.Line[FMath::Clamp(LineIndex, 0, 5)];
		const FLinearColor TextColor = (LineIndex == 3) ? Style.C.Ink : Style.C.White;
		return SNew(SBox)
			.WidthOverride(Height * 1.4f)
			.HeightOverride(Height)
			[
				SNew(SBorder)
				.BorderImage(Style.Round(Fill, Height * 0.24f))
				.BorderBackgroundColor(FLinearColor::White)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0.f)
				[
					Text(FText::FromString(FKioskData::Line(LineIndex).Code), Height * 0.56f, true, TextColor)
				]
			];
	}

	TSharedRef<SWidget> LineBadges(uint8 LineMask, float Height, float Gap)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		bool bFirst = true;
		for (int32 Line = 0; Line < FKioskData::NumLines; ++Line)
		{
			if (LineMask & (1 << Line))
			{
				Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(bFirst ? 0.f : Gap, 0.f, 0.f, 0.f)
				[
					LineBadge(Line, Height)
				];
				bFirst = false;
			}
		}
		return Row;
	}

	TSharedRef<SWidget> Pill(const FText& Label, const FLinearColor& Fill, const FLinearColor& TextColor, float Px, bool bBold,
		const FLinearColor& Border, const FMargin& Padding)
	{
		return Panel(Text(Label, Px, bBold, TextColor), Fill, Px * 0.62f + Padding.Top, Border, Border.A > 0.f ? 2.f : 0.f, Padding);
	}

	TSharedRef<SWidget> Icon(EKioskIcon Which, float Size, const FLinearColor& Color, float Thickness)
	{
		return SNew(SKioskIcon).Icon(Which).Size(Size).Color(Color).Thickness(Thickness);
	}

	TSharedRef<SWidget> Dot(const FLinearColor& Color, float Diameter)
	{
		return SNew(SBox)
			.WidthOverride(Diameter)
			.HeightOverride(Diameter)
			[
				SNew(SBorder)
				.BorderImage(FKioskStyle::Get().Round(Color, Diameter * 0.5f))
				.BorderBackgroundColor(FLinearColor::White)
			];
	}

	TSharedRef<SWidget> Gap(float Width, float Height)
	{
		return SNew(SSpacer).Size(FVector2D(Width, Height));
	}

	TSharedRef<SWidget> Rule(const FLinearColor& Color, float Thickness)
	{
		return SNew(SBox)
			.HeightOverride(Thickness)
			[
				SNew(SBorder)
				.BorderImage(FKioskStyle::Get().Solid(Color))
				.BorderBackgroundColor(FLinearColor::White)
			];
	}

	TSharedRef<SWidget> SimControl(const FText& Label, FOnClicked OnClicked)
	{
		const FKioskColors& C = KC();
		return SNew(SKioskDashedBox)
			.Fill(C.SimFill)
			.DashColor(C.SimInk)
			.Padding(FMargin(4.f))
			.Radius(10.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Card)
				.Label(Label)
				.ButtonTag(FName(*Label.ToString()))
				.Padding(FMargin(16.f, 10.f))
				.OnClicked(OnClicked)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
					[
						Icon(EKioskIcon::ArrowRight, 18.f, C.SimInk, 2.5f)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						Text(Label, 18.f, true, C.SimInk)
					]
				]
			];
	}

	TSharedRef<SWidget> ProgressBar(TFunction<float()> Fraction, float Width, float Height, const FLinearColor& Track, const FLinearColor& Fill)
	{
		FKioskStyle& Style = FKioskStyle::Get();
		return SNew(SBox)
			.WidthOverride(Width > 0.f ? FOptionalSize(Width) : FOptionalSize())
			.HeightOverride(Height)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder).BorderImage(Style.Round(Track, Height * 0.5f)).BorderBackgroundColor(FLinearColor::White)
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Fraction]() { return FMath::Max(0.001f, FMath::Clamp(Fraction(), 0.f, 1.f)); }))
					[
						SNew(SBorder).BorderImage(Style.Round(Fill, Height * 0.5f)).BorderBackgroundColor(FLinearColor::White)
					]
					+ SHorizontalBox::Slot().FillWidth(TAttribute<float>::CreateLambda([Fraction]() { return FMath::Max(0.001f, 1.f - FMath::Clamp(Fraction(), 0.f, 1.f)); }))
					[
						SNullWidget::NullWidget
					]
				]
			];
	}
}
