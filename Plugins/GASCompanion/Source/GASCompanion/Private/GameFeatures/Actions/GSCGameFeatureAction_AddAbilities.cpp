// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GameFeatures/Actions/GSCGameFeatureAction_AddAbilities.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GSCLog.h"
#include "GameFeaturesSubsystemSettings.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "Abilities/GSCAbilitySystemUtils.h"
#include "Components/GSCAbilityInputBindingComponent.h"
#include "Components/GSCCoreComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "GameFeatures/GSCGameFeatureTypes.h"
#include "Engine/Engine.h" // for FWorldContext
#include "Engine/GameInstance.h"
#include "Engine/World.h" // for FWorldDelegates::OnStartGameInstance
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "GASCompanion"

void UGSCGameFeatureAction_AddAbilities::OnGameFeatureActivating()
{
	if (!ensureAlways(ActiveExtensions.Num() == 0) || !ensureAlways(ComponentRequests.Num() == 0))
	{
		Reset();
	}

	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGSCGameFeatureAction_AddAbilities::HandleGameInstanceStart);

	check(ComponentRequests.Num() == 0);

	// Add to any worlds with associated game instances that have already been initialized
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		AddToWorld(WorldContext);
	}

	Super::OnGameFeatureActivating();
}

void UGSCGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);

	Reset();
}

#if WITH_EDITORONLY_DATA
void UGSCGameFeatureAction_AddAbilities::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	if (!UAssetManager::IsInitialized())
	{
		return;
	}
#else
	if (!UAssetManager::IsValid())
	{
		return;
	}
#endif

	auto AddBundleAsset = [&AssetBundleData](const FSoftObjectPath& SoftObjectPath)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, SoftObjectPath.GetAssetPath());
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, SoftObjectPath.GetAssetPath());
#else
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, SoftObjectPath);
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateServer, SoftObjectPath);
#endif
	};

	for (const FGSCGameFeatureAbilitiesEntry& Entry : AbilitiesList)
	{
		for (const FGSCGameFeatureAbilityMapping& Ability : Entry.GrantedAbilities)
		{
			AddBundleAsset(Ability.AbilityType.ToSoftObjectPath());
			if (!Ability.InputAction.IsNull())
			{
				AddBundleAsset(Ability.InputAction.ToSoftObjectPath());
			}
		}

		for (const FGSCGameFeatureAttributeSetMapping& Attributes : Entry.GrantedAttributes)
		{
			AddBundleAsset(Attributes.AttributeSet.ToSoftObjectPath());
			if (!Attributes.InitializationData.IsNull())
			{
				AddBundleAsset(Attributes.InitializationData.ToSoftObjectPath());
			}
		}

		for (const FGSCGameFeatureGameplayEffectMapping& Effect : Entry.GrantedEffects) 
		{
			AddBundleAsset(Effect.EffectType.ToSoftObjectPath());
		}
	}
}
#endif

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
EDataValidationResult UGSCGameFeatureAction_AddAbilities::IsDataValid(FDataValidationContext& Context) const
#else
EDataValidationResult UGSCGameFeatureAction_AddAbilities::IsDataValid(FDataValidationContext& Context)
#endif
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 EntryIndex = 0;
	for (const FGSCGameFeatureAbilitiesEntry& Entry : AbilitiesList)
	{
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("EntryHasNullActor", "Null ActorClass at index {0} in AbilitiesList"), FText::AsNumber(EntryIndex)));
		}

		if (Entry.GrantedAbilities.IsEmpty() && Entry.GrantedAttributes.IsEmpty() && Entry.GrantedEffects.IsEmpty())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(LOCTEXT("EntryHasNoAddOns", "Granted Abilities / Attributes / Effects are all empty. This action should grant at least one of these."));
		}

		int32 AbilityIndex = 0;
		for (const FGSCGameFeatureAbilityMapping& Ability : Entry.GrantedAbilities)
		{
			if (Ability.AbilityType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAbility", "Null AbilityType at index {0} in AbilitiesList[{1}].GrantedAbilities"), FText::AsNumber(AbilityIndex), FText::AsNumber(EntryIndex)));
			}
			++AbilityIndex;
		}

		int32 AttributesIndex = 0;
		for (const FGSCGameFeatureAttributeSetMapping& Attributes : Entry.GrantedAttributes)
		{
			if (Attributes.AttributeSet.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAttributeSet", "Null AttributeSetType at index {0} in AbilitiesList[{1}].GrantedAttributes"), FText::AsNumber(AttributesIndex), FText::AsNumber(EntryIndex)));
			}
			++AttributesIndex;
		}

		int32 EffectsIndex = 0;
		for (const FGSCGameFeatureGameplayEffectMapping& Effect : Entry.GrantedEffects) 
		{
			if (Effect.EffectType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullEffect", "Null GameplayEffectType at index {0} in AbilitiesList[{1}].GrantedEffects"), FText::AsNumber(EffectsIndex), FText::AsNumber(EntryIndex)));
			}
			++EffectsIndex;
		}

		++EntryIndex;
	}

	return Result;
}
#endif

