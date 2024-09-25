// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GSCNativeAnimInstanceInterface.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GSCNativeAnimInstance.generated.h"

/**
 * Anim instance implementing IGSCNativeAnimInstanceInterface to allow Gameplay Tag Blueprint Property Mapping.
 *
 * The interface has only one overridable method `InitializeWithAbilitySystem()` that must be implemented in subclasses.
 * 
 * GSCAbilitySystemComponent will call this method via an interface call when InitAbilityActorInfo happens.
 *
 * The same pattern is used in both Lyra and the Ancient Demo, only difference here is that we rely on an interface to be able to operate better with other plugins / assets.
 */
UCLASS()
class GASCOMPANION_API UGSCNativeAnimInstance : public UAnimInstance, public IGSCNativeAnimInstanceInterface
{
	GENERATED_BODY()

public:
	/**
	 * Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	 * 
	 * These should be used instead of manually querying for the gameplay tags.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC) override
	{
		GameplayTagPropertyMap.Initialize(this, ASC);
	}

	//~ Begin UObject interface
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#else
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
#endif
	//~ End UObject interface

	//~ Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	//~ End UAnimInstance interface
};
