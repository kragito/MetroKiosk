#include "SKioskMap.h"

#include "KioskData.h"
#include "KioskAutomation.h"
#include "KioskLoc.h"
#include "KioskPlaces.h"
#include "KioskStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Rendering/DrawElements.h"

namespace
{
	const FVector2f GMapMin(40.f, 20.f);
	const FVector2f GMapMax(930.f, 580.f);
	constexpr float HitRadius = 30.f;
	constexpr float SlackDefault = 0.06f; // measured text width can undershoot the shaped width, so leave this fraction spare
	constexpr float SlackTamil = 0.14f;   // the "You are here" pill in Tamil only; its text shaped tight at 6%
	constexpr float MarkerNearGap = 12.f; // "You are here" pill stays this close to its dot unless a leader line joins them

	/** Extra width to add to a measured text width so shaped text fits; callers centre the text inside it. */
	float TextSlack(float MeasuredWidth, bool bTamilWider = false)
	{
		return MeasuredWidth * ((bTamilWider && FKioskLoc::GetLanguage() == EKioskLanguage::Tamil) ? SlackTamil : SlackDefault);
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

FVector2f SKioskMap::ToLocal(const FVector2f& MapPos, const FVector2f& LocalSize) const
{
	const FVector2f MapSize = GMapMax - GMapMin;
	const float BaseScale = FMath::Min((LocalSize.X - 90.f) / MapSize.X, (LocalSize.Y - 110.f) / MapSize.Y);
	const FVector2f Center = (GMapMin + GMapMax) * 0.5f;
	FVector2f Focus = Center;
	const TArray<FKioskStation>& Stations = FKioskData::Stations();
	if (Zoom > 1.01f && Stations.IsValidIndex(SelectedStation))
	{
		Focus = Stations[SelectedStation].MapPos;
	}
	FVector2f Pan = (Zoom > 1.01f && Session.IsValid()) ? Session->MapPan : FVector2f::ZeroVector;
	const FVector2f PanLimit = LocalSize * 0.5f * (Zoom - 0.5f);
	Pan.X = FMath::Clamp(Pan.X, -PanLimit.X, PanLimit.X);
	Pan.Y = FMath::Clamp(Pan.Y, -PanLimit.Y, PanLimit.Y);
	return LocalSize * 0.5f + Pan + (MapPos - Focus) * (BaseScale * Zoom);
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
		for (const TArray<int32>& Path : FKioskData::LinePaths(Line))
		{
			TArray<FVector2f> Points;
			for (int32 StationIndex : Path)
			{
				Points.Add(ToLocal(Stations[StationIndex].MapPos, Size));
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
		// The default spot is kept unless it hides a station dot's centre (so languages that fit today do not move).
		auto HidesDotCentre = [&](const FVector2f& Off)
		{
			const FVector2f Try = HerePos + Off;
			const FSlateRect Inner(Try.X + 6.f, Try.Y + 6.f, Try.X + BoxSize.X - 6.f, Try.Y + BoxSize.Y - 6.f);
			for (const TPair<int32, FSlateRect>& Dot : DotRects)
			{
				if (Dot.Key != HereIndex && Inner.ContainsPoint(Dot.Value.GetCenter()))
				{
					return true;
				}
			}
			return false;
		};
		FVector2f BoxPos = HerePos + Default;
		bool bLeader = false;
		if (HidesDotCentre(Default))
		{
			// Candidate offsets: a grid around the dot, nearest to the default first. Built only when the default is blocked.
			TArray<FVector2f> Offsets;
			for (float Dy = -90.f; Dy <= 60.f; Dy += 6.f)
			{
				for (float Dx = -BoxSize.X - HereDraw - 90.f; Dx <= HereDraw + 90.f; Dx += 6.f)
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
				const FSlateRect R = BoxRect(Off);
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
			// 2: last resort when nothing above exists: touches no dot, but may leave the widget or cross a dot with its leader.
			// A neighbour dot can be closer to the pill than City Hall's own dot; the leader line is what ties them together.
			bool bFound = false;
			for (int32 Tier = 0; Tier < 3 && !bFound; ++Tier)
			{
				for (const FVector2f& Off : Offsets)
				{
					const float Gap = DistToBox(Off, HerePos) - HereDraw;
					if (TouchesDot(Off) || (Tier < 2 && !InsideWidget(Off)) || (Tier == 0 && Gap > MarkerNearGap)
						|| (Tier < 2 && Gap > MarkerNearGap && LeaderCrossesDot(Off)))
					{
						continue;
					}
					BoxPos = HerePos + Off;
					bLeader = Gap > MarkerNearGap;
					bFound = true;
					break;
				}
			}
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
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 5,
			AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(BoxPos + FVector2f(11.f + Slack * 0.5f, 5.f))),
			Text, MarkerFont, ESlateDrawEffect::None, C.White);
	}

	// Label collision avoidance: highest priority first (stable by station index), first free candidate wins.
	Pending.StableSort([](const FPendingLabel& A, const FPendingLabel& B) { return A.Priority < B.Priority; });
	const FSlateBrush* PillBrush = Style.Round(C.MapLabelBg, 5.f);
	TArray<TPair<FVector2f, FVector2f>> PlacedLeaders; // leader segments already drawn (dot centre to pill edge)
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
	for (const FPendingLabel& Label : Pending)
	{
		const float PillW = Label.Width + 8.f;
		constexpr float PillH = 24.f;
		const float R = Label.DotRadius;
		// Candidate text-origin offsets from the dot centre; the pill is the text box grown by (4, 2).
		// Order: right/left/above/below at the normal gap, then (priority labels only) the four diagonals,
		// then right/left/above/below again at +14 and +28 px with a leader line back to the dot.
		TArray<FVector2f, TInlineAllocator<16>> Candidates;
		TArray<float, TInlineAllocator<16>> Gaps;
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
		if (Label.bMustShow)
		{
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
			AddSides(14.f);
			AddSides(28.f);
		}

		auto PillRect = [&](const FVector2f& Offset)
		{
			const FVector2f Min = Label.Pos + Offset + FVector2f(-4.f, -2.f);
			return FSlateRect(Min.X, Min.Y, Min.X + PillW, Min.Y + PillH);
		};
		auto Overlap = [&](const FSlateRect& Rect, bool bLeader)
		{
			float Total = 0.f;
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
			// Falling outside the map area counts as overlap too.
			const float OutX = FMath::Max(0.f, -Rect.Left) + FMath::Max(0.f, Rect.Right - Size.X);
			const float OutY = FMath::Max(0.f, -Rect.Top) + FMath::Max(0.f, Rect.Bottom - Size.Y);
			return Total + (OutX * PillH + OutY * PillW) * 10.f;
		};

		int32 Chosen = INDEX_NONE;
		int32 Least = 0;
		float LeastValue = TNumericLimits<float>::Max();
		for (int32 Slot = 0; Slot < Candidates.Num(); ++Slot)
		{
			const float Value = Overlap(PillRect(Candidates[Slot]), Gaps[Slot] > 10.f);
			if (Value <= 0.f)
			{
				Chosen = Slot;
				break;
			}
			if (Value < LeastValue)
			{
				LeastValue = Value;
				Least = Slot;
			}
		}
		if (Chosen == INDEX_NONE)
		{
			if (!Label.bMustShow)
			{
				continue; // no free spot: skip a minor label rather than cover another
			}
			Chosen = Least;
		}
		const FSlateRect Rect = PillRect(Candidates[Chosen]);
		Placed.Add(Rect);
		const FVector2f LabelPos = Label.Pos + Candidates[Chosen];
		if (Gaps[Chosen] > 10.f)
		{
			// Detached label: thin leader from the dot to the nearest point of the pill so it still reads as belonging to the dot.
			const FVector2f Anchor(FMath::Clamp(Label.Pos.X, Rect.Left, Rect.Right), FMath::Clamp(Label.Pos.Y, Rect.Top, Rect.Bottom));
			PlacedLeaders.Emplace(Label.Pos, Anchor);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Paint, TArray<FVector2f>({ Label.Pos, Anchor }),
				ESlateDrawEffect::None, C.Ink, true, 1.5f);
		}
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(PillW, PillH), FSlateLayoutTransform(FVector2f(Rect.Left, Rect.Top))), PillBrush,
			ESlateDrawEffect::None, FKioskStyle::BrushTint(PillBrush));
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2f(Label.Width, 24.f), FSlateLayoutTransform(LabelPos + FVector2f(Label.Slack * 0.5f, 0.f))),
			Label.Text, LabelFont, ESlateDrawEffect::None, Label.bSelected ? C.Primary : C.Ink);
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
		const FVector2f Limit = MyGeometry.GetLocalSize() * 0.5f * (Zoom - 0.5f);
		FVector2f Pan = Session->MapPan + (Local - DragLast);
		Pan.X = FMath::Clamp(Pan.X, -Limit.X, Limit.X);
		Pan.Y = FMath::Clamp(Pan.Y, -Limit.Y, Limit.Y);
		Session->MapPan = Pan;
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
