// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Abilities/GSCAbilitySystemComponent.h"

#include "GSCLog.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Abilities/GSCGameplayAbility_MeleeBase.h"
#include "Animation/AnimInstance.h"
#include "Animations/GSCNativeAnimInstanceInterface.h"
#include "Components/GSCAbilityInputBindingComponent.h"
#include "Components/GSCAbilityQueueComponent.h"
#include "Components/GSCComboManagerComponent.h"
#include "Components/GSCCoreComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Runtime/Launch/Resources/Version.h"

void UGSCAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	AbilityActivatedCallbacks.AddUObject(this, &UGSCAbilitySystemComponent::OnAbilityActivatedCallback);
	AbilityFailedCallbacks.AddUObject(this, &UGSCAbilitySystemComponent::OnAbilityFailedCallback);
	AbilityEndedCallbacks.AddUObject(this, &UGSCAbilitySystemComponent::OnAbilityEndedCallback);

	// Grant startup effects on begin play instead of from within InitAbilityActorInfo to avoid
	// "ticking" periodic effects when BP is first opened
	GrantStartupEffects();
}

void UGSCAbilitySystemComponent::BeginDestroy()
{
	// Reset ...

	// Clear any delegate handled bound previously for this component
	if (AbilityActorInfo && AbilityActorInfo->OwnerActor.IsValid())
	{
		if (UGameInstance* GameInstance = AbilityActorInfo->OwnerActor->GetGameInstance())
		{
			GameInstance->GetOnPawnControllerChanged().RemoveAll(this);
		}
	}

	OnGiveAbilityDelegate.RemoveAll(this);

	// Remove any added attributes
	for (UAttributeSet* AttribSetInstance : AddedAttributes)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		RemoveSpawnedAttribute(AttribSetInstance);
#else
		GetSpawnedAttributes_Mutable().Remove(AttribSetInstance);
#endif
	}

	// Clear up abilities / bindings
	UGSCAbilityInputBindingComponent* InputComponent = AbilityActorInfo && AbilityActorInfo->AvatarActor.IsValid() ? AbilityActorInfo->AvatarActor->FindComponentByClass<UGSCAbilityInputBindingComponent>() : nullptr;

	for (const FGSCMappedAbility& DefaultAbilityHandle : AddedAbilityHandles)
	{
		if (InputComponent)
		{
			InputComponent->ClearInputBinding(DefaultAbilityHandle.Handle);
		}

		// Only Clear abilities on authority
		if (IsOwnerActorAuthoritative())
		{
			SetRemoveAbilityOnEnd(DefaultAbilityHandle.Handle);
		}
	}

	// Clear up any bound delegates in Core Component that were registered from InitAbilityActorInfo
	UGSCCoreComponent* CoreComponent = AbilityActorInfo && AbilityActorInfo->AvatarActor.IsValid() ? AbilityActorInfo->AvatarActor->FindComponentByClass<UGSCCoreComponent>() : nullptr;
	if (CoreComponent)
	{
		CoreComponent->ShutdownAbilitySystemDelegates(this);
	}

	AddedAbilityHandles.Reset();
	AddedAttributes.Reset();
	AddedEffects.Reset();
	AddedAbilitySets.Reset();

	Super::BeginDestroy();
}

void UGSCAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	GSC_WLOG(Verbose, TEXT("Owner: %s, Avatar: %s, World: %s (IsGameWorld: %s)"), *GetNameSafe(InOwnerActor), *GetNameSafe(InAvatarActor), *GetNameSafe(GetWorld()), GetWorld()->IsGameWorld() ? TEXT("true") : TEXT("false"))

	// Only run if we're any type of game world (including PIE)
	// This prevents unwanted initialization of Ability System during Editor initialization times (ActorPreview, Thumbnail, etc.)
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return;
	}

	if (AbilityActorInfo && InOwnerActor)
	{
		if (AbilityActorInfo->AnimInstance == nullptr)
		{
			AbilityActorInfo->AnimInstance = AbilityActorInfo->GetAnimInstance();
		}

		if (UGameInstance* GameInstance = InOwnerActor->GetGameInstance())
		{
			// Sign up for possess / un possess events so that we can update the cached AbilityActorInfo accordingly
			if (!GameInstance->GetOnPawnControllerChanged().Contains(this, TEXT("OnPawnControllerChanged")))
			{
				GameInstance->GetOnPawnControllerChanged().AddDynamic(this, &UGSCAbilitySystemComponent::OnPawnControllerChanged);
			}
		}

		UAnimInstance* AnimInstance = AbilityActorInfo->GetAnimInstance();
		if (IGSCNativeAnimInstanceInterface* AnimInstanceInterface = Cast<IGSCNativeAnimInstanceInterface>(AnimInstance))
		{
			GSC_WLOG(Verbose, TEXT("Initialize `%s` AnimInstance with Ability System"), *GetNameSafe(AnimInstance))
			AnimInstanceInterface->InitializeWithAbilitySystem(this);
		}
	}

	GrantDefaultAbilitiesAndAttributes(InOwnerActor, InAvatarActor);
	GrantDefaultAbilitySets(InOwnerActor, InAvatarActor);

	// For PlayerState client pawns, setup and update owner on companion components if pawns have them
	UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(InAvatarActor);
	if (CoreComponent)
	{
		CoreComponent->SetupOwner();
		CoreComponent->RegisterAbilitySystemDelegates(this);
		CoreComponent->SetStartupAbilitiesGranted(true);
	}

	// Broadcast to Blueprint InitAbilityActorInfo was called
	//
	// This will happen multiple times for both client / server
	OnInitAbilityActorInfo.Broadcast();
	if (CoreComponent)
	{
		CoreComponent->OnInitAbilityActorInfo.Broadcast();
	}
}


void UGSCAbilitySystemComponent::AbilityLocalInputPressed(const int32 InputID)
{
	// Consume the input if this InputID is overloaded with GenericConfirm/Cancel and the GenericConfim/Cancel callback is bound
	if (IsGenericConfirmInputBound(InputID))
	{
		LocalInputConfirm();
		return;
	}

	if (IsGenericCancelInputBound(InputID))
	{
		LocalInputCancel();
		return;
	}

	// ---------------------------------------------------------

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.InputID == InputID && Spec.Ability)
		{
			Spec.InputPressed = true;

			if (Spec.Ability->IsA(UGSCGameplayAbility_MeleeBase::StaticClass()))
			{
				// Ability is a combo ability, try to activate via Combo Component
				if (!IsValid(ComboComponent))
				{
					// Combo Component ref is not set yet, set it once
					ComboComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(GetAvatarActor());
					if (ComboComponent)
					{
						ComboComponent->SetupOwner();
					}
				}

				// Regardless of active or not active, always try to activate the combo. Combo Component will take care of gating activation or queuing next combo
				if (IsValid(ComboComponent))
				{
					// We have a valid combo component, active combo
					ComboComponent->ActivateComboAbility(Spec.Ability->GetClass());
				}
				else
				{
					GSC_LOG(Error, TEXT("UGSCAbilitySystemComponent::AbilityLocalInputPressed - Trying to activate combo without a Combo Manager Component on the Avatar Actor. Make sure to add the component in Blueprint."))
				}
			}
			else
			{
				// Ability is not a combo ability, go through normal workflow
				if (Spec.IsActive())
				{
					if (Spec.Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
					{
						ServerSetInputPressed(Spec.Handle);
					}

					AbilitySpecInputPressed(Spec);

					// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
				}
				else
				{
					TryActivateAbility(Spec.Handle);
				}
			}
		}
	}
}

FGameplayAbilitySpecHandle UGSCAbilitySystemComponent::GrantAbility(const TSubclassOf<UGameplayAbility> Ability, const bool bRemoveAfterActivation)
{
	FGameplayAbilitySpecHandle AbilityHandle;
	if (!IsOwnerActorAuthoritative())
	{
		GSC_LOG(Error, TEXT("UGSCAbilitySystemComponent::GrantAbility Called on non authority"));
		return AbilityHandle;
	}

	if (Ability)
	{
		FGameplayAbilitySpec AbilitySpec(Ability);
		AbilitySpec.RemoveAfterActivation = bRemoveAfterActivation;

		AbilityHandle = GiveAbility(AbilitySpec);
	}
	return AbilityHandle;
}

