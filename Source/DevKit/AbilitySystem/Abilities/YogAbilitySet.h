// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "YogGameplayAbility.h"
#include "GameplayAbilitySpecHandle.h"
#include "YogAbilitySet.generated.h"





USTRUCT(BlueprintType)
struct FYogAbilitySet_GameplayAbility 
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 AbilityLevel = 1;
	
};

USTRUCT(BlueprintType)
struct FYogAbilitySet_GameplayEffect 
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;

};

USTRUCT(BlueprintType)
struct FYogAbilitySet_AttributeSet 
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<UAttributeSet> AttributeSet;


};


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


	//TODO: Defined in GameplayAbility, consider move to here in in future
	//UFUNCTION(BlueprintCallable, Category = Ability)
	//virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FYogGameplayEffectContainerSpec& ContainerSpec);


	//UFUNCTION(BlueprintCallable, Category = Ability, meta = (AutoCreateRefTerm = "EventData"))
	//virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);


 protected:

// 	// Gameplay abilities to grant when this ability set is granted.
 	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
 	TArray<FYogAbilitySet_GameplayAbility> GrantedGameplayAbilities;

// 	// Gameplay effects to grant when this ability set is granted.
 	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
 	TArray<FYogAbilitySet_GameplayEffect> GrantedGameplayEffects;

// 	// Attribute sets to grant when this ability set is granted.
 	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
 	TArray<FYogAbilitySet_AttributeSet> GrantedAttributes;
};
