#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "HUDRootFrameBrushSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UHUDRootFrameBrushSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UHUDRootFrameBrushSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};