bool UGSCAbilitySystemComponent::GiveAbilitySet(const UGSCAbilitySet* InAbilitySet, FGSCAbilitySetHandle& OutHandle)
{
	GSC_WLOG(Verbose, TEXT("Granting Ability Set \"%s\" (Owner: %s, Avatar: %s)"), *GetNameSafe(InAbilitySet), *GetNameSafe(GetOwnerActor()), *GetNameSafe(GetAvatarActor_Direct()));

	if (!InAbilitySet)
	{
		GSC_PLOG(Error, TEXT("Called with an invalid Ability Set"));
		return false;
	}

	FText ErrorText;
	if (!InAbilitySet->GrantToAbilitySystem(this, OutHandle, &ErrorText))
	{
		GSC_PLOG(Error, TEXT("Error trying to grant ability set %s - %s"), *GetNameSafe(InAbilitySet), *ErrorText.ToString());
		return false;
	}
	
	return true;
}

bool UGSCAbilitySystemComponent::ClearAbilitySet(FGSCAbilitySetHandle& InAbilitySetHandle)
{
	if (!InAbilitySetHandle.IsValid())
	{
		GSC_PLOG(Error, TEXT("Called with an invalid Ability Set (The handle doesn't have AbilitySetPathName defined)"));
		return false;
	}

	FText ErrorText;
	if (!UGSCAbilitySet::RemoveFromAbilitySystem(this, InAbilitySetHandle, &ErrorText))
	{
		GSC_PLOG(Error, TEXT("Error trying to remove ability set %s - %s"), *InAbilitySetHandle.AbilitySetPathName, *ErrorText.ToString());
		return false;
	}
	
	return true;
}

void UGSCAbilitySystemComponent::OnAbilityActivatedCallback(UGameplayAbility* Ability)
{
	GSC_LOG(Log, TEXT("UGSCAbilitySystemComponent::OnAbilityActivatedCallback %s"), *Ability->GetName());
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		GSC_LOG(Error, TEXT("UGSCAbilitySystemComponent::OnAbilityActivated No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	}

	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityActivated.Broadcast(Ability);
	}
}

void UGSCAbilitySystemComponent::OnAbilityFailedCallback(const UGameplayAbility* Ability, const FGameplayTagContainer& Tags)
{
	GSC_LOG(Log, TEXT("UGSCAbilitySystemComponent::OnAbilityFailedCallback %s"), *Ability->GetName());

	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		GSC_LOG(Warning, TEXT("UGSCAbilitySystemComponent::OnAbilityFailed No OwnerActor for this ability: %s Tags: %s"), *Ability->GetName(), *Tags.ToString());
		return;
	}

	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	UGSCAbilityQueueComponent* AbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityFailed.Broadcast(Ability, Tags);
	}

	if (AbilityQueueComponent)
	{
		AbilityQueueComponent->OnAbilityFailed(Ability, Tags);
	}
}

void UGSCAbilitySystemComponent::OnAbilityEndedCallback(UGameplayAbility* Ability)
{
	GSC_LOG(Log, TEXT("UGSCAbilitySystemComponent::OnAbilityEndedCallback %s"), *Ability->GetName());
	const AActor* Avatar = GetAvatarActor();
	if (!Avatar)
	{
		GSC_LOG(Warning, TEXT("UGSCAbilitySystemComponent::OnAbilityEndedCallback No OwnerActor for this ability: %s"), *Ability->GetName());
		return;
	}

	const UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Avatar);
	UGSCAbilityQueueComponent* AbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Avatar);
	if (CoreComponent)
	{
		CoreComponent->OnAbilityEnded.Broadcast(Ability);
	}

	if (AbilityQueueComponent)
	{
		AbilityQueueComponent->OnAbilityEnded(Ability);
	}
}

