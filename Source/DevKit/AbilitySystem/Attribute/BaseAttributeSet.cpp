// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogCharacterBase.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UBaseAttributeSet::UBaseAttributeSet()
{

}


void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, BaseDMG, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, WeaponDMG, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, BuffAmplify, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, DMGAbsorb, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, ActResist, COND_None, REPNOTIFY_Always);

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

	//Get the source actor
	AActor* SourceActor = nullptr;
	AController* SourceController = nullptr;
	AYogCharacterBase* SourceCharacter = nullptr;
	
	if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
	{
		SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
		SourceController = Source->AbilityActorInfo->PlayerController.Get();

		if (SourceController)
		{
			SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
		}
		else
		{
			SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
		}
	}

	//Get the target actor
	AActor* targetActor = nullptr;
	AController* targetController = nullptr;
	AYogCharacterBase* targetCharacter = nullptr;

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		targetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		targetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		targetCharacter = Cast<AYogCharacterBase>(targetActor);
	}

	// data modification different set 
	float deltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Override)
	{
		// If this was additive, store the raw delta value to be passed along later
		deltaValue = Data.EvaluatedData.Magnitude;
		if (Data.EvaluatedData.Attribute == GetWeaponDMGAttribute())
		{
			//effect by self
			if (SourceController == targetController)
			{
				UE_LOG(LogTemp, Log, TEXT("SELF BUFF"));
				SetWeaponDMG(deltaValue);
			}
		}

	}
	
	else if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		deltaValue = Data.EvaluatedData.Magnitude;
		if (Data.EvaluatedData.Attribute == GetDamageAttribute())
		{
			//effect by self

			deltaValue = Data.EvaluatedData.Magnitude;
			if (SourceController != targetController)
			{
				UE_LOG(LogTemp, Log, TEXT("DAMAGE CAST, Final DAMAGE: %f"), deltaValue);

				UE_LOG(LogTemp, Log, TEXT("DAMAGE CAST, TargetCharacter: %s, SourceCharacter: %s"), *targetCharacter->GetName(), *SourceCharacter->GetName());
				
			}
		}
		else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
			SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
		}
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{	
		FHitResult HitResult;
		if (Context.GetHitResult())
		{
			HitResult = *Context.GetHitResult();
		}

		
		const float LocalDamageDone = GetDamage();
		SetDamage(0.f);


		if (LocalDamageDone > 0.0f) 
		{

			bool WasAlive = true;
			if (targetCharacter)
			{
				WasAlive = targetCharacter->IsAlive();
			}


			// --------------------------------------
			// health = Damage (In DamageExecution) * (1 - DMGAbsorb)
			// --------------------------------------

			const float NewHealth = GetHealth() - LocalDamageDone * (1.0f - GetDMGAbsorb());
			SetHealth(NewHealth);

			const FHitResult* Hit = Data.EffectSpec.GetContext().GetHitResult();
			if (Hit) {
				UE_LOG(LogTemp, Log, TEXT("Hit effect cause here"));
			}


		}
	}
}


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

void UBaseAttributeSet::OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DMGAbsorb, OldValue);
}

void UBaseAttributeSet::OnRep_ActResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, ActResist, OldValue);
}

void UBaseAttributeSet::InitAttribute()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitBaseDMG(10.f);
	InitWeaponDMG(1.0f);
	InitBuffAmplify(1.2f);
	InitDMGAbsorb(0.2f);
	InitActResist(0.0f);
}


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
