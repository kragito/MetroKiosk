// Builds the concourse (floor, walls, ceiling lights, platform edge, benches, pillars) and the kiosk cabinet from /Engine/BasicShapes.
#include "KioskEnvironment.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "KioskStyle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SKioskRoot.h"
#include "Widgets/SNullWidget.h"

namespace
{
	// Layout in centimetres. The kiosk screen faces +X; the concourse runs along +X from the kiosk wall to the platform edge.
	constexpr float XMin = -110.f;
	constexpr float XMax = 1520.f;
	constexpr float PlatformEdgeX = 1100.f;
	constexpr float HalfWidth = 820.f;
	constexpr float WallHeight = 420.f;
	constexpr float ScreenWidthCm = 100.f;
	constexpr float ScreenCentreZ = 129.f;
	constexpr float ScreenCentreY = 25.f;  // the screen sits left of centre, the hardware column to its right (as in the Figma hardware frame)
	constexpr float TrayFloorZ = 68.f;     // top of the collection tray floor
	constexpr int32 MaxChangePieces = 6;   // notes and coins drawn for the change
}

AKioskEnvironment::AKioskEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = CubeFinder.Object;
	CylinderMesh = CylinderFinder.Object;
	BaseMaterial = MaterialFinder.Object;
}

UStaticMeshComponent* AKioskEnvironment::AddBlock(UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour)
{
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, Name);
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetStaticMesh(Mesh);
	Block->SetupAttachment(RootComponent);
	Block->SetRelativeLocation(Centre);
	Block->SetRelativeScale3D(SizeCm / 100.f); // basic shapes are 100 cm
	Block->SetCastShadow(false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), Colour);
		Block->SetMaterial(0, Material);
	}
	Block->RegisterComponent();
	return Block;
}

void AKioskEnvironment::AddLight(const FName& Name, const FVector& Location)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, Name);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(Location);
	Light->SetIntensity(400.f);
	Light->SetAttenuationRadius(900.f);
	Light->SetLightColor(FLinearColor(1.f, 0.97f, 0.92f));
	Light->SetCastShadows(false);
	Light->RegisterComponent();
}

