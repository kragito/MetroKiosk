// The kiosk cabinet placed in the concourse level: screen, reader, slots, card pad and collection tray as editable components.
#include "KioskCabinet.h"

#include "Components/WidgetComponent.h"
#include "KioskBlock.h"
#include "KioskStyle.h"
#include "Math/RotationMatrix.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SKioskRoot.h"
#include "Widgets/SNullWidget.h"

namespace
{
	// Default layout in centimetres. The kiosk screen faces +X. A placed instance in the level may override any of it.
	constexpr float ScreenWidthCm = 100.f;
	constexpr float ScreenCentreZ = 129.f;
	constexpr float ScreenCentreY = 25.f;  // the screen sits left of centre, the hardware column to its right (as in the Figma hardware frame)
	constexpr float TrayFloorZ = 68.f;     // top of the collection tray floor
	constexpr int32 MaxChangePieces = 6;   // notes and coins drawn for the change
}

AKioskCabinet::AKioskCabinet()
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Cabinet: front face at X = 0, standing against the wall. Y runs to the viewer's left, so the hardware column is at negative Y (their right).
	AddBlock(TEXT("KioskCabinet"), false, FVector(-30.f, 0.f, 95.f), FVector(60.f, 164.f, 190.f), TEXT("Brand"));
	AddBlock(TEXT("KioskSign"), false, FVector(-29.f, 0.f, 203.f), FVector(62.f, 164.f, 26.f), TEXT("Black"));
	AddBlock(TEXT("KioskSignLogo"), false, FVector(2.5f, 66.f, 203.f), FVector(1.f, 16.f, 16.f), TEXT("Logo"));
	AddBlock(TEXT("KioskSignTicket"), false, FVector(2.5f, -44.f, 203.f), FVector(1.f, 18.f, 18.f), TEXT("Primary"));
	AddBlock(TEXT("KioskSignHelp"), false, FVector(2.5f, -66.f, 203.f), FVector(1.f, 18.f, 18.f), TEXT("Help"));
	AddBlock(TEXT("KioskToeSpace"), false, FVector(-25.f, 0.f, 3.f), FVector(50.f, 156.f, 6.f), TEXT("Ink"));

	// Screen bezel (just behind the screen plane) and the screen face: the live kiosk UI, hosted in a world-space widget component sized to 1 m wide.
	AddBlock(TEXT("KioskBezel"), false, FVector(0.2f, ScreenCentreY, ScreenCentreZ), FVector(0.4f, 106.f, 66.f), TEXT("Black"));
	Screen = CreateDefaultSubobject<UWidgetComponent>(TEXT("KioskScreen"));
	Screen->SetMobility(EComponentMobility::Movable);
	Screen->SetupAttachment(RootComponent);
	Screen->SetRelativeLocation(FVector(0.6f, ScreenCentreY, ScreenCentreZ));
	const float PixelToCm = ScreenWidthCm / SKioskRoot::CanvasWidth;
	Screen->SetRelativeScale3D(FVector(1.f, PixelToCm, PixelToCm));
	Screen->SetWidgetSpace(EWidgetSpace::World);
	Screen->SetDrawSize(FVector2D(SKioskRoot::CanvasWidth, SKioskRoot::CanvasHeight));
	Screen->SetPivot(FVector2D(0.5f, 0.5f));
	Screen->SetBlendMode(EWidgetBlendMode::Opaque);
	Screen->SetTwoSided(false);
	Screen->SetCastShadow(false);

	// Hardware column, to the viewer's right of the screen (Figma parts 5, 6, 7).
	AddBlock(TEXT("KioskPanel"), false, FVector(1.f, -54.f, 112.f), FVector(2.f, 44.f, 108.f), TEXT("Surface"));

	// Contactless reader.
	AddPart(EKioskPart::Contactless, TEXT("PartContactless"), false, FVector(3.5f, -54.f, 146.f), FVector(3.f, 32.f, 24.f), TEXT("Ink"));
	UKioskBlockComponent* ContactlessMark = AddBlock(TEXT("ContactlessMark"), false, FVector(5.2f, -54.f, 146.f), FVector(0.4f, 12.f, 12.f), TEXT("OnBrandSoft"));
	AddLightBar(EKioskPart::Contactless, TEXT("LightBarContactless"), FVector(2.5f, -54.f, 161.f), FVector(1.f, 16.f, 1.6f));

	// Chip card / NETS slot.
	AddPart(EKioskPart::ChipSlot, TEXT("PartChipSlot"), false, FVector(3.5f, -54.f, 118.f), FVector(3.f, 32.f, 20.f), TEXT("DeviceDark"));
	UKioskBlockComponent* ChipSlit = AddBlock(TEXT("ChipSlotSlit"), false, FVector(5.2f, -54.f, 123.f), FVector(0.4f, 22.f, 1.6f), TEXT("Chip"));
	AddLightBar(EKioskPart::ChipSlot, TEXT("LightBarChipSlot"), FVector(2.5f, -54.f, 131.f), FVector(1.f, 16.f, 1.6f));

	// Note slot and coin slot on one plate.
	AddBlock(TEXT("CashPlate"), false, FVector(3.f, -55.f, 90.f), FVector(2.f, 42.f, 14.f), TEXT("Ink"));
	AddPart(EKioskPart::NoteSlot, TEXT("PartNoteSlot"), false, FVector(3.3f, -61.f, 90.f), FVector(2.6f, 22.f, 4.f), TEXT("Black"));
	AddPart(EKioskPart::CoinSlot, TEXT("PartCoinSlot"), false, FVector(3.3f, -43.f, 90.f), FVector(2.6f, 6.f, 10.f), TEXT("Black"));
	AddLightBar(EKioskPart::NoteSlot, TEXT("LightBarNoteSlot"), FVector(2.5f, -61.f, 99.f), FVector(1.f, 14.f, 1.6f));
	AddLightBar(EKioskPart::CoinSlot, TEXT("LightBarCoinSlot"), FVector(2.5f, -43.f, 99.f), FVector(1.f, 8.f, 1.6f));

	// Metro card pad: a shallow round bowl under the screen with a light ring; the Metro card rests on top.
	LightBars.Add(EKioskPart::CardPad, AddBlock(TEXT("LightBarCardPad"), true, FVector(10.f, -8.f, 82.6f), FVector(27.f, 27.f, 1.f), TEXT("Muted")));
	AddPart(EKioskPart::CardPad, TEXT("PartCardPad"), true, FVector(10.f, -8.f, 84.f), FVector(24.f, 24.f, 3.f), TEXT("Surface"));
	CardOnPad = AddBlock(TEXT("MetroCardOnPad"), false, FVector(10.f, -8.f, 86.2f), FVector(8.5f, 5.4f, 0.4f), TEXT("Help"));
	CardOnPad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardOnPad->SetVisibility(false);

	// Ticket and receipt slot above the collection tray.
	AddBlock(TEXT("TicketPlate"), false, FVector(1.f, 52.f, 87.f), FVector(2.f, 44.f, 13.f), TEXT("Ink"));
	AddPart(EKioskPart::TicketSlot, TEXT("PartTicketSlot"), false, FVector(3.3f, 52.f, 87.f), FVector(2.6f, 34.f, 2.5f), TEXT("Black"));
	AddLightBar(EKioskPart::TicketSlot, TEXT("LightBarTicketSlot"), FVector(2.5f, 52.f, 92.f), FVector(1.f, 14.f, 1.2f));

	// Collection tray: floor, side walls and a front lip; the floor is the clickable part.
	AddPart(EKioskPart::ChangeTray, TEXT("PartChangeTray"), false, FVector(10.f, 52.f, TrayFloorZ - 2.f), FVector(20.f, 44.f, 4.f), TEXT("BorderStrong"));
	AddBlock(TEXT("TrayWallA"), false, FVector(10.f, 30.f, TrayFloorZ + 2.f), FVector(20.f, 2.f, 8.f), TEXT("Ink"));
	AddBlock(TEXT("TrayWallB"), false, FVector(10.f, 74.f, TrayFloorZ + 2.f), FVector(20.f, 2.f, 8.f), TEXT("Ink"));
	AddBlock(TEXT("TrayLip"), false, FVector(19.f, 52.f, TrayFloorZ + 0.5f), FVector(2.f, 44.f, 5.f), TEXT("Ink"));
	AddLightBar(EKioskPart::ChangeTray, TEXT("LightBarChangeTray"), FVector(2.5f, 52.f, 77.f), FVector(1.f, 14.f, 1.2f));

	for (UKioskBlockComponent* Decoration : { ContactlessMark, ChipSlit })
	{
		Decoration->SetCollisionEnabled(ECollisionEnabled::NoCollision); // lies on a part: clicks must reach the part behind it
	}

	// Things that appear in the tray: ticket, receipt, change (hidden until the game shows them).
	const float ItemZ = TrayFloorZ + 0.3f;
	TicketMesh = AddBlock(TEXT("TrayTicket"), false, FVector(9.f, 60.f, ItemZ), FVector(7.f, 15.f, 0.6f), TEXT("PrimaryChip"));
	TicketMesh->SetRelativeRotation(FRotator(0.f, 8.f, 0.f));
	ReceiptMesh = AddBlock(TEXT("TrayReceipt"), false, FVector(10.f, 41.f, ItemZ), FVector(5.f, 9.f, 0.5f), TEXT("White"));
	ReceiptMesh->SetRelativeRotation(FRotator(0.f, -12.f, 0.f));
	for (UKioskBlockComponent* Item : { TicketMesh, ReceiptMesh })
	{
		Item->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Item->SetVisibility(false);
	}
	for (int32 Slot = 0; Slot < MaxChangePieces; ++Slot)
	{
		const FVector Spot(16.f, 34.f + Slot * 6.f, ItemZ);
		UKioskBlockComponent* Coin = AddBlock(*FString::Printf(TEXT("TrayCoin%d"), Slot), true, Spot, FVector(3.4f, 3.4f, 0.5f), TEXT("Chip"));
		UKioskBlockComponent* Note = AddBlock(*FString::Printf(TEXT("TrayNote%d"), Slot), false, Spot, FVector(4.f, 5.5f, 0.3f), TEXT("Primary"));
		for (UKioskBlockComponent* Piece : { Coin, Note })
		{
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Piece->SetVisibility(false);
		}
		ChangeCoins.Add(Coin);
		ChangeNotes.Add(Note);
	}
}

