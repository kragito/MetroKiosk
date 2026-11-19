// Game mode that hosts the kiosk UI in the 3D concourse by default, or in the game viewport (UI-only input, visible cursor) with -Kiosk2D.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KioskGameMode.generated.h"

class FKioskQARunner;
class FKioskWorld;
class FKioskSession;
class SKioskRoot;

UCLASS()
class METROKIOSK_API AKioskGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AKioskGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RemoveKioskWidget();

	TSharedPtr<FKioskSession> Session;
	TSharedPtr<SKioskRoot> Root;
	TSharedPtr<FKioskQARunner> QARunner;
	TSharedPtr<FKioskWorld> World3D;
	TWeakObjectPtr<UGameViewportClient> HostViewport;
};
