// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Components/GSCCoreComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GSCGameplayAbility.h"
#include "Abilities/Attributes/GSCAttributeSet.h"
#include "Core/Settings/GSCDeveloperSettings.h"
#include "GameFramework/Character.h"
#include "GSCLog.h"

// Sets default values for this component's properties
UGSCCoreComponent::UGSCCoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGSCCoreComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	SetupOwner();
}

void UGSCCoreComponent::BeginDestroy()
{
	// Clean up any bound delegates when component is destroyed
	ShutdownAbilitySystemDelegates(OwnerAbilitySystemComponent);

	Super::BeginDestroy();
}

void UGSCCoreComponent::SetupOwner()
{
	if(!GetOwner())
	{
		return;
	}

	OwnerActor = GetOwner();
	if(!OwnerActor)
	{
		return;
	}

	OwnerPawn = Cast<APawn>(OwnerActor);
	OwnerCharacter = Cast<ACharacter>(OwnerActor);

	OwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
}

void UGSCCoreComponent::RegisterAbilitySystemDelegates(UAbilitySystemComponent* ASC)
{
	GSC_WLOG(Verbose, TEXT("Registering delegates for ASC: %s"), *GetNameSafe(ASC))

	if (!ASC)
	{
		return;
	}
	
	// Make sure to shut down delegates previously registered, if RegisterAbilitySystemDelegates is called more than once (likely from AbilityActorInfo)
	ShutdownAbilitySystemDelegates(ASC);

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (FGameplayAttribute Attribute : Attributes)
	{
		if (Attribute == UGSCAttributeSet::GetDamageAttribute() || Attribute == UGSCAttributeSet::GetStaminaDamageAttribute())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UGSCCoreComponent::OnDamageAttributeChanged);
		}
		else
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &UGSCCoreComponent::OnAttributeChanged);
		}
	}

	// Handle GameplayEffects added / remove
	ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UGSCCoreComponent::OnActiveGameplayEffectAdded);
	ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &UGSCCoreComponent::OnAnyGameplayEffectRemoved);

	// Handle generic GameplayTags added / removed
	ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UGSCCoreComponent::OnAnyGameplayTagChanged);

	// Handle Ability Commit events
	ASC->AbilityCommittedCallbacks.AddUObject(this, &UGSCCoreComponent::OnAbilityCommitted);
}

void UGSCCoreComponent::ShutdownAbilitySystemDelegates(UAbilitySystemComponent* ASC)
{
	GSC_LOG(Log, TEXT("UGSCCoreComponent::ShutdownAbilitySystemDelegates for ASC: %s"), ASC ? *ASC->GetName() : TEXT("NONE"))

	if (!ASC)
	{
		return;
	}

	TArray<FGameplayAttribute> Attributes;
	ASC->GetAllAttributes(Attributes);

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
	}

	ASC->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
	ASC->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
	ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	ASC->AbilityCommittedCallbacks.RemoveAll(this);

	for (const FActiveGameplayEffectHandle GameplayEffectAddedHandle : GameplayEffectAddedHandles)
	{
		if (GameplayEffectAddedHandle.IsValid())
		{
			FOnActiveGameplayEffectStackChange* EffectStackChangeDelegate = ASC->OnGameplayEffectStackChangeDelegate(GameplayEffectAddedHandle);
			if (EffectStackChangeDelegate)
			{
				EffectStackChangeDelegate->RemoveAll(this);
			}

			FOnActiveGameplayEffectTimeChange* EffectTimeChangeDelegate = ASC->OnGameplayEffectTimeChangeDelegate(GameplayEffectAddedHandle);
			if (EffectTimeChangeDelegate)
			{
				EffectTimeChangeDelegate->RemoveAll(this);
			}
		}
	}

	for (const FGameplayTag GameplayTagBoundToDelegate : GameplayTagBoundToDelegates)
	{
		ASC->RegisterGameplayTagEvent(GameplayTagBoundToDelegate).RemoveAll(this);
	}
}

void UGSCCoreComponent::HandleDamage(const float DamageAmount, const FGameplayTagContainer& DamageTags, AActor* SourceActor)
{
	OnDamage.Broadcast(DamageAmount, SourceActor, DamageTags);

	// TODO: Damage Attribute not replicated, have to figure out a way to broadcast to clients (mostly to keep SourceActor)
	// Broadcast damage to status bar (if any)
	// BroadcastDamageToStatusBar(DamageAmount, DamageTags, SourceActor);
}