UKioskBlockComponent* AKioskCabinet::AddBlock(const TCHAR* Name, bool bCylinder, const FVector& Centre, const FVector& SizeCm, const TCHAR* Token)
{
	UKioskBlockComponent* Block = CreateDefaultSubobject<UKioskBlockComponent>(Name);
	Block->SetupAttachment(RootComponent);
	Block->SetRelativeLocation(Centre);
	Block->SetRelativeScale3D(SizeCm / 100.f); // basic shapes are 100 cm
	Block->Shape = bCylinder ? EKioskShape::Cylinder : EKioskShape::Cube;
	Block->ColourToken = Token;
	Blocks.Add(Block);
	return Block;
}

UKioskBlockComponent* AKioskCabinet::AddPart(EKioskPart Part, const TCHAR* Name, bool bCylinder, const FVector& Centre, const FVector& SizeCm, const TCHAR* Token)
{
	UKioskBlockComponent* Block = AddBlock(Name, bCylinder, Centre, SizeCm, Token);
	PartBlocks.Add(Part, Block);
	return Block;
}

void AKioskCabinet::AddLightBar(EKioskPart Part, const TCHAR* Name, const FVector& Centre, const FVector& SizeCm)
{
	LightBars.Add(Part, AddBlock(Name, false, Centre, SizeCm, TEXT("Muted")));
}