void UGSCGameFeatureAction_AddAbilities::Reset()
{
	while (ActiveExtensions.Num() != 0)
	{
		const auto ExtensionIt = ActiveExtensions.CreateIterator();
		RemoveActorAbilities(ExtensionIt->Key);
	}

	ComponentRequests.Empty();
}

void UGSCGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Actor, const FName EventName, const int32 EntryIndex)
{
	if (AbilitiesList.IsValidIndex(EntryIndex))
	{
		GSC_LOG(Verbose, TEXT("UGSCGameFeatureAction_AddAbilities::HandleActorExtension '%s'. EventName: %s"), *Actor->GetPathName(), *EventName.ToString());
		const FGSCGameFeatureAbilitiesEntry& Entry = AbilitiesList[EntryIndex];
		if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
		{
			GSC_LOG(Verbose, TEXT("UGSCGameFeatureAction_AddAbilities::HandleActorExtension remove '%s'. Abilities will be removed."), *Actor->GetPathName());
			RemoveActorAbilities(Actor);
		}
		else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
		{
			GSC_LOG(Verbose, TEXT("UGSCGameFeatureAction_AddAbilities::HandleActorExtension add '%s'. Abilities will be granted."), *Actor->GetPathName());
			AddActorAbilities(Actor, Entry);
		}
	}
}

