// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "UI/GSCUserWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "GSCLog.h"

void UGSCUserWidget::SetOwnerActor(AActor* Actor)
{
	OwnerActor = Actor;
	OwnerCoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(Actor);
	AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
}

// ReSharper disable once CppParameterNamesMismatch
void UGSCUserWidget::InitializeWithAbilitySystem(const UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		GSC_LOG(Error, TEXT("UGSCUserWidget::InitializeWithAbilitySystem - Called with invalid ASC"))
		return;
	}

	if (!OwnerActor)
	{
		SetOwnerActor(InASC->GetOwnerActor());
	}

	if (AbilitySystemComponent && AbilitySystemComponent != InASC)
	{
		ResetAbilitySystem();
	}

	AbilitySystemComponent = const_cast<UAbilitySystemComponent*>(InASC);
	RegisterAbilitySystemDelegates();

	// Broadcast info to Blueprints
	OnAbilitySystemInitialized();
}

void UGSCUserWidget::ResetAbilitySystem()
{
	ShutdownAbilitySystemComponentListeners();
	AbilitySystemComponent = nullptr;
}

void UGSCUserWidget::RegisterAbilitySystemDelegates()
{
	if (!AbilitySystemComponent)
	{
		// Ability System may not have been available yet for character (PlayerState setup on clients)
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	AbilitySystemComponent->GetAllAttributes(Attributes);

	for (FGameplayAttribute Attribute : Attributes)
	{
		GSC_LOG(Verbose, TEXT("UGSCUserWidget::SetupAbilitySystemComponentListeners - Setup callback for %s (%s)"), *Attribute.GetName(), *GetNameSafe(OwnerActor));
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UGSCUserWidget::OnAttributeChanged);
	}

	// Handle GameplayEffects added / remove
	AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UGSCUserWidget::OnActiveGameplayEffectAdded);
	AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UGSCUserWidget::OnAnyGameplayEffectRemoved);

	// Handle generic GameplayTags added / removed
	AbilitySystemComponent->RegisterGenericGameplayTagEvent().AddUObject(this, &UGSCUserWidget::OnAnyGameplayTagChanged);

	// Handle Ability Commit events
	AbilitySystemComponent->AbilityCommittedCallbacks.AddUObject(this, &UGSCUserWidget::OnAbilityCommitted);
}

void UGSCUserWidget::ShutdownAbilitySystemComponentListeners() const
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	AbilitySystemComponent->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
	}

	AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
	AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
	AbilitySystemComponent->RegisterGenericGameplayTagEvent().RemoveAll(this);
	AbilitySystemComponent->AbilityCommittedCallbacks.RemoveAll(this);

	for (const FActiveGameplayEffectHandle GameplayEffectAddedHandle : GameplayEffectAddedHandles)
	{
		if (GameplayEffectAddedHandle.IsValid())
		{
			FOnActiveGameplayEffectStackChange* EffectStackChangeDelegate = AbilitySystemComponent->OnGameplayEffectStackChangeDelegate(GameplayEffectAddedHandle);
			if (EffectStackChangeDelegate)
			{
				EffectStackChangeDelegate->RemoveAll(this);
			}

			FOnActiveGameplayEffectTimeChange* EffectTimeChangeDelegate = AbilitySystemComponent->OnGameplayEffectTimeChangeDelegate(GameplayEffectAddedHandle);
			if (EffectTimeChangeDelegate)
			{
				EffectTimeChangeDelegate->RemoveAll(this);
			}
		}
	}

	for (const FGameplayTag GameplayTagBoundToDelegate : GameplayTagBoundToDelegates)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(GameplayTagBoundToDelegate).RemoveAll(this);
	}
}

float UGSCUserWidget::GetPercentForAttributes(const FGameplayAttribute Attribute, const FGameplayAttribute MaxAttribute) const
{
	const float AttributeValue = GetAttributeValue(Attribute);
	const float MaxAttributeValue = GetAttributeValue(MaxAttribute);

	if (MaxAttributeValue == 0.f)
	{
		GSC_LOG(Warning, TEXT("UGSCUserWidget::GetPercentForAttributes() MaxAttribute %s value is 0. This leads to divide by Zero errors, will return 0"), *MaxAttribute.GetName())
		return 0.f;
	}

	return AttributeValue / MaxAttributeValue;
}

