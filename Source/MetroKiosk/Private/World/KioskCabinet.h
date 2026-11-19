// The kiosk cabinet placed in the concourse level: screen, reader, slots, card pad and collection tray as editable components.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KioskWallet.h"
#include "KioskCabinet.generated.h"

class SBox;
class SWidget;
class UKioskBlockComponent;
class UPrimitiveComponent;
class UWidgetComponent;

/**
 * The kiosk. Its parts are default subobjects with the layout of the Figma hardware frame (centimetres, front face at X = 0, screen facing +X);
 * a placed instance can move, scale or recolour any of them in the editor and the game follows.
 */
UCLASS()
class AKioskCabinet : public AActor
{
	GENERATED_BODY()

public:
	AKioskCabinet();

	/** Shows a Slate widget on the kiosk screen face (the widget may only have one parent at a time). */
	void SetScreenContent(const TSharedRef<SWidget>& Content);
	void ClearScreenContent();

	FVector GetScreenCenter() const;
	/** World position of a pixel on the 1917x1105 kiosk canvas. SignU / SignV flip the axes (used by the self-test to find the mapping). */
	FVector CanvasToWorld(const FVector2D& Pixel, float SignU = 1.f, float SignV = 1.f) const;
	/** True while Widget is the content shown on the 3D screen. */
	bool ScreenContentIs(const TSharedRef<SWidget>& Widget) const;
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

protected:
	virtual void BeginPlay() override;

private:
	UKioskBlockComponent* AddBlock(const TCHAR* Name, bool bCylinder, const FVector& Centre, const FVector& SizeCm, const TCHAR* Token);
	UKioskBlockComponent* AddPart(EKioskPart Part, const TCHAR* Name, bool bCylinder, const FVector& Centre, const FVector& SizeCm, const TCHAR* Token);
	void AddLightBar(EKioskPart Part, const TCHAR* Name, const FVector& Centre, const FVector& SizeCm);
	void RefreshPartColour(EKioskPart Part);

	UPROPERTY(VisibleAnywhere, Category = "Kiosk")
	TObjectPtr<UWidgetComponent> Screen;

	/** Every block of the cabinet. Listed here so the level saves what you change on them (a component nothing points at is not saved with the level). */
	UPROPERTY(VisibleAnywhere, Category = "Kiosk")
	TArray<TObjectPtr<UKioskBlockComponent>> Blocks;

	TSharedPtr<SBox> ScreenHost;

	// Plain pointers: the actor owns every component.
	TMap<EKioskPart, UKioskBlockComponent*> PartBlocks;
	TMap<EKioskPart, UKioskBlockComponent*> LightBars;
	TSet<EKioskPart> LitParts;
	EKioskPart HoverPart = EKioskPart::None;
	UKioskBlockComponent* CardOnPad = nullptr;
	UKioskBlockComponent* TicketMesh = nullptr;
	UKioskBlockComponent* ReceiptMesh = nullptr;
	TArray<UKioskBlockComponent*> ChangeCoins; // one slot = one coin cylinder ...
	TArray<UKioskBlockComponent*> ChangeNotes; // ... or one note, drawn at the same place
	bool bCardOnPad = false;
};
