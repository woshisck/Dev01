// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Components/GSCAbilityInputBindingComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GSCLog.h"

namespace GSCAbilityInputBindingComponent_Impl
{
	constexpr int32 InvalidInputID = 0;
	int32 IncrementingInputID = InvalidInputID;

	static int32 GetNextInputID()
	{
		return ++IncrementingInputID;
	}
}

void UGSCAbilityInputBindingComponent::SetupPlayerControls_Implementation(UEnhancedInputComponent* PlayerInputComponent)
{
	ResetBindings();
	
	GSC_WLOG(Verbose, TEXT("Setup player controls (MappedAbilities: %d)"), MappedAbilities.Num())

	for (const auto& Ability : MappedAbilities)
	{
		UInputAction* InputAction = Ability.Key;
		const FGSCAbilityInputBinding AbilityInputBinding = Ability.Value;

		// Convert out internal enum to the real Input Trigger Event for Enhanced Input
		const ETriggerEvent TriggerEvent = GetInputActionTriggerEvent(AbilityInputBinding.TriggerEvent);

		GSC_PLOG(Log, TEXT("setup pressed handle for %s with %s"), *GetNameSafe(InputAction), *UEnum::GetValueAsName(TriggerEvent).ToString())

		// Pressed event
		const uint32 PressedHandle = InputComponent->BindAction(InputAction, TriggerEvent, this, &UGSCAbilityInputBindingComponent::OnAbilityInputPressed, InputAction).GetHandle();
		RegisteredInputHandles.AddUnique(PressedHandle);

		// Released event
		const uint32 ReleasedHandle = InputComponent->BindAction(InputAction, ETriggerEvent::Completed, this, &UGSCAbilityInputBindingComponent::OnAbilityInputReleased, InputAction).GetHandle();
		RegisteredInputHandles.AddUnique(ReleasedHandle);
	}

	if (TargetInputConfirm)
	{
		// Convert out internal enum to the real Input Trigger Event for Enhanced Input
		const ETriggerEvent TriggerEvent = GetInputActionTriggerEvent(TargetConfirmTriggerEvent);
		OnConfirmHandle = InputComponent->BindAction(TargetInputConfirm, TriggerEvent, this, &UGSCAbilityInputBindingComponent::OnLocalInputConfirm).GetHandle();
	}

	if (TargetInputCancel)
	{
		// Convert out internal enum to the real Input Trigger Event for Enhanced Input
		const ETriggerEvent TriggerEvent = GetInputActionTriggerEvent(TargetCancelTriggerEvent);
		OnCancelHandle = InputComponent->BindAction(TargetInputCancel, TriggerEvent, this, &UGSCAbilityInputBindingComponent::OnLocalInputCancel).GetHandle();
	}

	RunAbilitySystemSetup();
}

void UGSCAbilityInputBindingComponent::ReleaseInputComponent(AController* OldController)
{
	ResetBindings();

	Super::ReleaseInputComponent();
}

FGameplayAbilitySpecHandle UGSCAbilityInputBindingComponent::GiveAbilityWithInput(const TSubclassOf<UGameplayAbility> Ability, const int32 Level, UInputAction* InputAction, const EGSCAbilityTriggerEvent TriggerEvent)
{
	GSC_WLOG(
		Verbose,
		TEXT("Granting Ability \"%s\" (Level: %d) with Input: %s (TriggerEvent): %s"),
		*GetNameSafe(Ability),
		Level,
		*GetNameSafe(InputAction),
		*UEnum::GetValueAsString(TriggerEvent)
	);

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		GSC_PLOG(Error, TEXT("Unable to retrieve Ability System Component from Actor `%s`"), *GetNameSafe(GetOwner()));
		return FGameplayAbilitySpecHandle();
	}

	// Build and validate the ability spec
	const FGameplayAbilitySpec AbilitySpec = ASC->BuildAbilitySpecFromClass(Ability, Level);

	// Validate the class
	if (!IsValid(AbilitySpec.Ability))
	{
		GSC_PLOG(Error, TEXT("Called with an invalid Ability Class."));
		return FGameplayAbilitySpecHandle();
	}

	// Must be called from Authority
	if (!ASC->IsOwnerActorAuthoritative())
	{
		GSC_PLOG(Error, TEXT("Called on ability %s on the client, must be run from authority."), *AbilitySpec.Ability->GetName());
		return FGameplayAbilitySpecHandle();
	}
	
	// Grant the ability. This will run validation and authority checks
	const FGameplayAbilitySpecHandle AbilityHandle = ASC->GiveAbility(AbilitySpec);

	// Try setup the ability input binding on client if InputAction is passed in
	if (AbilityHandle.IsValid() && InputAction)
	{
		ClientBindInput(InputAction, TriggerEvent, AbilityHandle);
	}
	
	return AbilityHandle;
}

