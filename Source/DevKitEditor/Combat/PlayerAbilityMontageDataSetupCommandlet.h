#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PlayerAbilityMontageDataSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UPlayerAbilityMontageDataSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UPlayerAbilityMontageDataSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
