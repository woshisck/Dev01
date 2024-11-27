#pragma once

#include "CoreMinimal.h"
#include "YogBaseCharacter.h"
#include "../AbilitySystem/YogAbilitySystemComponent.h"

#include "YogCharacterWithAbility.generated.h"



UCLASS()
class DEVKIT_API AYogCharacterWithAbility : public AYogBaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AYogCharacterWithAbility(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;

	

};