void UGSCCoreComponent::HandleHealthChange(const float DeltaValue, const FGameplayTagContainer& EventTags)
{
	// We only call the BP callbacks if this is not the initial ability setup
	if (!bStartupAbilitiesGranted)
	{
		return;
	}

	OnHealthChange.Broadcast(DeltaValue, EventTags);
	if (!IsAlive())
	{
		Die();
	}
}

void UGSCCoreComponent::HandleStaminaChange(const float DeltaValue, const FGameplayTagContainer& EventTags)
{
	// We only call the BP callbacks if this is not the initial ability setup
	if (!bStartupAbilitiesGranted)
	{
		return;
	}

	OnStaminaChange.Broadcast(DeltaValue, EventTags);
}

void UGSCCoreComponent::HandleManaChange(const float DeltaValue, const FGameplayTagContainer& EventTags)
{
	// We only call the BP callbacks if this is not the initial ability setup
	if (!bStartupAbilitiesGranted)
	{
		return;
	}

	OnManaChange.Broadcast(DeltaValue, EventTags);
}

void UGSCCoreComponent::HandleAttributeChange(const FGameplayAttribute Attribute, const float DeltaValue, const FGameplayTagContainer& EventTags)
{
	OnAttributeChange.Broadcast(Attribute, DeltaValue, EventTags);
}

void UGSCCoreComponent::OnAttributeChanged(const FOnAttributeChangeData& Data)
{
	const float NewValue = Data.NewValue;
	const float OldValue = Data.OldValue;

	// Prevent broadcast Attribute changes if New and Old values are the same
	// most likely because of clamping in post gameplay effect execute
	if (OldValue == NewValue)
	{
		return;
	}

	const FGameplayEffectModCallbackData* ModData = Data.GEModData;
	FGameplayTagContainer SourceTags = FGameplayTagContainer();
	if (ModData)
	{
		SourceTags = *ModData->EffectSpec.CapturedSourceTags.GetAggregatedTags();
	}

	// Broadcast attribute change to component
	OnAttributeChange.Broadcast(Data.Attribute, NewValue - OldValue, SourceTags);
}

void UGSCCoreComponent::OnDamageAttributeChanged(const FOnAttributeChangeData& Data)
{
	// if we ever need to broadcast via delegate
	// GSC_LOG(Warning, TEXT("OnDamageAttributeChanged %s"), *Data.Attribute.GetName())
}

void UGSCCoreComponent::Die()
{
	OnDeath.Broadcast();
}

float UGSCCoreComponent::GetHealth() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetHealthAttribute());
}

float UGSCCoreComponent::GetMaxHealth() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetMaxHealthAttribute());
}

float UGSCCoreComponent::GetStamina() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetStaminaAttribute());
}

float UGSCCoreComponent::GetMaxStamina() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetMaxStaminaAttribute());
}

float UGSCCoreComponent::GetMana() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetManaAttribute());
}

float UGSCCoreComponent::GetMaxMana() const
{
	if (!OwnerAbilitySystemComponent)
	{
		return 0.0f;
	}

	return GetCurrentAttributeValue(UGSCAttributeSet::GetMaxManaAttribute());
}

float UGSCCoreComponent::GetAttributeValue(const FGameplayAttribute Attribute) const
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_WLOG(Error, TEXT("The owner AbilitySystemComponent seems to be invalid. Will return 0.f."))
		return 0.0f;
	}

	if (!Attribute.IsValid())
	{
		GSC_WLOG(Error, TEXT("Passed in Attribute is invalid (None). Will return 0.f."))
		return 0.f;
	}

	if (!OwnerAbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		const UClass* AttributeSet = Attribute.GetAttributeSetClass();
		GSC_WLOG(
			Error,
			TEXT("Trying to get value of attribute [%s.%s]. %s doesn't seem to be granted to %s. Returning 0.f"),
			*GetNameSafe(AttributeSet),
			*Attribute.GetName(),
			*GetNameSafe(AttributeSet),
			*GetNameSafe(OwnerAbilitySystemComponent)
		);

		return 0.f;
	}

	return OwnerAbilitySystemComponent->GetNumericAttributeBase(Attribute);
}