bool UGSCAbilitySystemComponent::ShouldGrantAbility(const TSubclassOf<UGameplayAbility> InAbility, const int32 InLevel)
{
	if (bResetAbilitiesOnSpawn)
	{
		// User wants abilities to be granted each time InitAbilityActor is called
		return true;
	}

	// Check for activatable abilities, if one is matching the given Ability type, prevent re adding again
	TArray<FGameplayAbilitySpec> AbilitySpecs = GetActivatableAbilities();
	for (const FGameplayAbilitySpec& ActivatableAbility : AbilitySpecs)
	{
		if (!ActivatableAbility.Ability)
		{
			continue;
		}

		if (ActivatableAbility.Ability->GetClass() == InAbility && ActivatableAbility.Level == InLevel)
		{
			return false;
		}
	}

	return true;
}

bool UGSCAbilitySystemComponent::ShouldGrantAbilitySet(const UGSCAbilitySet* InAbilitySet) const
{
	check(InAbilitySet);

	// Forcefully re-grant ability sets in case owner is PlayerState, this is to ensure input binding still works after a respawn
	// ASC living on Pawns that don't have this problem.
	if (IsPlayerStateOwner())
	{
		return true;
	}

	// ReSharper disable once CppUseStructuredBinding
	for (const FGSCAbilitySetHandle& Handle : AddedAbilitySets)
	{
		if (Handle.AbilitySetPathName == InAbilitySet->GetPathName())
		{
			return false;
		}
	}
	
	return true;
}

bool UGSCAbilitySystemComponent::IsPlayerStateOwner() const
{
	const AActor* LocalOwnerActor = GetOwnerActor();
	return LocalOwnerActor && LocalOwnerActor->IsA<APlayerState>();
}

