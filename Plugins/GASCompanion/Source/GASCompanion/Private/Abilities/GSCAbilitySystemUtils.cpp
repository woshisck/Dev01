// Copyright 2021-2022 Mickael Daniel. All Rights Reserved.

#include "Abilities/GSCAbilitySystemUtils.h"

#include "GSCLog.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GSCAbilityInputBindingComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"

void FGSCAbilitySystemUtils::TryGrantAbility(UAbilitySystemComponent* InASC, const FGSCGameFeatureAbilityMapping& InAbilityMapping, FGameplayAbilitySpecHandle& OutAbilityHandle, FGameplayAbilitySpec& OutAbilitySpec)
{
	check(InASC);
	
	if (InAbilityMapping.AbilityType.IsNull())
	{
		GSC_PLOG(Error, TEXT("Failed to Grant Ability \"%s\" because SoftClassPtr is null"), *InAbilityMapping.AbilityType.ToString())
		return;
	}
	
	const TSubclassOf<UGameplayAbility> AbilityType = InAbilityMapping.AbilityType.LoadSynchronous();
	check(AbilityType);

	UGSCAbilitySystemComponent* ASC = Cast<UGSCAbilitySystemComponent>(InASC);
	if (!ASC)
	{
		GSC_PLOG(Error, TEXT("Failed to Grant Ability \"%s\" because ASC \"%s\" is not a UGSCAbilitySystemComponent"), *GetNameSafe(AbilityType), *GetNameSafe(InASC))
		return;
	}

	OutAbilitySpec = ASC->BuildAbilitySpecFromClass(AbilityType, InAbilityMapping.Level);
	
	// Try to grant the ability first
	if (ASC->IsOwnerActorAuthoritative())
	{
		// Only Grant abilities on authority, and only if we should (ability not granted yet or wants reset on spawn)
		if (!IsAbilityGranted(ASC, AbilityType, InAbilityMapping.Level))
		{
			GSC_PLOG(Verbose, TEXT("Authority, Grant Ability (%s)"), *AbilityType->GetName())
			OutAbilityHandle = ASC->GiveAbility(OutAbilitySpec);
		}
		else
		{
			// In case granting is prevented because of ability already existing, return the existing handle
			const FGameplayAbilitySpec* ExistingAbilitySpec = ASC->FindAbilitySpecFromClass(AbilityType);
			if (ExistingAbilitySpec)
			{
				OutAbilityHandle = ExistingAbilitySpec->Handle;
			}
		}
	}
	else
	{
		// For clients, try to get ability spec and update handle used later on for input binding
		const FGameplayAbilitySpec* ExistingAbilitySpec = ASC->FindAbilitySpecFromClass(AbilityType);
		if (ExistingAbilitySpec)
		{
			OutAbilityHandle = ExistingAbilitySpec->Handle;
		}
		
		GSC_LOG(Verbose, TEXT("AddActorAbilities: Not Authority, try to find ability handle from spec: %s"), *OutAbilityHandle.ToString())
	}
}

