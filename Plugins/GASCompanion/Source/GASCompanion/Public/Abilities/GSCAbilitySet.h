// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Engine/DataAsset.h"
#include "GameFeatures/GSCGameFeatureTypes.h"
#include "GSCAbilitySet.generated.h"

class UAbilitySystemComponent;

/**
 * Data used to store handles to what has been granted by the ability set.
 */
USTRUCT(BlueprintType)
struct FGSCAbilitySetHandle
{
	GENERATED_BODY()

	/** Handles to the granted abilities */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> Abilities;

	/** Handles to the granted gameplay effects. */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;

	/** Pointers to the granted attribute sets */
	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> Attributes;

	/** Copy of the tag container that was used for OwnedTags */
	UPROPERTY()
	FGameplayTagContainer OwnedTags;

	/** DisplayName of the Ability Set represented by this handle (stored here for debug reason) */
	UPROPERTY(BlueprintReadOnly, Category="Ability Sets")
	FString AbilitySetPathName;

	/** List of delegate that may have been registered to handle input binding when the ability is given on client */
	TArray<FDelegateHandle> InputBindingDelegateHandles;

	/** Default constructor */
	FGSCAbilitySetHandle() = default;

	/** Returns whether the handle is valid by checking the original Ability Set pathname */
	bool IsValid() const
	{
		return !AbilitySetPathName.IsEmpty();
	}

	/** Cleans up the handle structure for any stored handles and pathname*/
	void Invalidate()
	{
		AbilitySetPathName = TEXT("");
		Abilities.Empty();
		EffectHandles.Empty();
		Attributes.Empty();
		OwnedTags.Reset();
	}

	/** Returns a String representation of the Ability Set handle */
	FString ToString(const bool bVerbose = false) const
	{
		TArray<FString> Results;
		Results.Add(FString::Printf(
			TEXT("AbilitySetPathName: %s, Abilities Handles: %d, Effect Handles: %d, Attribute Sets: %d, Owned Tags: %d"),
			*AbilitySetPathName,
			Abilities.Num(),
			EffectHandles.Num(),
			Attributes.Num(),
			OwnedTags.Num()
		));

		// No verbose output, only print high lvl info
		if (!bVerbose)
		{
			return FString::Join(Results, LINE_TERMINATOR);
		}

		Results.Add(FString::Printf(TEXT("Abilities Handles: %d"), Abilities.Num()));
		for (const FGameplayAbilitySpecHandle& AbilityHandle : Abilities)
		{
			Results.Add(FString::Printf(TEXT("\t - Ability Handle: %s"), *AbilityHandle.ToString()));
		}

		Results.Add(FString::Printf(TEXT("Effect Handles: %d"), EffectHandles.Num()));
		for (const FActiveGameplayEffectHandle& EffectHandle : EffectHandles)
		{
			Results.Add(FString::Printf(TEXT("\t - Effect Handle: %s"), *EffectHandle.ToString()));
		}
		
		Results.Add(FString::Printf(TEXT("Attribute Sets: %d"), Attributes.Num()));
		for (const UAttributeSet* AttributeSet : Attributes)
		{
			Results.Add(FString::Printf(TEXT("\t - Attribute Set: %s"), *GetNameSafe(AttributeSet)));
		}

		Results.Add(FString::Printf(TEXT("Owned Tags: %d"), OwnedTags.Num()));
		Results.Add(FString::Printf(TEXT("\t - Owned Tags: %s"), *OwnedTags.ToStringSimple()));

		return FString::Join(Results, LINE_TERMINATOR);
	}

	friend bool operator==(const FGSCAbilitySetHandle& LHS, const FGSCAbilitySetHandle& RHS)
	{
		return LHS.AbilitySetPathName == RHS.AbilitySetPathName;
	}

	friend bool operator!=(const FGSCAbilitySetHandle& LHS, const FGSCAbilitySetHandle& RHS)
	{
		return !(LHS == RHS);
	}
};

/**
 * DataAsset that can be used to define and give to an AbilitySystemComponent a set of:
 *
 * - Abilities, with optional enhanced input binding
 * - Attribute Sets
 * - Gameplay Effects
 * - Owned Tags
 */
