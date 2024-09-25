// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GSCTypes.h"
#include "GSCGameFeatureTypes.generated.h"

class UInputAction;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FGSCGameFeatureAbilityMapping
{
	GENERATED_BODY()

	/** Type of ability to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	TSoftClassPtr<UGameplayAbility> AbilityType;

	/** Level to grant the ability at */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	int32 Level = 1;
	
	/** Input action to bind the ability to, if any (can be left unset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability)
	TSoftObjectPtr<UInputAction> InputAction;

	/** The enhanced input action event to use for ability activation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Ability, meta=(EditCondition = "InputAction != nullptr", EditConditionHides))
	EGSCAbilityTriggerEvent TriggerEvent = EGSCAbilityTriggerEvent::Started;

	/** Default constructor */
	FGSCGameFeatureAbilityMapping() = default;
};

USTRUCT(BlueprintType)
struct FGSCGameFeatureAttributeSetMapping
{
	GENERATED_BODY()

	/** Attribute Set to grant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Attributes)
	TSoftClassPtr<UAttributeSet> AttributeSet;

	/** Data table referent to initialize the attributes with, if any (can be left unset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Attributes, meta = (RequiredAssetDataTags = "RowStructure=/Script/GameplayAbilities.AttributeMetaData"))
	TSoftObjectPtr<UDataTable> InitializationData;

	/** Default constructor */
	FGSCGameFeatureAttributeSetMapping() = default;
};

USTRUCT(BlueprintType)
struct FGSCGameFeatureGameplayEffectMapping
{
	GENERATED_BODY()

	/** Gameplay Effect to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay Effect")
	TSoftClassPtr<UGameplayEffect> EffectType;

	/** Level for the Gameplay Effect to apply */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gameplay Effect")
	float Level = 1.f;

	/** Default constructor */
	FGSCGameFeatureGameplayEffectMapping() = default;
};