void FGSCAbilitySystemUtils::TryBindAbilityInput(UAbilitySystemComponent* InASC, const FGSCGameFeatureAbilityMapping& InAbilityMapping, const FGameplayAbilitySpecHandle& InAbilityHandle, const FGameplayAbilitySpec& InAbilitySpec, FDelegateHandle& OutOnGiveAbilityDelegateHandle, TArray<TSharedPtr<FComponentRequestHandle>>* OutComponentRequests)
{
	check(InASC);
	
	UGSCAbilitySystemComponent* ASC = Cast<UGSCAbilitySystemComponent>(InASC);
	if (!ASC)
	{
		GSC_PLOG(Error, TEXT("Failed to bind Ability \"%s\" because ASC \"%s\" is not a UGSCAbilitySystemComponent"), *InAbilityMapping.AbilityType.ToString(), *GetNameSafe(InASC))
		return;
	}
	
	AActor* OwnerActor = ASC->GetOwnerActor();
	AActor* AvatarActor = ASC->GetAvatarActor();

	// UGSCAbilityInputBindingComponent is a PawnComponent, ensure owner of it is actually a pawn
	APawn* TargetPawn = Cast<APawn>(OwnerActor);
	if (!TargetPawn)
	{
		if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
		{
			TargetPawn = AvatarPawn;
		}
	}

	if (!TargetPawn)
	{
		// May happen for PlayerState characters on BeginPlay of PlayerState, while we really want to wait for OnPossess of the character
		return;
	}

	// AddComponentForActor will only work properly in the context of a Game Feature activating, for all other use case outside of a Game Feature action, it is expected to have the Pawn already
	// have all required components
	UGSCAbilityInputBindingComponent* InputComponent = OutComponentRequests != nullptr  ?
		Cast<UGSCAbilityInputBindingComponent>(FindOrAddComponentForActor(UGSCAbilityInputBindingComponent::StaticClass(), TargetPawn, *OutComponentRequests)) :
		TargetPawn->FindComponentByClass<UGSCAbilityInputBindingComponent>();
	
	if (InputComponent)
	{
		GSC_PLOG(Verbose, TEXT("Try to setup input binding for '%s': '%s' (%s)"), *InAbilityMapping.InputAction.ToString(), *InAbilityHandle.ToString(), *InAbilitySpec.Handle.ToString())
		if (InAbilityHandle.IsValid())
		{
			// Setup input binding if AbilityHandle is valid and already granted (on authority, or when Game Features is active by default)
			InputComponent->SetInputBinding(InAbilityMapping.InputAction.LoadSynchronous(), InAbilityMapping.TriggerEvent, InAbilityHandle);
		}
		else
		{
			// Register a delegate triggered when ability is granted and available on clients (needed when Game Features are made active during play)
			OutOnGiveAbilityDelegateHandle = ASC->OnGiveAbilityDelegate.AddStatic(
				&FGSCAbilitySystemUtils::HandleOnGiveAbility,
				MakeWeakObjectPtr(InputComponent),
				MakeWeakObjectPtr(InAbilityMapping.InputAction.LoadSynchronous()),
				InAbilityMapping.TriggerEvent,
				InAbilitySpec
			);
		}
	}
	else
	{
		GSC_PLOG(
			Error,
			TEXT(
				"Failed to add an ability input binding component to '%s' -- FindOrAddComponentForActor failed (if added from a GameFeature activating)"
				"or the Pawn doesn't have a valid UGSCAbilityInputBindingComponent actor component."
			),
			*GetNameSafe(TargetPawn)
		);
	}
}

void FGSCAbilitySystemUtils::TryGrantAttributes(UAbilitySystemComponent* InASC, const FGSCGameFeatureAttributeSetMapping& InAttributeSetMapping, UAttributeSet*& OutAttributeSet)
{
	check(InASC);

	AActor* OwnerActor = InASC->GetOwnerActor();
	if (!IsValid(OwnerActor))
	{
		GSC_PLOG(Error, TEXT("Ability System Component owner actor is not valid"))
		return;
	}

	const TSubclassOf<UAttributeSet> AttributeSetType = InAttributeSetMapping.AttributeSet.LoadSynchronous();
	if (!AttributeSetType)
	{
		GSC_PLOG(Error, TEXT("AttributeSet class is invalid"))
		return;
	}

	// Prevent adding the same attribute set multiple times (if already registered by another GF or on Actor ASC directly)
	if (UAttributeSet* AttributeSet = GetAttributeSet(InASC, AttributeSetType))
	{
		OutAttributeSet = AttributeSet;
		// GSC_PLOG(Warning, TEXT("%s AttributeSet is already added to %s"), *AttributeSetType->GetName(), *OwnerActor->GetName())
		return;
	}

	OutAttributeSet = NewObject<UAttributeSet>(OwnerActor, AttributeSetType);
	if (!InAttributeSetMapping.InitializationData.IsNull())
	{
		const UDataTable* InitData = InAttributeSetMapping.InitializationData.LoadSynchronous();
		if (InitData)
		{
			OutAttributeSet->InitFromMetaDataTable(InitData);
		}
	}

	InASC->AddAttributeSetSubobject(OutAttributeSet);
}