float UGSCCoreComponent::GetCurrentAttributeValue(const FGameplayAttribute Attribute) const
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_WLOG(Error, TEXT("The owner AbilitySystemComponent seems to be invalid. Will return 0.f."))
		return 0.0f;
	}
	
	if (!Attribute.IsValid())
	{
		GSC_WLOG(Error, TEXT("Passed in Attribute is invalid (None). Will return 0.f."))
		return 0.f;
	}

	if (!OwnerAbilitySystemComponent->HasAttributeSetForAttribute(Attribute))
	{
		const UClass* AttributeSet = Attribute.GetAttributeSetClass();
		GSC_WLOG(
			Error,
			TEXT("Trying to get value of attribute [%s.%s]. %s doesn't seem to be granted to %s. Returning 0.f"),
			*GetNameSafe(AttributeSet),
			*Attribute.GetName(),
			*GetNameSafe(AttributeSet),
			*GetNameSafe(OwnerAbilitySystemComponent)
		);
		return 0.0f;
	}

	return OwnerAbilitySystemComponent->GetNumericAttribute(Attribute);
}

bool UGSCCoreComponent::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void UGSCCoreComponent::GrantAbility(TSubclassOf<UGameplayAbility> Ability, int32 Level)
{
	if (!OwnerActor || !OwnerAbilitySystemComponent || !Ability)
	{
		return;
	}

	if (!OwnerAbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		GSC_LOG(Warning, TEXT("UGSCCoreComponent::GrantAbility Called on non authority"));
		return;
	}

	FGameplayAbilitySpec Spec;
	Spec.Ability = Ability.GetDefaultObject();

	FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, Level, INDEX_NONE, OwnerActor);
	OwnerAbilitySystemComponent->GiveAbility(AbilitySpec);
}

void UGSCCoreComponent::ClearAbility(const TSubclassOf<UGameplayAbility> Ability)
{
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToRemove;
	AbilitiesToRemove.Add(Ability);
	return ClearAbilities(AbilitiesToRemove);
}

void UGSCCoreComponent::ClearAbilities(const TArray<TSubclassOf<UGameplayAbility>> Abilities)
{
	if (!OwnerActor || !OwnerAbilitySystemComponent || !OwnerAbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}

	// Remove any abilities added from a previous call.
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : OwnerAbilitySystemComponent->GetActivatableAbilities())
	{
		if (Abilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	// Do in two passes so the removal happens after we have the full list
	for (const FGameplayAbilitySpecHandle AbilityToRemove : AbilitiesToRemove)
	{
		OwnerAbilitySystemComponent->ClearAbility(AbilityToRemove);
	}
}

bool UGSCCoreComponent::HasAnyMatchingGameplayTags(const FGameplayTagContainer TagContainer) const
{
	if (OwnerAbilitySystemComponent)
	{
		return OwnerAbilitySystemComponent->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool UGSCCoreComponent::HasMatchingGameplayTag(const FGameplayTag TagToCheck) const
{
	if (OwnerAbilitySystemComponent)
	{
		return OwnerAbilitySystemComponent->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool UGSCCoreComponent::IsUsingAbilityByClass(const TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent::IsUsingAbilityByClass() provided AbilityClass is null"))
		return false;
	}

	return GetActiveAbilitiesByClass(AbilityClass).Num() > 0;
}

bool UGSCCoreComponent::IsUsingAbilityByTags(const FGameplayTagContainer AbilityTags)
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent::IsUsingAbilityByClass() ASC is not valid"))
		return false;
	}

	return GetActiveAbilitiesByTags(AbilityTags).Num() > 0;
}

TArray<UGameplayAbility*> UGSCCoreComponent::GetActiveAbilitiesByClass(TSubclassOf<UGameplayAbility> AbilityToSearch) const
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent::GetActiveAbilitiesByClass() ASC is not valid"))
		return {};
	}

	TArray<FGameplayAbilitySpec> Specs = OwnerAbilitySystemComponent->GetActivatableAbilities();
	TArray<struct FGameplayAbilitySpec*> MatchingGameplayAbilities;
	TArray<UGameplayAbility*> ActiveAbilities;

	// First, search for matching Abilities for this class
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (Spec.Ability && Spec.Ability->GetClass()->IsChildOf(AbilityToSearch))
		{
			MatchingGameplayAbilities.Add(const_cast<FGameplayAbilitySpec*>(&Spec));
		}
	}

	// Iterate the list of all ability specs
	for (const FGameplayAbilitySpec* Spec : MatchingGameplayAbilities)
	{
		// Iterate all instances on this ability spec, which can include instance per execution abilities
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			if (ActiveAbility->IsActive())
			{
				ActiveAbilities.Add(ActiveAbility);
			}
		}
	}

	return ActiveAbilities;
}