void UGSCGameFeatureAction_AddAbilities::AddActorAbilities(AActor* Actor, const FGSCGameFeatureAbilitiesEntry& AbilitiesEntry)
{
	if (!IsValid(Actor))
	{
		GSC_LOG(Error, TEXT("Failed to find/add an ability component. Target Actor is not valid"));
		return;
	}

	// TODO: Remove coupling to UGSCAbilitySystemComponent. Should work off just an UAbilitySystemComponent
	// Right now, required because of TryBindAbilityInput and necessity for OnGiveAbilityDelegate, but delegate could be reworked to be from an Interface

	// Go through IAbilitySystemInterface::GetAbilitySystemComponent() to handle target pawn using ASC on Player State
	UGSCAbilitySystemComponent* ExistingASC = Cast<UGSCAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor));
	// Not using the template version of FindOrAddComponentForActor() due a "use of function template name with no prior declaration in function call with explicit template arguments is a C++20 extension" only on linux 5.0 with strict includes,
	// ending up in this very long line
	UGSCAbilitySystemComponent* AbilitySystemComponent = ExistingASC ? ExistingASC : Cast<UGSCAbilitySystemComponent>(FGSCAbilitySystemUtils::FindOrAddComponentForActor(UGSCAbilitySystemComponent::StaticClass(), Actor, ComponentRequests));
	
	if (!AbilitySystemComponent)
	{
		GSC_LOG(Error, TEXT("Failed to find/add an ability component to '%s'. Abilities will not be granted."), *Actor->GetPathName());
		return;
	}

	AActor* OwnerActor = AbilitySystemComponent->GetOwnerActor();
	AActor* AvatarActor = AbilitySystemComponent->GetAvatarActor();
	
	GSC_LOG(Display, TEXT("Trying to add actor abilities from Game Feature action for Owner: %s, Avatar: %s, Original Actor: %s"), *GetNameSafe(OwnerActor), *GetNameSafe(AvatarActor), *GetNameSafe(Actor));

	// Handle cleaning up of previous attributes / abilities in case of respawns
	FActorExtensions* ActorExtensions = ActiveExtensions.Find(OwnerActor);
	if (ActorExtensions)
	{
		if (AbilitySystemComponent->bResetAttributesOnSpawn)
		{
			// ASC wants reset, remove attributes
			for (UAttributeSet* AttribSetInstance : ActorExtensions->Attributes)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
#else
				AbilitySystemComponent->GetSpawnedAttributes_Mutable().Remove(AttribSetInstance);
#endif
			}
		}

		if (AbilitySystemComponent->bResetAbilitiesOnSpawn)
		{
			// ASC wants reset, remove abilities
			UGSCAbilityInputBindingComponent* InputComponent = AvatarActor ? AvatarActor->FindComponentByClass<UGSCAbilityInputBindingComponent>() : nullptr;
			for (const FGameplayAbilitySpecHandle& AbilityHandle : ActorExtensions->Abilities)
			{
				if (InputComponent)
				{
					InputComponent->ClearInputBinding(AbilityHandle);
				}

				// Only Clear abilities on authority
				if (AbilitySystemComponent->IsOwnerActorAuthoritative())
				{
					AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
				}
			}

			// Clear any delegate handled bound previously for this actor
			for (FDelegateHandle DelegateHandle : ActorExtensions->InputBindingDelegateHandles)
			{
				AbilitySystemComponent->OnGiveAbilityDelegate.Remove(DelegateHandle);
				DelegateHandle.Reset();
			}
		}

		// Remove effects
		for (const FActiveGameplayEffectHandle& EffectHandle : ActorExtensions->EffectHandles)
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
		}

		ActiveExtensions.Remove(OwnerActor);
	}
	

	FActorExtensions AddedExtensions;
	AddedExtensions.Abilities.Reserve(AbilitiesEntry.GrantedAbilities.Num());
	AddedExtensions.Attributes.Reserve(AbilitiesEntry.GrantedAttributes.Num());
	AddedExtensions.EffectHandles.Reserve(AbilitiesEntry.GrantedEffects.Num());
	AddedExtensions.AbilitySetHandles.Reserve(AbilitiesEntry.GrantedAbilitySets.Num());

	for (const FGSCGameFeatureAbilityMapping& AbilityMapping : AbilitiesEntry.GrantedAbilities)
	{
		if (!AbilityMapping.AbilityType.IsNull())
		{
			// Try to grant the ability first
			FGameplayAbilitySpec AbilitySpec;
			FGameplayAbilitySpecHandle AbilityHandle;
			FGSCAbilitySystemUtils::TryGrantAbility(AbilitySystemComponent, AbilityMapping, AbilityHandle, AbilitySpec);

			// Handle Input Mapping now
			if (!AbilityMapping.InputAction.IsNull())
			{
				FDelegateHandle DelegateHandle;
				FGSCAbilitySystemUtils::TryBindAbilityInput(AbilitySystemComponent, AbilityMapping, AbilityHandle, AbilitySpec, DelegateHandle, &ComponentRequests);
				AddedExtensions.InputBindingDelegateHandles.Add(MoveTemp(DelegateHandle));
			}

			AddedExtensions.Abilities.Add(AbilityHandle);
		}
	}

	for (const FGSCGameFeatureAttributeSetMapping& Attributes : AbilitiesEntry.GrantedAttributes)
	{
		if (!Attributes.AttributeSet.IsNull() && AbilitySystemComponent->IsOwnerActorAuthoritative())
		{
			UAttributeSet* AddedAttributeSet = nullptr;
			FGSCAbilitySystemUtils::TryGrantAttributes(AbilitySystemComponent, Attributes, AddedAttributeSet);

			if (AddedAttributeSet)
			{
				AddedExtensions.Attributes.Add(AddedAttributeSet);
			}
		}
	}

	for (const FGSCGameFeatureGameplayEffectMapping& Effect : AbilitiesEntry.GrantedEffects)
	{
		if (!Effect.EffectType.IsNull())
		{
			FGSCAbilitySystemUtils::TryGrantGameplayEffect(AbilitySystemComponent, Effect.EffectType.LoadSynchronous(), Effect.Level, AddedExtensions.EffectHandles);
		}
	}

	for (const TSoftObjectPtr<UGSCAbilitySet>& AbilitySetEntry : AbilitiesEntry.GrantedAbilitySets)
	{
		const UGSCAbilitySet* AbilitySet = AbilitySetEntry.LoadSynchronous();
		if (!AbilitySet)
		{
			continue;
		}

		FGSCAbilitySetHandle Handle;
		if (!FGSCAbilitySystemUtils::TryGrantAbilitySet(AbilitySystemComponent, AbilitySet, Handle, &ComponentRequests))
		{
			continue;
		}

		AddedExtensions.AbilitySetHandles.Add(Handle);
	}

	// GSCCore component could be added to avatars
	UGSCCoreComponent* CoreComponent = AvatarActor ? AvatarActor->FindComponentByClass<UGSCCoreComponent>() : nullptr;
	if (CoreComponent)
	{
		// Make sure to notify we may have added attributes
		CoreComponent->RegisterAbilitySystemDelegates(AbilitySystemComponent);
	}

	ActiveExtensions.Add(OwnerActor, AddedExtensions);
}