void UGSCAbilityInputBindingComponent::ClientBindInput_Implementation(UInputAction* InInputAction, const EGSCAbilityTriggerEvent InTriggerEvent, const FGameplayAbilitySpecHandle& InAbilityHandle)
{
	SetInputBinding(InInputAction, InTriggerEvent, InAbilityHandle);
}

void UGSCAbilityInputBindingComponent::SetInputBinding(UInputAction* InputAction, const EGSCAbilityTriggerEvent TriggerEvent, const FGameplayAbilitySpecHandle AbilityHandle)
{
	using namespace GSCAbilityInputBindingComponent_Impl;

	FGameplayAbilitySpec* BindingAbility = FindAbilitySpec(AbilityHandle);

	FGSCAbilityInputBinding* AbilityInputBinding = MappedAbilities.Find(InputAction);
	if (AbilityInputBinding)
	{
		FGameplayAbilitySpec* OldBoundAbility = FindAbilitySpec(AbilityInputBinding->BoundAbilitiesStack.Top());
		if (OldBoundAbility && OldBoundAbility->InputID == AbilityInputBinding->InputID)
		{
			OldBoundAbility->InputID = InvalidInputID;
		}
	}
	else
	{
		AbilityInputBinding = &MappedAbilities.Add(TObjectPtr<UInputAction>(InputAction));
		AbilityInputBinding->InputID = GetNextInputID();
		AbilityInputBinding->TriggerEvent = TriggerEvent;
	}

	if (BindingAbility)
	{
		BindingAbility->InputID = AbilityInputBinding->InputID;
	}

	AbilityInputBinding->BoundAbilitiesStack.Push(AbilityHandle);
	TryBindAbilityInput(InputAction, *AbilityInputBinding);
}

void UGSCAbilityInputBindingComponent::ClearInputBinding(const FGameplayAbilitySpecHandle AbilityHandle)
{
	using namespace GSCAbilityInputBindingComponent_Impl;

	FGameplayAbilitySpec* FoundAbility = FindAbilitySpec(AbilityHandle);
	if (!FoundAbility)
	{
		return;
	}

	// Find the mapping for this ability
	auto MappedIterator = MappedAbilities.CreateIterator();
	while (MappedIterator)
	{
		if (MappedIterator.Value().InputID == FoundAbility->InputID)
		{
			break;
		}

		++MappedIterator;
	}

	if (MappedIterator)
	{
		FGSCAbilityInputBinding& AbilityInputBinding = MappedIterator.Value();

		if (AbilityInputBinding.BoundAbilitiesStack.Remove(AbilityHandle) > 0)
		{
			if (AbilityInputBinding.BoundAbilitiesStack.Num() > 0)
			{
				FGameplayAbilitySpec* StackedAbility = FindAbilitySpec(AbilityInputBinding.BoundAbilitiesStack.Top());
				if (StackedAbility && StackedAbility->InputID == 0)
				{
					StackedAbility->InputID = AbilityInputBinding.InputID;
				}
			}
			else
			{
				// NOTE: This will invalidate the `AbilityInputBinding` ref above
				RemoveEntry(MappedIterator.Key());
			}
			// DO NOT act on `AbilityInputBinding` after here (it could have been removed)


			FoundAbility->InputID = InvalidInputID;
		}
	}
}

void UGSCAbilityInputBindingComponent::ClearAbilityBindings(UInputAction* InputAction)
{
	RemoveEntry(InputAction);
}

UInputAction* UGSCAbilityInputBindingComponent::GetBoundInputActionForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
	{
		GSC_WLOG(Error, TEXT("Passed in ability is null."))
		return nullptr;
	}

	UAbilitySystemComponent* AbilitySystemComponent = Ability->IsInstantiated() && Ability->GetCurrentActorInfo() ?
		Ability->GetAbilitySystemComponentFromActorInfo() :
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	
	if (!AbilitySystemComponent)
	{
		GSC_WLOG(Error, TEXT("Trying to find input action for %s but unable to find an AbilitySystemComponent."), *GetNameSafe(Ability))
		return nullptr;
	}

	// Ensure and update inputs ID for specs based on mapped abilities.
	UpdateAbilitySystemBindings(AbilitySystemComponent);

	const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(Ability->GetClass());
	if (!AbilitySpec)
	{
		GSC_WLOG(Error, TEXT("AbilitySystemComponent could not return Ability Spec for %s."), *GetNameSafe(Ability->GetClass()))
		return nullptr;
	}

	return GetBoundInputActionForAbilitySpec(AbilitySpec);
}

