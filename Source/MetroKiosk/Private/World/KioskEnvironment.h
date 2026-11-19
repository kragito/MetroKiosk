// Actor that builds the 3D metro concourse and the kiosk cabinet (screen, readers, slots, tray, card pad) at runtime from engine basic shapes only.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KioskWallet.h"
#include "KioskEnvironment.generated.h"

class SBox;
class SWidget;
class UMaterialInterface;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS()
class AKioskEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AKioskEnvironment();

	/** Creates every mesh, light and the screen component. Call once after spawning. */
	void Build();

	/** Shows a Slate widget on the kiosk screen face (the widget may only have one parent at a time). */
	void SetScreenContent(const TSharedRef<SWidget>& Content);
	void ClearScreenContent();

	FVector GetScreenCenter() const;
	/** World position of a pixel on the 1917x1105 kiosk canvas. SignU / SignV flip the axes (used by the self-test to find the mapping). */
	FVector CanvasToWorld(const FVector2D& Pixel, float SignU = 1.f, float SignV = 1.f) const;
	/** True while Widget is the content shown on the 3D screen. */
	bool ScreenContentIs(const TSharedRef<SWidget>& Widget) const;
	/** Where the visitor starts: a few metres in front of the kiosk, facing it. */
	FTransform GetVisitorStart() const;
	/** World transform of the camera when the visitor has stepped up to the kiosk: square on, the whole front in view. */
	FTransform GetLockOnView() const;
	/** Camera for the -Kiosk3DShotView=kiosk hardware close-up. */
	FTransform GetHardwareShotView() const;

	// --- hardware parts --------------------------------------------------------------------------
	/** Which kiosk part a traced component belongs to (Screen for the screen, None for anything else). */
	EKioskPart PartForComponent(const UPrimitiveComponent* Component) const;
	/** Centre of a part in the world, for aiming the cursor at it. */
	FVector GetPartLocation(EKioskPart Part) const;
	/** Hover highlight: the part under the cursor (None clears it). */
	void SetPartHighlight(EKioskPart Part);
	/** The green light next to a part: on while the screen asks for it. */
	void SetPartLight(EKioskPart Part, bool bOn);
	/** The Metro card lying on the card pad. */
	void SetCardOnPad(bool bOnPad);
	bool IsCardOnPad() const { return bCardOnPad; }
	/** What lies in the collection tray: printed ticket, receipt and returned change (coins / notes drawn for the amount). */
	void SetTrayContents(bool bTicket, bool bReceipt, int32 ChangeCents);

private:
	UStaticMeshComponent* AddBlock(UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour);
	UStaticMeshComponent* AddPart(EKioskPart Part, UStaticMesh* Mesh, const FName& Name, const FVector& Centre, const FVector& SizeCm, const FLinearColor& Colour);
	void AddLightBar(EKioskPart Part, const FVector& Centre, const FVector& SizeCm);
	void AddLight(const FName& Name, const FVector& Location);
	void BuildKiosk();
	void BuildTrayItems();
	void SetBlockColour(UStaticMeshComponent* Block, const FLinearColor& Colour) const;
	void RefreshPartColour(EKioskPart Part);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Screen;

	TSharedPtr<SBox> ScreenHost;

	// Hardware (plain pointers: the actor owns every component).
	TMap<EKioskPart, UStaticMeshComponent*> PartBlocks;
	TMap<EKioskPart, FLinearColor> PartBaseColours;
	TMap<EKioskPart, UStaticMeshComponent*> LightBars;
	TSet<EKioskPart> LitParts;
	EKioskPart HoverPart = EKioskPart::None;
	UStaticMeshComponent* CardOnPad = nullptr;
	UStaticMeshComponent* TicketMesh = nullptr;
	UStaticMeshComponent* ReceiptMesh = nullptr;
	TArray<UStaticMeshComponent*> ChangeCoins; // one slot = one coin cylinder ...
	TArray<UStaticMeshComponent*> ChangeNotes; // ... or one note, drawn at the same place
	bool bCardOnPad = false;
};
