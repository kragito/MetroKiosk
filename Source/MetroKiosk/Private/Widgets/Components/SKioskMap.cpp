#include "SKioskMap.h"

#include "KioskData.h"
#include "KioskAutomation.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskStyle.h"
#include "Fonts/FontCache.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"

/** Key for a shaped label: the pieces that change the glyphs (language and scale clear the whole cache). */
struct FKioskMapShapeKey
{
	FString Text;
	float Size = 0.f;
	FName Face;
	uint32 FontObjectId = 0;
	bool operator==(const FKioskMapShapeKey& Other) const
	{
		return Size == Other.Size && FontObjectId == Other.FontObjectId && Face == Other.Face && Text == Other.Text;
	}
	friend uint32 GetTypeHash(const FKioskMapShapeKey& Key)
	{
		return HashCombine(HashCombine(GetTypeHash(Key.Text), GetTypeHash(Key.Size)), HashCombine(GetTypeHash(Key.Face), Key.FontObjectId));
	}
};

struct FKioskMapShapeCache
{
	TMap<FKioskMapShapeKey, FShapedGlyphSequenceRef> Map;
	int32 Language = -1;
	float Scale = -1.f;
};

/** One label pill placed by the collision pass. */
struct FKioskMapPlacedLabel
{
	FSlateRect Rect;
	FVector2f TextPos = FVector2f::ZeroVector;
	FVector2f Dot = FVector2f::ZeroVector;    // leader start
	FVector2f Anchor = FVector2f::ZeroVector; // leader end
	bool bLeader = false;
	bool bSelected = false;
	float Width = 0.f;
	float Slack = 0.f;
	FString Text;
};

/** The last placement result and the inputs it was computed from; reused until an input changes. */
struct FKioskMapLayout
{
	bool bValid = false;
	FVector2f Size = FVector2f::ZeroVector;
	float Zoom = 0.f;
	FVector2f Pan = FVector2f::ZeroVector;
	int32 Selected = INDEX_NONE;
	int32 Filter = INDEX_NONE;
	bool bDim = false;
	TSet<int32> Lit;
	int32 Language = -1;
	float Scale = 0.f;
	float WidthSum = 0.f;
	FVector2f MarkerPos = FVector2f::ZeroVector;
	bool bMarkerLeader = false;
	TArray<FKioskMapPlacedLabel> Labels;
};

namespace
{
	const FVector2f GMapMin(30.f, 12.f);
	const FVector2f GMapMax(900.f, 592.f);
	const FVector2f GOpeningNudge(-5.f, -40.f); // map units added to City Hall as the centre of the opening view
	constexpr float HitRadius = 30.f;
	constexpr float SlackDefault = 0.06f; // measured text width can undershoot the shaped width, so leave this fraction spare
	constexpr float SlackTamil = 0.14f;   // the "You are here" pill in Tamil only; its text shaped tight at 6%
	constexpr float MarkerFarGap = 64.f;  // longest leader line the "You are here" pill may use
	constexpr float MarkerNearGap = 12.f; // "You are here" pill stays this close to its dot unless a leader line joins them

	/** Extra width to add to a measured text width so shaped text fits; callers centre the text inside it. */
	float TextSlack(float MeasuredWidth, bool bTamilWider = false)
	{
		return MeasuredWidth * ((bTamilWider && FKioskLoc::GetLanguage() == EKioskLanguage::Tamil) ? SlackTamil : SlackDefault);
	}

	/** Draws text through HarfBuzz shaping (shaped sequences come from the widget's cache). FSlateDrawElement::MakeText(FString) draws one glyph per code point with no shaping, which breaks Tamil vowel signs and conjuncts. */
	void MakeShapedKioskText(FKioskMapShapeCache& Cache, FSlateWindowElementList& OutDrawElements, int32 Layer, const FGeometry& AllottedGeometry, const FPaintGeometry& Paint,
		const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Tint)
	{
		if (Text.IsEmpty() || !FSlateApplication::IsInitialized())
		{
			return;
		}
		const float FontScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
		const int32 Language = static_cast<int32>(FKioskLoc::GetLanguage());
		if (Language != Cache.Language || !FMath::IsNearlyEqual(FontScale, Cache.Scale) || Cache.Map.Num() > 800)
		{
			Cache.Map.Reset();
			Cache.Language = Language;
			Cache.Scale = FontScale;
		}
		FKioskMapShapeKey Key;
		Key.Text = Text;
		Key.Size = Font.Size;
		Key.Face = Font.TypefaceFontName;
		Key.FontObjectId = Font.FontObject ? Font.FontObject->GetUniqueID() : 0u;
		const FShapedGlyphSequenceRef* Found = Cache.Map.Find(Key);
		if (Found && ((*Found)->GetGlyphsToRender().Num() == 0 || (*Found)->IsDirty()))
		{
			Found = nullptr; // shaped before its font face was usable (or the face was unloaded): shape again
		}
		if (!Found)
		{
			const TSharedRef<FSlateFontCache> FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
			const FShapedGlyphSequenceRef Shaped = FontCache->ShapeUnidirectionalText(Text, Font, FontScale, TextBiDi::ETextDirection::LeftToRight, GetDefaultTextShapingMethod());
			if (Shaped->GetGlyphsToRender().Num() == 0)
			{
				// Nothing to draw yet (font still loading): draw nothing and do not remember this result, so the next paint shapes again.
				Cache.Map.Remove(Key);
				return;
			}
			Found = &Cache.Map.Add(Key, Shaped);
		}
		FSlateDrawElement::MakeShapedText(OutDrawElements, Layer, Paint, *Found, ESlateDrawEffect::None, Tint, Tint);
	}

