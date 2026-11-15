#include "SKioskButton.h"

#include "KioskAutomation.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

float KioskButtonRoleFont(EKioskButtonKind Kind)
{
	switch (Kind)
	{
	case EKioskButtonKind::Primary: return KioskType::BtnPrimary;
	case EKioskButtonKind::PillSelected:
	case EKioskButtonKind::PillOutline: return KioskType::BtnPill;
	case EKioskButtonKind::Key: return KioskType::BtnKey;
	case EKioskButtonKind::KeyMuted:
	case EKioskButtonKind::Compact:
	case EKioskButtonKind::Link:
	case EKioskButtonKind::Tint: return KioskType::Body;
	default: return KioskType::BtnStd;
	}
}

float KioskButtonRoleHeight(EKioskButtonKind Kind)
{
	switch (Kind)
	{
	case EKioskButtonKind::Primary: return KioskLayout::BtnPrimaryH;
	case EKioskButtonKind::Strong:
	case EKioskButtonKind::Outline:
	case EKioskButtonKind::OutlineMuted:
	case EKioskButtonKind::Help:
	case EKioskButtonKind::HelpLight:
	case EKioskButtonKind::DangerOutline: return KioskLayout::BtnStdH;
	case EKioskButtonKind::Compact:
	case EKioskButtonKind::Link: return KioskLayout::BtnCompactH;
	case EKioskButtonKind::PillSelected:
	case EKioskButtonKind::PillOutline: return KioskLayout::PillH;
	case EKioskButtonKind::Key:
	case EKioskButtonKind::KeyMuted: return KioskLayout::KeyH;
	default: return 0.f; // Card, Tile, Tint: sized by their container
	}
}

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

	/** Cards and links keep their look when disabled; every other kind goes flat grey (see FKioskStyle). */
	bool GreysWhenDisabled(EKioskButtonKind Kind)
	{
		return Kind != EKioskButtonKind::Card && Kind != EKioskButtonKind::Link;
	}
}

void SKioskButton::Construct(const FArguments& InArgs)
{
	FKioskStyle& Style = FKioskStyle::Get();
	const TAttribute<bool> Enabled = InArgs._Enabled;
	const bool bPressable = Enabled.Get(true);
	const bool bCustomContent = InArgs._Content.Widget != SNullWidget::NullWidget;
	const EKioskButtonKind Kind = InArgs._Kind;
	const FLinearColor Normal = InArgs._TextColor.IsSet() ? InArgs._TextColor.GetValue() : DefaultTextColor(Kind);
	const FLinearColor DisabledColor = Style.C.DisabledInk;
	const bool bGrey = GreysWhenDisabled(Kind);
	const float FontPx = InArgs._FontPx > 0.f ? InArgs._FontPx : KioskButtonRoleFont(Kind);

	TSharedRef<SWidget> Inner = SNullWidget::NullWidget;
	if (bCustomContent)
	{
		Inner = InArgs._Content.Widget;
	}
	else
	{
		const float IconSize = InArgs._IconPx > 0.f ? InArgs._IconPx : FontPx * 1.05f;
		auto IconColor = [Enabled, Normal, DisabledColor, bGrey]()
		{
			return (Enabled.Get(true) || !bGrey) ? Normal : DisabledColor;
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
			// Labels wrap to a second line instead of clipping, and centre in whatever width the button is given.
			const FSlateFontInfo LabelFont = Style.Font(FontPx, InArgs._Bold);
			// The label never asks for less than its own width (+ slack), so a wrap that happened while the button was squeezed
			// cannot stick once there is room again; when the button is capped (MaxWidth) the fill slot shrinks and the text wraps.
			float Measured = 0.f;
			if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
			{
				Measured = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(InArgs._Label, LabelFont).X;
			}
			TSharedRef<STextBlock> LabelText = SNew(STextBlock)
				.Text(InArgs._Label)
				.Font(LabelFont)
				.ColorAndOpacity_Lambda([=]() { return FSlateColor((Enabled.Get(true) || !bGrey) ? Normal : DisabledColor); })
				.Justification(ETextJustify::Center)
				.AutoWrapText(true);
			TSharedRef<SWidget> Label = SNew(SBox).MinDesiredWidth(Measured > 0.f ? FOptionalSize(Measured + 6.f) : FOptionalSize())[ LabelText ];
			if (Kind == EKioskButtonKind::Link)
			{
				Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Label ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(SBox).HeightOverride(2.f)[ SNew(SBorder).BorderImage(Style.Solid(Normal)).BorderBackgroundColor(FLinearColor::White) ]
					]
				];
			}
			else
			{
				// A fill slot (not auto) is what lets a long label shrink to the button's width and wrap instead of being clipped.
				Row->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)[ Label ];
			}
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
		.ButtonStyle(bPressable ? &Style.ButtonStyle(Kind) : &Style.DisabledButtonStyle(Kind))
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
		TSharedRef<FKioskAutomationMeta> Meta = MakeShared<FKioskAutomationMeta>();
		Meta->Name = Description.ToString();
		Meta->Label = Label.ToString();
		Meta->Enabled = Enabled;
		Button->AddMetadata(Meta);
#if WITH_ACCESSIBILITY
		Button->SetAccessibleBehavior(EAccessibleBehavior::Custom, Description);
#endif
	}
	else if (!InArgs._ButtonTag.IsNone())
	{
		Button->SetTag(InArgs._ButtonTag);
	}

	const float RoleHeight = InArgs._MinHeight < 0.f ? KioskButtonRoleHeight(Kind) : InArgs._MinHeight;
	const float MinWidth = InArgs._MinWidth > 0.f
		? InArgs._MinWidth
		: ((Kind == EKioskButtonKind::PillSelected || Kind == EKioskButtonKind::PillOutline) ? KioskLayout::PillW : 0.f);

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(InArgs._Width > 0.f ? FOptionalSize(InArgs._Width) : FOptionalSize())
		.HeightOverride(InArgs._Height > 0.f ? FOptionalSize(InArgs._Height) : FOptionalSize())
		.MinDesiredWidth(MinWidth > 0.f ? FOptionalSize(MinWidth) : FOptionalSize())
		.MinDesiredHeight(InArgs._Height > 0.f || RoleHeight <= 0.f ? FOptionalSize() : FOptionalSize(RoleHeight))
		.MaxDesiredWidth(InArgs._MaxWidth > 0.f ? FOptionalSize(InArgs._MaxWidth) : FOptionalSize())
		[
			Button
		]
	];
}