TArray<UGameplayAbility*> UGSCCoreComponent::GetActiveAbilitiesByTags(const FGameplayTagContainer GameplayTagContainer) const
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent::GetActiveAbilitiesByClass() ASC is not valid"))
		return {};
	}

	TArray<UGameplayAbility*> ActiveAbilities;
	TArray<FGameplayAbilitySpec*> MatchingGameplayAbilities;
	OwnerAbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, MatchingGameplayAbilities, false);

	// Iterate the list of all ability specs
	for (const FGameplayAbilitySpec* Spec : MatchingGameplayAbilities)
	{
		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			if (ActiveAbility->IsActive())
			{
				ActiveAbilities.Add(ActiveAbility);
			}
		}
	}

	return ActiveAbilities;;
}

bool UGSCCoreComponent::ActivateAbilityByClass(const TSubclassOf<UGameplayAbility> AbilityClass, UGSCGameplayAbility*& ActivatedAbility, const bool bAllowRemoteActivation)
{
	if (!OwnerAbilitySystemComponent || !AbilityClass)
	{
		return false;
	}

	const bool bSuccess = OwnerAbilitySystemComponent->TryActivateAbilityByClass(AbilityClass, bAllowRemoteActivation);

	TArray<UGameplayAbility*> ActiveAbilities = GetActiveAbilitiesByClass(AbilityClass);
	if (ActiveAbilities.Num() == 0)
	{
		GSC_LOG(Verbose, TEXT("UGSCCoreComponent::ActivateAbilityByClass Couldn't get back active abilities with Class %s. Won't be able to return ActivatedAbility instance."), *AbilityClass->GetName());
	}

	if (bSuccess && ActiveAbilities.Num() > 0)
	{
		UGSCGameplayAbility* GSCAbility = Cast<UGSCGameplayAbility>(ActiveAbilities[0]);
		if (GSCAbility)
		{
			ActivatedAbility = GSCAbility;
		}
	}

	return bSuccess;
}

bool UGSCCoreComponent::ActivateAbilityByTags(const FGameplayTagContainer AbilityTags, UGSCGameplayAbility*& ActivatedAbility, const bool bAllowRemoteActivation)
{
	if (!OwnerAbilitySystemComponent)
	{
		return false;
	}

	TArray<FGameplayAbilitySpec*> AbilitiesToActivate;
	OwnerAbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, AbilitiesToActivate);

	const uint32 Count = AbilitiesToActivate.Num();

	if (Count == 0)
	{
		GSC_LOG(Warning, TEXT("UGSCCoreComponent::ActivateAbilityByTags No matching Ability for %s"), *AbilityTags.ToStringSimple())
        return false;
	}

	const FGameplayAbilitySpec* Spec = AbilitiesToActivate[FMath::RandRange(0, Count - 1)];

	// actually trigger the ability
	const bool bSuccess = OwnerAbilitySystemComponent->TryActivateAbility(Spec->Handle, bAllowRemoteActivation);

	TArray<UGameplayAbility*> ActiveAbilities = GetActiveAbilitiesByTags(AbilityTags);
	if (ActiveAbilities.Num() == 0)
	{
		GSC_LOG(Warning, TEXT("UGSCCoreComponent::ActivateAbilityByTags Couldn't get back active abilities with tags %s"), *AbilityTags.ToStringSimple());
	}

	if (bSuccess && ActiveAbilities.Num() > 0)
	{
		UGSCGameplayAbility* GSCAbility = Cast<UGSCGameplayAbility>(ActiveAbilities[0]);
		if (GSCAbility)
		{
			ActivatedAbility = GSCAbility;
		}
	}

	return bSuccess;
}

void UGSCCoreComponent::SetAttributeValue(const FGameplayAttribute Attribute, const float NewValue)
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_LOG(Warning, TEXT("SetAttributeValue() No Owner AbilitySystemComponent, abort set attribute."));
		return;
	}

	OwnerAbilitySystemComponent->SetNumericAttributeBase(Attribute, NewValue);
}

void UGSCCoreComponent::ClampAttributeValue(const FGameplayAttribute Attribute, const float MinValue, const float MaxValue)
{
	const float CurrentValue = GetAttributeValue(Attribute);
	const float NewValue = FMath::Clamp(CurrentValue, MinValue, MaxValue);
	SetAttributeValue(Attribute, NewValue);
}