void AKioskEnvironment::Build()
{
	if (!CubeMesh || !CylinderMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Kiosk3D: engine basic shapes are missing; the scene is empty"));
		return;
	}
	const FKioskColors& C = KC();
	const float Length = XMax - XMin;
	const float MidX = (XMax + XMin) * 0.5f;
	const float FloorLength = PlatformEdgeX - XMin;
	const float FloorMidX = (PlatformEdgeX + XMin) * 0.5f;
	const float FullWidth = HalfWidth * 2.f;

	// Shell.
	AddBlock(CubeMesh, TEXT("Floor"), FVector(FloorMidX, 0.f, -10.f), FVector(FloorLength, FullWidth, 20.f), C.Muted);
	AddBlock(CubeMesh, TEXT("Ceiling"), FVector(MidX, 0.f, WallHeight + 10.f), FVector(Length, FullWidth, 20.f), C.Surface);
	AddBlock(CubeMesh, TEXT("KioskWall"), FVector(XMin + 10.f, 0.f, WallHeight * 0.5f), FVector(20.f, FullWidth, WallHeight), C.Bg);
	AddBlock(CubeMesh, TEXT("FarWall"), FVector(XMax - 10.f, 0.f, WallHeight * 0.5f), FVector(20.f, FullWidth, WallHeight), C.BorderStrong);
	AddBlock(CubeMesh, TEXT("WallLeft"), FVector(MidX, -HalfWidth - 10.f, WallHeight * 0.5f), FVector(Length, 20.f, WallHeight), C.Bg);
	AddBlock(CubeMesh, TEXT("WallRight"), FVector(MidX, HalfWidth + 10.f, WallHeight * 0.5f), FVector(Length, 20.f, WallHeight), C.Bg);

	// Platform edge: tactile yellow strip, a railing, and the track bed below.
	const float TrackLength = XMax - PlatformEdgeX;
	const float TrackMidX = (PlatformEdgeX + XMax) * 0.5f;
	AddBlock(CubeMesh, TEXT("EdgeStrip"), FVector(PlatformEdgeX - 30.f, 0.f, 1.f), FVector(40.f, FullWidth, 2.f), C.Chip);
	AddBlock(CubeMesh, TEXT("TrackBed"), FVector(TrackMidX, 0.f, -110.f), FVector(TrackLength, FullWidth, 20.f), C.Ink);
	AddBlock(CubeMesh, TEXT("RailA"), FVector(TrackMidX, -70.f, -96.f), FVector(TrackLength, 6.f, 4.f), C.Muted);
	AddBlock(CubeMesh, TEXT("RailB"), FVector(TrackMidX, 70.f, -96.f), FVector(TrackLength, 6.f, 4.f), C.Muted);
	AddBlock(CubeMesh, TEXT("RailingBar"), FVector(PlatformEdgeX, 0.f, 105.f), FVector(6.f, FullWidth, 6.f), C.Help);
	for (int32 Index = 0; Index <= 8; ++Index)
	{
		const float Y = -HalfWidth + FullWidth * Index / 8.f;
		AddBlock(CylinderMesh, *FString::Printf(TEXT("RailingPost%d"), Index), FVector(PlatformEdgeX, Y, 52.f), FVector(8.f, 8.f, 104.f), C.Help);
	}

	// Pillars and benches.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Y = Side == 0 ? -420.f : 420.f;
		AddBlock(CylinderMesh, *FString::Printf(TEXT("Pillar%d"), Side), FVector(550.f, Y, WallHeight * 0.5f), FVector(60.f, 60.f, WallHeight), C.Surface);
		AddBlock(CubeMesh, *FString::Printf(TEXT("Bench%d"), Side), FVector(780.f, Y * 0.6f, 22.f), FVector(50.f, 180.f, 45.f), C.Brand);
	}

	// Ceiling light panels with a point light under each.
	int32 LightIndex = 0;
	for (float X : { 200.f, 600.f, 1000.f })
	{
		for (float Y : { -300.f, 300.f })
		{
			AddBlock(CubeMesh, *FString::Printf(TEXT("LightPanel%d"), LightIndex), FVector(X, Y, WallHeight - 2.f), FVector(120.f, 60.f, 4.f), C.White);
			AddLight(*FString::Printf(TEXT("Light%d"), LightIndex), FVector(X, Y, WallHeight - 40.f));
			++LightIndex;
		}
	}

	// Three weak shadowless fills (two from opposite sides, one up at the ceiling) so no wall, pillar or ceiling face is pure black.
	const FRotator FillAngles[3] = { FRotator(-50.f, 210.f, 0.f), FRotator(-35.f, 40.f, 0.f), FRotator(60.f, 120.f, 0.f) }; // the last one shines up onto the ceiling
	const float FillLux[3] = { 2.5f, 1.5f, 1.5f };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UDirectionalLightComponent* Fill = NewObject<UDirectionalLightComponent>(this, *FString::Printf(TEXT("Fill%d"), Index));
		Fill->SetMobility(EComponentMobility::Movable);
		Fill->SetupAttachment(RootComponent);
		Fill->SetRelativeRotation(FillAngles[Index]);
		Fill->SetIntensity(FillLux[Index]);
		Fill->SetCastShadows(false);
		Fill->ForwardShadingPriority = 3 - Index; // one clear winner, no engine warning
		Fill->RegisterComponent();
	}

	BuildKiosk();
}

