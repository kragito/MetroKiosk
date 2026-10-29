#include "SKioskButton.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FLinearColor DefaultTextColor(EKioskButtonKind Kind)
	{
		const FKioskColors& C = KC();
		switch (Kind)
		{
		case EKioskButtonKind::Primary:
		case EKioskButtonKind::Strong:
		case EKioskButtonKind::Help:
		case EKioskButtonKind::Tile:
		case EKioskButtonKind::PillSelected:
			return C.White;
		case EKioskButtonKind::DangerOutline:
			return C.Danger;
		default:
			return C.Ink;
		}
	}
}

void SKioskButton::Construct(const FArguments& InArgs)
{
	FKioskStyle& Style = FKioskStyle::Get();
	const TAttribute<bool> Enabled = InArgs._Enabled;
	const bool bPressable = Enabled.Get(true);
	const bool bCustomContent = InArgs._Content.Widget != SNullWidget::NullWidget;
	const FLinearColor Normal = InArgs._TextColor.IsSet() ? InArgs._TextColor.GetValue() : DefaultTextColor(InArgs._Kind);
	const FLinearColor DisabledColor = Style.C.DisabledInk;
	const bool bGreyDisabled = InArgs._Kind == EKioskButtonKind::Primary || InArgs._Kind == EKioskButtonKind::Strong
		|| InArgs._Kind == EKioskButtonKind::Help || InArgs._Kind == EKioskButtonKind::Tile;

	TSharedRef<SWidget> Inner = SNullWidget::NullWidget;
	if (InArgs._Content.Widget != SNullWidget::NullWidget)
	{
		Inner = InArgs._Content.Widget;
	}
	else
	{
		const float IconSize = InArgs._IconPx > 0.f ? InArgs._IconPx : InArgs._FontPx * 1.05f;
		auto IconColor = [Enabled, Normal, DisabledColor, bGreyDisabled]()
		{
			return (Enabled.Get(true) || !bGreyDisabled) ? Normal : DisabledColor;
		};

		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		if (InArgs._LeftIcon != EKioskIcon::None)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, InArgs._Spacing, 0.f)
			[
				SNew(SKioskIcon).Icon(InArgs._LeftIcon).Size(IconSize).Color(IconColor())
			];
		}
		if (!InArgs._Label.IsEmpty())
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(InArgs._Label)
				.Font(Style.Font(InArgs._FontPx, InArgs._Bold))
				.ColorAndOpacity_Lambda([=]() { return FSlateColor((Enabled.Get(true) || !bGreyDisabled) ? Normal : DisabledColor); })
				.Justification(ETextJustify::Center)
			];
		}
		if (InArgs._RightIcon != EKioskIcon::None)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(InArgs._Spacing, 0.f, 0.f, 0.f)
			[
				SNew(SKioskIcon).Icon(InArgs._RightIcon).Size(IconSize).Color(IconColor())
			];
		}
		Inner = Row;
	}

	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(bPressable ? &Style.ButtonStyle(InArgs._Kind) : &Style.DisabledButtonStyle(InArgs._Kind))
		.ContentPadding(InArgs._Padding)
		.HAlign(bCustomContent ? HAlign_Fill : HAlign_Center)
		.VAlign(bCustomContent ? VAlign_Fill : VAlign_Center)
		// Visuals are chosen at construction (screens rebuild on state change); the bound attribute is re-read on every press.
		.OnClicked_Lambda([Enabled, Clicked = InArgs._OnClicked]()
		{
			return (Enabled.Get(true) && Clicked.IsBound()) ? Clicked.Execute() : FReply::Handled();
		})
		[
			Inner
		];

	const FText Label = InArgs._Label;
	if (!Label.IsEmpty())
	{
		const FText Description = InArgs._ButtonTag.IsNone() ? Label : FText::FromName(InArgs._ButtonTag);
		Button->SetTag(InArgs._ButtonTag.IsNone() ? FName(*Label.ToString()) : InArgs._ButtonTag);
#if WITH_ACCESSIBILITY
		Button->SetAccessibleBehavior(EAccessibleBehavior::Custom, Description);
#endif
	}
	else if (!InArgs._ButtonTag.IsNone())
	{
		Button->SetTag(InArgs._ButtonTag);
	}

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(InArgs._Width > 0.f ? FOptionalSize(InArgs._Width) : FOptionalSize())
		.HeightOverride(InArgs._Height > 0.f ? FOptionalSize(InArgs._Height) : FOptionalSize())
		[
			Button
		]
	];
}