	/** A station label waiting for collision-free placement. */
	struct FPendingLabel
	{
		int32 Index = INDEX_NONE;
		FVector2f Pos = FVector2f::ZeroVector; // dot centre, widget-local
		float DotRadius = 0.f;
		float Width = 0.f; // measured text width plus slack for complex scripts
		float Slack = 0.f; // extra width inside Width; the text is shifted by half of it to stay centred
		FString Text;      // active-language station name (English when there is no local name)
		int32 Priority = 4; // 0 selected, 1 lit, 2 on highlighted line, 3 interchange, 4 rest
		bool bPreferLeft = false;
		bool bSelected = false;
		bool bMustShow = false; // selected or lit: drawn even if every spot overlaps
	};
}

void SKioskMap::Construct(const FArguments& InArgs)
{
	SelectedStation = InArgs._SelectedStation;
	Zoom = FMath::Max(1.f, InArgs._Zoom);
	Session = InArgs._Session;
	LitStations = InArgs._LitStations;
	bDimOthers = InArgs._bDimOthers;
	SetClipping(EWidgetClipping::ClipToBounds);
	TSharedRef<FKioskAutomationMeta> Meta = MakeShared<FKioskAutomationMeta>();
	Meta->Name = TEXT("Map");
	Meta->Label = TEXT("Metro map");
	AddMetadata(Meta);
	OnStationTapped = InArgs._OnStationTapped;
	SetCanTick(false);
}

void SKioskMap::ViewParams(const FVector2f& LocalSize, FVector2f& OutScale, FVector2f& OutFocus) const
{
	const FVector2f MapSize = GMapMax - GMapMin;
	// Height sets the scale; width may stretch up to 30% more so the network uses the card width (dots stay round).
	const float ScaleY = (LocalSize.Y - 44.f) / MapSize.Y;
	const float ScaleX = FMath::Min((LocalSize.X - 40.f) / MapSize.X, ScaleY * 1.3f);
	OutScale = FVector2f(ScaleX, ScaleY);
	OutFocus = (GMapMin + GMapMax) * 0.5f;
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	if (Zoom > 1.01f && Stations.IsValidIndex(SelectedStation))
	{
		// Centre on the selected station, but never further than the viewport edge reaches the network edge (no blank band).
		FVector2f Focus = Stations[SelectedStation].MapPos;
		const FVector2f Half = LocalSize * 0.5f / (OutScale * Zoom); // viewport half size in map units
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			if (Half[Axis] * 2.f < MapSize[Axis])
			{
				Focus[Axis] = FMath::Clamp(Focus[Axis], GMapMin[Axis] + Half[Axis], GMapMax[Axis] - Half[Axis]);
			}
			else
			{
				Focus[Axis] = OutFocus[Axis];
			}
		}
		OutFocus = Focus;
	}
	else if (Zoom > 1.01f && Stations.IsValidIndex(FKioskData::CityHallIndex()))
	{
		OutFocus = Stations[FKioskData::CityHallIndex()].MapPos + GOpeningNudge; // opening view: around City Hall, nudged to keep the CC loop in
	}
}

FVector2f SKioskMap::ClampPan(const FVector2f& Pan, const FVector2f& LocalSize) const
{
	FVector2f Scale, Focus;
	ViewParams(LocalSize, Scale, Focus);
	const FVector2f Centre = (GMapMin + GMapMax) * 0.5f;
	const FVector2f Half = (GMapMax - GMapMin) * 0.5f * Scale * Zoom;
	const FVector2f C0 = LocalSize * 0.5f + (Centre - Focus) * Scale * Zoom; // network centre with no pan
	FVector2f Out = Pan;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		// The network's far edge may not pass 90% of the way across the viewport, nor its near edge 10%: most of the viewport stays covered.
		const float Lo = LocalSize[Axis] * 0.9f - (C0[Axis] + Half[Axis]);
		const float Hi = LocalSize[Axis] * 0.1f - (C0[Axis] - Half[Axis]);
		Out[Axis] = Lo <= Hi ? FMath::Clamp(Pan[Axis], Lo, Hi) : (Lo + Hi) * 0.5f;
	}
	return Out;
}

FVector2f SKioskMap::ToLocal(const FVector2f& MapPos, const FVector2f& LocalSize) const
{
	FVector2f BaseScale, Focus;
	ViewParams(LocalSize, BaseScale, Focus);
	FVector2f Pan = (Zoom > 1.01f && Session.IsValid()) ? Session->MapPan : FVector2f::ZeroVector;
	if (Zoom > 1.01f)
	{
		Pan = ClampPan(Pan, LocalSize);
	}
	return LocalSize * 0.5f + Pan + (MapPos - Focus) * BaseScale * Zoom;
}

