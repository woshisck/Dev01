// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GSCAbilitySet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GSCBlueprintFunctionLibrary.generated.h"

class UGSCAbilityInputBindingComponent;
class UGSCAbilityQueueComponent;
class UGSCAbilitySystemComponent;
class UGSCComboManagerComponent;
class UGSCCoreComponent;

/**
* Ability specific blueprint library
*
* Most games will need to implement one or more blueprint function libraries to expose their native code to blueprints
*/
UCLASS()
class GASCOMPANION_API UGSCBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	/** Tries to find an ability system component on the actor and cast to UGSCAbilitySystemComponent, will use AbilitySystemInterface or fall back to a component search */
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Components")
	static UGSCAbilitySystemComponent* GetCompanionAbilitySystemComponent(const AActor* Actor);

	/**
	* Tries to find a combo manager component on the actor
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Components")
	static UGSCComboManagerComponent* GetComboManagerComponent(const AActor* Actor);

	/**
	* Tries to find a companion core component on the actor
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Components")
	static UGSCCoreComponent* GetCompanionCoreComponent(const AActor* Actor);

	/**
	* Tries to find an ability queue core component on the actor
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Components")
	static UGSCAbilityQueueComponent* GetAbilityQueueComponent(const AActor* Actor);
	
	/**
	* Tries to find an ability input binding component on the actor
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Components")
	static UGSCAbilityInputBindingComponent* GetAbilityInputBindingComponent(const AActor* Actor);

	/** Gameplay Tags */

	/**
	* Tries to find an ability system component on the actor, using AbilitySystemInterface, and
	* add loose GameplayTags which are not backed by a GameplayEffect.
	*
	* Tags added this way are not replicated!
	*
	* It is up to the calling GameCode to make sure these tags are added on clients/server where necessary
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayTags")
	static bool AddLooseGameplayTagsToActor(AActor* Actor, const FGameplayTagContainer GameplayTags);

	/**
	* Tries to find an ability system component on the actor, using AbilitySystemInterface, and
	* remove loose GameplayTags which are not backed by a GameplayEffect.
	*
	* Tags added this way are not replicated!
	*
	* It is up to the calling GameCode to make sure these tags are added on clients/server where necessary
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayTags")
	static bool RemoveLooseGameplayTagsFromActor(AActor* Actor, const FGameplayTagContainer GameplayTags);

	/**
	* Returns true if the passed in Actor's AbilitySystemComponent has a gameplay tag that matches against the specified tag (expands to include parents of asset tags)
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Abilities|GameplayTags")
	static bool HasMatchingGameplayTag(AActor* Actor, const FGameplayTag GameplayTag);

	/**
	* Returns true if the passed in Actor's AbilitySystemComponent has any of the matching tags (expands to include parents of asset tags)
	*/
	UFUNCTION(BlueprintPure, Category = "GAS Companion|Abilities|GameplayTags")
	static bool HasAnyMatchingGameplayTag(AActor* Actor, const FGameplayTagContainer GameplayTags);

	/**
	 * Returns the Attribute name
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS Companion|Attribute Set")
	static FString GetDebugStringFromAttribute(FGameplayAttribute Attribute);

	/** Returns all defined Gameplay Attributes for the provided AttributeSet class */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS Companion|Attribute Set")
	static void GetAllAttributes(TSubclassOf<UAttributeSet> AttributeSetClass, TArray<FGameplayAttribute>& OutAttributes);

	/** Checks if a gameplay attribute's name and a string are not equal to one another */
	UFUNCTION(BlueprintPure, Category = "GAS Companion|PinOptions", meta = (BlueprintInternalUseOnly = "TRUE"))
	static bool NotEqual_GameplayAttributeGameplayAttribute(FGameplayAttribute A, FString B);

	// -------------------------------------
	//	GameplayCue
	//	Can invoke GameplayCues without having to create GameplayEffects
	// -------------------------------------

	/** Invoke a gameplay cue on the actor's ability system component */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void ExecuteGameplayCueForActor(AActor* Actor, FGameplayTag GameplayCueTag, FGameplayEffectContextHandle Context);

	/** Invoke a gameplay cue on the actor's ability system component, with extra parameters */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void ExecuteGameplayCueWithParams(AActor* Actor, FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters);

	/** Adds a persistent gameplay cue to the actor's ability system component. Optionally will remove if ability ends */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void AddGameplayCue(AActor* Actor, FGameplayTag GameplayCueTag, FGameplayEffectContextHandle Context);

	/** Adds a persistent gameplay cue to the actor's ability system component. Optionally will remove if ability ends */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void AddGameplayCueWithParams(AActor* Actor, FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameter);

	/** Removes a persistent gameplay cue from the actor's ability system component */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void RemoveGameplayCue(AActor* Actor, FGameplayTag GameplayCueTag);

	/** Removes any GameplayCue added on its own, i.e. not as part of a GameplayEffect. */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities|GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	static void RemoveAllGameplayCues(AActor* Actor);

	// -------------------------------------
	//	Ability Sets
	// -------------------------------------

	/** Returns a String representation of the Ability Set handle */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability Sets")
	static FString DebugAbilitySetHandle(const FGSCAbilitySetHandle& InAbilitySetHandle, const bool bVerbose = false);
};
