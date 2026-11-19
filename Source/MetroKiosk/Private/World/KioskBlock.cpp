// A basic-shape mesh coloured by a KC() token name, plus the actor that places one block in the concourse level.
#include "KioskBlock.h"

#include "Engine/StaticMesh.h"
#include "KioskStyle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UKioskBlockComponent::UKioskBlockComponent()
{
	SetMobility(EComponentMobility::Movable);
	SetCastShadow(false);
}

FLinearColor UKioskBlockComponent::GetBaseColour() const
{
	FLinearColor Colour;
	if (FKioskColors::Lookup(ColourToken, Colour))
	{
		return Colour;
	}
	return FLinearColor(1.f, 0.f, 1.f);
}

void UKioskBlockComponent::ApplyLook()
{
	const TCHAR* MeshPath = Shape == EKioskShape::Cylinder ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube");
	UStaticMesh* Wanted = LoadObject<UStaticMesh>(nullptr, MeshPath);
	if (Wanted && GetStaticMesh() != Wanted)
	{
		SetStaticMesh(Wanted);
	}
	if (!Material)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base)
		{
			Material = UMaterialInstanceDynamic::Create(Base, this);
			Material->SetFlags(RF_Transient); // rebuilt on every load; never saved into the level
			SetMaterial(0, Material);
		}
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), GetBaseColour());
	}
}

void UKioskBlockComponent::SetColourOverride(const FLinearColor& Colour)
{
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Colour);
	}
}

void UKioskBlockComponent::ClearColourOverride()
{
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), GetBaseColour());
	}
}

void UKioskBlockComponent::OnRegister()
{
	ApplyLook();
	Super::OnRegister();
}

#if WITH_EDITOR
void UKioskBlockComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyLook();
}
#endif

AKioskBlock::AKioskBlock()
{
	PrimaryActorTick.bCanEverTick = false;
	Block = CreateDefaultSubobject<UKioskBlockComponent>(TEXT("Block"));
	RootComponent = Block;
}
