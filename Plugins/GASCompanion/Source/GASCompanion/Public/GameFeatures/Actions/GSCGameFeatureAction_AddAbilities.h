// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "Abilities/GSCAbilitySet.h"
#include "Abilities/GameplayAbility.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GSCGameFeatureAction_AddAbilities.generated.h"

struct FComponentRequestHandle;
struct FGSCGameFeatureAbilityMapping;
struct FGSCGameFeatureAttributeSetMapping;
struct FGSCGameFeatureGameplayEffectMapping;

/** Data holder for a list of Abilities / Attributes / Effects to grant to actors of the specified class from a Game Feature Action */
USTRUCT()
struct FGSCGameFeatureAbilitiesEntry
{
	GENERATED_BODY()

	/** The base actor class to add to */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TSoftClassPtr<AActor> ActorClass;

	/** List of abilities to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TArray<FGSCGameFeatureAbilityMapping> GrantedAbilities;

	/** List of attribute sets to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FGSCGameFeatureAttributeSetMapping> GrantedAttributes;
	
	/** List of gameplay effects to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FGSCGameFeatureGameplayEffectMapping> GrantedEffects;
	
	/** List of Gameplay Ability Sets to actors of the specified class */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Abilities")
	TArray<TSoftObjectPtr<UGSCAbilitySet>> GrantedAbilitySets;

	/** Default constructor */
	FGSCGameFeatureAbilitiesEntry() = default;
};

/**
 * GameFeatureAction responsible for granting abilities (and attributes, effects or ability sets) to actors of a specified type.
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Abilities (GAS Companion)"))
class UGSCGameFeatureAction_AddAbilities : public UGameFeatureAction
{
	GENERATED_BODY()
	
public:
	
	struct FActorExtensions
	{
		TArray<FGameplayAbilitySpecHandle> Abilities;
		TArray<UAttributeSet*> Attributes;
		TArray<FDelegateHandle> InputBindingDelegateHandles;
		TArray<FActiveGameplayEffectHandle> EffectHandles;
		TArray<FGSCAbilitySetHandle> AbilitySetHandles;
	};

	//~ Begin UGameFeatureAction interface
	virtual void OnGameFeatureActivating() override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
#if WITH_EDITORONLY_DATA
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif
	//~ End UGameFeatureAction interface

	//~ Begin UObject interface
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#else
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) override;
#endif
#endif
	//~ End UObject interface

	/** List of Ability to grant to actors of the specified class */
	UPROPERTY(EditAnywhere, Category="Abilities", meta=(TitleProperty="ActorClass", ShowOnlyInnerProperties))
	TArray<FGSCGameFeatureAbilitiesEntry> AbilitiesList;

	void Reset();
	void HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex);

	void AddActorAbilities(AActor* Actor, const FGSCGameFeatureAbilitiesEntry& AbilitiesEntry);
	void RemoveActorAbilities(const AActor* Actor);

private:

	FDelegateHandle GameInstanceStartHandle;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TMap<AActor*, FActorExtensions> ActiveExtensions;

	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;

	virtual void AddToWorld(const FWorldContext& WorldContext);
	void HandleGameInstanceStart(UGameInstance* GameInstance);
};
