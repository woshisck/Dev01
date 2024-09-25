// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Abilities/Attributes/GSCAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Components/GSCCoreComponent.h"
#include "Net/UnrealNetwork.h"
#include "GSCLog.h"

void UGSCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    // This is called whenever attributes change, so for max health/mana we want to scale the current totals to match
    Super::PreAttributeChange(Attribute, NewValue);

	// Handle Max Attributes
	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
		return;
	}

	if (Attribute == GetMaxStaminaAttribute())
	{
		AdjustAttributeForMaxChange(Stamina, MaxStamina, NewValue, GetStaminaAttribute());
		return;
	}

	if (Attribute == GetMaxManaAttribute())
	{
		AdjustAttributeForMaxChange(Mana, MaxMana, NewValue, GetManaAttribute());
		return;
	}
}

void UGSCAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

	GSC_WLOG(VeryVerbose, TEXT("PostGameplayEffectExecute called for %s.%s"), *GetName(), *Data.EvaluatedData.Attribute.AttributeName)

	FGSCAttributeSetExecutionData ExecutionData;
	GetExecutionDataFromMod(Data, ExecutionData);

    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
    	HandleDamageAttribute(ExecutionData);
    }
	else if (Data.EvaluatedData.Attribute == GetStaminaDamageAttribute())
	{
		HandleStaminaDamageAttribute(ExecutionData);
	}
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
    	HandleHealthAttribute(ExecutionData);
    }
    else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
    	HandleStaminaAttribute(ExecutionData);
    }
    else if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
    	HandleManaAttribute(ExecutionData);
    }
}

void UGSCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, Mana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UGSCAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
}

void UGSCAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, Health, OldHealth);
}

void UGSCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, MaxHealth, OldMaxHealth);
}

void UGSCAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldHealthRegenRate)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, HealthRegenRate, OldHealthRegenRate);
}

void UGSCAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, Mana, OldMana);
}

void UGSCAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, MaxMana, OldMaxMana);
}

void UGSCAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldManaRegenRate)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, ManaRegenRate, OldManaRegenRate);
}

void UGSCAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, Stamina, OldStamina);
}

void UGSCAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, MaxStamina, OldMaxStamina);
}

void UGSCAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UGSCAttributeSet, StaminaRegenRate, OldStaminaRegenRate);
}

void UGSCAttributeSet::SetAttributeClamped(const FGameplayAttribute& Attribute, const float Value, const float MaxValue)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const float Min = GetClampMinimumValueFor(Attribute);
	const float NewValue = FMath::Clamp(Value, Min, MaxValue);

	ASC->SetNumericAttributeBase(Attribute, NewValue);
}

void UGSCAttributeSet::HandleDamageAttribute(const FGSCAttributeSetExecutionData& ExecutionData)
{
	AActor* SourceActor = ExecutionData.SourceActor;
	AActor* TargetActor = ExecutionData.TargetActor;
	UGSCCoreComponent* TargetCoreComponent = ExecutionData.TargetCoreComponent;
	const FGameplayTagContainer SourceTags = ExecutionData.SourceTags;

	// Store a local copy of the amount of Damage done and clear the Damage attribute.
	const float LocalDamageDone = GetDamage();
	SetDamage(0.f);

	if (LocalDamageDone > 0.f)
	{
		// If actor was alive before damage is added, handle damage
		// This prevents damage being added to dead things and replaying death animations
		bool bAlive = true;

		if (TargetCoreComponent)
		{
			bAlive = TargetCoreComponent->IsAlive();
			if (!bAlive)
			{
				GSC_LOG(Warning, TEXT("UGSCAttributeSet::PostGameplayEffectExecute() %s character or pawn is NOT alive when receiving damage"), *TargetActor->GetName());
			}
		}

		if (bAlive)
		{
			// Apply the Health change and then clamp it.
			const float NewHealth = GetHealth() - LocalDamageDone;
			const float ClampMinimumValue = GetClampMinimumValueFor(GetHealthAttribute());
			SetHealth(FMath::Clamp(NewHealth, ClampMinimumValue, GetMaxHealth()));

			if (TargetCoreComponent)
			{
				TargetCoreComponent->HandleDamage(LocalDamageDone, SourceTags, SourceActor);
				TargetCoreComponent->HandleHealthChange(-LocalDamageDone, SourceTags);
			}
		}
	}
}

void UGSCAttributeSet::HandleStaminaDamageAttribute(const FGSCAttributeSetExecutionData& ExecutionData)
{
	UGSCCoreComponent* TargetCoreComponent = ExecutionData.TargetCoreComponent;
	const FGameplayTagContainer SourceTags = ExecutionData.SourceTags;

	// Store a local copy of the amount of damage done and clear the damage attribute
	const float LocalStaminaDamageDone = GetStaminaDamage();
	SetStaminaDamage(0.f);

	if (LocalStaminaDamageDone > 0.0f)
	{
		// Apply the stamina change and then clamp it
		const float NewStamina = GetStamina() - LocalStaminaDamageDone;
		SetStamina(FMath::Clamp(NewStamina, 0.0f, GetMaxStamina()));

		if (TargetCoreComponent)
		{
			TargetCoreComponent->HandleStaminaChange(-LocalStaminaDamageDone, SourceTags);
		}
	}
}

void UGSCAttributeSet::HandleHealthAttribute(const FGSCAttributeSetExecutionData& ExecutionData)
{
	UGSCCoreComponent* TargetCoreComponent = ExecutionData.TargetCoreComponent;
	const float ClampMinimumValue = GetClampMinimumValueFor(GetHealthAttribute());

	SetHealth(FMath::Clamp(GetHealth(), ClampMinimumValue, GetMaxHealth()));

	if (TargetCoreComponent)
	{
		const float DeltaValue = ExecutionData.DeltaValue;
		const FGameplayTagContainer SourceTags = ExecutionData.SourceTags;
		TargetCoreComponent->HandleHealthChange(DeltaValue, SourceTags);
	}
}

void UGSCAttributeSet::HandleStaminaAttribute(const FGSCAttributeSetExecutionData& ExecutionData)
{
	UGSCCoreComponent* TargetCoreComponent = ExecutionData.TargetCoreComponent;
	const float ClampMinimumValue = GetClampMinimumValueFor(GetStaminaAttribute());

	SetStamina(FMath::Clamp(GetStamina(), ClampMinimumValue, GetMaxStamina()));

	if (TargetCoreComponent)
	{
		const float DeltaValue = ExecutionData.DeltaValue;
		const FGameplayTagContainer SourceTags = ExecutionData.SourceTags;
		TargetCoreComponent->HandleStaminaChange(DeltaValue, SourceTags);
	}
}

void UGSCAttributeSet::HandleManaAttribute(const FGSCAttributeSetExecutionData& ExecutionData)
{
	UGSCCoreComponent* TargetCoreComponent = ExecutionData.TargetCoreComponent;
	const float ClampMinimumValue = GetClampMinimumValueFor(GetManaAttribute());

	SetMana(FMath::Clamp(GetMana(), ClampMinimumValue, GetMaxMana()));

	if (TargetCoreComponent)
	{
		const float DeltaValue = ExecutionData.DeltaValue;
		const FGameplayTagContainer SourceTags = ExecutionData.SourceTags;
		TargetCoreComponent->HandleManaChange(DeltaValue, SourceTags);
	}
}