void UGSCCoreComponent::AdjustAttributeForMaxChange(UGSCAttributeSetBase* AttributeSet, const FGameplayAttribute AffectedAttributeProperty, const FGameplayAttribute MaxAttribute, const float NewMaxValue)
{
	if (!OwnerAbilitySystemComponent)
	{
		GSC_LOG(Warning, TEXT("AdjustAttributeForMaxChange() No Owner AbilitySystemComponent, abort AdjustAttributeForMaxChange."));
		return;
	}

	FGameplayAttributeData* AttributeData = AffectedAttributeProperty.GetGameplayAttributeData(AttributeSet);
	if (!AttributeData)
	{
		GSC_LOG(Warning, TEXT("AdjustAttributeForMaxChange() AttributeData returned by AffectedAttributeProperty.GetGameplayAttributeData() seems to be invalid."))
		return;
	}

	const FGameplayAttributeData* MaxAttributeData = MaxAttribute.GetGameplayAttributeData(AttributeSet);
	if (!AttributeData)
	{
		GSC_LOG(Warning, TEXT("AdjustAttributeForMaxChange() MaxAttributeData returned by MaxAttribute.GetGameplayAttributeData() seems to be invalid."))
		return;
	}

	AttributeSet->AdjustAttributeForMaxChange(*AttributeData, *MaxAttributeData, NewMaxValue, AffectedAttributeProperty);
}

void UGSCCoreComponent::PreAttributeChange(UGSCAttributeSetBase* AttributeSet, const FGameplayAttribute& Attribute, const float NewValue)
{
	OnPreAttributeChange.Broadcast(AttributeSet, Attribute, NewValue);
}

void UGSCCoreComponent::PostGameplayEffectExecute(UGSCAttributeSetBase* AttributeSet, const FGameplayEffectModCallbackData& Data)
{
	if (!AttributeSet)
	{
		GSC_LOG(Error, TEXT("UGSCCoreComponent:PostGameplayEffectExecute() Owner AttributeSet isn't valid"));
		return;
	}

	AActor* SourceActor = nullptr;
	AActor* TargetActor = nullptr;
	AttributeSet->GetSourceAndTargetFromContext<AActor>(Data, SourceActor, TargetActor);

	const FGameplayTagContainer SourceTags = AttributeSet->GetSourceTagsFromContext(Data);
	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();

	// Get Minimum Clamp value for this attribute, if it is available
	const float ClampMinimumValue = AttributeSet->GetClampMinimumValueFor(Data.EvaluatedData.Attribute);

	// Compute the delta between old and new, if it is available
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	// Delegate any attribute handling to Blueprints
	FGSCGameplayEffectExecuteData Payload;
	Payload.AttributeSet = AttributeSet;
	Payload.AbilitySystemComponent = AttributeSet->GetOwningAbilitySystemComponent();
	Payload.DeltaValue = DeltaValue;
	Payload.ClampMinimumValue = ClampMinimumValue;
	OnPostGameplayEffectExecute.Broadcast(Data.EvaluatedData.Attribute, SourceActor, TargetActor, SourceTags, Payload);
}

void UGSCCoreComponent::SetStartupAbilitiesGranted(const bool bGranted)
{
	bStartupAbilitiesGranted = bGranted;
}

void UGSCCoreComponent::OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, const FActiveGameplayEffectHandle ActiveHandle)
{
	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	SpecApplied.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectAdded.Broadcast(AssetTags, GrantedTags, ActiveHandle);

	if (FOnActiveGameplayEffectStackChange* Delegate = OwnerAbilitySystemComponent->OnGameplayEffectStackChangeDelegate(ActiveHandle))
	{
		Delegate->AddUObject(this, &UGSCCoreComponent::OnActiveGameplayEffectStackChanged);
	}

	if (FOnActiveGameplayEffectTimeChange* Delegate = OwnerAbilitySystemComponent->OnGameplayEffectTimeChangeDelegate(ActiveHandle))
	{
		Delegate->AddUObject(this, &UGSCCoreComponent::OnActiveGameplayEffectTimeChanged);
	}

	// Store active handles to clear out bound delegates when shutting down listeners
	GameplayEffectAddedHandles.AddUnique(ActiveHandle);
}