UCLASS(BlueprintType)
class GASCOMPANION_API UGSCAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UGSCAbilitySet() = default;

	/** List of Gameplay Abilities to grant when the Ability System Component is initialized */
	UPROPERTY(EditDefaultsOnly, Category="Abilities", meta=(TitleProperty=AbilityType))
	TArray<FGSCGameFeatureAbilityMapping> GrantedAbilities;

	/** List of Attribute Sets to grant when the Ability System Component is initialized, with optional initialization data */
	UPROPERTY(EditDefaultsOnly, Category="Attributes", meta=(TitleProperty=AttributeSet))
	TArray<FGSCGameFeatureAttributeSetMapping> GrantedAttributes;

	/** List of GameplayEffects to apply when the Ability System Component is initialized (typically on begin play) */
	UPROPERTY(EditDefaultsOnly, Category="Effects", meta=(TitleProperty=EffectType))
	TArray<FGSCGameFeatureGameplayEffectMapping> GrantedEffects;
	
	/** An optional set of Gameplay Tags to grant to the ASC when the Ability Set is applied */
	UPROPERTY(EditDefaultsOnly, Category="Owned Gameplay Tags")
	FGameplayTagContainer OwnedTags;

	/**
	 * Grants itself (Ability Set) to the passed in ASC, adding defined Abilities, Attributes and Effects.
	 *
	 * @param InASC AbilitySystemComponent pointer to operate on
	 * @param OutAbilitySetHandle Handle that can be used to remove the set later on
	 * @param OutErrorText Reason of error in case of failed operation
	 * @param bShouldRegisterCoreDelegates Whether the set on successful application should try to register GSCCoreComponent delegates on Avatar Actor.
	 *
	 * @return True if the ability set was granted successfully, false otherwise
	 */
	bool GrantToAbilitySystem(UAbilitySystemComponent* InASC, FGSCAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText = nullptr, const bool bShouldRegisterCoreDelegates = true) const;

	/**
	 * Grants itself (Ability Set) to the passed in actor, adding defined Abilities, Attributes and Effects.
	 *
	 * Actor must implement IAbilitySystemInterface or have an AbilitySystemComponent component.
	 *
	 * @param InActor Actor (with an ASC) pointer to operate on
	 * @param OutAbilitySetHandle Handle that can be used to remove the set later on
	 * @param OutErrorText Reason of error in case of failed operation
	 *
	 * @return True if the ability set was granted successfully, false otherwise
	 */
	bool GrantToAbilitySystem(const AActor* InActor, FGSCAbilitySetHandle& OutAbilitySetHandle, FText* OutErrorText = nullptr) const;

	/**
	 * Removes the AbilitySet represented by InAbilitySetHandle from the passed in ASC. Clears out any previously granted Abilities,
	 * Attributes and Effects from the set.
	 * 
	 * @param InASC AbilitySystemComponent pointer to operate on
	 * @param InAbilitySetHandle Handle of the Ability Set to remove
	 * @param OutErrorText Reason of error in case of failed operation
	 * @param bShouldRegisterCoreDelegates Whether the set on successful application should try to register GSCCoreComponent delegates on Avatar Actor.
	 * 
	 * @return True if the ability set was removed successfully, false otherwise
	 */
	static bool RemoveFromAbilitySystem(UAbilitySystemComponent* InASC, FGSCAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText = nullptr, const bool bShouldRegisterCoreDelegates = true);

	/**
	 * Removes the AbilitySet represented by InAbilitySetHandle from the passed in actor. Clears out any previously granted Abilities,
	 * Attributes and Effects from the set.
	 * 
	 * Actor must implement IAbilitySystemInterface or have an AbilitySystemComponent component.
	 * 
	 * @param InActor Actor (with an ASC) pointer to operate on
	 * @param InAbilitySetHandle Handle of the Ability Set to remove
	 * @param OutErrorText Reason of error in case of failed operation
	 * 
	 * @return True if the ability set was removed successfully, false otherwise
	 */
	static bool RemoveFromAbilitySystem(const AActor* InActor, FGSCAbilitySetHandle& InAbilitySetHandle, FText* OutErrorText = nullptr);

	/** Returns whether this Ability Set needs Input Binding, eg. does any of the Granted Abilities in this set have a defined Input Action to bind */
	bool HasInputBinding() const;

protected:

	/** For avatar actors with a GSCCoreComponent, make sure to notify we may have added attributes, and register delegates for those */
	static void TryRegisterCoreComponentDelegates(UAbilitySystemComponent* InASC);
	
	/** For avatar actors with a GSCCoreComponent, makes sure to cleanup delegates */
	static void TryUnregisterCoreComponentDelegates(UAbilitySystemComponent* InASC);
};