// ReSharper disable once CppPassValueParameterByConstReference
UInputAction* UGSCAbilityInputBindingComponent::GetBoundInputActionForAbilityClass(TSubclassOf<UGameplayAbility> InAbilityClass)
{
	if (!InAbilityClass)
	{
		GSC_WLOG(Error, TEXT("Passed in ability class is invalid."))
		return nullptr;
	}
	
	return GetBoundInputActionForAbility(InAbilityClass.GetDefaultObject());
}

UInputAction* UGSCAbilityInputBindingComponent::GetBoundInputActionForAbilitySpec(const FGameplayAbilitySpec* AbilitySpec) const
{
	check(AbilitySpec);

	UInputAction* FoundInputAction = nullptr;
	for (const TPair<TObjectPtr<UInputAction>, FGSCAbilityInputBinding>& MappedAbility : MappedAbilities)
	{
		const FGSCAbilityInputBinding AbilityInputBinding = MappedAbility.Value;
		if (AbilityInputBinding.InputID == AbilitySpec->InputID)
		{
			FoundInputAction = MappedAbility.Key;
			break;
		}
	}

	return FoundInputAction;
}

void UGSCAbilityInputBindingComponent::ResetBindings()
{
	for (const TPair<TObjectPtr<UInputAction>, FGSCAbilityInputBinding>& InputBinding : MappedAbilities)
	{
		if (AbilityComponent)
		{
			const int32 ExpectedInputID = InputBinding.Value.InputID;

			for (const FGameplayAbilitySpecHandle AbilityHandle : InputBinding.Value.BoundAbilitiesStack)
			{
				FGameplayAbilitySpec* FoundAbility = AbilityComponent->FindAbilitySpecFromHandle(AbilityHandle);
				if (FoundAbility && FoundAbility->InputID == ExpectedInputID)
				{
					FoundAbility->InputID = GSCAbilityInputBindingComponent_Impl::InvalidInputID;
				}
			}
		}
	}

	if (InputComponent)
	{
		InputComponent->RemoveBindingByHandle(OnConfirmHandle);
		InputComponent->RemoveBindingByHandle(OnCancelHandle);

		// Note: OnConfirmHandle and OnCancelHandle could be added to this array instead now as well.
		for (const uint32 InputHandle : RegisteredInputHandles)
		{
			InputComponent->RemoveBindingByHandle(InputHandle);
		}

		RegisteredInputHandles.Reset();
	}

	AbilityComponent = nullptr;
}

void UGSCAbilityInputBindingComponent::RunAbilitySystemSetup()
{
	const AActor* MyOwner = GetOwner();
	check(MyOwner);

	AbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyOwner);
	if (AbilityComponent)
	{
		for (auto& InputBinding : MappedAbilities)
		{
			const int32 NewInputID = GSCAbilityInputBindingComponent_Impl::GetNextInputID();
			InputBinding.Value.InputID = NewInputID;

			for (const FGameplayAbilitySpecHandle AbilityHandle : InputBinding.Value.BoundAbilitiesStack)
			{
				FGameplayAbilitySpec* FoundAbility = AbilityComponent->FindAbilitySpecFromHandle(AbilityHandle);
				if (FoundAbility != nullptr)
				{
					FoundAbility->InputID = NewInputID;
				}
			}
		}
	}
}

