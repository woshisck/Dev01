#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "DamageExecution.h"

#include "HealthPercentExecution.generated.h"

class UObject;


UCLASS()
class DEVKIT_API UHealthPercentExecution : public UDamageExecution
{
	GENERATED_BODY()


public:
	UHealthPercentExecution();


protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
