#include "KioskGameMode.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "KioskLoc.h"
#include "KioskQA.h"
#include "KioskSession.h"
#include "Widgets/SKioskRoot.h"
#include "World/KioskVisitorPawn.h"
#include "World/KioskWorld.h"

AKioskGameMode::AKioskGameMode()
{
	// 3D mode: the visitor is placed in the level (nothing is spawned for the player); the flat kiosk uses a spectator pawn.
	DefaultPawnClass = FKioskWorld::IsRequested() ? nullptr : TSubclassOf<APawn>(ASpectatorPawn::StaticClass());
	HUDClass = nullptr;
}

void AKioskGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return; // dedicated server / commandlet: nothing to show
	}

	FKioskLoc::SetLanguage(EKioskLanguage::English);
	Session = MakeShared<FKioskSession>();
	Root = SNew(SKioskRoot, Session.ToSharedRef());
	HostViewport = Viewport;

	bool bFlat = !FKioskWorld::IsRequested();
	if (!bFlat)
	{
		// 3D mode (the default; -Kiosk2D opts out): the kiosk UI lives on the screen of the kiosk placed in the concourse level (see World/KioskWorld.cpp).
		World3D = MakeShared<FKioskWorld>(GetWorld(), Viewport, Root.ToSharedRef(), Session.ToSharedRef());
		if (!World3D->Start())
		{
			World3D.Reset();
			bFlat = true; // the loaded level has no kiosk: show the flat kiosk instead of an empty view
		}
	}
	if (bFlat)
	{
		Viewport->AddViewportWidgetContent(Root.ToSharedRef(), 100);

		if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
		{
			Controller->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(Root);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Controller->SetInputMode(InputMode);
		}
	}

	QARunner = MakeShared<FKioskQARunner>();
	QARunner->Initialise(Session.ToSharedRef(), Root.ToSharedRef());
}

void AKioskGameMode::RemoveKioskWidget()
{
	if (World3D.IsValid())
	{
		World3D->Shutdown();
		World3D.Reset();
	}
	if (QARunner.IsValid())
	{
		QARunner->Shutdown();
		QARunner.Reset();
	}
	if (Root.IsValid())
	{
		if (UGameViewportClient* Viewport = HostViewport.Get())
		{
			Viewport->RemoveViewportWidgetContent(Root.ToSharedRef());
		}
	}
	Root.Reset();
	Session.Reset();
	HostViewport.Reset();
}

void AKioskGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveKioskWidget();
	Super::EndPlay(EndPlayReason);
}