void AKioskEnvironment::BuildKiosk()
{
	const FKioskColors& C = KC();

	// Cabinet: front face at X = 0, standing against the wall. Y runs to the viewer's left, so the hardware column is at negative Y (their right).
	AddBlock(CubeMesh, TEXT("KioskCabinet"), FVector(-30.f, 0.f, 95.f), FVector(60.f, 164.f, 190.f), C.Brand);
	AddBlock(CubeMesh, TEXT("KioskSign"), FVector(-29.f, 0.f, 203.f), FVector(62.f, 164.f, 26.f), C.Black);
	AddBlock(CubeMesh, TEXT("KioskSignLogo"), FVector(2.5f, 66.f, 203.f), FVector(1.f, 16.f, 16.f), C.Logo);
	AddBlock(CubeMesh, TEXT("KioskSignTicket"), FVector(2.5f, -44.f, 203.f), FVector(1.f, 18.f, 18.f), C.Primary);
	AddBlock(CubeMesh, TEXT("KioskSignHelp"), FVector(2.5f, -66.f, 203.f), FVector(1.f, 18.f, 18.f), C.Help);
	AddBlock(CubeMesh, TEXT("KioskToeSpace"), FVector(-25.f, 0.f, 3.f), FVector(50.f, 156.f, 6.f), C.Ink);

	// Screen bezel (just behind the screen plane) and the screen face: the live kiosk UI, hosted in a world-space widget component sized to 1 m wide.
	AddBlock(CubeMesh, TEXT("KioskBezel"), FVector(0.2f, ScreenCentreY, ScreenCentreZ), FVector(0.4f, 106.f, 66.f), C.Black);
	ScreenHost = SNew(SBox);
	Screen = NewObject<UWidgetComponent>(this, TEXT("KioskScreen"));
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
	Screen->RegisterComponent();
	Screen->SetSlateWidget(ScreenHost);

	// Hardware column, to the viewer's right of the screen (Figma parts 5, 6, 7).
	AddBlock(CubeMesh, TEXT("KioskPanel"), FVector(1.f, -54.f, 112.f), FVector(2.f, 44.f, 108.f), C.Surface);

	// Contactless reader.
	AddPart(EKioskPart::Contactless, CubeMesh, TEXT("PartContactless"), FVector(3.5f, -54.f, 146.f), FVector(3.f, 32.f, 24.f), C.Ink);
	UStaticMeshComponent* ContactlessMark = AddBlock(CubeMesh, TEXT("ContactlessMark"), FVector(5.2f, -54.f, 146.f), FVector(0.4f, 12.f, 12.f), C.OnBrandSoft);
	AddLightBar(EKioskPart::Contactless, FVector(2.5f, -54.f, 161.f), FVector(1.f, 16.f, 1.6f));

	// Chip card / NETS slot.
	AddPart(EKioskPart::ChipSlot, CubeMesh, TEXT("PartChipSlot"), FVector(3.5f, -54.f, 118.f), FVector(3.f, 32.f, 20.f), C.DeviceDark);
	UStaticMeshComponent* ChipSlit = AddBlock(CubeMesh, TEXT("ChipSlotSlit"), FVector(5.2f, -54.f, 123.f), FVector(0.4f, 22.f, 1.6f), C.Chip);
	AddLightBar(EKioskPart::ChipSlot, FVector(2.5f, -54.f, 131.f), FVector(1.f, 16.f, 1.6f));

	// Note slot and coin slot on one plate.
	AddBlock(CubeMesh, TEXT("CashPlate"), FVector(3.f, -55.f, 90.f), FVector(2.f, 42.f, 14.f), C.Ink);
	AddPart(EKioskPart::NoteSlot, CubeMesh, TEXT("PartNoteSlot"), FVector(3.3f, -61.f, 90.f), FVector(2.6f, 22.f, 4.f), C.Black);
	AddPart(EKioskPart::CoinSlot, CubeMesh, TEXT("PartCoinSlot"), FVector(3.3f, -43.f, 90.f), FVector(2.6f, 6.f, 10.f), C.Black);
	AddLightBar(EKioskPart::NoteSlot, FVector(2.5f, -61.f, 99.f), FVector(1.f, 14.f, 1.6f));
	AddLightBar(EKioskPart::CoinSlot, FVector(2.5f, -43.f, 99.f), FVector(1.f, 8.f, 1.6f));

	// Metro card pad: a shallow round bowl under the screen with a light ring; the Metro card rests on top.
	UStaticMeshComponent* PadRing = AddBlock(CylinderMesh, TEXT("CardPadRing"), FVector(10.f, -8.f, 82.6f), FVector(27.f, 27.f, 1.f), C.Muted);
	LightBars.Add(EKioskPart::CardPad, PadRing);
	AddPart(EKioskPart::CardPad, CylinderMesh, TEXT("PartCardPad"), FVector(10.f, -8.f, 84.f), FVector(24.f, 24.f, 3.f), C.Surface);
	CardOnPad = AddBlock(CubeMesh, TEXT("MetroCardOnPad"), FVector(10.f, -8.f, 86.2f), FVector(8.5f, 5.4f, 0.4f), C.Help);
	CardOnPad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardOnPad->SetVisibility(false);

	// Ticket and receipt slot above the collection tray.
	AddBlock(CubeMesh, TEXT("TicketPlate"), FVector(1.f, 52.f, 87.f), FVector(2.f, 44.f, 13.f), C.Ink);
	AddPart(EKioskPart::TicketSlot, CubeMesh, TEXT("PartTicketSlot"), FVector(3.3f, 52.f, 87.f), FVector(2.6f, 34.f, 2.5f), C.Black);
	AddLightBar(EKioskPart::TicketSlot, FVector(2.5f, 52.f, 92.f), FVector(1.f, 14.f, 1.2f));

	// Collection tray: floor, side walls and a front lip; the floor is the clickable part.
	AddPart(EKioskPart::ChangeTray, CubeMesh, TEXT("PartChangeTray"), FVector(10.f, 52.f, TrayFloorZ - 2.f), FVector(20.f, 44.f, 4.f), C.BorderStrong);
	AddBlock(CubeMesh, TEXT("TrayWallA"), FVector(10.f, 30.f, TrayFloorZ + 2.f), FVector(20.f, 2.f, 8.f), C.Ink);
	AddBlock(CubeMesh, TEXT("TrayWallB"), FVector(10.f, 74.f, TrayFloorZ + 2.f), FVector(20.f, 2.f, 8.f), C.Ink);
	AddBlock(CubeMesh, TEXT("TrayLip"), FVector(19.f, 52.f, TrayFloorZ + 0.5f), FVector(2.f, 44.f, 5.f), C.Ink);
	AddLightBar(EKioskPart::ChangeTray, FVector(2.5f, 52.f, 77.f), FVector(1.f, 14.f, 1.2f));

	for (UStaticMeshComponent* Decoration : { ContactlessMark, ChipSlit })
	{
		Decoration->SetCollisionEnabled(ECollisionEnabled::NoCollision); // lies on a part: clicks must reach the part behind it
	}

	BuildTrayItems();
}

