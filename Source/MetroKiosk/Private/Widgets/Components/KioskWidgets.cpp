#include "KioskWidgets.h"

#include "KioskData.h"
#include "KioskLoc.h"
#include "KioskSession.h"
#include "SKioskButton.h"
#include "SKioskDashedBox.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
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

	TSharedRef<SWidget> Badge(EKioskIcon Which, float Size, const FLinearColor& Fill, const FLinearColor& IconColor, float Thickness)
	{
		return SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			[
				SNew(SBorder)
				.BorderImage(FKioskStyle::Get().Round(Fill, Size * 0.5f))
				.BorderBackgroundColor(FLinearColor::White)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0.f)
				[
					Icon(Which, Size * 0.55f, IconColor, Thickness)
				]
			];
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

	TSharedRef<SBorder> Card(const TSharedRef<SWidget>& Child, const FLinearColor& Fill, const FLinearColor& Border, float BorderWidth, const FMargin& Padding)
	{
		return Panel(Child, Fill, KioskLayout::CardRadius, Border, BorderWidth, Padding);
	}

	TSharedRef<SWidget> ActionBar(const FText& Label, bool bArrow)
	{
		const FKioskColors& C = KC();
		const FSlateFontInfo Font = FKioskStyle::Get().Font(KioskType::BtnStd, true);
		float Measured = 0.f;
		if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			Measured = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font).X;
		}
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).MinDesiredWidth(Measured > 0.f ? FOptionalSize(Measured + 6.f) : FOptionalSize())
				[
					SNew(STextBlock).Text(Label).Font(Font).ColorAndOpacity(C.White).Justification(ETextJustify::Center).AutoWrapText(true)
				]
			];
		if (bArrow)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)[ Icon(EKioskIcon::ArrowRight, KioskType::BtnStd, C.White) ];
		}
		return SNew(SBox).MinDesiredHeight(KioskLayout::BtnStdH)
		[
			Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Row ], C.Ink, 14.f, C.Transparent, 0.f, FMargin(20.f, 10.f))
		];
	}

	TSharedRef<SWidget> ChangeButton(FOnClicked OnClicked, const FName& Tag)
	{
		return SNew(SKioskButton)
			.Kind(EKioskButtonKind::Compact)
			.Label(KT("Change"))
			.ButtonTag(Tag)
			.Padding(FMargin(26.f, 8.f))
			.OnClicked(OnClicked);
	}

	FText HardwareName(EHardware Which)
	{
		switch (Which)
		{
		case EHardware::MetroReader: return KT("Metro card reader · below the screen, right of centre");
		case EHardware::Contactless: return KT("Contactless reader · right column, top");
		case EHardware::ChipSlot: return KT("Chip and NETS slot, PIN pad · right column, middle");
		case EHardware::CashSlots: return KT("Note and coin slots · right column, bottom");
		default: return KT("Collection tray · below the screen, left");
		}
	}

	TSharedRef<SWidget> HardwareDiagram(uint32 HighlightMask, float Scale)
	{
		FKioskStyle& Style = FKioskStyle::Get();
		const FKioskColors& C = Style.C;
		constexpr float W = 400.f;
		constexpr float H = 236.f;

		auto Part = [&](EHardware Which, FVector2D Pos, FVector2D Size, EKioskIcon IconType, float IconPx, float Radius)
		{
			const bool bLit = (HighlightMask & Mask(Which)) != 0;
			TSharedRef<SWidget> Inner = IconType == EKioskIcon::None
				? StaticCastSharedRef<SWidget>(SNew(SSpacer))
				: StaticCastSharedRef<SWidget>(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ Icon(IconType, IconPx, bLit ? C.White : C.Muted, 3.f) ]);
			return Positioned(
				Panel(Inner, bLit ? C.Primary : C.Band, Radius, bLit ? C.Ink : C.BorderStrong, bLit ? 3.f : 2.f),
				Pos, Size);
		};

		TSharedRef<SOverlay> Body = SNew(SOverlay)
			+ SOverlay::Slot()[ Positioned(Panel(SNew(SSpacer), C.Surface, 22.f, C.Ink, 3.f), FVector2D(0.f, 0.f), FVector2D(W, H)) ]
			+ SOverlay::Slot()[ Positioned(Panel(SNew(SSpacer), C.Video, 12.f), FVector2D(24.f, 20.f), FVector2D(262.f, 150.f)) ]
			+ SOverlay::Slot()[ Part(EHardware::Contactless, FVector2D(304.f, 20.f), FVector2D(72.f, 46.f), EKioskIcon::Contactless, 30.f, 10.f) ]
			+ SOverlay::Slot()[ Part(EHardware::ChipSlot, FVector2D(304.f, 74.f), FVector2D(72.f, 46.f), EKioskIcon::ChipCard, 30.f, 10.f) ]
			+ SOverlay::Slot()[ Part(EHardware::CashSlots, FVector2D(304.f, 128.f), FVector2D(72.f, 42.f), EKioskIcon::Cash, 28.f, 10.f) ]
			+ SOverlay::Slot()[ Part(EHardware::MetroReader, FVector2D(176.f, 184.f), FVector2D(110.f, 34.f), EKioskIcon::Contactless, 24.f, 17.f) ]
			+ SOverlay::Slot()[ Part(EHardware::Tray, FVector2D(24.f, 184.f), FVector2D(124.f, 34.f), EKioskIcon::Tray, 24.f, 10.f) ];

		return SNew(SBox).WidthOverride(W * Scale).HeightOverride(H * Scale)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[
				SNew(SBox).WidthOverride(W).HeightOverride(H)[ Body ]
			]
		];
	}

	TSharedRef<SWidget> HardwareHint(uint32 HighlightMask, float Scale, float CaptionPx)
	{
		const FKioskColors& C = KC();
		TSharedRef<SVerticalBox> Captions = SNew(SVerticalBox);
		bool bFirst = true;
		for (EHardware Which : { EHardware::MetroReader, EHardware::Contactless, EHardware::ChipSlot, EHardware::CashSlots, EHardware::Tray })
		{
			if (HighlightMask & Mask(Which))
			{
				Captions->AddSlot().AutoHeight().Padding(0.f, bFirst ? 0.f : 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 7.f, 12.f, 0.f)[ Dot(C.Primary, 14.f) ]
					+ SHorizontalBox::Slot().FillWidth(1.f)[ TextWrap(HardwareName(Which), CaptionPx, true, C.Ink, 1.1f) ]
				];
				bFirst = false;
			}
		}
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ HardwareDiagram(HighlightMask, Scale) ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20.f, 0.f, 0.f, 0.f)[ Captions ];
	}

	TSharedRef<SWidget> SimControl(const FText& Label, FOnClicked OnClicked, const FName& Tag)
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
				.ButtonTag(Tag.IsNone() ? FName(*Label.ToString()) : Tag)
				.Padding(FMargin(16.f, 10.f))
				.OnClicked(OnClicked)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
					[
						Icon(EKioskIcon::ArrowRight, KioskType::Caption, C.SimInk, 2.5f)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						Text(Label, KioskType::Caption, true, C.SimInk)
					]
				]
			];
	}

	TSharedRef<SWidget> Positioned(const TSharedRef<SWidget>& Child, FVector2D Pos, FVector2D Size, float AngleDeg)
	{
		TSharedRef<SBox> Box = SNew(SBox).WidthOverride(Size.X).HeightOverride(Size.Y)[ Child ];
		if (!FMath::IsNearlyZero(AngleDeg))
		{
			Box->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Box->SetRenderTransform(FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(AngleDeg))));
		}
		return SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0.f, 0.f)).Offset(FMargin(Pos.X, Pos.Y, Size.X, Size.Y)).Alignment(FVector2D(0.f, 0.f))[ Box ];
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

	TSharedRef<SWidget> ReceiptActions(const TSharedRef<FKioskSession>& Session, bool bSideBySide)
	{
		const FKioskColors& C = KC();
		const TSharedRef<bool> bPrinted = MakeShared<bool>(false);
		const TSharedRef<bool> bEmailed = MakeShared<bool>(false);

		TSharedRef<SWidget> PrintButton = SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Print receipt"))
			.ButtonTag(TEXT("Print receipt"))
			.LeftIcon(EKioskIcon::Printer)
			.OnClicked_Lambda([Session, bPrinted]() { *bPrinted = true; Session->NotifyChanged(); return FReply::Handled(); });
		TSharedRef<SWidget> EmailButton = SNew(SKioskButton)
			.Kind(EKioskButtonKind::Outline)
			.Label(KT("Email receipt"))
			.ButtonTag(TEXT("Email receipt"))
			.LeftIcon(EKioskIcon::Mail)
			.OnClicked_Lambda([Session, bEmailed]() { *bEmailed = true; Session->NotifyChanged(); return FReply::Handled(); });

		TSharedRef<SWidget> Buttons = SNullWidget::NullWidget;
		if (bSideBySide)
		{
			Buttons = SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 8.f, 0.f)[ PrintButton ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8.f, 0.f, 0.f, 0.f)[ EmailButton ];
		}
		else
		{
			Buttons = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)[ PrintButton ]
				+ SVerticalBox::Slot().AutoHeight()[ EmailButton ];
		}

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ Buttons ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 16.f, 0.f, 0.f)
			[
				TextDyn([bPrinted, bEmailed]()
				{
					return *bPrinted ? KT("Your receipt is printing in the tray.")
						: (*bEmailed ? KT("Enter your email on the next screen.") : FText::GetEmpty());
				}, KioskType::Body, false, C.Muted)
			];
	}

	TSharedRef<SWidget> ReturnCountdown(const TSharedRef<FKioskSession>& Session)
	{
		const FKioskColors& C = KC();
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					ProgressBar([Session]() { return Session->ReturnFractionLeft(); }, 0.f, 14.f, C.Disabled, C.Ink)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					TextDyn([Session]() { return FText::Format(KT("Returning to start in {0} s"), FText::AsNumber(Session->ReturnSecondsLeft())); }, KioskType::Caption, false, C.Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(24.f, 0.f, 0.f, 0.f)
			[
				SNew(SKioskButton)
				.Kind(EKioskButtonKind::Outline)
				.Label(KT("I need more time"))
				.ButtonTag(TEXT("I need more time"))
				.MaxWidth(420.f)
				.OnClicked_Lambda([Session]() { Session->ExtendReturn(); return FReply::Handled(); })
			];
	}
}
