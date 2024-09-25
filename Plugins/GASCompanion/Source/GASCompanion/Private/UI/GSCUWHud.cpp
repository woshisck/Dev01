// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "UI/GSCUWHud.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GSCLog.h"
#include "Abilities/Attributes/GSCAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UGSCUWHud::NativeConstruct()
{
	Super::NativeConstruct();

	if (APawn* OwningPlayerPawn = GetOwningPlayerPawn())
	{
		SetOwnerActor(OwningPlayerPawn);

		if (!TryInitAbilitySystem())
		{
			GSC_LOG(Warning, TEXT("UGSCUWHud::NativeConstruct called too early: %s (%s)"), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(OwningPlayerPawn))
			bLazyAbilitySystemInitialization = true;
		}
	}
}

void UGSCUWHud::NativeDestruct()
{
	// Clean up previously registered delegates for OwningPlayer AbilitySystemComponent
	ResetAbilitySystem();

	Super::NativeDestruct();
}

void UGSCUWHud::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Lazy init disabled, was either good from native construct or initialized once here
	if (!bLazyAbilitySystemInitialization)
	{
		return;
	}

	if (TryInitAbilitySystem())
	{
		// We now have proper ASC and initialized widget, prevent further tick run
		bLazyAbilitySystemInitialization = false;
		
		GSC_LOG(Warning, TEXT("UGSCUWHud::NativeTick reconciliated with ASC. We now have a reference to it: %s (%s)"), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(OwnerActor))
	}
}

void UGSCUWHud::InitFromCharacter()
{
	if (!AbilitySystemComponent)
	{
		// Prevent calls to GetAttributeValue if no ASC yet, which may happen for clients (and prevent warning logs during initialization)
		return;
	}

	SetHealth(GetAttributeValue(UGSCAttributeSet::GetHealthAttribute()));
	SetStamina(GetAttributeValue(UGSCAttributeSet::GetStaminaAttribute()));
	SetMana(GetAttributeValue(UGSCAttributeSet::GetManaAttribute()));
}

void UGSCUWHud::SetMaxHealth(const float MaxHealth)
{
	const float Health = GetAttributeValue(UGSCAttributeSet::GetHealthAttribute());
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(GetAttributeFormatString(Health, MaxHealth)));
	}

	if (MaxHealth != 0)
	{
		SetHealthPercentage(Health / MaxHealth);
	}
}

void UGSCUWHud::SetHealth(const float Health)
{
	const float MaxHealth = GetAttributeValue(UGSCAttributeSet::GetMaxHealthAttribute());
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(GetAttributeFormatString(Health, MaxHealth)));
	}

	if (MaxHealth != 0)
	{
		SetHealthPercentage(Health / MaxHealth);
	}
}

void UGSCUWHud::SetHealthPercentage(const float HealthPercentage)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthPercentage);
	}
}

void UGSCUWHud::SetMaxStamina(const float MaxStamina)
{
	const float Stamina = GetAttributeValue(UGSCAttributeSet::GetStaminaAttribute());
	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(GetAttributeFormatString(Stamina, MaxStamina)));
	}

	if (MaxStamina != 0)
	{
		SetStaminaPercentage(Stamina / MaxStamina);
	}
}

void UGSCUWHud::SetStamina(const float Stamina)
{
	const float MaxStamina = GetAttributeValue(UGSCAttributeSet::GetMaxStaminaAttribute());
	if (StaminaText)
	{
		StaminaText->SetText(FText::FromString(GetAttributeFormatString(Stamina, MaxStamina)));
	}

	if (MaxStamina != 0)
	{
		SetStaminaPercentage(Stamina / MaxStamina);
	}
}

void UGSCUWHud::SetStaminaPercentage(const float StaminaPercentage)
{
	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(StaminaPercentage);
	}
}

void UGSCUWHud::SetMaxMana(const float MaxMana)
{
	const float Mana = GetAttributeValue(UGSCAttributeSet::GetManaAttribute());
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(GetAttributeFormatString(Mana, MaxMana)));
	}

	if (MaxMana != 0)
	{
		SetManaPercentage(Mana / MaxMana);
	}
}

void UGSCUWHud::SetMana(const float Mana)
{
	const float MaxMana = GetAttributeValue(UGSCAttributeSet::GetMaxManaAttribute());
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(GetAttributeFormatString(Mana, MaxMana)));
	}

	if (MaxMana != 0)
	{
		SetManaPercentage(Mana / MaxMana);
	}
}

void UGSCUWHud::SetManaPercentage(const float ManaPercentage)
{
	if (ManaProgressBar)
	{
		ManaProgressBar->SetPercent(ManaPercentage);
	}
}

void UGSCUWHud::HandleAttributeChange(const FGameplayAttribute Attribute, const float NewValue, const float OldValue)
{
	if (Attribute == UGSCAttributeSet::GetHealthAttribute())
	{
		SetHealth(NewValue);
	}
	else if (Attribute == UGSCAttributeSet::GetStaminaAttribute())
	{
		SetStamina(NewValue);
	}
	else if (Attribute == UGSCAttributeSet::GetManaAttribute())
	{
		SetMana(NewValue);
	}
	else if (Attribute == UGSCAttributeSet::GetMaxHealthAttribute())
	{
		SetMaxHealth(NewValue);
	}
	else if (Attribute == UGSCAttributeSet::GetMaxStaminaAttribute())
	{
		SetMaxStamina(NewValue);
	}
	else if (Attribute == UGSCAttributeSet::GetMaxManaAttribute())
	{
		SetMaxMana(NewValue);
	}
}

FString UGSCUWHud::GetAttributeFormatString(const float BaseValue, const float MaxValue)
{
	return FString::Printf(TEXT("%d / %d"), FMath::FloorToInt(BaseValue), FMath::FloorToInt(MaxValue));
}

bool UGSCUWHud::TryInitAbilitySystem()
{
	GSC_LOG(Verbose, TEXT("UGSCUWHud::TryInitAbilitySystem check for ASC: %s (%s)"), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(OwnerActor))

	if (!AbilitySystemComponent)
	{
		AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	}

	if (AbilitySystemComponent)
	{
		bLazyAbilitySystemInitialization = false;

		GSC_LOG(Verbose, TEXT("UGSCUWHud::TryInitAbilitySystem check found a new ASC: %s (%s)"), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(OwnerActor))

		InitializeWithAbilitySystem(AbilitySystemComponent);

		// Init Stats
		InitFromCharacter();
	}

	return AbilitySystemComponent != nullptr;
}