void UGSCAbilityInputBindingComponent::UpdateAbilitySystemBindings(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		GSC_LOG(Error, TEXT("UGSCAbilityInputBindingComponent::UpdateAbilitySystemBindings - Passed in ASC is invalid"))
		return;
	}

	for (auto& InputBinding : MappedAbilities)
	{
		const int32 InputID = InputBinding.Value.InputID;
		if (InputID <= 0)
		{
			continue;
		}

		for (const FGameplayAbilitySpecHandle AbilityHandle : InputBinding.Value.BoundAbilitiesStack)
		{
			FGameplayAbilitySpec* FoundAbility = AbilitySystemComponent->FindAbilitySpecFromHandle(AbilityHandle);
			if (FoundAbility != nullptr)
			{
				FoundAbility->InputID = InputID;
			}
		}
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCAbilityInputBindingComponent::OnAbilityInputPressed(UInputAction* InputAction)
{
	// The AbilitySystemComponent may not have been valid when we first bound input... try again.
	if (AbilityComponent)
	{
		UpdateAbilitySystemBindings(AbilityComponent);
	}
	else
	{
		RunAbilitySystemSetup();
	}

	if (AbilityComponent)
	{
		using namespace GSCAbilityInputBindingComponent_Impl;

		const FGSCAbilityInputBinding* FoundBinding = MappedAbilities.Find(InputAction);
		if (FoundBinding && ensure(FoundBinding->InputID != InvalidInputID))
		{
			AbilityComponent->AbilityLocalInputPressed(FoundBinding->InputID);
		}
	}
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UGSCAbilityInputBindingComponent::OnAbilityInputReleased(UInputAction* InputAction)
{
	// The AbilitySystemComponent may need to have specs inputID updated here for clients... try again.
	UpdateAbilitySystemBindings(AbilityComponent);

	if (AbilityComponent)
	{
		using namespace GSCAbilityInputBindingComponent_Impl;

		const FGSCAbilityInputBinding* FoundBinding = MappedAbilities.Find(InputAction);
		if (FoundBinding && ensure(FoundBinding->InputID != InvalidInputID))
		{
			AbilityComponent->AbilityLocalInputReleased(FoundBinding->InputID);
		}
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGSCAbilityInputBindingComponent::OnLocalInputConfirm()
{
	GSC_LOG(Verbose, TEXT("UGSCAbilityInputBindingComponent::OnLocalInputConfirm"))

	if (AbilityComponent)
	{
		AbilityComponent->LocalInputConfirm();
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UGSCAbilityInputBindingComponent::OnLocalInputCancel()
{
	GSC_LOG(Verbose, TEXT("UGSCAbilityInputBindingComponent::OnLocalInputCancel"))
	if (AbilityComponent)
	{
		AbilityComponent->LocalInputCancel();
	}
}

void UGSCAbilityInputBindingComponent::RemoveEntry(const UInputAction* InputAction)
{
	if (FGSCAbilityInputBinding* Bindings = MappedAbilities.Find(InputAction))
	{
		if (InputComponent)
		{
			InputComponent->RemoveBindingByHandle(Bindings->OnPressedHandle);
			InputComponent->RemoveBindingByHandle(Bindings->OnReleasedHandle);
		}

		for (const FGameplayAbilitySpecHandle AbilityHandle : Bindings->BoundAbilitiesStack)
		{
			using namespace GSCAbilityInputBindingComponent_Impl;

			FGameplayAbilitySpec* AbilitySpec = FindAbilitySpec(AbilityHandle);
			if (AbilitySpec && AbilitySpec->InputID == Bindings->InputID)
			{
				AbilitySpec->InputID = InvalidInputID;
			}
		}

		MappedAbilities.Remove(InputAction);
	}
}

FGameplayAbilitySpec* UGSCAbilityInputBindingComponent::FindAbilitySpec(const FGameplayAbilitySpecHandle Handle) const
{
	FGameplayAbilitySpec* FoundAbility = nullptr;
	if (AbilityComponent)
	{
		FoundAbility = AbilityComponent->FindAbilitySpecFromHandle(Handle);
	}
	return FoundAbility;
}

void UGSCAbilityInputBindingComponent::TryBindAbilityInput(UInputAction* InputAction, FGSCAbilityInputBinding& AbilityInputBinding)
{
	GSC_WLOG(Verbose, TEXT("Setup player controls for %s (InputID: %d)"), *GetNameSafe(InputAction), AbilityInputBinding.InputID)
	
	if (InputComponent)
	{
		// Pressed event
		if (AbilityInputBinding.OnPressedHandle == 0)
		{
			// Convert out internal enum to the real Input Trigger Event for Enhanced Input
			const ETriggerEvent TriggerEvent = GetInputActionTriggerEvent(AbilityInputBinding.TriggerEvent);

			GSC_PLOG(Log, TEXT("Setup player controls pressed handle for %s with %s"), *GetNameSafe(InputAction), *UEnum::GetValueAsName(TriggerEvent).ToString())
			AbilityInputBinding.OnPressedHandle = InputComponent->BindAction(InputAction, TriggerEvent, this, &UGSCAbilityInputBindingComponent::OnAbilityInputPressed, InputAction).GetHandle();
			RegisteredInputHandles.AddUnique(AbilityInputBinding.OnPressedHandle);
		}

		// Released event
		if (AbilityInputBinding.OnReleasedHandle == 0)
		{
			AbilityInputBinding.OnReleasedHandle = InputComponent->BindAction(InputAction, ETriggerEvent::Completed, this, &UGSCAbilityInputBindingComponent::OnAbilityInputReleased, InputAction).GetHandle();
			RegisteredInputHandles.AddUnique(AbilityInputBinding.OnReleasedHandle);
		}
	}
}

ETriggerEvent UGSCAbilityInputBindingComponent::GetInputActionTriggerEvent(const EGSCAbilityTriggerEvent TriggerEvent)
{
	return TriggerEvent == EGSCAbilityTriggerEvent::Started ? ETriggerEvent::Started :
		TriggerEvent == EGSCAbilityTriggerEvent::Triggered ? ETriggerEvent::Triggered :
		ETriggerEvent::Started;
}
