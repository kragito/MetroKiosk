#include "KioskStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

namespace
{
	FLinearColor Hex(const TCHAR* Value)
	{
		return FLinearColor::FromSRGBColor(FColor::FromHex(Value));
	}

	TUniquePtr<FKioskStyle>& InstanceHolder()
	{
		static TUniquePtr<FKioskStyle> Instance;
		return Instance;
	}

	const FName RegularName(TEXT("Regular"));
	const FName BoldName(TEXT("Bold"));

	/** Roboto for Latin, DroidSansFallback for CJK, Nirmala UI (system) for Tamil. */
	TSharedRef<const FCompositeFont> BuildCompositeFont()
	{
		const FString FontDir = FPaths::EngineContentDir() / TEXT("Slate/Fonts");
		TSharedRef<FCompositeFont> Font = MakeShared<FCompositeFont>();

		Font->DefaultTypeface.AppendFont(RegularName, FontDir / TEXT("Roboto-Regular.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		Font->DefaultTypeface.AppendFont(BoldName, FontDir / TEXT("Roboto-Bold.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);

		{
			FCompositeSubFont Cjk;
			const FString Droid = FontDir / TEXT("DroidSansFallback.ttf");
			Cjk.Typeface.AppendFont(RegularName, Droid, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			Cjk.Typeface.AppendFont(BoldName, Droid, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			Cjk.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x2E80), FInt32RangeBound::Inclusive(0x9FFF)));
			Cjk.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0xFF00), FInt32RangeBound::Inclusive(0xFFEF)));
			Cjk.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x2190), FInt32RangeBound::Inclusive(0x21FF)));
			Cjk.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x3000), FInt32RangeBound::Inclusive(0x303F)));
			Cjk.ScalingFactor = 0.92f;
			Font->SubTypefaces.Add(Cjk);
		}

		{
			// Tamil is not covered by the engine fonts; use the Windows system face (collection: regular, bold).
			const FString Nirmala = TEXT("C:/Windows/Fonts/Nirmala.ttc");
			if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*Nirmala))
			{
				UE_LOG(LogTemp, Warning, TEXT("Kiosk: Tamil font %s not found; Tamil text will not render"), *Nirmala);
			}
			else
			{
				FCompositeSubFont Tamil;
				Tamil.Typeface.AppendFont(RegularName, Nirmala, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				Tamil.Typeface.AppendFont(BoldName, Nirmala, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				Tamil.Typeface.Fonts[1].Font.SetSubFaceIndex(1);
				Tamil.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x0B80), FInt32RangeBound::Inclusive(0x0BFF)));
				Tamil.ScalingFactor = 0.78f;
				Font->SubTypefaces.Add(Tamil);
			}
		}

		return Font;
	}

	struct FButtonLook
	{
		FLinearColor Fill;
		FLinearColor Hover;
		FLinearColor Pressed;
		FLinearColor Border;
		float BorderWidth;
		float Radius;
		bool bGreyWhenDisabled;
	};
}

