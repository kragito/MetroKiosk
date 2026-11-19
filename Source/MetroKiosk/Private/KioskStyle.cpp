#include "KioskStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"

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
			// Tamil is not covered by the engine fonts; use the Windows system face. Windows 11 ships
			// Nirmala.ttc (collection: regular, bold); Windows 10 ships Nirmala.ttf and NirmalaB.ttf.
			FString WinDir = FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR"));
			if (WinDir.IsEmpty())
			{
				WinDir = TEXT("C:/Windows");
			}
			const FString WinFonts = WinDir / TEXT("Fonts");
			IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
			const FString NirmalaTtc = WinFonts / TEXT("Nirmala.ttc");
			const FString NirmalaReg = WinFonts / TEXT("Nirmala.ttf");
			const FString NirmalaBold = WinFonts / TEXT("NirmalaB.ttf");
			const bool bTtc = PF.FileExists(*NirmalaTtc);
			if (!bTtc && !PF.FileExists(*NirmalaReg))
			{
				UE_LOG(LogTemp, Warning, TEXT("Kiosk: Tamil font Nirmala not found in %s; Tamil text will not render"), *WinFonts);
			}
			else
			{
				const FString RegPath = bTtc ? NirmalaTtc : NirmalaReg;
				const FString BoldPath = bTtc ? NirmalaTtc : (PF.FileExists(*NirmalaBold) ? NirmalaBold : NirmalaReg);
				FCompositeSubFont Tamil;
				Tamil.Typeface.AppendFont(RegularName, RegPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				Tamil.Typeface.AppendFont(BoldName, BoldPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
				if (bTtc)
				{
					Tamil.Typeface.Fonts[1].Font.SetSubFaceIndex(1);
				}
				Tamil.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x0B80), FInt32RangeBound::Inclusive(0x0BFF)));
					// Joiners and the dotted circle stay in the Tamil run so a word is never split between fonts.
					Tamil.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x200C), FInt32RangeBound::Inclusive(0x200D)));
					Tamil.CharacterRanges.Add(FInt32Range(FInt32RangeBound::Inclusive(0x25CC), FInt32RangeBound::Inclusive(0x25CC)));
				Tamil.ScalingFactor = 0.9f;
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
	, DisabledInk(Hex(TEXT("5F6F88")))
	, Danger(Hex(TEXT("C62828")))
	, AmberFill(Hex(TEXT("FFF3CD")))
	, AmberStroke(Hex(TEXT("E0A100")))
	, AmberStrong(Hex(TEXT("C77700")))
	, Video(Hex(TEXT("222F4F")))
	, Transparent(0.f, 0.f, 0.f, 0.f)
	, White(1.f, 1.f, 1.f, 1.f)
	, Black(0.f, 0.f, 0.f, 1.f)
	, Chip(Hex(TEXT("E0AE2E")))
	, OnBrandDivider(1.f, 1.f, 1.f, 0.3f)
	, OnBrandSoft(1.f, 1.f, 1.f, 0.88f)
	, MapLabelBg(1.f, 1.f, 1.f, 0.82f)
	, DeviceDark(Hex(TEXT("0D1424")))
	, Scrim(0.05f, 0.08f, 0.15f, 0.62f)
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
	case EKioskButtonKind::Compact:
		Look = { C.Surface, C.HelpTint, C.Band, C.Ink, 2.f, 14.f, false };
		break;
	case EKioskButtonKind::Link:
		Look = { Clear, FLinearColor(C.Ink.R, C.Ink.G, C.Ink.B, 0.04f), FLinearColor(C.Ink.R, C.Ink.G, C.Ink.B, 0.1f), Clear, 0.f, 12.f, false };
		break;
	}

	TSharedPtr<FButtonStyle> Style = MakeShared<FButtonStyle>();
	Style->SetNormal(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	// Touch kiosk: no hover feedback, only a pressed state.
	Style->SetHovered(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	Style->SetPressed(*Round(Look.Pressed, Look.Radius, Look.Border, Look.BorderWidth));
	if (Kind == EKioskButtonKind::Card || Kind == EKioskButtonKind::Link)
	{
		Style->SetDisabled(*Round(Look.Fill, Look.Radius, Look.Border, Look.BorderWidth));
	}
	else
	{
		// Every disabled control looks the same: flat grey with a hairline, so it can never be mistaken for a choice.
		Style->SetDisabled(*Round(C.Disabled, Look.Radius, Look.Border.A > 0.f ? C.BorderStrong : Clear, Look.Border.A > 0.f ? 2.f : 0.f));
	}
	Style->SetNormalPadding(FMargin(0.f));
	Style->SetPressedPadding(FMargin(0.f));
	ButtonCache.Add(static_cast<uint8>(Kind), Style);
	return *Style;
}

bool FKioskColors::Lookup(const FName& Token, FLinearColor& OutColour)
{
	static const FKioskColors Colours;
	if (Token == TEXT("Bg")) { OutColour = Colours.Bg; return true; }
	if (Token == TEXT("Surface")) { OutColour = Colours.Surface; return true; }
	if (Token == TEXT("BorderStrong")) { OutColour = Colours.BorderStrong; return true; }
	if (Token == TEXT("Ink")) { OutColour = Colours.Ink; return true; }
	if (Token == TEXT("Muted")) { OutColour = Colours.Muted; return true; }
	if (Token == TEXT("Brand")) { OutColour = Colours.Brand; return true; }
	if (Token == TEXT("Logo")) { OutColour = Colours.Logo; return true; }
	if (Token == TEXT("Primary")) { OutColour = Colours.Primary; return true; }
	if (Token == TEXT("PrimaryTint")) { OutColour = Colours.PrimaryTint; return true; }
	if (Token == TEXT("PrimaryChip")) { OutColour = Colours.PrimaryChip; return true; }
	if (Token == TEXT("Help")) { OutColour = Colours.Help; return true; }
	if (Token == TEXT("HelpTint")) { OutColour = Colours.HelpTint; return true; }
	if (Token == TEXT("Band")) { OutColour = Colours.Band; return true; }
	if (Token == TEXT("Disabled")) { OutColour = Colours.Disabled; return true; }
	if (Token == TEXT("DisabledInk")) { OutColour = Colours.DisabledInk; return true; }
	if (Token == TEXT("Danger")) { OutColour = Colours.Danger; return true; }
	if (Token == TEXT("AmberFill")) { OutColour = Colours.AmberFill; return true; }
	if (Token == TEXT("AmberStroke")) { OutColour = Colours.AmberStroke; return true; }
	if (Token == TEXT("AmberStrong")) { OutColour = Colours.AmberStrong; return true; }
	if (Token == TEXT("Video")) { OutColour = Colours.Video; return true; }
	if (Token == TEXT("Transparent")) { OutColour = Colours.Transparent; return true; }
	if (Token == TEXT("White")) { OutColour = Colours.White; return true; }
	if (Token == TEXT("Black")) { OutColour = Colours.Black; return true; }
	if (Token == TEXT("Chip")) { OutColour = Colours.Chip; return true; }
	if (Token == TEXT("OnBrandDivider")) { OutColour = Colours.OnBrandDivider; return true; }
	if (Token == TEXT("OnBrandSoft")) { OutColour = Colours.OnBrandSoft; return true; }
	if (Token == TEXT("MapLabelBg")) { OutColour = Colours.MapLabelBg; return true; }
	if (Token == TEXT("DeviceDark")) { OutColour = Colours.DeviceDark; return true; }
	if (Token == TEXT("Scrim")) { OutColour = Colours.Scrim; return true; }
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (Token == FName(*FString::Printf(TEXT("Line%d"), Index))) { OutColour = Colours.Line[Index]; return true; }
	}
	return false;
}
