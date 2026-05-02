#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "GameplayAbilityComboGraphFactory.generated.h"

UCLASS()
class UGameplayAbilityComboGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphFactory(const FObjectInitializer& ObjectInitializer);

	virtual FText GetDisplayName() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
