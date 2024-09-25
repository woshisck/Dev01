// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ComboGraphBlueprintLibrary.generated.h"

class UInputAction;
class UComboGraphGameplayTasksComponent;
struct FGameplayEventData;
class UComboGraph;
class UAbilitySystemComponent;
/**
 *
 */
UCLASS()
class COMBOGRAPH_API UComboGraphBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * This function is functionally the same as SendGameplayEventToActor (from Ability System Blueprint Library),
	 * except it can be used on actors that do not implement IAbilitySystemInterface. In ue5, this custom version
	 * won't be necessary anymore.
	 *
	 * This function can be used to trigger an ability on the actor in question with useful payload data.
	 * NOTE: GetAbilitySystemComponent is called on the actor to find a good component, and if the component isn't
	 * found, the event will not be sent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph|Ability", Meta = (DisplayName="Send Gameplay Event to Actor (Combo Graph)"))
	static void SendGameplayEventToActor(AActor* Actor, FGameplayTag EventTag, FGameplayEventData Payload);

	/** Searches the passed in actor for a Combo Graph gameplay tasks component, will use component search */
	UFUNCTION(BlueprintPure, Category = "Combo Graph|Components")
	static UComboGraphGameplayTasksComponent* GetComboGraphGameplayTasksComponent(const AActor* Actor);

	/** Extracts Cue Params resolved Objects (set in a combo graph node) from Gameplay Effect Context */
	UFUNCTION(BlueprintPure, Category = "Combo Graph|Effect Context")
	static TArray<UObject*> GetCueParamsObjectsFromContext(const FGameplayEffectContextHandle& EffectContext);

	/** Checks if Gameplay Effect Context has any Cue Params resolved Objects (set in a combo graph node) */
	UFUNCTION(BlueprintPure, Category = "Combo Graph|Effect Context")
	static bool HasCueParamsObjectsFromContext(const FGameplayEffectContextHandle& EffectContext);

	/** Extracts Cue Params Soft Object Paths (set in a combo graph node) from Gameplay Effect Context */
	UFUNCTION(BlueprintPure, Category = "Combo Graph|Effect Context")
	static TArray<FSoftObjectPath> GetCueParamsObjectsPathsFromContext(const FGameplayEffectContextHandle& EffectContext);

	/** Checks if Gameplay Effect Context has any Cue Params Soft Object Paths (set in a combo graph node) */
	UFUNCTION(BlueprintPure, Category = "Combo Graph|Effect Context")
	static bool HasCueParamsObjectsPathsFromContext(const FGameplayEffectContextHandle& EffectContext);

	/**
	 * Simulate user input for the passed in actor running a combo graph.
	 *
	 * This will send a gameplay event with relevant gameplay event data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combo Graph", Meta = (DisplayName="Simulate Combo Input (Combo Graph)"))
	static void SimulateComboInput(AActor* Actor, UInputAction* InputAction);

	/** Checks that given Effect Context is of expected type FComboGraphGameplayEffectContext */
	static bool IsValidContext(const FGameplayEffectContext* Context);

	/**
	 * Searches the passed in actor for an ability system component, will use the AbilitySystemInterface
	 *
	 * (in ue5, can be replaced by UAbilitySystemGlobals, right now using that version to not have the ability log warning for Actor not using the interface)
	 */
	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = true);
};
