// Metadata that lets the scripted test harness (-KioskScript) find interactive widgets by name.
#pragma once

#include "CoreMinimal.h"
#include "Types/ISlateMetaData.h"

class FKioskAutomationMeta : public ISlateMetaData
{
public:
	SLATE_METADATA_TYPE(FKioskAutomationMeta, ISlateMetaData)

	FString Name;   // accessible name, e.g. "Station HarbourFront"
	FString Label;  // visible label, e.g. "HarbourFront"
	TAttribute<bool> Enabled = true;
};