void AKioskCabinet::BeginPlay()
{
	Super::BeginPlay();
	ScreenHost = SNew(SBox);
	Screen->SetSlateWidget(ScreenHost);
	// The card, ticket, receipt and change pieces always start hidden, whatever was saved in the level.
	SetCardOnPad(false);
	SetTrayContents(false, false, 0);
}

void AKioskCabinet::RefreshPartColour(EKioskPart Part)
{
	const FKioskColors& C = KC();
	if (UKioskBlockComponent* const* Block = PartBlocks.Find(Part))
	{
		if (Part == HoverPart)
		{
			(*Block)->SetColourOverride(C.AmberStroke);
		}
		else
		{
			(*Block)->ClearColourOverride();
		}
	}
	if (UKioskBlockComponent* const* Bar = LightBars.Find(Part))
	{
		(*Bar)->SetColourOverride(LitParts.Contains(Part) ? C.Primary : C.Muted);
	}
}

EKioskPart AKioskCabinet::PartForComponent(const UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return EKioskPart::None;
	}
	if (Component == Screen.Get())
	{
		return EKioskPart::Screen;
	}
	for (const TPair<EKioskPart, UKioskBlockComponent*>& Pair : PartBlocks)
	{
		if (Pair.Value == Component)
		{
			return Pair.Key;
		}
	}
	return EKioskPart::None;
}

