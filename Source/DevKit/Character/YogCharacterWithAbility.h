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

	virtual UYogAbilitySystemComponent* GetASC() const;


private:

	UPROPERTY(VisibleAnywhere, Category = "ASC")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class UYogHealthSet> HealthSet;
	// Combat attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UYogCombatSet> CombatSet;

};
