// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Abilities/GSCBlueprintFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GSCLog.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GSCAbilityInputBindingComponent.h"
#include "Components/GSCAbilityQueueComponent.h"
#include "Components/GSCComboManagerComponent.h"
#include "Components/GSCCoreComponent.h"

UGSCAbilitySystemComponent* UGSCBlueprintFunctionLibrary::GetCompanionAbilitySystemComponent(const AActor* Actor)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		return nullptr;
	}

	if (UGSCAbilitySystemComponent* CompanionASC = Cast<UGSCAbilitySystemComponent>(ASC))
	{
		return CompanionASC;
	}

	GSC_PLOG(Warning, TEXT("ASC `%s` from `%s` Owner is not of UGSCAbilitySystemComponent type."), *GetNameSafe(ASC), *GetNameSafe(Actor));
	return nullptr;
}

UGSCComboManagerComponent* UGSCBlueprintFunctionLibrary::GetComboManagerComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// Fall back to a component search to better support BP-only actors
	return Actor->FindComponentByClass<UGSCComboManagerComponent>();
}

UGSCCoreComponent* UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// Fall back to a component search to better support BP-only actors
	return Actor->FindComponentByClass<UGSCCoreComponent>();
}

UGSCAbilityQueueComponent* UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UGSCAbilityQueueComponent>();
}

UGSCAbilityInputBindingComponent* UGSCBlueprintFunctionLibrary::GetAbilityInputBindingComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// Fall back to a component search to better support BP-only actors
	return Actor->FindComponentByClass<UGSCAbilityInputBindingComponent>();
}

bool UGSCBlueprintFunctionLibrary::AddLooseGameplayTagsToActor(AActor* Actor, const FGameplayTagContainer GameplayTags)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTags(GameplayTags);
		return true;
	}

	return false;
}

bool UGSCBlueprintFunctionLibrary::RemoveLooseGameplayTagsFromActor(AActor* Actor, const FGameplayTagContainer GameplayTags)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTags(GameplayTags);
		return true;
	}

	return false;
}

bool UGSCBlueprintFunctionLibrary::HasMatchingGameplayTag(AActor* Actor, const FGameplayTag GameplayTag)
{
	UGSCCoreComponent* CCC = GetCompanionCoreComponent(Actor);
	if (!CCC)
	{
		return false;
	}

	return CCC->HasMatchingGameplayTag(GameplayTag);
}

bool UGSCBlueprintFunctionLibrary::HasAnyMatchingGameplayTag(AActor* Actor, const FGameplayTagContainer GameplayTags)
{
	UGSCCoreComponent* CCC = GetCompanionCoreComponent(Actor);
	if (!CCC)
	{
		return false;
	}

	return CCC->HasAnyMatchingGameplayTags(GameplayTags);
}

FString UGSCBlueprintFunctionLibrary::GetDebugStringFromAttribute(const FGameplayAttribute Attribute)
{
	return Attribute.GetName();
}

void UGSCBlueprintFunctionLibrary::GetAllAttributes(const TSubclassOf<UAttributeSet> AttributeSetClass, TArray<FGameplayAttribute>& OutAttributes)
{
	const UClass* Class = AttributeSetClass.Get();
	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(*It))
		{
			OutAttributes.Push(FGameplayAttribute(FloatProperty));
		}
		else if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
		{
			OutAttributes.Push(FGameplayAttribute(*It));
		}
	}
}

bool UGSCBlueprintFunctionLibrary::NotEqual_GameplayAttributeGameplayAttribute(FGameplayAttribute A, FString B)
{
	return A.GetName() != B;
}

void UGSCBlueprintFunctionLibrary::ExecuteGameplayCueForActor(AActor* Actor, FGameplayTag GameplayCueTag, FGameplayEffectContextHandle Context)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ExecuteGameplayCue(GameplayCueTag, Context);
	}
}

void UGSCBlueprintFunctionLibrary::ExecuteGameplayCueWithParams(AActor* Actor, FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ExecuteGameplayCue(GameplayCueTag, GameplayCueParameters);
	}
}

void UGSCBlueprintFunctionLibrary::AddGameplayCue(AActor* Actor, FGameplayTag GameplayCueTag, FGameplayEffectContextHandle Context)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddGameplayCue(GameplayCueTag, Context);
	}
}

void UGSCBlueprintFunctionLibrary::AddGameplayCueWithParams(AActor* Actor, FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameter)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddGameplayCue(GameplayCueTag, GameplayCueParameter);
	}
}

void UGSCBlueprintFunctionLibrary::RemoveGameplayCue(AActor* Actor, FGameplayTag GameplayCueTag)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveGameplayCue(GameplayCueTag);
	}
}

void UGSCBlueprintFunctionLibrary::RemoveAllGameplayCues(AActor* Actor)
{
	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, true);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveAllGameplayCues();
	}
}

FString UGSCBlueprintFunctionLibrary::DebugAbilitySetHandle(const FGSCAbilitySetHandle& InAbilitySetHandle, const bool bVerbose)
{
	return InAbilitySetHandle.ToString(bVerbose);
}
