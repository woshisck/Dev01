// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

#include "GameplayAbilitySpecHandle.h"
#include "YogAbilitySet.generated.h"


/**
 * UYogAbilitySet
 *
 *	Non-mutable data asset used to grant gameplay abilities and gameplay effects.
 */
UCLASS(BlueprintType, Const)
class UYogAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UYogAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	// void GiveToAbilitySystem(UGameAbilitySystemComponent* YogASC, FYogAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

// protected:

// 	// Gameplay abilities to grant when this ability set is granted.
// 	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
// 	TArray<FYogAbilitySet_GameplayAbility> GrantedGameplayAbilities;

// 	// Gameplay effects to grant when this ability set is granted.
// 	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
// 	TArray<FYogAbilitySet_GameplayEffect> GrantedGameplayEffects;

// 	// Attribute sets to grant when this ability set is granted.
// 	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
// 	TArray<FYogAbilitySet_AttributeSet> GrantedAttributes;
};
