// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogBaseCharacter.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UBaseAttributeSet::UBaseAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitBaseDMG(10.f);
	InitWeaponDMG(0.f);
	InitBuffAmplify(1.2f);
}


void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, BaseDMG, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, WeaponDMG, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, BuffAmplify, COND_None, REPNOTIFY_Always);


}


UWorld* UBaseAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UYogAbilitySystemComponent* UBaseAttributeSet::GetASC() const
{
	return Cast<UYogAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);


	//HARDCODE
	float MinimumHealth = 0.0f;

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	//Set Owner 
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AYogBaseCharacter* TargetCharacter = nullptr;

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AYogBaseCharacter>(TargetActor);
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute()) {

		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		AYogBaseCharacter* SourceCharacter = nullptr;

		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn* Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<AYogBaseCharacter>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<AYogBaseCharacter>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		// Try to extract a hit result
		FHitResult HitResult;
		if (Context.GetHitResult())
		{
			HitResult = *Context.GetHitResult();
		}


		// Store a local copy of the amount of damage done and clear the damage attribute
		const float LocalDamageDone = GetDamage();
		UE_LOG(LogTemp, Log, TEXT("[LocalDamageDone] %f"), LocalDamageDone);
		SetDamage(0.f);

		if (LocalDamageDone > 0)
		{
			// Apply the health change and then clamp it
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, GetMaxHealth()));

			if (TargetCharacter)
			{
				// This is proper damage
				TargetCharacter->HandleDamage(LocalDamageDone, HitResult, SourceTags, SourceCharacter, SourceActor);

				// Call for all health changes
				TargetCharacter->HandleHealthChanged(-LocalDamageDone, SourceTags);
			}
		}


		SetHealth(FMath::Clamp(GetHealth() - GetDamage(), MinimumHealth, GetMaxHealth()));
		SetDamage(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
		SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
	}

}

//void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
//	DOREPLIFETIME(UBaseAttributeSet, Health);
//	DOREPLIFETIME(UBaseAttributeSet, MaxHealth);
//	DOREPLIFETIME(UBaseAttributeSet, BaseDMG);
//	DOREPLIFETIME(UBaseAttributeSet, WeaponDMG);
//	DOREPLIFETIME(UBaseAttributeSet, BuffAmplify);
//	DOREPLIFETIME(UBaseAttributeSet, BuffingATK);
//	DOREPLIFETIME(UBaseAttributeSet, OwnerSpeed);
//	DOREPLIFETIME(UBaseAttributeSet, DMGCorrect);
//	DOREPLIFETIME(UBaseAttributeSet, HitRate);
//	DOREPLIFETIME(UBaseAttributeSet, Evade);
//
//}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldValue);
}

void UBaseAttributeSet::OnRep_BaseDMG(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, BaseDMG, OldValue);
}

void UBaseAttributeSet::OnRep_WeaponDMG(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, WeaponDMG, OldValue);
}

void UBaseAttributeSet::OnRep_BuffAmplify(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, BuffAmplify, OldValue);
}

//void UBaseAttributeSet::OnRep_BuffingATK(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, BuffingATK, OldValue);
//}
//
//void UBaseAttributeSet::OnRep_OwnerSpeed(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, OwnerSpeed, OldValue);
//}
//
//void UBaseAttributeSet::OnRep_DMGCorrect(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DMGCorrect, OldValue);
//}
//
//void UBaseAttributeSet::OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DMGAbsorb, OldValue);
//}
//
//void UBaseAttributeSet::OnRep_HitRate(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, HitRate, OldValue);
//}
//
//void UBaseAttributeSet::OnRep_Evade(const FGameplayAttributeData& OldValue)
//{
//	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Evade, OldValue);

void UBaseAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}
