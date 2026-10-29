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

AKioskGameMode::AKioskGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
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
	Viewport->AddViewportWidgetContent(Root.ToSharedRef(), 100);

	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(Root);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
	}

	QARunner = MakeShared<FKioskQARunner>();
	QARunner->Initialise(Session.ToSharedRef());
}

void AKioskGameMode::RemoveKioskWidget()
{
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