void UGSCGameFeatureAction_AddAbilities::RemoveActorAbilities(const AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	if (FActorExtensions* ActorExtensions = ActiveExtensions.Find(Actor))
	{
		UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
		if (AbilitySystemComponent)
		{
			// Remove effects
			for (const FActiveGameplayEffectHandle& EffectHandle : ActorExtensions->EffectHandles)
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
			}
			
			// Remove attributes
			for (UAttributeSet* AttribSetInstance : ActorExtensions->Attributes)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
#else
				AbilitySystemComponent->GetSpawnedAttributes_Mutable().Remove(AttribSetInstance);
#endif
			}

			// Remove abilities
			UGSCAbilityInputBindingComponent* InputComponent = Actor->FindComponentByClass<UGSCAbilityInputBindingComponent>();
			for (const FGameplayAbilitySpecHandle& AbilityHandle : ActorExtensions->Abilities)
			{
				if (InputComponent)
				{
					InputComponent->ClearInputBinding(AbilityHandle);
				}

				// Only Clear abilities on authority
				if (AbilitySystemComponent->IsOwnerActorAuthoritative())
				{
					AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
				}
			}

			// Remove sets
			for (FGSCAbilitySetHandle& Handle : ActorExtensions->AbilitySetHandles)
			{
				FText ErrorText;
				if (!UGSCAbilitySet::RemoveFromAbilitySystem(AbilitySystemComponent, Handle, &ErrorText, false))
				{
					GSC_PLOG(Error, TEXT("Error trying to remove ability set %s - %s"), *Handle.AbilitySetPathName, *ErrorText.ToString());
				}
			}
		}
		else
		{
			GSC_PLOG(Warning, TEXT("Not able to find AbilitySystemComponent for %s. This may happen for Player State ASC when game is shut downed."), *GetNameSafe(Actor))
		}

		// We need to clean up give ability delegates
		if (UGSCAbilitySystemComponent* ASC = Cast<UGSCAbilitySystemComponent>(AbilitySystemComponent))
		{
			// Clear any delegate handled bound previously for this actor
			for (FDelegateHandle InputBindingDelegateHandle : ActorExtensions->InputBindingDelegateHandles)
			{
				ASC->OnGiveAbilityDelegate.Remove(InputBindingDelegateHandle);
				InputBindingDelegateHandle.Reset();
			}
		}

		ActiveExtensions.Remove(Actor);
	}
}

void UGSCGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
{
	const UWorld* World = WorldContext.World();
	const UGameInstance* GameInstance = WorldContext.OwningGameInstance;

	if ((GameInstance != nullptr) && (World != nullptr) && World->IsGameWorld())
	{
		UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);

		if (ComponentMan)
		{
			int32 EntryIndex = 0;

			GSC_LOG(Verbose, TEXT("Adding abilities for %s to world %s"), *GetPathNameSafe(this), *World->GetDebugDisplayName());

			for (const FGSCGameFeatureAbilitiesEntry& Entry : AbilitiesList)
			{
				if (!Entry.ActorClass.IsNull())
				{
					const UGameFrameworkComponentManager::FExtensionHandlerDelegate AddAbilitiesDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
						this,
						&UGSCGameFeatureAction_AddAbilities::HandleActorExtension,
						EntryIndex
					);
					TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentMan->AddExtensionHandler(Entry.ActorClass, AddAbilitiesDelegate);

					ComponentRequests.Add(ExtensionRequestHandle);
					EntryIndex++;
				}
			}
		}
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCGameFeatureAction_AddAbilities::HandleGameInstanceStart(UGameInstance* GameInstance)
{
	if (const FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		AddToWorld(*WorldContext);
	}
}

#undef LOCTEXT_NAMESPACE