FKioskColors::FKioskColors()
	: Bg(Hex(TEXT("EEF4FB")))
	, Surface(Hex(TEXT("FFFFFF")))
	, BorderStrong(Hex(TEXT("B8C7DA")))
	, Ink(Hex(TEXT("1B2A4A")))
	, Muted(Hex(TEXT("4B5B74")))
	, Brand(Hex(TEXT("0B7A75")))
	, Logo(Hex(TEXT("1F3A6B")))
	, Primary(Hex(TEXT("1F8A43")))
	, PrimaryTint(Hex(TEXT("E3F4E8")))
	, PrimaryChip(Hex(TEXT("92E0AC")))
	, Help(Hex(TEXT("1E5AA8")))
	, HelpTint(Hex(TEXT("E3EDF8")))
	, Band(Hex(TEXT("DAE5F1")))
	, Disabled(Hex(TEXT("D5DFEA")))
	, DisabledInk(Hex(TEXT("1B2A4A")))
	, Danger(Hex(TEXT("C62828")))
	, AmberFill(Hex(TEXT("FFF3CD")))
	, AmberStroke(Hex(TEXT("E0A100")))
	, AmberStrong(Hex(TEXT("C77700")))
	, Video(Hex(TEXT("222F4F")))
	, SimInk(Hex(TEXT("A04E00")))
	, SimFill(Hex(TEXT("FFF8EC")))
	, Transparent(0.f, 0.f, 0.f, 0.f)
	, White(1.f, 1.f, 1.f, 1.f)
	, Black(0.f, 0.f, 0.f, 1.f)
	, Chip(Hex(TEXT("E0AE2E")))
	, OnBrandDivider(1.f, 1.f, 1.f, 0.3f)
	, OnBrandSoft(1.f, 1.f, 1.f, 0.88f)
	, MapLabelBg(1.f, 1.f, 1.f, 0.82f)
	, DeviceDark(Hex(TEXT("0D1424")))
{
	Line[0] = Hex(TEXT("D42E12"));
	Line[1] = Hex(TEXT("009645"));
	Line[2] = Hex(TEXT("9900AA"));
	Line[3] = Hex(TEXT("FA9E0D"));
	Line[4] = Hex(TEXT("005EC4"));
	Line[5] = Hex(TEXT("9D5B25"));
}

FKioskStyle::FKioskStyle()
	: CompositeFont(BuildCompositeFont())
{
}

FKioskStyle& FKioskStyle::Get()
{
	TUniquePtr<FKioskStyle>& Holder = InstanceHolder();
	if (!Holder.IsValid())
	{
		Holder.Reset(new FKioskStyle());
	}
	return *Holder;
}

void FKioskStyle::Shutdown()
{
	InstanceHolder().Reset();
}

FSlateFontInfo FKioskStyle::Font(float DesignPx, bool bBold) const
{
	// Slate font sizes are points at 96 DPI, so 0.75 converts design pixels to a point size.
	return FSlateFontInfo(CompositeFont, DesignPx * 0.75f, bBold ? BoldName : RegularName);
}

const FSlateBrush* FKioskStyle::Round(const FLinearColor& Fill, float Radius, const FLinearColor& Border, float BorderWidth)
{
	const FString Key = FString::Printf(TEXT("R%08x%08x%08x%08x|%.1f|%.1f"),
		Fill.ToFColor(true).DWColor(), Border.ToFColor(true).DWColor(), (uint32)(Fill.A * 255.f), (uint32)(Border.A * 255.f), Radius, BorderWidth);
	if (const TSharedPtr<FSlateBrush>* Found = BrushCache.Find(Key))
	{
		return Found->Get();
	}
	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Border, BorderWidth);
	BrushCache.Add(Key, Brush);
	return Brush.Get();
}

const FSlateBrush* FKioskStyle::Solid(const FLinearColor& Fill)
{
	const FString Key = FString::Printf(TEXT("S%08x%u"), Fill.ToFColor(true).DWColor(), (uint32)(Fill.A * 255.f));
	if (const TSharedPtr<FSlateBrush>* Found = BrushCache.Find(Key))
	{
		return Found->Get();
	}
	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateColorBrush>(Fill);
	BrushCache.Add(Key, Brush);
	return Brush.Get();
}

const FButtonStyle& FKioskStyle::DisabledButtonStyle(EKioskButtonKind Kind)
{
	const uint8 Key = 100 + static_cast<uint8>(Kind);
	if (const TSharedPtr<FButtonStyle>* Found = ButtonCache.Find(Key))
	{
		return **Found;
	}
	const FButtonStyle& Enabled = ButtonStyle(Kind);
	TSharedPtr<FButtonStyle> Style = MakeShared<FButtonStyle>(Enabled);
	const FSlateBrush& Off = Enabled.Disabled;
	Style->SetNormal(Off);
	Style->SetHovered(Off);
	Style->SetPressed(Off);
	ButtonCache.Add(Key, Style);
	return *Style;
}

