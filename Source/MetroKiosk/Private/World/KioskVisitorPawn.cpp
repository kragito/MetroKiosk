// Walking and looking are read straight from the player controller's keys and mouse delta, so no input assets are needed.
#include "KioskVisitorPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	constexpr float WalkSpeed = 220.f; // cm per second
	constexpr float ExposureBrightness = 1.f; // min = max pins auto-exposure
	constexpr float EyeHeight = 70.f;         // camera above the capsule centre
	constexpr float ViewBlendSeconds = 0.4f;  // stepping up to / back from the kiosk
}

AKioskVisitorPawn::AKioskVisitorPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(34.f, 90.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = Capsule;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Capsule);
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);
	// Fixed exposure so the concourse lighting is the same everywhere and in every screenshot.
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = AEM_Histogram;
	// No bloom: the bright kiosk screen would wash out its own text.
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = 0.f;
	Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	Camera->PostProcessSettings.AutoExposureMinBrightness = ExposureBrightness;
	Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	Camera->PostProcessSettings.AutoExposureMaxBrightness = ExposureBrightness;

	Pointer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("Pointer"));
	Pointer->SetupAttachment(Camera);
	Pointer->InteractionDistance = 400.f;
	Pointer->InteractionSource = EWidgetInteractionSource::World;
	Pointer->bEnableHitTesting = true;
}

void AKioskVisitorPawn::SetWalkingEnabled(bool bEnabled)
{
	if (bWalking == bEnabled)
	{
		return;
	}
	bWalking = bEnabled;
	if (!bWalking && Pointer)
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

FVector AKioskVisitorPawn::WalkEyeLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, EyeHeight);
}

void AKioskVisitorPawn::LockView(const FTransform& View, float FieldOfViewDegrees)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (ViewPhase == EViewPhase::Locked || ViewPhase == EViewPhase::BlendIn)
	{
		return;
	}
	if (ViewPhase == EViewPhase::Free)
	{
		// From now the camera is positioned by hand, so freeze it where the walking view has it.
		WalkFov = Camera->FieldOfView;
		Camera->bUsePawnControlRotation = false;
		Camera->SetWorldLocationAndRotation(WalkEyeLocation(), PC ? PC->GetControlRotation() : GetActorRotation());
		BlendAlpha = 0.f;
	}
	LockedView = View;
	LockedFov = FieldOfViewDegrees;
	ViewPhase = EViewPhase::BlendIn;
}

void AKioskVisitorPawn::ReleaseView()
{
	if (ViewPhase == EViewPhase::Locked || ViewPhase == EViewPhase::BlendIn)
	{
		ViewPhase = EViewPhase::BlendOut;
	}
}

void AKioskVisitorPawn::TickView(float DeltaSeconds)
{
	if (ViewPhase == EViewPhase::Free)
	{
		return;
	}
	if (ViewPhase == EViewPhase::BlendIn || ViewPhase == EViewPhase::BlendOut)
	{
		BlendAlpha = FMath::Clamp(BlendAlpha + (ViewPhase == EViewPhase::BlendIn ? 1.f : -1.f) * DeltaSeconds / ViewBlendSeconds, 0.f, 1.f);
	}
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const FQuat WalkRot = (PC ? PC->GetControlRotation() : GetActorRotation()).Quaternion();
	const float T = FMath::SmoothStep(0.f, 1.f, BlendAlpha);
	const FVector Location = FMath::Lerp(WalkEyeLocation(), LockedView.GetLocation(), T);
	const FQuat Rotation = FQuat::Slerp(WalkRot, LockedView.GetRotation(), T);
	Camera->SetWorldLocationAndRotation(Location, Rotation);
	Camera->SetFieldOfView(FMath::Lerp(WalkFov, LockedFov, T));
	if (ViewPhase == EViewPhase::BlendIn && BlendAlpha >= 1.f)
	{
		ViewPhase = EViewPhase::Locked;
	}
	else if (ViewPhase == EViewPhase::BlendOut && BlendAlpha <= 0.f)
	{
		// Back at the eye: hand the camera to mouse look again.
		ViewPhase = EViewPhase::Free;
		Camera->bUsePawnControlRotation = true;
		Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeight));
		Camera->SetFieldOfView(WalkFov);
	}
}

void AKioskVisitorPawn::AimPointerAway()
{
	Pointer->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
}

void AKioskVisitorPawn::ScrollPointer(float Delta)
{
	Pointer->ScrollWheel(Delta);
}

void AKioskVisitorPawn::AimPointerAt(const FVector& WorldPosition)
{
	Pointer->SetWorldRotation(FRotationMatrix::MakeFromX(WorldPosition - Pointer->GetComponentLocation()).Rotator());
}

void AKioskVisitorPawn::ResetPointerAim()
{
	Pointer->SetRelativeRotation(FRotator::ZeroRotator);
}

void AKioskVisitorPawn::InjectPointer(bool bDown)
{
	if (bDown)
	{
		Pointer->PressPointerKey(EKeys::LeftMouseButton);
	}
	else
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

bool AKioskVisitorPawn::GetPointerHit(FVector2D& Out2D) const
{
	if (!Pointer->GetHoveredWidgetComponent())
	{
		return false;
	}
	Out2D = Pointer->Get2DHitLocation();
	return true;
}

void AKioskVisitorPawn::SetFieldOfView(float Degrees)
{
	Camera->SetFieldOfView(Degrees);
}

FVector AKioskVisitorPawn::GetEyeLocation() const
{
	return Camera ? Camera->GetComponentLocation() : GetActorLocation();
}

FVector AKioskVisitorPawn::GetEyeForward() const
{
	return Camera ? Camera->GetForwardVector() : GetActorForwardVector();
}

void AKioskVisitorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickView(DeltaSeconds);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !bWalking)
	{
		return;
	}

	// Mouse look.
	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	FRotator Look = PC->GetControlRotation();
	Look.Yaw += MouseX * LookSensitivity;
	Look.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Look.Pitch) + MouseY * LookSensitivity, -80.f, 80.f);
	Look.Roll = 0.f;
	PC->SetControlRotation(Look);

	// Walk on the floor plane.
	const float Forward = InjectedForward + (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up) ? 1.f : 0.f)
		- (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down) ? 1.f : 0.f);
	const float Right = InjectedRight + (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right) ? 1.f : 0.f)
		- (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left) ? 1.f : 0.f);
	if (Forward != 0.f || Right != 0.f)
	{
		const FRotationMatrix Basis(FRotator(0.f, Look.Yaw, 0.f));
		const FVector Wish = (Basis.GetScaledAxis(EAxis::X) * Forward + Basis.GetScaledAxis(EAxis::Y) * Right).GetSafeNormal();
		const FVector Delta = Wish * WalkSpeed * DeltaSeconds;
		FHitResult Hit;
		AddActorWorldOffset(Delta, true, &Hit);
		if (Hit.IsValidBlockingHit())
		{
			// Slide along the wall or the kiosk instead of stopping dead.
			const FVector Slide = FVector::VectorPlaneProject(Delta, Hit.ImpactNormal) * (1.f - Hit.Time);
			AddActorWorldOffset(Slide, true);
		}
	}

	// Pointing and clicking on the kiosk screen.
	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		Pointer->PressPointerKey(EKeys::LeftMouseButton);
	}
	if (PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
	const float Wheel = PC->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (Wheel != 0.f)
	{
		Pointer->ScrollWheel(Wheel);
	}
}
