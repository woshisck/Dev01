#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CombatMontageSyncCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UCombatMontageSyncCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCombatMontageSyncCommandlet();

	virtual int32 Main(const FString& Params) override;
};