const FButtonStyle& FKioskStyle::ButtonStyle(EKioskButtonKind Kind)
{
	if (const TSharedPtr<FButtonStyle>* Found = ButtonCache.Find(static_cast<uint8>(Kind)))
	{
		return **Found;
	}

	const FLinearColor Clear = C.Transparent;
	FButtonLook Look{ C.Primary, C.Primary, C.Primary, Clear, 0.f, 14.f, true };
	auto Darker = [](const FLinearColor& Color, float Amount) { return FLinearColor(Color.R * Amount, Color.G * Amount, Color.B * Amount, Color.A); };

	switch (Kind)
	{
	case EKioskButtonKind::Primary:
		Look = { C.Primary, Darker(C.Primary, 1.12f), Darker(C.Primary, 0.8f), Clear, 0.f, 14.f, true };
		break;
	case EKioskButtonKind::Strong:
		Look = { C.Ink, Darker(C.Ink, 1.5f), Darker(C.Ink, 0.7f), Clear, 0.f, 14.f, true };
		break;
	case EKioskButtonKind::Outline:
		Look = { C.Surface, C.HelpTint, C.Band, C.Ink, 2.f, 14.f, false };
		break;
	case EKioskButtonKind::OutlineMuted:
		Look = { C.Surface, C.HelpTint, C.Band, C.BorderStrong, 2.f, 14.f, false };
		break;
	case EKioskButtonKind::Help:
		Look = { C.Help, Darker(C.Help, 1.15f), Darker(C.Help, 0.8f), Clear, 0.f, 14.f, true };
		break;
	case EKioskButtonKind::HelpLight:
		Look = { C.HelpTint, C.Band, C.BorderStrong, C.Ink, 2.f, 14.f, false };
		break;
	case EKioskButtonKind::DangerOutline:
		Look = { C.Surface, C.AmberFill, C.AmberFill, C.Danger, 2.f, 14.f, false };
		break;
	case EKioskButtonKind::Tint:
		Look = { C.Band, Darker(C.Band, 1.04f), Darker(C.Band, 0.92f), Clear, 0.f, 12.f, false };
		break;
	case EKioskButtonKind::Card:
		Look = { Clear, FLinearColor(C.Ink.R, C.Ink.G, C.Ink.B, 0.04f), FLinearColor(C.Ink.R, C.Ink.G, C.Ink.B, 0.1f), Clear, 0.f, 22.f, false };
		break;
	case EKioskButtonKind::Tile:
		Look = { C.Primary, Darker(C.Primary, 1.12f), Darker(C.Primary, 0.8f), Clear, 0.f, 20.f, true };
		break;
	case EKioskButtonKind::PillSelected:
		Look = { C.Ink, Darker(C.Ink, 1.5f), Darker(C.Ink, 0.7f), Clear, 0.f, 32.f, false };
		break;
	case EKioskButtonKind::PillOutline:
		Look = { C.Surface, C.HelpTint, C.Band, C.BorderStrong, 2.f, 32.f, false };
		break;
	case EKioskButtonKind::Key:
		Look = { C.Surface, C.HelpTint, C.Band, C.BorderStrong, 2.f, 12.f, false };
		break;
	case EKioskButtonKind::KeyMuted:
		Look = { C.Band, Darker(C.Band, 1.04f), Darker(C.Band, 0.92f), C.BorderStrong, 2.f, 12.f, false };
		break;
	}

	TSharedPtr<FButtonStyle> Style = MakeShared<FButtonStyle>();
	Style->SetNormal(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	// Touch kiosk: no hover feedback, only a pressed state.
	Style->SetHovered(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	Style->SetPressed(*Round(Look.Pressed, Look.Radius, Look.Border, Look.BorderWidth));
	if (Look.bGreyWhenDisabled)
	{
		Style->SetDisabled(*Round(C.Disabled, Look.Radius));
	}
	else
	{
		Style->SetDisabled(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	}
	Style->SetNormalPadding(FMargin(0.f));
	Style->SetPressedPadding(FMargin(0.f));
	ButtonCache.Add(static_cast<uint8>(Kind), Style);
	return *Style;
}