void FGSCAbilitySystemUtils::TryGrantGameplayEffect(UAbilitySystemComponent* InASC, const TSubclassOf<UGameplayEffect> InEffectType, const float InLevel, TArray<FActiveGameplayEffectHandle>& OutEffectHandles)
{
	check(InASC);

	if (!InASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	if (!InEffectType)
	{
		GSC_PLOG(Warning, TEXT("Trying to apply an effect from an invalid class"))
		return;
	}

	// Don't apply if ASC already has it
	TArray<FActiveGameplayEffectHandle> ActiveEffects;
	if (HasGameplayEffectApplied(InASC, InEffectType, ActiveEffects))
	{
		// Return the list of found effects already applied as part of this ability set handle (would be removed when set is removed)
		OutEffectHandles = MoveTemp(ActiveEffects);
		return;
	}

	const FGameplayEffectContextHandle EffectContext = InASC->MakeEffectContext();
	const FGameplayEffectSpecHandle NewHandle = InASC->MakeOutgoingSpec(InEffectType, InLevel, EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle EffectHandle = InASC->ApplyGameplayEffectSpecToSelf(*NewHandle.Data.Get());
		OutEffectHandles.Add(MoveTemp(EffectHandle));
	}
}

bool FGSCAbilitySystemUtils::TryGrantAbilitySet(UAbilitySystemComponent* InASC, const UGSCAbilitySet* InAbilitySet, FGSCAbilitySetHandle& OutAbilitySetHandle, TArray<TSharedPtr<FComponentRequestHandle>>* OutComponentRequests)
{
	check(InASC);
	
	if (!InAbilitySet)
	{
		return false;
	}

	{
		using namespace UE::GASCompanion::Log;
		const FString WorldPrefix = GetWorldLogPrefix(InASC->GetWorld());
		GSC_PLOG(
			Verbose,
			TEXT("%s: IsOwnerActorAuthoritative: %s, OuterOwner: %s, Role: %s, HasAuthority: %s"),
			*WorldPrefix,
			*GetBoolText(InASC->IsOwnerActorAuthoritative()),
			*GetNameSafe(InASC->GetOwner()),
			*UEnum::GetValueAsString<ENetRole>(InASC->GetOwnerRole()),
			*GetBoolText(InASC->GetOwner()->HasAuthority())
		);
	}

	// Add Abilities
	int32 AbilitiesIndex = 0;
	for (const FGSCGameFeatureAbilityMapping& AbilityMapping : InAbilitySet->GrantedAbilities)
	{
		AbilitiesIndex++;
		
		if (AbilityMapping.AbilityType.IsNull())
		{
			GSC_PLOG(Error, TEXT("GrantedAbilities AbilityType on ability set %s is not valid at Index %d"), *GetNameSafe(InAbilitySet), AbilitiesIndex  - 1);
			continue;
		}

		// Try to grant the ability first
		FGameplayAbilitySpec AbilitySpec;
		FGameplayAbilitySpecHandle AbilityHandle;
		TryGrantAbility(InASC, AbilityMapping, AbilityHandle, AbilitySpec);
		OutAbilitySetHandle.Abilities.Add(AbilityHandle);

		// Handle Input Mapping now
		if (!AbilityMapping.InputAction.IsNull())
		{
			FDelegateHandle DelegateHandle;
			TryBindAbilityInput(InASC, AbilityMapping, AbilityHandle, AbilitySpec, DelegateHandle, OutComponentRequests);
			OutAbilitySetHandle.InputBindingDelegateHandles.Add(MoveTemp(DelegateHandle));
		}
	}

	// Add Attributes
	int32 AttributesIndex = 0;
	for (const FGSCGameFeatureAttributeSetMapping& Attributes : InAbilitySet->GrantedAttributes)
	{
		AttributesIndex++;

		if (Attributes.AttributeSet.IsNull())
		{
			GSC_PLOG(Error, TEXT("GrantedAttributes AttributeSet on ability set %s is not valid at Index %d"), *GetNameSafe(InAbilitySet), AttributesIndex - 1);
			continue;
		}

		UAttributeSet* AddedAttributeSet = nullptr;
		TryGrantAttributes(InASC, Attributes, AddedAttributeSet);

		if (AddedAttributeSet)
		{
			OutAbilitySetHandle.Attributes.Add(AddedAttributeSet);
		}
	}

	// Add Effects
	int32 EffectsIndex = 0;
	for (const FGSCGameFeatureGameplayEffectMapping& Effect : InAbilitySet->GrantedEffects)
	{
		EffectsIndex++;
		
		if (Effect.EffectType.IsNull())
		{
			GSC_PLOG(Error, TEXT("GrantedEffects EffectType on ability set %s is not valid at Index %d"), *GetNameSafe(InAbilitySet), EffectsIndex - 1);
			continue;
		}

		TryGrantGameplayEffect(InASC, Effect.EffectType.LoadSynchronous(), Effect.Level, OutAbilitySetHandle.EffectHandles);
	}

	// Add Owned Gameplay Tags
	if (InAbilitySet->OwnedTags.IsValid())
	{		
		AddLooseGameplayTagsUnique(InASC, InAbilitySet->OwnedTags);

		// Store a copy of the tags, so that they can be removed later on from handle
		OutAbilitySetHandle.OwnedTags = InAbilitySet->OwnedTags;
	}
	
	// Store the name of the Ability Set "instigator"
	OutAbilitySetHandle.AbilitySetPathName = InAbilitySet->GetPathName();
	return true;
}

UAttributeSet* FGSCAbilitySystemUtils::GetAttributeSet(const UAbilitySystemComponent* InASC, const TSubclassOf<UAttributeSet> InAttributeSet)
{
	check(InASC);

	for (UAttributeSet* SpawnedAttribute : InASC->GetSpawnedAttributes())
	{
		if (SpawnedAttribute && SpawnedAttribute->IsA(InAttributeSet))
		{
			return SpawnedAttribute;
		}
	}

	return nullptr;
}

bool FGSCAbilitySystemUtils::HasGameplayEffectApplied(const UAbilitySystemComponent* InASC, const TSubclassOf<UGameplayEffect>& InEffectType, TArray<FActiveGameplayEffectHandle>& OutEffectHandles)
{
	check(InASC);

	// Note: Now thinking about the case of stacking GEs, we might want to always allow a GE to apply
	
	// For now, don't allow sets to add multiple instance of the same effect, even if applied from different ability sets or subsequent add calls of the same set
	// This is mostly to stay consistent with the behavior of abilities / attributes and tags, where if it were already applied, those are not applied or granted again

	FGameplayEffectQuery Query;
	Query.EffectDefinition = InEffectType;
	OutEffectHandles = InASC->GetActiveEffects(Query);
	return !OutEffectHandles.IsEmpty();
}

bool FGSCAbilitySystemUtils::IsAbilityGranted(const UAbilitySystemComponent* InASC, TSubclassOf<UGameplayAbility> InAbility, const int32 InLevel)
{
	check(InASC);
	
	// Check for activatable abilities, if one is matching the given Ability type, prevent re adding again
	TArray<FGameplayAbilitySpec> AbilitySpecs = InASC->GetActivatableAbilities();
	for (const FGameplayAbilitySpec& ActivatableAbility : AbilitySpecs)
	{
		if (!ActivatableAbility.Ability)
		{
			continue;
		}

		if (ActivatableAbility.Ability->GetClass() == InAbility && ActivatableAbility.Level == InLevel)
		{
			return true;
		}
	}

	return false;
}

UActorComponent* FGSCAbilitySystemUtils::FindOrAddComponentForActor(UClass* InComponentType, const AActor* InActor, TArray<TSharedPtr<FComponentRequestHandle>>& OutComponentRequests)
{
	check(InActor);
	
	UActorComponent* Component = InActor->FindComponentByClass(InComponentType);

	bool bMakeComponentRequest = Component == nullptr;
	if (Component)
	{
		// Check to see if this component was created from a different `UGameFrameworkComponentManager` request.
		// `Native` is what `CreationMethod` defaults to for dynamically added components.
		if (Component->CreationMethod == EComponentCreationMethod::Native)
		{
			// Attempt to tell the difference between a true native component and one created by the GameFrameworkComponent system.
			// If it is from the UGameFrameworkComponentManager, then we need to make another request (requests are ref counted).
			const UObject* ComponentArchetype = Component->GetArchetype();
			bMakeComponentRequest = ComponentArchetype->HasAnyFlags(RF_ClassDefaultObject);
		}
	}

	if (bMakeComponentRequest)
	{
		const UWorld* World = InActor->GetWorld();
		const UGameInstance* GameInstance = World->GetGameInstance();

		if (UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			const TSoftClassPtr<AActor> ActorClass = InActor->GetClass();
			const TSharedPtr<FComponentRequestHandle> RequestHandle = ComponentMan->AddComponentRequest(ActorClass, InComponentType);
			OutComponentRequests.Add(RequestHandle);
		}

		if (!Component)
		{
			Component = InActor->FindComponentByClass(InComponentType);
			ensureAlways(Component);
		}
	}

	return Component;
}

void FGSCAbilitySystemUtils::AddLooseGameplayTagsUnique(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InTags, const bool bReplicated)
{
	check(InASC);

	FGameplayTagContainer ExistingTags;
	InASC->GetOwnedGameplayTags(ExistingTags);

	TArray<FGameplayTag> TagsToCheck;
	InTags.GetGameplayTagArray(TagsToCheck);

	FGameplayTagContainer TagsToAdd;
	
	// Check existing tags and build the container to add
	for (const FGameplayTag& Tag : TagsToCheck)
	{
		if (ExistingTags.HasTagExact(Tag))
		{
			// Tag already present
			continue;
		}

		TagsToAdd.AddTag(Tag);
	}

	// Add the resulting tags container, with all tags that were not owned by the ASC yet
	InASC->AddLooseGameplayTags(TagsToAdd);
	if (bReplicated)
	{
		InASC->AddReplicatedLooseGameplayTags(TagsToAdd);
	}
}

void FGSCAbilitySystemUtils::RemoveLooseGameplayTagsUnique(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InTags, const bool bReplicated)
{
	check(InASC);

	FGameplayTagContainer ExistingTags;
	InASC->GetOwnedGameplayTags(ExistingTags);

	TArray<FGameplayTag> TagsToCheck;
	InTags.GetGameplayTagArray(TagsToCheck);

	FGameplayTagContainer TagsToRemove;
	
	// Check existing tags and build the container to add
	for (const FGameplayTag& Tag : TagsToCheck)
	{
		if (ExistingTags.HasTagExact(Tag))
		{
			// Tag is present
			TagsToRemove.AddTag(Tag);
		}
	}

	// Add the resulting tags container, with all tags that were not owned by the ASC yet
	InASC->RemoveLooseGameplayTags(TagsToRemove);
	if (bReplicated)
	{
		InASC->RemoveReplicatedLooseGameplayTags(TagsToRemove);
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
// ReSharper disable once CppPassValueParameterByConstReference
void FGSCAbilitySystemUtils::HandleOnGiveAbility(FGameplayAbilitySpec& InAbilitySpec, TWeakObjectPtr<UGSCAbilityInputBindingComponent> InInputComponent, TWeakObjectPtr<UInputAction> InInputAction, const EGSCAbilityTriggerEvent InTriggerEvent, FGameplayAbilitySpec InNewAbilitySpec)
{
	GSC_PLOG(
		Verbose,
		TEXT("Handle: %s, Ability: %s, Input: %s (TriggerEvent: %s) - (InputComponent: %s)"),
		*InAbilitySpec.Handle.ToString(),
		*GetNameSafe(InAbilitySpec.Ability),
		*GetNameSafe(InInputAction.Get()),
		*UEnum::GetValueAsName(InTriggerEvent).ToString(),
		*GetNameSafe(InInputComponent.Get())
	);

	if (InInputComponent.IsValid() && InInputAction.IsValid() && InAbilitySpec.Ability == InNewAbilitySpec.Ability)
	{
		InInputComponent->SetInputBinding(InInputAction.Get(), InTriggerEvent, InAbilitySpec.Handle);
	}
}
