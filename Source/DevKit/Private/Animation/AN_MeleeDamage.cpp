// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"

UAN_MeleeDamage::UAN_MeleeDamage()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack"));
}

void UAN_MeleeDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	AYogCharacterBase* Character = Cast<AYogCharacterBase>(Owner);
	if (!Character) return;

	const UMontageAttackDataAsset* EffectiveAttackData = AttackDataOverride;
	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			if (MeleeGA->HasConfiguredAttackData())
			{
				EffectiveAttackData = MeleeGA->GetConfiguredAttackData();
			}
		}
	}

	const TArray<TObjectPtr<URuneDataAsset>>& EffectiveRuneEffects = EffectiveAttackData
		? EffectiveAttackData->AdditionalRuneEffects
		: AdditionalRuneEffects;

	// 将附加 Rune 暂存到角色，GA_MeleeAttack::OnEventReceived 会读取并触发到命中目标
	if (EffectiveRuneEffects.Num() > 0)
	{
		Character->PendingAdditionalHitRunes = EffectiveRuneEffects;
	}

	const EHitStopMode EffectiveHitStopMode = EffectiveAttackData ? EffectiveAttackData->HitStopMode : HitStopMode;
	if (EffectiveHitStopMode != EHitStopMode::None)
	{
		auto& Override = Character->PendingHitStopOverride;
		Override.bActive = true;
		Override.Mode = EffectiveHitStopMode;
		Override.FrozenDuration = EffectiveAttackData ? EffectiveAttackData->HitStopFrozenDuration : HitStopFrozenDuration;
		Override.SlowDuration = EffectiveAttackData ? EffectiveAttackData->HitStopSlowDuration : HitStopSlowDuration;
		Override.SlowRate = EffectiveAttackData ? EffectiveAttackData->HitStopSlowRate : HitStopSlowRate;
		Override.CatchUpRate = EffectiveAttackData ? EffectiveAttackData->HitStopCatchUpRate : HitStopCatchUpRate;
	}

	const TArray<FGameplayTag>& EffectiveOnHitEventTags = EffectiveAttackData
		? EffectiveAttackData->OnHitEventTags
		: OnHitEventTags;
	if (EffectiveOnHitEventTags.Num() > 0)
	{
		Character->PendingOnHitEventTags = EffectiveOnHitEventTags;
	}

	FGameplayEventData EventData;
	EventData.Instigator   = Character;
	EventData.EventTag     = EffectiveAttackData && EffectiveAttackData->EventTag.IsValid() ? EffectiveAttackData->EventTag : EventTag;
	// 将自身作为 OptionalObject 传递，TargetType 通过它直接读取 HitboxTypes / ActRange 等参数
	EventData.OptionalObject = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventData.EventTag, EventData);

	// 挥刀模式刀光符文：每次攻击帧均发送 Action.Attack.Swing，供 GA_SlashWaveCounter 计数
	// （无论是否命中敌人，只要动画播放到攻击帧就触发）
	static const FGameplayTag TAG_Swing =
		FGameplayTag::RequestGameplayTag(FName("Action.Attack.Swing"));
	FGameplayEventData SwingEventData;
	SwingEventData.Instigator = Character;
	SwingEventData.EventTag   = TAG_Swing;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, TAG_Swing, SwingEventData);
}

FString UAN_MeleeDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Melee Dmg | %.0f / R:%.0f"), ActDamage, ActRange);
}

FActionData UAN_MeleeDamage::BuildActionData() const
{
	if (AttackDataOverride)
	{
		return AttackDataOverride->BuildActionData();
	}

	FActionData Out;
	Out.ActDamage     = ActDamage;
	Out.ActRange      = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce  = ActDmgReduce;
	Out.hitboxTypes   = HitboxTypes;
	return Out;
}
