// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Utils/ComboGraphBlueprintLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "ComboGraphAbilityTypes.h"
#include "ComboGraphLog.h"
#include "InputAction.h"
#include "Abilities/ComboGraphNativeAbility.h"
#include "Abilities/ComboGraphNativeTags.h"
#include "Components/ComboGraphGameplayTasksComponent.h"

void UComboGraphBlueprintLibrary::SendGameplayEventToActor(AActor* Actor, const FGameplayTag EventTag, const FGameplayEventData Payload)
{
	if (!IsValid(Actor))
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::SendGameplayEventToActor - Passed in actor is not valid"));
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActor(Actor);
	if (IsValid(AbilitySystemComponent))
	{
		FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
	}
	else
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::SendGameplayEventToActor: Invalid ability system component retrieved from Actor %s. EventTag was %s"), *Actor->GetName(), *EventTag.ToString());
	}
}

UComboGraphGameplayTasksComponent* UComboGraphBlueprintLibrary::GetComboGraphGameplayTasksComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::GetComboGraphGameplayTasksComponent - Passed in actor is not valid"));
		return nullptr;
	}

	// Fall back to a component search to better support BP-only actors
	return Actor->FindComponentByClass<UComboGraphGameplayTasksComponent>();
}

TArray<UObject*> UComboGraphBlueprintLibrary::GetCueParamsObjectsFromContext(const FGameplayEffectContextHandle& EffectContext)
{
	TArray<UObject*> Result;

	const FGameplayEffectContext* Context = EffectContext.Get();
	if (!IsValidContext(Context))
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::GetCueParamsObjectsFromContext - Effect Context is not of type FComboGraphGameplayEffectContext. Make sure AbilitySystemGlobalsClassName is set to \"/Script/ComboGraph.ComboGraphAbilitySystemGlobals\" in config."))
		return Result;
	}

	const FComboGraphGameplayEffectContext* ComboGraphEffectContext = static_cast<const FComboGraphGameplayEffectContext*>(Context);
	if (ComboGraphEffectContext)
	{
		TArray<TWeakObjectPtr<UObject>> ObjectPtrs = ComboGraphEffectContext->GetCueParamsObjects();
		for (TWeakObjectPtr<UObject> ObjectPtr : ObjectPtrs)
		{
			if (ObjectPtr.IsValid())
			{
				Result.Add(ObjectPtr.Get());
			}
		}
	}

	return Result;
}

bool UComboGraphBlueprintLibrary::HasCueParamsObjectsFromContext(const FGameplayEffectContextHandle& EffectContext)
{
	return GetCueParamsObjectsFromContext(EffectContext).Num() > 0;
}

TArray<FSoftObjectPath> UComboGraphBlueprintLibrary::GetCueParamsObjectsPathsFromContext(const FGameplayEffectContextHandle& EffectContext)
{
	TArray<FSoftObjectPath> Result;

	const FGameplayEffectContext* Context = EffectContext.Get();
	if (!IsValidContext(Context))
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::GetCueParamsObjectsPathsFromContext - Effect Context is not of type FComboGraphGameplayEffectContext. Make sure AbilitySystemGlobalsClassName is set to \"/Script/ComboGraph.ComboGraphAbilitySystemGlobals\" in config."))
		return Result;
	}

	const FComboGraphGameplayEffectContext* ComboGraphEffectContext = static_cast<const FComboGraphGameplayEffectContext*>(Context);
	if (ComboGraphEffectContext)
	{
		Result = ComboGraphEffectContext->GetCueParamsObjectsPaths();
	}

	return Result;
}

bool UComboGraphBlueprintLibrary::HasCueParamsObjectsPathsFromContext(const FGameplayEffectContextHandle& EffectContext)
{
	return GetCueParamsObjectsPathsFromContext(EffectContext).Num() > 0;
}

void UComboGraphBlueprintLibrary::SimulateComboInput(AActor* Actor, UInputAction* InputAction)
{
	if (!IsValid(Actor))
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::SimulateComboInput - Passed in actor is not valid"));
		return;
	}

	if (!InputAction)
	{
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphBlueprintLibrary::SimulateComboInput - Passed in input action is not valid"));
		return;
	}

	const FGameplayTag EventTag = FComboGraphNativeTags::Get().Input;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.OptionalObject = InputAction;

	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphBlueprintLibrary::SimulateComboInput - Sending Event %s with Input Action %s"), *EventTag.ToString(), *GetNameSafe(InputAction));
	SendGameplayEventToActor(Actor, EventTag, Payload);
}

bool UComboGraphBlueprintLibrary::IsValidContext(const FGameplayEffectContext* Context)
{
	if (!Context)
	{
		return false;
	}

	UScriptStruct* ScriptStruct = Context->GetScriptStruct();
	return ScriptStruct && ScriptStruct->IsChildOf(FComboGraphGameplayEffectContext::StaticStruct());
}

UAbilitySystemComponent* UComboGraphBlueprintLibrary::GetAbilitySystemComponentFromActor(const AActor* Actor, const bool LookForComponent)
{
	if (Actor == nullptr)
	{
		return nullptr;
	}

	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	if (ASI)
	{
		return ASI->GetAbilitySystemComponent();
	}

	if (LookForComponent)
	{
		/** This is slow and not desirable */
		return Actor->FindComponentByClass<UAbilitySystemComponent>();
	}

	return nullptr;
}