float UGSCUserWidget::GetAttributeValue(const FGameplayAttribute Attribute) const
{
	if (!AbilitySystemComponent)
	{
		GSC_WLOG(Error, TEXT("The owner AbilitySystemComponent seems to be invalid. Will return 0.f."))
		return 0.f;
	}

	if (!Attribute.IsValid())
	{
		GSC_WLOG(Error, TEXT("Passed in Attribute is invalid (None). Will return 0.f."))
		return 0.f;
	}

	if (!AbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		const UClass* AttributeSet = Attribute.GetAttributeSetClass();
		GSC_WLOG(
			Error,
			TEXT("Trying to get value of attribute [%s.%s]. %s doesn't seem to be granted to %s. Returning 0.f"),
			*GetNameSafe(AttributeSet),
			*Attribute.GetName(),
			*GetNameSafe(AttributeSet),
			*GetNameSafe(AbilitySystemComponent)
		);
		
		return 0.f;
	}

	return AbilitySystemComponent->GetNumericAttribute(Attribute);
}

void UGSCUserWidget::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Broadcast event to Blueprint
	OnAttributeChange(Data.Attribute, Data.NewValue, Data.OldValue);

	// Trigger post attribute change event for subclass that needs further handling
	HandleAttributeChange(Data.Attribute, Data.NewValue, Data.OldValue);
}

void UGSCUserWidget::OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, const FActiveGameplayEffectHandle ActiveHandle)
{
	FGameplayTagContainer AssetTags;
	SpecApplied.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);

	if (AbilitySystemComponent)
	{
		if (FOnActiveGameplayEffectStackChange* Delegate = AbilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveHandle))
		{
			Delegate->AddUObject(this, &UGSCUserWidget::OnActiveGameplayEffectStackChanged);
		}

		if (FOnActiveGameplayEffectTimeChange* Delegate = AbilitySystemComponent->OnGameplayEffectTimeChangeDelegate(ActiveHandle))
		{
			Delegate->AddUObject(this, &UGSCUserWidget::OnActiveGameplayEffectTimeChanged);
		}

		// Store active handles to clear out bound delegates when shutting down listeners
		GameplayEffectAddedHandles.AddUnique(ActiveHandle);
	}

	HandleGameplayEffectAdded(AssetTags, GrantedTags, ActiveHandle);
}

void UGSCUserWidget::OnActiveGameplayEffectStackChanged(const FActiveGameplayEffectHandle ActiveHandle, const int32 NewStackCount, const int32 PreviousStackCount)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FActiveGameplayEffect* GameplayEffect = AbilitySystemComponent->GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	HandleGameplayEffectStackChange(AssetTags, GrantedTags, ActiveHandle, NewStackCount, PreviousStackCount);
}

void UGSCUserWidget::OnActiveGameplayEffectTimeChanged(const FActiveGameplayEffectHandle ActiveHandle, const float NewStartTime, const float NewDuration)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FActiveGameplayEffect* GameplayEffect = AbilitySystemComponent->GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	HandleGameplayEffectTimeChange(AssetTags, GrantedTags, ActiveHandle, NewStartTime, NewDuration);
}

void UGSCUserWidget::OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	EffectRemoved.Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	EffectRemoved.Spec.GetAllGrantedTags(GrantedTags);

	// Broadcast any GameplayEffect change to HUD
	HandleGameplayEffectStackChange(AssetTags, GrantedTags, EffectRemoved.Handle, 0, 1);
	HandleGameplayEffectRemoved(AssetTags, GrantedTags, EffectRemoved.Handle);
}

void UGSCUserWidget::OnAnyGameplayTagChanged(const FGameplayTag GameplayTag, const int32 NewCount)
{
	HandleGameplayTagChange(GameplayTag, NewCount);
}

