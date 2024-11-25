#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"


#include "DamageExecution.generated.h"

class UObject;


UCLASS()
class DEVKIT_API UDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()


public:
	UDamageExecution();


protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