void UGSCAbilitySystemComponent::GrantDefaultAbilitiesAndAttributes(AActor* InOwnerActor, AActor* InAvatarActor)
{
	GSC_WLOG(Verbose, TEXT("Owner: %s, Avatar: %s"), *GetNameSafe(InOwnerActor), *GetNameSafe(InAvatarActor))

	if (bResetAttributesOnSpawn)
	{
		// Reset/Remove abilities if we had already added them
		for (UAttributeSet* AttributeSet : AddedAttributes)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			RemoveSpawnedAttribute(AttributeSet);
#else
			GetSpawnedAttributes_Mutable().Remove(AttributeSet);
#endif
		}

		AddedAttributes.Empty(GrantedAttributes.Num());
	}


	if (bResetAbilitiesOnSpawn)
	{
		for (const FGSCMappedAbility& DefaultAbilityHandle : AddedAbilityHandles)
		{
			SetRemoveAbilityOnEnd(DefaultAbilityHandle.Handle);
		}

		for (FDelegateHandle InputBindingDelegateHandle : InputBindingDelegateHandles)
		{
			// Clear any delegate handled bound previously for this actor
			OnGiveAbilityDelegate.Remove(InputBindingDelegateHandle);
			InputBindingDelegateHandle.Reset();
		}

		AddedAbilityHandles.Empty(GrantedAbilities.Num());
		InputBindingDelegateHandles.Empty();
	}

	UGSCAbilityInputBindingComponent* InputComponent = IsValid(InAvatarActor) ? InAvatarActor->FindComponentByClass<UGSCAbilityInputBindingComponent>() : nullptr;

	// Startup abilities
	// ReSharper disable once CppUseStructuredBinding
	for (const FGSCAbilityInputMapping& GrantedAbility : GrantedAbilities)
	{
		const TSubclassOf<UGameplayAbility> Ability = GrantedAbility.Ability;
		UInputAction* InputAction = GrantedAbility.InputAction;

		if (!Ability)
		{
			continue;
		}

		FGameplayAbilitySpec NewAbilitySpec = BuildAbilitySpecFromClass(Ability, GrantedAbility.Level);

		// Try to grant the ability first
		if (IsOwnerActorAuthoritative() && ShouldGrantAbility(Ability, GrantedAbility.Level))
		{
			// Only Grant abilities on authority
			GSC_LOG(Log, TEXT("UGSCAbilitySystemComponent::GrantDefaultAbilitiesAndAttributes - Authority, Grant Ability (%s)"), *NewAbilitySpec.Ability->GetClass()->GetName())
			FGameplayAbilitySpecHandle AbilityHandle = GiveAbility(NewAbilitySpec);
			AddedAbilityHandles.Add(FGSCMappedAbility(AbilityHandle, NewAbilitySpec, InputAction));
		}

		// We don't grant here but try to get the spec already granted or register delegate to handle input binding
		if (InputComponent && InputAction)
		{
			// Handle for server or standalone game, clients need to bind OnGiveAbility
			const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(Ability);
			if (AbilitySpec)
			{
				InputComponent->SetInputBinding(InputAction, GrantedAbility.TriggerEvent, AbilitySpec->Handle);
			}
			else
			{
				// Register a delegate triggered when ability is granted and available on clients
				FDelegateHandle DelegateHandle = OnGiveAbilityDelegate.AddUObject(this, &UGSCAbilitySystemComponent::HandleOnGiveAbility, InputComponent, InputAction, GrantedAbility.TriggerEvent, NewAbilitySpec);
				InputBindingDelegateHandles.Add(DelegateHandle);
			}
		}
	}

	// Startup attributes
	for (const FGSCAttributeSetDefinition& AttributeSetDefinition : GrantedAttributes)
	{
		if (AttributeSetDefinition.AttributeSet)
		{
			const bool bHasAttributeSet = GetAttributeSubobject(AttributeSetDefinition.AttributeSet) != nullptr;
			GSC_LOG(
				Verbose,
				TEXT("UGSCAbilitySystemComponent::GrantDefaultAbilitiesAndAttributes - HasAttributeSet: %s (%s)"),
				bHasAttributeSet ? TEXT("true") : TEXT("false"),
				*GetNameSafe(AttributeSetDefinition.AttributeSet)
			)

			// Prevent adding attribute set if already granted
			if (!bHasAttributeSet && InOwnerActor)
			{
				UAttributeSet* AttributeSet = NewObject<UAttributeSet>(InOwnerActor, AttributeSetDefinition.AttributeSet);
				if (AttributeSetDefinition.InitializationData)
				{
					AttributeSet->InitFromMetaDataTable(AttributeSetDefinition.InitializationData);
				}
				AddedAttributes.Add(AttributeSet);
				AddAttributeSetSubobject(AttributeSet);
			}
		}
	}
}

void UGSCAbilitySystemComponent::GrantDefaultAbilitySets(AActor* InOwnerActor, AActor* InAvatarActor)
{
	GSC_WLOG(Verbose, TEXT("Initialize Ability Sets - InOwnerActor: %s, InAvatarActor: %s"), *GetNameSafe(InOwnerActor), *GetNameSafe(InAvatarActor))
	
	if (!IsValid(InOwnerActor) || !IsValid(InAvatarActor))
	{
		return;
	}

	for (const TSoftObjectPtr<UGSCAbilitySet>& AbilitySetEntry : GrantedAbilitySets)
	{
		if (const UGSCAbilitySet* AbilitySet = AbilitySetEntry.LoadSynchronous())
		{
			if (!ShouldGrantAbilitySet(AbilitySet))
			{
				continue;
			}
			
			// Check for input bindings, if we need some, then ensure AvatarActor has the required component to issue a warning if not
			//
			// This also take care of order of initialization in case of Player State characters. On first invocation from InitializeComponent(), the Avatar Actor is likely
			// not yet set and points to owner actor (PlayerState). This is only after ACharacter::PossessedBy() that proper Avatar Actor is set, thus allowing us to get back
			// the Input Binding component and set up the ability bindings.

			// If the set doesn't have any Ability with bindings, then we can grant early.
			if (AbilitySet->HasInputBinding())
			{
				// Binding will only ever happen on Pawn actors, not when AvatarActor is set to PlayerState early on in initialization order
				const APawn* AvatarPawn = Cast<APawn>(InAvatarActor);
				if (!AvatarPawn)
				{
					// Try next time
					return;
				}

				const UGSCAbilityInputBindingComponent* InputBindingComponent = AvatarPawn->FindComponentByClass<UGSCAbilityInputBindingComponent>();
				if (!InputBindingComponent)
				{
					const FText FormatText = NSLOCTEXT(
						"GSCAbilitySystemComponent",
						"Error_AbilitySet_Invalid_InputBindingComponent",
						"The set contains Abilities with Input bindings but {0} is missing the required UGSCAbilityInputBindingComponent actor component."
					);

					const FText ErrorText = FText::Format(FormatText, FText::FromString(AvatarPawn->GetName()));
					GSC_PLOG(Error, TEXT("Error trying to grant ability set %s - %s"), *GetNameSafe(AbilitySet), *ErrorText.ToString());
					return;
				}
			}
			
			FText ErrorText;
			FGSCAbilitySetHandle Handle;
			if (!AbilitySet->GrantToAbilitySystem(this, Handle, &ErrorText, false))
			{
				GSC_PLOG(Error, TEXT("Error trying to grant ability set %s - %s"), *GetNameSafe(AbilitySet), *ErrorText.ToString());
				continue;
			}

			AddedAbilitySets.AddUnique(Handle);
		}
	}
}

void UGSCAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	GSC_WLOG(Verbose, TEXT("%s"), *AbilitySpec.GetDebugString());
	OnGiveAbilityDelegate.Broadcast(AbilitySpec);
}

void UGSCAbilitySystemComponent::GrantStartupEffects()
{
	if (!IsOwnerActorAuthoritative())
	{
		return;
	}

	// Reset/Remove effects if we had already added them
	for (const FActiveGameplayEffectHandle AddedEffect : AddedEffects)
	{
		RemoveActiveGameplayEffect(AddedEffect);
	}

	FGameplayEffectContextHandle EffectContext = MakeEffectContext();
	EffectContext.AddSourceObject(this);

	AddedEffects.Empty(GrantedEffects.Num());

	for (const TSubclassOf<UGameplayEffect>& GameplayEffect : GrantedEffects)
	{
		FGameplayEffectSpecHandle NewHandle = MakeOutgoingSpec(GameplayEffect, 1, EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), this);
			AddedEffects.Add(EffectHandle);
		}
	}
}

// ReSharper disable CppParameterMayBeConstPtrOrRef
void UGSCAbilitySystemComponent::OnPawnControllerChanged(APawn* Pawn, AController* NewController)
{
	if (AbilityActorInfo && AbilityActorInfo->OwnerActor == Pawn && AbilityActorInfo->PlayerController != NewController)
	{
		if (!NewController)
		{
			// NewController null, prevent refresh actor info. Needed to ensure TargetActor EndPlay properly unbind from GenericLocalConfirmCallbacks/GenericLocalCancelCallbacks
			// and avoid an ensure error if ActorInfo PlayerController is invalid
			return;
		}

		AbilityActorInfo->InitFromActor(AbilityActorInfo->OwnerActor.Get(), AbilityActorInfo->AvatarActor.Get(), this);
	}
}

void UGSCAbilitySystemComponent::HandleOnGiveAbility(FGameplayAbilitySpec& AbilitySpec, UGSCAbilityInputBindingComponent* InputComponent, UInputAction* InputAction, EGSCAbilityTriggerEvent TriggerEvent, FGameplayAbilitySpec NewAbilitySpec)
{
	GSC_LOG(
		Log,
		TEXT("UGSCAbilitySystemComponent::HandleOnGiveAbility: %s, Ability: %s, Input: %s (TriggerEvent: %s) - (InputComponent: %s)"),
		*AbilitySpec.Handle.ToString(),
		*GetNameSafe(AbilitySpec.Ability),
		*GetNameSafe(InputAction),
		*UEnum::GetValueAsName(TriggerEvent).ToString(),
		*GetNameSafe(InputComponent)
	);

	if (InputComponent && InputAction && AbilitySpec.Ability == NewAbilitySpec.Ability)
	{
		InputComponent->SetInputBinding(InputAction, TriggerEvent, AbilitySpec.Handle);
	}
}