void UGSCCoreComponent::OnActiveGameplayEffectStackChanged(const FActiveGameplayEffectHandle ActiveHandle, const int32 NewStackCount, const int32 PreviousStackCount)
{
	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	const FActiveGameplayEffect* GameplayEffect = OwnerAbilitySystemComponent->GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectStackChange.Broadcast(AssetTags, GrantedTags, ActiveHandle, NewStackCount, PreviousStackCount);
}

void UGSCCoreComponent::OnActiveGameplayEffectTimeChanged(const FActiveGameplayEffectHandle ActiveHandle, const float NewStartTime, const float NewDuration)
{
	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	const FActiveGameplayEffect* GameplayEffect = OwnerAbilitySystemComponent->GetActiveGameplayEffect(ActiveHandle);
	if (!GameplayEffect)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	GameplayEffect->Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GameplayEffect->Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectTimeChange.Broadcast(AssetTags, GrantedTags, ActiveHandle, NewStartTime, NewDuration);
}

void UGSCCoreComponent::OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved)
{
	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	FGameplayTagContainer AssetTags;
	EffectRemoved.Spec.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	EffectRemoved.Spec.GetAllGrantedTags(GrantedTags);

	OnGameplayEffectStackChange.Broadcast(AssetTags, GrantedTags, EffectRemoved.Handle, 0, 1);
	OnGameplayEffectRemoved.Broadcast(AssetTags, GrantedTags, EffectRemoved.Handle);
}

void UGSCCoreComponent::OnAnyGameplayTagChanged(const FGameplayTag GameplayTag, const int32 NewCount) const
{
	OnGameplayTagChange.Broadcast(GameplayTag, NewCount);
}

void UGSCCoreComponent::OnAbilityCommitted(UGameplayAbility* ActivatedAbility)
{
	if (!ActivatedAbility)
	{
		return;
	}

	// Trigger AbilityCommit event
	OnAbilityCommit.Broadcast(ActivatedAbility);

	HandleCooldownOnAbilityCommit(ActivatedAbility);
}

void UGSCCoreComponent::OnCooldownGameplayTagChanged(const FGameplayTag GameplayTag, const int32 NewCount, const FGameplayAbilitySpecHandle AbilitySpecHandle, const float Duration)
{
	if (NewCount != 0)
	{
		return;
	}

	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	FGameplayAbilitySpec* AbilitySpec = OwnerAbilitySystemComponent->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec)
	{
		// Ability might have been cleared when cooldown expires
		return;
	}

	UGameplayAbility* Ability = AbilitySpec->Ability;

	// Broadcast cooldown expiration to BP
	if (IsValid(Ability))
	{
		OnCooldownEnd.Broadcast(Ability, GameplayTag, Duration);
	}

	OwnerAbilitySystemComponent->RegisterGameplayTagEvent(GameplayTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
}

void UGSCCoreComponent::HandleCooldownOnAbilityCommit(UGameplayAbility* ActivatedAbility)
{
	if (!OwnerAbilitySystemComponent)
	{
		return;
	}

	if (!IsValid(ActivatedAbility))
	{
		GSC_LOG(Warning, TEXT("UGSCCoreComponent::HandleCooldownOnAbilityCommit() Activated ability not valid"))
		return;
	}

	// Figure out cooldown
	UGameplayEffect* CooldownGE = ActivatedAbility->GetCooldownGameplayEffect();
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

	FGameplayAbilityActorInfo ActorInfo = ActivatedAbility->GetActorInfo();
	const FGameplayAbilitySpecHandle AbilitySpecHandle = ActivatedAbility->GetCurrentAbilitySpecHandle();

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	ActivatedAbility->GetCooldownTimeRemainingAndDuration(AbilitySpecHandle, &ActorInfo, TimeRemaining, Duration);

	OnCooldownStart.Broadcast(ActivatedAbility, *CooldownTags, TimeRemaining, Duration);

	// Register delegate to monitor any change to cooldown gameplay tag to be able to figure out when a cooldown expires
	TArray<FGameplayTag> GameplayTags;
	CooldownTags->GetGameplayTagArray(GameplayTags);
	for (const FGameplayTag GameplayTag : GameplayTags)
	{
		OwnerAbilitySystemComponent->RegisterGameplayTagEvent(GameplayTag)
			.AddUObject(this, &UGSCCoreComponent::OnCooldownGameplayTagChanged, AbilitySpecHandle, Duration);

		GameplayTagBoundToDelegates.AddUnique(GameplayTag);
	}
}
