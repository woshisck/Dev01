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

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Sanity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Dodge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Resist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Crit_Rate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Crit_Damage, COND_None, REPNOTIFY_Always);

}

UYogAbilitySystemComponent* UBaseAttributeSet::GetASC() const
{
	return Cast<UYogAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}

void UBaseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
	Super::PreAttributeBaseChange(attribute, newValue);

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

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	//// Compute the delta between old and new, if it is available
	// DeltaValue = 0;
	//if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	//{
	//	// If this was additive, store the raw delta value to be passed along later
	//	DeltaValue = Data.EvaluatedData.Magnitude;
	//}

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AYogCharacterBase* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AYogCharacterBase>(TargetActor);
	}



	//if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	//{
	//	AActor* SourceActor = nullptr;
	//	AController* SourceController = nullptr;
	//	AYogCharacterBase* SourceCharacter = nullptr;

	//	if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
	//	{
	//		SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
	//		SourceController = Source->AbilityActorInfo->PlayerController.Get();
	//		if (SourceController == nullptr && SourceActor != nullptr)
	//		{
	//			if (APawn* Pawn = Cast<APawn>(SourceActor))
	//			{
	//				SourceController = Pawn->GetController();
	//			}
	//		}

	//		// Use the controller to find the source pawn
	//		if (SourceController)
	//		{
	//			SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
	//		}
	//		else
	//		{
	//			SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
	//		}
	//		// Set the causer actor based on context if it's set
	//		if (Context.GetEffectCauser())
	//		{
	//			SourceActor = Context.GetEffectCauser();
	//		}
	//	}


	//	// IF HOLY DMAGE \
	//	const  LocalDamageDone = GetDamage(); + HOLY DAMAGE;
	//	const  LocalDamageDone = GetDamage();
	//	SetDamage(0.f);

	//	if (LocalDamageDone > 0)
	//	{
	//		// Apply the health change and then clamp it
	//		const  OldHealth = GetHealth();
	//		SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, GetMaxHealth()));
	//		UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
	//		if (ASC)
	//		{
	//			//UYogAbilitySystemComponent* SourceASC,  Damage
	//			ASC->ReceiveDamage(ASC, GetDamage());
	//			 percent = GetHealth() / GetMaxHealth();
	//			TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent);
	//			// This is proper damage
	//		}
	//	}
	//}

}

void UBaseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

}


void UBaseAttributeSet::Init(UCharacterData* data)
{
	const FYogBaseAttributeData& YogBaseData = data->GetBaseAttributeData();

	SetAttack(YogBaseData.Attack);
	SetAttackPower(YogBaseData.AttackPower);
	SetMaxHealth(YogBaseData.MaxHealth);
	SetShield(YogBaseData.Shield);
	SetAttackSpeed(YogBaseData.AttackSpeed);
	SetAttackRange(YogBaseData.AttackRange);
	SetSanity(YogBaseData.Sanity);
	SetMoveSpeed(YogBaseData.MoveSpeed);
	SetDodge(YogBaseData.Dodge);
	SetResilience(YogBaseData.Resilience);
	SetResist(YogBaseData.Resist);
	SetDmgTaken(YogBaseData.DmgTaken);
	SetCrit_Rate(YogBaseData.Crit_Rate);
	SetCrit_Damage(YogBaseData.Crit_Damage);

}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldValue);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldValue);
}

void UBaseAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Attack, OldValue);
}

void UBaseAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackPower, OldValue);
}

void UBaseAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Shield, OldValue);
}

void UBaseAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_Sanity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Sanity, OldValue);
}

void UBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MoveSpeed, OldValue);
}

void UBaseAttributeSet::OnRep_Dodge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Dodge, OldValue);
}

void UBaseAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Resilience, OldValue);
}

void UBaseAttributeSet::OnRep_Resist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Resist, OldValue);
}

void UBaseAttributeSet::OnRep_DmgTaken(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, DmgTaken, OldValue);
}

void UBaseAttributeSet::OnRep_Crit_Rate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Crit_Rate, OldValue);
}

void UBaseAttributeSet::OnRep_Crit_Damage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Crit_Damage, OldValue);
}

void UBaseAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackRange, OldValue);
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

FGameplayCueParameters UBaseAttributeSet::MakeCueParams(const FGameplayEffectCustomExecutionParameters& ExecutionParams, float DamageAmount, bool bWasCritical) const
{
	return FGameplayCueParameters();
}
