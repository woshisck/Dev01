// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogCharacterBase.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UBaseAttributeSet::UBaseAttributeSet()
{	

	////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
	InitAttack(0);
	InitAttackPower(1);
	InitMiscNum(1);
	InitSkillCD(1);
	InitMAX_PassiveGA(1);
	InitMAX_OffensiveGA(1);
	InitMaxHealth(30);
	InitOutRoundLifeTime(0);
	InitMoveSpeed(6);
	InitDash(1);
	InitDashCD(1);
	InitDashDist(4);
	InitDodge(0);
	InitResilience(0);
	InitResist(0);
	InitShield(0);

	////////////////////////////////////////////////// Ability Attribute ////////////////////////////////////////////////
	//HARD CODE INIT VALUE;

	//float ActDamage = 20;
	//float ActRange = 400;
	//float ActResilience = 20;
	//float ActDmgReduce = 0;
	//float ActRotateSpeed = 360;
	//float JumpFrameTime = 0.15;
	//float FreezeFrameTime = 0.15;
	
	
	InitActDamage(0);
	InitActRange(0);
	InitActResilience(0);
	InitActDmgReduce(0);
	InitActRotateSpeed(0);
	InitJumpFrameTime(0);
	InitFreezeFrameTime(0);




	////////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////
	InitWeaponAtk(0);
	InitWeaponAtkSpeed(1);
	InitWeaponRange(1);
	InitCriticalRate(0);
	InitCriticalDamage(1);





}


void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);

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

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	// Compute the delta between old and new, if it is available
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

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



	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		AYogCharacterBase* SourceCharacter = nullptr;

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
				SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		const float LocalDamageDone = GetDamage();
		SetDamage(0.f);

		if (LocalDamageDone > 0)
		{
			// Apply the health change and then clamp it
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, GetMaxHealth()));

			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();

			if (ASC)
			{

				//UYogAbilitySystemComponent* SourceASC, float Damage


				ASC->ReceiveDamage(ASC, GetDamage());
				float percent = GetHealth() / GetMaxHealth();
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent);
				// This is proper damage

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



void UBaseAttributeSet::InitAttribute()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);

}

void UBaseAttributeSet::ResetAbilityAttribute()
{
	//float INIT_ActDamage = 20;
	//float INIT_ActRange = 400;
	//float INIT_ActResilience = 20;
	//float INIT_ActDmgReduce = 0;
	//float INIT_ActRotateSpeed = 360;
	//float INIT_JumpFrameTime = 0.15;
	//float INIT_FreezeFrameTime = 0.15;


	ActDamage.SetBaseValue(INIT_ActDamage);
	ActDamage.SetCurrentValue(INIT_ActDamage);

	ActRange.SetBaseValue(INIT_ActRange);
	ActRange.SetCurrentValue(INIT_ActRange);

	ActResilience.SetBaseValue(INIT_ActResilience);
	ActResilience.SetCurrentValue(INIT_ActResilience);

	ActDmgReduce.SetBaseValue(INIT_ActDmgReduce);
	ActDmgReduce.SetCurrentValue(INIT_ActDmgReduce);

	ActRotateSpeed.SetBaseValue(INIT_ActRotateSpeed);
	ActRotateSpeed.SetCurrentValue(INIT_ActRotateSpeed);

	JumpFrameTime.SetBaseValue(INIT_JumpFrameTime);
	JumpFrameTime.SetCurrentValue(INIT_JumpFrameTime);

	FreezeFrameTime.SetBaseValue(INIT_FreezeFrameTime);
	FreezeFrameTime.SetCurrentValue(INIT_FreezeFrameTime);


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