int32 SKioskMap::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FKioskStyle& Style = FKioskStyle::Get();
	const FKioskColors& C = Style.C;
	const FVector2f Size = AllottedGeometry.GetLocalSize();
	const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	const float LineWidth = 7.f * FMath::Sqrt(Zoom);
	const int32 Filter = Session.IsValid() ? Session->State.LineFilter : INDEX_NONE;

	// Lines
	for (int32 Line = 0; Line < FKioskData::NumLines; ++Line)
	{
		for (const TArray<FKioskPathPoint>& Shape : FKioskData::LineShapes(Line))
		{
			TArray<FVector2f> Points; // stations and bends, so the line follows the system map's shape
			for (const FKioskPathPoint& Point : Shape)
			{
				Points.Add(ToLocal(Point.Pos, Size));
			}
			FLinearColor LineColor = C.Line[Line];
			LineColor.A = (Filter != INDEX_NONE && Filter != Line) ? 0.18f : 1.f; // optional highlighter dims the other lines
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Paint, Points, ESlateDrawEffect::None, LineColor, true, LineWidth);
		}
	}

	// Station dots and labels
	const FSlateFontInfo LabelFont = Style.Font(KioskType::MapLabel, true);
	const FSlateFontInfo MarkerFont = Style.Font(KioskType::Caption, true);
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	TArray<FPendingLabel> Pending; // labels wait until every dot and the "You are here" chip are known
	TArray<FSlateRect> Placed;     // rectangles already taken, in local space
	TArray<bool> DotFull; // parallel to DotRects: true when the dot is drawn at full strength (not dimmed)
	TArray<TPair<int32, FSlateRect>> DotRects; // every station dot (with 2 px margin); a label's own dot does not block it
	FVector2f HerePos = FVector2f::ZeroVector; // "You are here" dot; the marker is placed once every dot is known
	float HereDraw = 0.f;
	int32 HereIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Stations.Num(); ++Index)
	{
		const FKioskStation& Station = Stations[Index];
		const FVector2f Pos = ToLocal(Station.MapPos, Size);
		const bool bInterchange = FMath::CountBits(Station.LineMask) > 1;
		const bool bSelected = Index == SelectedStation;
		const bool bHere = Index == FKioskData::CityHallIndex();
		const bool bOnFilter = Filter != INDEX_NONE && Station.OnLine(Filter);
		const bool bLit = bDimOthers && LitStations.Contains(Index);
		const bool bSearchDim = bDimOthers && !bLit && !bSelected && !bHere;
		const bool bDim = (Filter != INDEX_NONE && !bOnFilter && !bSelected && !bHere) || bSearchDim;
		const float Radius = (bSelected ? 17.f : (bInterchange ? 11.f : (bLit ? 9.f : 7.f))) * FMath::Sqrt(Zoom);

		// When the line highlighter is on, or search text matches stations, dim every part of a station that is not
		// on that line / not a match (fill and ring). Matches keep full strength and show their name.
		const float Fade = bDim ? 0.25f : 1.f;
		FLinearColor Ring = bInterchange ? C.Ink : C.Line[Station.FirstLine()];
		Ring.A *= Fade;
		const FSlateBrush* Dot = bSelected
			? Style.Round(C.Primary, Radius, C.White, 4.f)
			: bHere
				? Style.Round(C.Help, Radius * 1.3f, C.White, 4.f)
				: Style.Round(C.White, Radius, Ring, bInterchange ? 4.f : 3.f);
		const float Draw = (bHere && !bSelected) ? Radius * 1.3f : Radius;
		DotRects.Emplace(Index, FSlateRect(Pos.X - Draw - 2.f, Pos.Y - Draw - 2.f, Pos.X + Draw + 2.f, Pos.Y + Draw + 2.f));
		DotFull.Add(!bDim);
		FLinearColor DotTint = FKioskStyle::BrushTint(Dot);
		DotTint.A *= bDim ? 0.25f : 1.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(Draw * 2.f, Draw * 2.f), FSlateLayoutTransform(Pos - FVector2f(Draw, Draw))), Dot,
			ESlateDrawEffect::None, DotTint);

		// City Hall shows the "You are here" marker instead of a plain label.
		if (bHere)
		{
			HerePos = Pos;
			HereDraw = Draw;
			HereIndex = Index;
		}
		else if (!bDim && (bSelected || bInterchange || bOnFilter || bLit || Zoom > 1.2f))
		{
			// Crowded stations put their label on the left; every label sits on a soft white pill so lines never cut through it.
			static const TSet<FString> LeftLabels = { TEXT("Chinatown"), TEXT("Outram Park"), TEXT("Dhoby Ghaut"),
				TEXT("Botanic Gardens"), TEXT("Caldecott"), TEXT("Orchard"), TEXT("Buona Vista"), TEXT("HarbourFront") };
			FPendingLabel Label;
			Label.Index = Index;
			Label.Pos = Pos;
			Label.DotRadius = Draw;
			// Show the same name the destination list shows; the English name stays the key for LeftLabels above.
			const FString Local = FKioskPlaces::LocalName(Station.Name);
			Label.Text = Local.IsEmpty() ? Station.Name : Local;
			const float TextWidth = FontMeasure->Measure(Label.Text, LabelFont).X;
			// Station labels use the default slack in every language (no Tamil clipping seen at 6%).
			Label.Slack = TextSlack(TextWidth);
			Label.Width = TextWidth + Label.Slack;
			Label.bPreferLeft = LeftLabels.Contains(Station.Name);
			Label.bSelected = bSelected;
			Label.bMustShow = bSelected || bLit;
			Label.Priority = bSelected ? 0 : (bLit ? 1 : (bOnFilter ? 2 : (bInterchange ? 3 : 4)));
			Pending.Add(Label);
		}
	}

	// Placement is the expensive part of a paint, so it is reused until an input that affects it changes.
	if (!ShapeCache.IsValid())
	{
		ShapeCache = MakeShared<FKioskMapShapeCache>();
	}
	if (!LayoutCache.IsValid())
	{
		LayoutCache = MakeShared<FKioskMapLayout>();
	}
	FKioskMapLayout& Layout = *LayoutCache;
	float WidthSum = 0.f;
	for (const FPendingLabel& Label : Pending)
	{
		WidthSum += Label.Width;
	}
	const int32 LayoutLanguage = static_cast<int32>(FKioskLoc::GetLanguage());
	const float LayoutScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const FVector2f LayoutPan = (Zoom > 1.01f && Session.IsValid()) ? Session->MapPan : FVector2f::ZeroVector;
	const bool bHit = Layout.bValid && Layout.Size == Size && Layout.Zoom == Zoom && Layout.Pan == LayoutPan
		&& Layout.Selected == SelectedStation && Layout.Filter == Filter && Layout.bDim == bDimOthers
		&& Layout.Lit.Num() == LitStations.Num() && Layout.Lit.Includes(LitStations)
		&& Layout.Language == LayoutLanguage && FMath::IsNearlyEqual(Layout.Scale, LayoutScale) && FMath::IsNearlyEqual(Layout.WidthSum, WidthSum);

	// "You are here" marker instead of a plain City Hall label. Default spot is above-left (never covers the Chinatown
	// label); when a wider translation would cover another station dot there, try the other spots around the dot.
	if (HereIndex != INDEX_NONE)
	{
		const FString Text = KT("You are here").ToString();
		const float TextWidth = FontMeasure->Measure(Text, MarkerFont).X;
		const float Slack = TextSlack(TextWidth, true); // the marker alone gets the wider Tamil slack; centre the text inside the slack
		const FVector2f BoxSize(TextWidth + Slack + 22.f, 32.f);
		const float Up = -BoxSize.Y * 0.5f - 14.f;
		const FVector2f Default(-HereDraw - 8.f - BoxSize.X, Up);
		// The default spot is kept unless the pill overlaps any other station dot (so languages that fit today do not move).
		auto OverlapsOtherDot = [&](const FVector2f& Off)
		{
			const FVector2f Try = HerePos + Off;
			// 3 px of air around the pill so it never touches a neighbouring dot's ring.
			const FSlateRect Box(Try.X - 3.f, Try.Y - 3.f, Try.X + BoxSize.X + 3.f, Try.Y + BoxSize.Y + 3.f);
			for (const TPair<int32, FSlateRect>& Dot : DotRects)
			{
				if (Dot.Key != HereIndex && FSlateRect::DoRectanglesIntersect(Box, Dot.Value))
				{
					return true;
				}
			}
			return false;
		};
		FVector2f BoxPos = bHit ? Layout.MarkerPos : HerePos + Default;
		bool bLeader = bHit && Layout.bMarkerLeader;
		if (!bHit && OverlapsOtherDot(Default))
		{
			// Candidate offsets: a grid around the dot, nearest to the default first. Built only when the default is blocked.
			TArray<FVector2f> Offsets;
			for (float Dy = -130.f; Dy <= 100.f; Dy += 6.f)
			{
				for (float Dx = -BoxSize.X - HereDraw - 130.f; Dx <= HereDraw + 130.f; Dx += 6.f)
				{
					Offsets.Add(FVector2f(Dx, Dy));
				}
			}
			Offsets.StableSort([&Default](const FVector2f& A, const FVector2f& B)
				{ return FVector2f::DistSquared(A, Default) < FVector2f::DistSquared(B, Default); });
			const FSlateRect Bounds(0.f, 0.f, Size.X, Size.Y);
			// Note: the hint bar, legend and zoom buttons are separate overlay widgets whose rects are not known here, so
			// they are not avoided; the search window is small and stays near the dot, well clear of them at the default zoom.
			auto BoxRect = [&](const FVector2f& Off)
			{
				const FVector2f Try = HerePos + Off;
				return FSlateRect(Try.X, Try.Y, Try.X + BoxSize.X, Try.Y + BoxSize.Y);
			};
			auto InsideWidget = [&](const FVector2f& Off)
			{
				const FSlateRect R = BoxRect(Off);
				return R.Left >= Bounds.Left && R.Top >= Bounds.Top && R.Right <= Bounds.Right && R.Bottom <= Bounds.Bottom;
			};
			auto TouchesDot = [&](const FVector2f& Off)
			{
				FSlateRect R = BoxRect(Off);
				R = FSlateRect(R.Left - 3.f, R.Top - 3.f, R.Right + 3.f, R.Bottom + 3.f); // 3 px of air around the pill
				for (const TPair<int32, FSlateRect>& Dot : DotRects)
				{
					// The marker's own dot only needs its drawn radius kept clear.
					const FSlateRect Dr = Dot.Key == HereIndex
						? FSlateRect(HerePos.X - HereDraw, HerePos.Y - HereDraw, HerePos.X + HereDraw, HerePos.Y + HereDraw) : Dot.Value;
					if (FSlateRect::DoRectanglesIntersect(R, Dr))
					{
						return true;
					}
				}
				return false;
			};
			// Distance from a point to the box placed at Off.
			auto DistToBox = [&](const FVector2f& Off, const FVector2f& P)
			{
				const FSlateRect R = BoxRect(Off);
				const float Dx = FMath::Max3(R.Left - P.X, 0.f, P.X - R.Right);
				const float Dy = FMath::Max3(R.Top - P.Y, 0.f, P.Y - R.Bottom);
				return FMath::Sqrt(Dx * Dx + Dy * Dy);
			};
			// True when the leader segment (dot to nearest box point) passes through another station dot.
			auto LeaderCrossesDot = [&](const FVector2f& Off)
			{
				const FSlateRect R = BoxRect(Off);
				const FVector2f End(FMath::Clamp(HerePos.X, R.Left, R.Right), FMath::Clamp(HerePos.Y, R.Top, R.Bottom));
				const FVector2f D = End - HerePos;
				const float Len2 = D.SizeSquared();
				for (const TPair<int32, FSlateRect>& Dot : DotRects)
				{
					if (Dot.Key == HereIndex)
					{
						continue;
					}
					const FVector2f Ctr = Dot.Value.GetCenter();
					const float T = Len2 > 0.f ? FMath::Clamp(FVector2f::DotProduct(Ctr - HerePos, D) / Len2, 0.f, 1.f) : 0.f;
					// The dot rect already carries a 2 px margin, so half its width is the dot radius + 2.
					if (FVector2f::Distance(Ctr, HerePos + D * T) < Dot.Value.GetSize().X * 0.5f)
					{
						return true;
					}
				}
				return false;
			};
			// Tiers, nearest to the default first within each:
			// 0: inside the widget, touches no dot, within MarkerNearGap of the dot (no leader line needed).
			// 1: inside the widget, touches no dot, further out; a leader line joins it to the dot and crosses no other dot.
			// When neither tier has a spot, the least-dot-overlap fallback below is used.
			// A neighbour dot can be closer to the pill than City Hall's own dot; the leader line is what ties them together.
			bool bFound = false;
			for (int32 Tier = 0; Tier < 2 && !bFound; ++Tier)
			{
				for (const FVector2f& Off : Offsets)
				{
					const float Gap = DistToBox(Off, HerePos) - HereDraw;
					if (TouchesDot(Off) || !InsideWidget(Off) || (Tier == 0 && Gap > MarkerNearGap) || Gap > MarkerFarGap
						|| (Gap > MarkerNearGap && LeaderCrossesDot(Off)))
					{
						continue;
					}
					BoxPos = HerePos + Off;
					bLeader = Gap > MarkerNearGap;
					bFound = true;
					break;
				}
			}
			if (!bFound)
			{
				// Nothing is clear of every dot: take the spot within MarkerFarGap that covers the least dot area (the default counts too).
				auto DotArea = [&](const FVector2f& Off)
				{
					const FSlateRect R = BoxRect(Off);
					float Area = 0.f;
					for (const TPair<int32, FSlateRect>& Dot : DotRects)
					{
						const FSlateRect Dr = Dot.Key == HereIndex
							? FSlateRect(HerePos.X - HereDraw, HerePos.Y - HereDraw, HerePos.X + HereDraw, HerePos.Y + HereDraw) : Dot.Value;
						const float W = FMath::Min(R.Right, Dr.Right) - FMath::Max(R.Left, Dr.Left);
						const float H = FMath::Min(R.Bottom, Dr.Bottom) - FMath::Max(R.Top, Dr.Top);
						Area += (W > 0.f && H > 0.f) ? W * H : 0.f;
					}
					return Area;
				};
				float BestArea = DotArea(Default);
				for (const FVector2f& Off : Offsets)
				{
					const float Gap = DistToBox(Off, HerePos) - HereDraw;
					if (Gap > MarkerFarGap || !InsideWidget(Off) || (Gap > MarkerNearGap && LeaderCrossesDot(Off)))
					{
						continue;
					}
					const float Area = DotArea(Off) + (Gap > MarkerNearGap ? 20.f : 0.f);
					if (Area + 1.f < BestArea)
					{
						BestArea = Area;
						BoxPos = HerePos + Off;
						bLeader = Gap > MarkerNearGap;
					}
				}
			}
		}
		if (!bHit)
		{
			Layout.MarkerPos = BoxPos;
			Layout.bMarkerLeader = bLeader;
		}
		if (bLeader)
		{
			const FVector2f Nearest(FMath::Clamp(HerePos.X, BoxPos.X, BoxPos.X + BoxSize.X), FMath::Clamp(HerePos.Y, BoxPos.Y, BoxPos.Y + BoxSize.Y));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, Paint, TArray<FVector2f>{ HerePos, Nearest },
				ESlateDrawEffect::None, C.Help, true, 2.f);
		}
		Placed.Add(FSlateRect(BoxPos.X, BoxPos.Y, BoxPos.X + BoxSize.X, BoxPos.Y + BoxSize.Y));
		const FSlateBrush* MarkerBrush = Style.Round(C.Help, 6.f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
			AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos)), MarkerBrush,
			ESlateDrawEffect::None, FKioskStyle::BrushTint(MarkerBrush));
		MakeShapedKioskText(*ShapeCache, OutDrawElements, LayerId + 5, AllottedGeometry,
			AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos + FVector2f(11.f + Slack * 0.5f, 5.f))),
			Text, MarkerFont, C.White);
	}

	// Label collision avoidance: highest priority first (stable by station index), first free candidate wins.
	Pending.StableSort([](const FPendingLabel& A, const FPendingLabel& B) { return A.Priority < B.Priority; });
	const FSlateBrush* PillBrush = Style.Round(C.MapLabelBg, 5.f);
	TArray<TPair<FVector2f, FVector2f>> PlacedLeaders; // leader segments already drawn (dot centre to pill edge)
	// True when segments AB and CD properly cross.
	auto SegmentsCross = [](const FVector2f& A, const FVector2f& B, const FVector2f& C, const FVector2f& D)
	{
		auto Side = [](const FVector2f& P, const FVector2f& Q, const FVector2f& R) { return (Q.X - P.X) * (R.Y - P.Y) - (Q.Y - P.Y) * (R.X - P.X); };
		return Side(A, B, C) * Side(A, B, D) < 0.f && Side(C, D, A) * Side(C, D, B) < 0.f;
	};
	// True when the segment A-B passes through Rect (Liang-Barsky clip against the rectangle shrunk by 1 px).
	auto SegmentHitsRect = [](const FVector2f& A, const FVector2f& B, const FSlateRect& Rect)
	{
		const float MinX = Rect.Left + 1.f, MaxX = Rect.Right - 1.f, MinY = Rect.Top + 1.f, MaxY = Rect.Bottom - 1.f;
		if (MinX >= MaxX || MinY >= MaxY)
		{
			return false;
		}
		const float D[2] = { B.X - A.X, B.Y - A.Y };
		const float P[4] = { -D[0], D[0], -D[1], D[1] };
		const float Q[4] = { A.X - MinX, MaxX - A.X, A.Y - MinY, MaxY - A.Y };
		float T0 = 0.f, T1 = 1.f;
		for (int32 Edge = 0; Edge < 4; ++Edge)
		{
			if (FMath::IsNearlyZero(P[Edge]))
			{
				if (Q[Edge] < 0.f)
				{
					return false;
				}
				continue;
			}
			const float T = Q[Edge] / P[Edge];
			if (P[Edge] < 0.f)
			{
				T0 = FMath::Max(T0, T);
			}
			else
			{
				T1 = FMath::Min(T1, T);
			}
			if (T0 > T1)
			{
				return false;
			}
		}
		return true;
	};
	const TArray<FPendingLabel> NoLabels;
	if (!bHit)
	{
		Layout.Labels.Reset();
	}
	for (const FPendingLabel& Label : (bHit ? NoLabels : Pending))
	{
		const float PillW = Label.Width + 8.f;
		constexpr float PillH = 24.f;
		const float R = Label.DotRadius;
		// Candidate text-origin offsets from the dot centre; the pill is the text box grown by (4, 2).
		// Order: right/left/above/below at the normal gap, then (priority labels only) the four diagonals,
		// then right/left/above/below again at +14 and +28 px with a leader line back to the dot.
		TArray<FVector2f, TInlineAllocator<64>> Candidates; // an interchange can have several hundred (leader grid)
		TArray<float, TInlineAllocator<64>> Gaps;
		auto AddSides = [&](float Gap)
		{
			const FVector2f Right(R + 6.f + Gap, -10.f);
			const FVector2f Left(-R - 6.f - Gap - Label.Width, -10.f);
			const FVector2f Above(-Label.Width * 0.5f, -R - 6.f - 22.f - Gap);
			const FVector2f Below(-Label.Width * 0.5f, R + 6.f + Gap);
			Candidates.Add(Label.bPreferLeft ? Left : Right);
			Candidates.Add(Label.bPreferLeft ? Right : Left);
			Candidates.Add(Above);
			Candidates.Add(Below);
			for (int32 K = 0; K < 4; ++K)
			{
				Gaps.Add(Gap);
			}
		};
		AddSides(0.f);
		if (Label.Priority <= 3)
		{
			// Interchanges try the diagonals too: the full network is dense and a side-on label often has no free spot.
			const float D = R * 0.7f + 3.f; // pill corner nearest the dot sits D px diagonally from its centre
			const FVector2f Corner(4.f, 2.f);
			Candidates.Add(FVector2f(D, -D - PillH) + Corner);
			Candidates.Add(FVector2f(-D - PillW, -D - PillH) + Corner);
			Candidates.Add(FVector2f(D, D) + Corner);
			Candidates.Add(FVector2f(-D - PillW, D) + Corner);
			for (int32 K = 0; K < 4; ++K)
			{
				Gaps.Add(0.f);
			}
		}
		if (Label.Priority == 3)
		{
			// An interchange that has no clean spot gets a short leader line instead of going unnamed.
			AddSides(11.f);
			// Any free spot within 60 px (grid search, nearest first) with a leader line, for interchanges buried in the crowded centre.
			{
				TArray<TPair<float, FVector2f>> Spots;
				Spots.Reserve(400);
				for (float Dy = -PillH - 70.f; Dy <= 70.f; Dy += 8.f)
				{
					for (float Dx = -PillW - 70.f; Dx <= 70.f; Dx += 8.f)
					{
						const float Ex = FMath::Max3(Dx, 0.f, -(Dx + PillW));
						const float Ey = FMath::Max3(Dy, 0.f, -(Dy + PillH));
						const float Gap = FMath::Sqrt(Ex * Ex + Ey * Ey) - R;
						if (Gap >= 14.f && Gap <= 60.f)
						{
							Spots.Emplace(Gap, FVector2f(Dx, Dy) + FVector2f(4.f, 2.f));
						}
					}
				}
				Spots.StableSort([](const TPair<float, FVector2f>& A, const TPair<float, FVector2f>& B) { return A.Key < B.Key; });
				for (const TPair<float, FVector2f>& Spot : Spots)
				{
					Candidates.Add(Spot.Value);
					Gaps.Add(Spot.Key);
				}
			}
		}
		if (Label.bMustShow)
		{
			AddSides(14.f);
			AddSides(28.f);
		}

		auto PillRect = [&](const FVector2f& Offset)
		{
			const FVector2f Min = Label.Pos + Offset + FVector2f(-4.f, -2.f);
			return FSlateRect(Min.X, Min.Y, Min.X + PillW, Min.Y + PillH);
		};
		// Cutoff: a total that reaches it cannot beat the best spot so far, so the scan stops early. bAmbiguous: another dot reads as nearer.
		auto Overlap = [&](const FSlateRect& Rect, bool bLeader, float Cutoff, bool& bAmbiguous)
		{
			float Total = 0.f;
			bAmbiguous = false;
			// A leader line from the dot to a detached pill must not run through another label or a full-strength dot,
			// and this pill must not sit on a leader that an earlier label already drew.
			constexpr float LeaderHit = 100000.f;
			if (bLeader)
			{
				const FVector2f A = Label.Pos;
				const FVector2f B(FMath::Clamp(A.X, Rect.Left, Rect.Right), FMath::Clamp(A.Y, Rect.Top, Rect.Bottom));
				for (const FSlateRect& Other : Placed)
				{
					if (SegmentHitsRect(A, B, Other))
					{
						Total += LeaderHit;
					}
				}
				for (int32 DotSlot = 0; DotSlot < DotRects.Num(); ++DotSlot)
				{
					if (DotRects[DotSlot].Key != Label.Index && DotFull[DotSlot] && SegmentHitsRect(A, B, DotRects[DotSlot].Value))
					{
						Total += LeaderHit;
					}
				}
			}
			for (const TPair<FVector2f, FVector2f>& Leader : PlacedLeaders)
			{
				if (SegmentHitsRect(Leader.Key, Leader.Value, Rect))
				{
					Total += LeaderHit;
				}
				const FVector2f End(FMath::Clamp(Label.Pos.X, Rect.Left, Rect.Right), FMath::Clamp(Label.Pos.Y, Rect.Top, Rect.Bottom));
				if (bLeader && SegmentsCross(Label.Pos, End, Leader.Key, Leader.Value))
				{
					Total += LeaderHit; // two leader lines never cross
				}
			}
			if (Total >= Cutoff)
			{
				return Total;
			}
			for (const FSlateRect& Other : Placed)
			{
				const float W = FMath::Min(Rect.Right, Other.Right) - FMath::Max(Rect.Left, Other.Left);
				const float H = FMath::Min(Rect.Bottom, Other.Bottom) - FMath::Max(Rect.Top, Other.Top);
				if (W > 0.f && H > 0.f)
				{
					Total += W * H;
				}
			}
			Total *= 10.f; // covering another label is the worst outcome
			if (Total >= Cutoff)
			{
				return Total;
			}
			for (int32 DotSlot = 0; DotSlot < DotRects.Num(); ++DotSlot)
			{
				const TPair<int32, FSlateRect>& Dot = DotRects[DotSlot];
				if (Dot.Key == Label.Index)
				{
					continue;
				}
				const float W = FMath::Min(Rect.Right, Dot.Value.Right) - FMath::Max(Rect.Left, Dot.Value.Left);
				const float H = FMath::Min(Rect.Bottom, Dot.Value.Bottom) - FMath::Max(Rect.Top, Dot.Value.Top);
				if (W > 0.f && H > 0.f)
				{
					Total += W * H * (DotFull[DotSlot] ? 10.f : 1.f); // dimmed dots are cheap to cover
				}
			}
			// A label must read as its own dot's: reject a spot where another full-strength dot's edge is nearer the pill than its own.
			{
				auto EdgeDist = [](const FVector2f& Centre, float Radius, const FSlateRect& R)
				{
					const float Dx = FMath::Max3(R.Left - Centre.X, 0.f, Centre.X - R.Right);
					const float Dy = FMath::Max3(R.Top - Centre.Y, 0.f, Centre.Y - R.Bottom);
					return FMath::Max(0.f, FMath::Sqrt(Dx * Dx + Dy * Dy) - Radius);
				};
				const float Own = EdgeDist(Label.Pos, Label.DotRadius, Rect); // a leader label is judged by its true gap too: no other dot may be nearer
				for (int32 DotSlot = 0; DotSlot < DotRects.Num(); ++DotSlot)
				{
					const FSlateRect& DotRect = DotRects[DotSlot].Value;
					if (DotRects[DotSlot].Key != Label.Index && DotFull[DotSlot]
						&& EdgeDist(DotRect.GetCenter(), DotRect.GetSize().X * 0.5f - 2.f, Rect) + 0.5f < Own) // half a pixel of slack for rounding
					{
						bAmbiguous = true;
						break;
					}
				}
			}
			// Falling outside the map area counts as overlap too.
			const float OutX = FMath::Max(0.f, -Rect.Left) + FMath::Max(0.f, Rect.Right - Size.X);
			const float OutY = FMath::Max(0.f, -Rect.Top) + FMath::Max(0.f, Rect.Bottom - Size.Y);
			return Total + (OutX * PillH + OutY * PillW) * 10.f;
		};

		int32 Chosen = INDEX_NONE;
		int32 BestAmbiguous = INDEX_NONE; // first spot whose only flaw is that another dot reads as nearer
		int32 Least = 0;
		float LeastValue = TNumericLimits<float>::Max();
		for (int32 Slot = 0; Slot < Candidates.Num(); ++Slot)
		{
			bool bAmbiguous = false;
			const float Value = Overlap(PillRect(Candidates[Slot]), Gaps[Slot] > 10.f, LeastValue, bAmbiguous);
			if (Value <= 0.f)
			{
				if (!bAmbiguous)
				{
					Chosen = Slot;
					break;
				}
				if (BestAmbiguous == INDEX_NONE && Gaps[Slot] <= 10.f) // a leader never excuses an ambiguous owner
				{
					BestAmbiguous = Slot;
				}
			}
			const float Effective = Value + (bAmbiguous ? 5000.f : 0.f);
			if (Effective < LeastValue)
			{
				LeastValue = Effective;
				Least = Slot;
			}
		}
		if (Chosen == INDEX_NONE)
		{
			if (Label.Priority == 3 && BestAmbiguous != INDEX_NONE)
			{
				Chosen = BestAmbiguous; // an interchange takes a clean-but-ambiguous spot rather than going unnamed
			}
			else if (Label.bMustShow)
			{
				Chosen = Least;
			}
			else
			{
				continue; // no free spot: skip a minor label rather than cover another
			}
		}
		const FSlateRect Rect = PillRect(Candidates[Chosen]);
		Placed.Add(Rect);
		FKioskMapPlacedLabel& Out = Layout.Labels.AddDefaulted_GetRef();
		Out.Rect = Rect;
		Out.TextPos = Label.Pos + Candidates[Chosen];
		Out.Dot = Label.Pos;
		Out.bLeader = Gaps[Chosen] > 10.f;
		Out.Anchor = FVector2f(FMath::Clamp(Label.Pos.X, Rect.Left, Rect.Right), FMath::Clamp(Label.Pos.Y, Rect.Top, Rect.Bottom));
		Out.bSelected = Label.bSelected;
		Out.Width = Label.Width;
		Out.Slack = Label.Slack;
		Out.Text = Label.Text;
		if (Out.bLeader)
		{
			PlacedLeaders.Emplace(Label.Pos, Out.Anchor);
		}
	}
	if (!bHit)
	{
		Layout.bValid = true;
		Layout.Size = Size;
		Layout.Zoom = Zoom;
		Layout.Pan = LayoutPan;
		Layout.Selected = SelectedStation;
		Layout.Filter = Filter;
		Layout.bDim = bDimOthers;
		Layout.Lit = LitStations;
		Layout.Language = LayoutLanguage;
		Layout.Scale = LayoutScale;
		Layout.WidthSum = WidthSum;
	}
	for (const FKioskMapPlacedLabel& Out : Layout.Labels)
	{
		if (Out.bLeader)
		{
			// Detached label: thin leader from the dot to the nearest point of the pill so it still reads as belonging to the dot.
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Paint, TArray<FVector2f>({ Out.Dot, Out.Anchor }),
				ESlateDrawEffect::None, C.Ink, true, 1.5f);
		}
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(Out.Rect.Right - Out.Rect.Left, Out.Rect.Bottom - Out.Rect.Top), FSlateLayoutTransform(FVector2f(Out.Rect.Left, Out.Rect.Top))), PillBrush,
			ESlateDrawEffect::None, FKioskStyle::BrushTint(PillBrush));
		MakeShapedKioskText(*ShapeCache, OutDrawElements, LayerId + 2, AllottedGeometry,
			AllottedGeometry.ToPaintGeometry(FVector2f(Out.Width, 24.f), FSlateLayoutTransform(Out.TextPos + FVector2f(Out.Slack * 0.5f, 0.f))),
			Out.Text, LabelFont, Out.bSelected ? C.Primary : C.Ink);
	}
	return LayerId + 6;
}