UStaticMeshComponent* AKioskEnvironment::AddPart(EKioskPart Part, UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour)
{
	UStaticMeshComponent* Block = AddBlock(Mesh, Name, Centre, SizeCm, Colour);
	PartBlocks.Add(Part, Block);
	PartBaseColours.Add(Part, Colour);
	return Block;
}

void AKioskEnvironment::AddLightBar(EKioskPart Part, const FVector& Centre, const FVector& SizeCm)
{
	UStaticMeshComponent* Bar = AddBlock(CubeMesh, *FString::Printf(TEXT("LightBar%d"), static_cast<int32>(Part)), Centre, SizeCm, KC().Muted);
	LightBars.Add(Part, Bar);
}

void AKioskEnvironment::BuildTrayItems()
{
	const FKioskColors& C = KC();
	const float ItemZ = TrayFloorZ + 0.3f;
	TicketMesh = AddBlock(CubeMesh, TEXT("TrayTicket"), FVector(9.f, 60.f, ItemZ), FVector(7.f, 15.f, 0.6f), C.PrimaryChip);
	TicketMesh->SetRelativeRotation(FRotator(0.f, 8.f, 0.f));
	ReceiptMesh = AddBlock(CubeMesh, TEXT("TrayReceipt"), FVector(10.f, 41.f, ItemZ), FVector(5.f, 9.f, 0.5f), C.White);
	ReceiptMesh->SetRelativeRotation(FRotator(0.f, -12.f, 0.f));
	for (UStaticMeshComponent* Item : { TicketMesh, ReceiptMesh })
	{
		Item->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Item->SetVisibility(false);
	}
	for (int32 Slot = 0; Slot < MaxChangePieces; ++Slot)
	{
		const FVector Spot(16.f, 34.f + Slot * 6.f, ItemZ);
		UStaticMeshComponent* Coin = AddBlock(CylinderMesh, *FString::Printf(TEXT("TrayCoin%d"), Slot), Spot, FVector(3.4f, 3.4f, 0.5f), C.Chip);
		UStaticMeshComponent* Note = AddBlock(CubeMesh, *FString::Printf(TEXT("TrayNote%d"), Slot), Spot, FVector(4.f, 5.5f, 0.3f), C.Primary);
		for (UStaticMeshComponent* Piece : { Coin, Note })
		{
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Piece->SetVisibility(false);
		}
		ChangeCoins.Add(Coin);
		ChangeNotes.Add(Note);
	}
}

void AKioskEnvironment::SetBlockColour(UStaticMeshComponent* Block, const FLinearColor& Colour) const
{
	if (UMaterialInstanceDynamic* Material = Block ? Cast<UMaterialInstanceDynamic>(Block->GetMaterial(0)) : nullptr)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Colour);
	}
}