void UGSCUserWidget::OnAbilityCommitted(UGameplayAbility* ActivatedAbility)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (!IsValid(ActivatedAbility))
	{
		GSC_LOG(Warning, TEXT("UGSCUserWidget::OnAbilityCommitted() Activated ability not valid"))
		return;
	}

	// Figure out cooldown
	const UGameplayEffect* CooldownGE = ActivatedAbility->GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	if (!ActivatedAbility->IsInstantiated())
	{
		return;
	}

	const FGameplayTagContainer* CooldownTags = ActivatedAbility->GetCooldownTags();
	if (!CooldownTags || CooldownTags->Num() <= 0)
	{
		return;
	}

	const FGameplayAbilityActorInfo ActorInfo = ActivatedAbility->GetActorInfo();
	const FGameplayAbilitySpecHandle AbilitySpecHandle = ActivatedAbility->GetCurrentAbilitySpecHandle();

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	ActivatedAbility->GetCooldownTimeRemainingAndDuration(AbilitySpecHandle, &ActorInfo, TimeRemaining, Duration);

	// Broadcast start of cooldown to HUD
	const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (AbilitySpec)
	{
		HandleCooldownStart(AbilitySpec->Ability, *CooldownTags, TimeRemaining, Duration);
	}

	// Register delegate to monitor any change to cooldown gameplay tag to be able to figure out when a cooldown expires
	TArray<FGameplayTag> GameplayTags;
	CooldownTags->GetGameplayTagArray(GameplayTags);
	for (const FGameplayTag GameplayTag : GameplayTags)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(GameplayTag).AddUObject(this, &UGSCUserWidget::OnCooldownGameplayTagChanged, AbilitySpecHandle, Duration);
		GameplayTagBoundToDelegates.AddUnique(GameplayTag);
	}
}

void UGSCUserWidget::OnCooldownGameplayTagChanged(const FGameplayTag GameplayTag, const int32 NewCount, FGameplayAbilitySpecHandle AbilitySpecHandle, float Duration)
{
	if (NewCount != 0)
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec)
	{
		// Ability might have been cleared when cooldown expires
		return;
	}

	const UGameplayAbility* Ability = AbilitySpec->Ability;

	// Broadcast cooldown expiration to HUD
	if (IsValid(Ability))
	{
		HandleCooldownEnd(AbilitySpec->Ability, GameplayTag, Duration);
	}

	AbilitySystemComponent->RegisterGameplayTagEvent(GameplayTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
}


void UGSCUserWidget::HandleGameplayEffectStackChange(const FGameplayTagContainer AssetTags, const FGameplayTagContainer GrantedTags, const FActiveGameplayEffectHandle ActiveHandle, const int32 NewStackCount, const int32 OldStackCount)
{
	OnGameplayEffectStackChange(AssetTags, GrantedTags, ActiveHandle, NewStackCount, OldStackCount);
}

void UGSCUserWidget::HandleGameplayEffectTimeChange(const FGameplayTagContainer AssetTags, const FGameplayTagContainer GrantedTags, const FActiveGameplayEffectHandle ActiveHandle, const float NewStartTime, const float NewDuration)
{
	OnGameplayEffectTimeChange(AssetTags, GrantedTags, ActiveHandle, NewStartTime, NewDuration);
}

void UGSCUserWidget::HandleGameplayEffectAdded(const FGameplayTagContainer AssetTags, const FGameplayTagContainer GrantedTags, const FActiveGameplayEffectHandle ActiveHandle)
{
	OnGameplayEffectAdded(AssetTags, GrantedTags, ActiveHandle, GetGameplayEffectUIData(ActiveHandle));
}

void UGSCUserWidget::HandleGameplayEffectRemoved(const FGameplayTagContainer AssetTags, const FGameplayTagContainer GrantedTags, const FActiveGameplayEffectHandle ActiveHandle)
{
	OnGameplayEffectRemoved(AssetTags, GrantedTags, ActiveHandle, GetGameplayEffectUIData(ActiveHandle));
}

void UGSCUserWidget::HandleGameplayTagChange(const FGameplayTag GameplayTag, const int32 NewTagCount)
{
	OnGameplayTagChange(GameplayTag, NewTagCount);
}

void UGSCUserWidget::HandleCooldownStart(UGameplayAbility* Ability, const FGameplayTagContainer CooldownTags, const float TimeRemaining, const float Duration)
{
	OnCooldownStart(Ability, CooldownTags, TimeRemaining, Duration);
}

void UGSCUserWidget::HandleCooldownEnd(UGameplayAbility* Ability, const FGameplayTag CooldownTag, const float Duration)
{
	OnCooldownEnd(Ability, CooldownTag, Duration);
}

FGSCGameplayEffectUIData UGSCUserWidget::GetGameplayEffectUIData(const FActiveGameplayEffectHandle ActiveHandle)
{
	return FGSCGameplayEffectUIData(
		UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectStartTime(ActiveHandle),
		UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectTotalDuration(ActiveHandle),
		UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectExpectedEndTime(ActiveHandle),
		UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectStackCount(ActiveHandle),
		UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectStackLimitCount(ActiveHandle)
	);
}
