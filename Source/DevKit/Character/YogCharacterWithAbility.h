#pragma once

#include "CoreMinimal.h"
#include "YogCharacterBase.h"
#include "../AbilitySystem/YogAbilitySystemComponent.h"

#include "YogCharacterWithAbility.generated.h"



UCLASS()
class DEVKIT_API AYogCharacterWithAbility : public AYogCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AYogCharacterWithAbility(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;

	

};