EKioskPart AKioskEnvironment::PartForComponent(const UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return EKioskPart::None;
	}
	if (Component == Screen.Get())
	{
		return EKioskPart::Screen;
	}
	for (const TPair<EKioskPart, UStaticMeshComponent*>& Pair : PartBlocks)
	{
		if (Pair.Value == Component)
		{
			return Pair.Key;
		}
	}
	return EKioskPart::None;
}

FVector AKioskEnvironment::GetPartLocation(EKioskPart Part) const
{
	if (Part == EKioskPart::Screen)
	{
		return GetScreenCenter();
	}
	UStaticMeshComponent* const* Block = PartBlocks.Find(Part);
	if (!Block || !*Block)
	{
		return GetActorLocation();
	}
	// The tray is seen from above over its front lip: aim at the top of its floor.
	const FVector Offset = Part == EKioskPart::ChangeTray ? GetActorTransform().TransformVector(FVector(-2.f, 0.f, 2.1f)) : FVector::ZeroVector;
	return (*Block)->GetComponentLocation() + Offset;
}

void AKioskEnvironment::RefreshPartColour(EKioskPart Part)
{
	const FKioskColors& C = KC();
	if (UStaticMeshComponent* const* Block = PartBlocks.Find(Part))
	{
		const FLinearColor* Base = PartBaseColours.Find(Part);
		SetBlockColour(*Block, Part == HoverPart ? C.AmberStroke : (Base ? *Base : C.Ink));
	}
	if (UStaticMeshComponent* const* Bar = LightBars.Find(Part))
	{
		SetBlockColour(*Bar, LitParts.Contains(Part) ? C.Primary : C.Muted);
	}
}

void AKioskEnvironment::SetPartHighlight(EKioskPart Part)
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

void AKioskEnvironment::SetPartLight(EKioskPart Part, bool bOn)
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

void AKioskEnvironment::SetCardOnPad(bool bOnPad)
{
	bCardOnPad = bOnPad;
	if (CardOnPad)
	{
		CardOnPad->SetVisibility(bOnPad);
	}
}

void AKioskEnvironment::SetTrayContents(bool bTicket, bool bReceipt, int32 ChangeCents)
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

void AKioskEnvironment::SetScreenContent(const TSharedRef<SWidget>& Content)
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(Content);
	}
}

void AKioskEnvironment::ClearScreenContent()
{
	if (ScreenHost.IsValid())
	{
		ScreenHost->SetContent(SNullWidget::NullWidget);
	}
}

FVector AKioskEnvironment::GetScreenCenter() const
{
	return GetActorTransform().TransformPosition(FVector(0.6f, ScreenCentreY, ScreenCentreZ));
}

FVector AKioskEnvironment::CanvasToWorld(const FVector2D& Pixel, float SignU, float SignV) const
{
	if (!Screen)
	{
		return GetScreenCenter();
	}
	const FVector Local(0.f, SignU * (Pixel.X - SKioskRoot::CanvasWidth * 0.5f), SignV * (SKioskRoot::CanvasHeight * 0.5f - Pixel.Y));
	return Screen->GetComponentTransform().TransformPosition(Local);
}

bool AKioskEnvironment::ScreenContentIs(const TSharedRef<SWidget>& Widget) const
{
	return ScreenHost.IsValid() && &ScreenHost->GetChildren()->GetChildAt(0).Get() == &Widget.Get();
}

FTransform AKioskEnvironment::GetVisitorStart() const
{
	// Capsule centre is 93 cm up (3 cm of clearance so floor contact does not block sweeps); facing -X, towards the screen.
	return FTransform(FRotator(0.f, 180.f, 0.f), GetActorTransform().TransformPosition(FVector(300.f, 0.f, 93.f)));
}

FTransform AKioskEnvironment::GetLockOnView() const
{
	// Square on, 1 m from the face, at the middle of the screen's height: the screen, the hardware column and the tray all fit with a 90 degree lens.
	return FTransform(FRotator(0.f, 180.f, 0.f), GetActorTransform().TransformPosition(FVector(115.f, 0.f, 105.f)));
}

FTransform AKioskEnvironment::GetHardwareShotView() const
{
	// Slightly from the viewer's left and above: the card pad, reader column and collection tray in one frame.
	const FVector Eye = GetActorTransform().TransformPosition(FVector(165.f, 70.f, 135.f));
	const FVector Target = GetActorTransform().TransformPosition(FVector(0.f, -10.f, 100.f));
	return FTransform(FRotationMatrix::MakeFromX(Target - Eye).Rotator(), Eye);
}
