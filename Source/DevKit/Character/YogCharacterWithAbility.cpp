#include "YogCharacterWithAbility.h"

#include "../AbilitySystem/Attribute/YogCombatSet.h"
#include "../AbilitySystem/Attribute/YogHealthSet.h"

// Sets default values
AYogCharacterWithAbility::AYogCharacterWithAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));


	HealthSet = CreateDefaultSubobject<UYogHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UYogCombatSet>(TEXT("CombatSet"));
}


void AYogCharacterWithAbility::PostInitializeComponents()
{
}

UYogAbilitySystemComponent* AYogCharacterWithAbility::GetASC() const
{
	return AbilitySystemComponent;
}