FReply SKioskMap::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = true;
	bDragging = false;
	DragStart = DragLast = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FReply::Handled().CaptureMouse(AsShared());
}

FReply SKioskMap::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed)
	{
		return FReply::Unhandled();
	}
	const FVector2f Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (!bDragging && FVector2f::Distance(Local, DragStart) > 10.f)
	{
		bDragging = true;
	}
	if (bDragging && Zoom > 1.01f && Session.IsValid())
	{
		Session->MapPan = ClampPan(Session->MapPan + (Local - DragLast), MyGeometry.GetLocalSize());
	}
	DragLast = Local;
	return FReply::Handled();
}

FReply SKioskMap::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bPressed || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	bPressed = false;
	const bool bWasDrag = bDragging;
	bDragging = false;
	FReply Reply = FReply::Handled().ReleaseMouseCapture();
	if (bWasDrag)
	{
		return Reply;
	}
	const FVector2f Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2f Size = MyGeometry.GetLocalSize();
	const TArray<FKioskStation>& Stations = FKioskData::Stations();

	int32 Best = INDEX_NONE;
	float BestDistance = HitRadius;
	for (int32 Index = 0; Index < Stations.Num(); ++Index)
	{
		if (Index == FKioskData::CityHallIndex())
		{
			continue; // you are already here
		}
		const float Distance = FVector2f::Distance(Local, ToLocal(Stations[Index].MapPos, Size));
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Index;
		}
	}
	if (Best != INDEX_NONE)
	{
		OnStationTapped.ExecuteIfBound(Best);
	}
	return Reply;
}

void SKioskMap::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bPressed = false;
	bDragging = false;
}

FCursorReply SKioskMap::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Hand);
}
