// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AActor;
class UAbilitySystemComponent;
class UActorComponent;
class UAttributeSet;
class UGSCAbilityInputBindingComponent;
class UGSCAbilitySet;
class UGameplayAbility;
class UGameplayEffect;
class UInputAction;
enum class EGSCAbilityTriggerEvent : uint8;
struct FActiveGameplayEffectHandle;
struct FComponentRequestHandle;
struct FGSCAbilitySetHandle;
struct FGSCGameFeatureAbilityMapping;
struct FGSCGameFeatureAttributeSetMapping;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;
struct FGameplayTagContainer;

/**
 * Utilities class with a bunch of statics to provide common shared code facilities to grant various things to an ASC.
 *
 * Used by both GameFeature Actions and GSCAbilitySystemComponent code path.
 */
class GASCOMPANION_API FGSCAbilitySystemUtils
{
public:
	static void TryGrantAbility(UAbilitySystemComponent* InASC, const FGSCGameFeatureAbilityMapping& InAbilityMapping, FGameplayAbilitySpecHandle& OutAbilityHandle, FGameplayAbilitySpec& OutAbilitySpec);

	static void TryBindAbilityInput(
		UAbilitySystemComponent* InASC,
		const FGSCGameFeatureAbilityMapping& InAbilityMapping,
		const FGameplayAbilitySpecHandle& InAbilityHandle,
		const FGameplayAbilitySpec& InAbilitySpec,
		FDelegateHandle& OutOnGiveAbilityDelegateHandle,
		TArray<TSharedPtr<FComponentRequestHandle>>* OutComponentRequests = nullptr
	);
	
	static void TryGrantAttributes(UAbilitySystemComponent* InASC, const FGSCGameFeatureAttributeSetMapping& InAttributeSetMapping, UAttributeSet*& OutAttributeSet);
	static void TryGrantGameplayEffect(UAbilitySystemComponent* InASC, const TSubclassOf<UGameplayEffect> InEffectType, const float InLevel, TArray<FActiveGameplayEffectHandle>& OutEffectHandles);
	static bool TryGrantAbilitySet(UAbilitySystemComponent* InASC, const UGSCAbilitySet* InAbilitySet, FGSCAbilitySetHandle& OutAbilitySetHandle, TArray<TSharedPtr<FComponentRequestHandle>>* OutComponentRequests = nullptr);
	
	/** Helper to return the AttributeSet UObject as a non const pointer, if the passed in ASC has it granted */
	static UAttributeSet* GetAttributeSet(const UAbilitySystemComponent* InASC, const TSubclassOf<UAttributeSet> InAttributeSet);

	/** Determine if a gameplay effect is already applied, same class and same level */
	static bool HasGameplayEffectApplied(const UAbilitySystemComponent* InASC, const TSubclassOf<UGameplayEffect>& InEffectType, TArray<FActiveGameplayEffectHandle>& OutEffectHandles);

	/** Determine if an ability is already granted, same class and same level */
	static bool IsAbilityGranted(const UAbilitySystemComponent* InASC, TSubclassOf<UGameplayAbility> InAbility, const int32 InLevel = 1);

	// Commented out due to a "use of function template name with no prior declaration in function call with explicit template arguments is a C++20 extension" warning only on linux 5.0 with strict includes
	
	// template<class ComponentType>
	// static ComponentType* FindOrAddComponentForActor(AActor* InActor, TArray<TSharedPtr<FComponentRequestHandle>>& OutComponentRequests)
	// {
	// 	return Cast<ComponentType>(FindOrAddComponentForActor(ComponentType::StaticClass(), InActor, OutComponentRequests));
	// }
	static UActorComponent* FindOrAddComponentForActor(UClass* InComponentType, const AActor* InActor, TArray<TSharedPtr<FComponentRequestHandle>>& OutComponentRequests);

	/** Adds a tag container to ASC, but only if ASC doesn't have said tags yet */
	static void AddLooseGameplayTagsUnique(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InTags, const bool bReplicated = true);
	
	/** Removes a tag container to ASC, but only if ASC doesn't have said tags yet */
	static void RemoveLooseGameplayTagsUnique(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InTags, const bool bReplicated = true);

private:
	/** Handler for AbilitySystem OnGiveAbility delegate. Sets up input binding for clients (not authority) when GameFeatures are activated during Play. */
	static void HandleOnGiveAbility(
		FGameplayAbilitySpec& InAbilitySpec,
		TWeakObjectPtr<UGSCAbilityInputBindingComponent> InInputComponent,
		TWeakObjectPtr<UInputAction> InInputAction,
		const EGSCAbilityTriggerEvent InTriggerEvent,
		FGameplayAbilitySpec InNewAbilitySpec
	);
};
