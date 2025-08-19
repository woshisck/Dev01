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
	InitWeaponAttackSpeed(1);
	InitWeaponRange(1);
	InitCrticalRate(0);
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