FVector AKioskCabinet::GetPartLocation(EKioskPart Part) const
{
	if (Part == EKioskPart::Screen)
	{
		return GetScreenCenter();
	}
	UKioskBlockComponent* const* Block = PartBlocks.Find(Part);
	if (!Block || !*Block)
	{
		return GetActorLocation();
	}
	// The tray is seen from above over its front lip: aim at the top of its floor.
	const FVector Offset = Part == EKioskPart::ChangeTray ? GetActorTransform().TransformVector(FVector(-2.f, 0.f, 2.1f)) : FVector::ZeroVector;
	return (*Block)->GetComponentLocation() + Offset;
}

void AKioskCabinet::SetPartHighlight(EKioskPart Part)
{
	if (Part == HoverPart)
	{
		return;
	}
	const EKioskPart Old = HoverPart;
	HoverPart = Part;
	RefreshPartColour(Old);
	RefreshPartColour(Part);
}

void AKioskCabinet::SetPartLight(EKioskPart Part, bool bOn)
{
	if (LitParts.Contains(Part) == bOn)
	{
		return;
	}
	if (bOn)
	{
		LitParts.Add(Part);
	}
	else
	{
		LitParts.Remove(Part);
	}
	RefreshPartColour(Part);
}

void AKioskCabinet::SetCardOnPad(bool bOnPad)
{
	bCardOnPad = bOnPad;
	if (CardOnPad)
	{
		CardOnPad->SetVisibility(bOnPad);
	}
}

void AKioskCabinet::SetTrayContents(bool bTicket, bool bReceipt, int32 ChangeCents)
{
	if (TicketMesh)
	{
		TicketMesh->SetVisibility(bTicket);
	}
	if (ReceiptMesh)
	{
		ReceiptMesh->SetVisibility(bReceipt);
	}
	// Greedy change: the biggest notes and coins first, at most MaxChangePieces drawn.
	static const int32 Values[] = { 5000, 1000, 500, 200, 100, 50, 20, 10 };
	int32 Remaining = FMath::Max(0, ChangeCents);
	int32 Slot = 0;
	for (int32 Value : Values)
	{
		while (Remaining >= Value && Slot < MaxChangePieces && ChangeNotes.IsValidIndex(Slot))
		{
			const bool bNote = Value >= 200;
			ChangeNotes[Slot]->SetVisibility(bNote);
			ChangeCoins[Slot]->SetVisibility(!bNote);
			Remaining -= Value;
			++Slot;
		}
	}
	for (; Slot < MaxChangePieces && ChangeNotes.IsValidIndex(Slot); ++Slot)
	{
		ChangeNotes[Slot]->SetVisibility(false);
		ChangeCoins[Slot]->SetVisibility(false);
	}
}

void AKioskCabinet::SetScreenContent(const TSharedRef<SWidget>& Content)
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(Content);
	}
}

void AKioskCabinet::ClearScreenContent()
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(SNullWidget::NullWidget);
	}
}

FVector AKioskCabinet::GetScreenCenter() const
{
	return Screen ? Screen->GetComponentLocation() : GetActorLocation();
}

FVector AKioskCabinet::CanvasToWorld(const FVector2D& Pixel, float SignU, float SignV) const
{
	if (!Screen)
	{
		return GetScreenCenter();
	}
	const FVector Local(0.f, SignU * (Pixel.X - SKioskRoot::CanvasWidth * 0.5f), SignV * (SKioskRoot::CanvasHeight * 0.5f - Pixel.Y));
	return Screen->GetComponentTransform().TransformPosition(Local);
}

bool AKioskCabinet::ScreenContentIs(const TSharedRef<SWidget>& Widget) const
{
	return ScreenHost.IsValid() && &ScreenHost->GetChildren()->GetChildAt(0).Get() == &Widget.Get();
}

FTransform AKioskCabinet::GetLockOnView() const
{
	// Square on, 1 m from the face, at the middle of the screen's height: the screen, the hardware column and the tray all fit with a 90 degree lens.
	return FTransform(GetActorQuat() * FRotator(0.f, 180.f, 0.f).Quaternion(), GetActorTransform().TransformPosition(FVector(115.f, 0.f, 105.f)));
}

FTransform AKioskCabinet::GetHardwareShotView() const
{
	// Slightly from the viewer's left and above: the card pad, reader column and collection tray in one frame.
	const FVector Eye = GetActorTransform().TransformPosition(FVector(165.f, 70.f, 135.f));
	const FVector Target = GetActorTransform().TransformPosition(FVector(0.f, -10.f, 100.f));
	return FTransform(FRotationMatrix::MakeFromX(Target - Eye).Rotator(), Eye);
}
