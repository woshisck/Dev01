// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystemBlueprintLibrary.h"
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

	// 将附加 Rune 暂存到角色，GA_MeleeAttack::OnEventReceived 会读取并触发到命中目标
	if (AdditionalRuneEffects.Num() > 0)
	{
		Character->PendingAdditionalHitRunes = AdditionalRuneEffects;
	}

	FGameplayEventData EventData;
	EventData.Instigator   = Character;
	EventData.EventTag     = EventTag;
	// 将自身作为 OptionalObject 传递，TargetType 通过它直接读取 HitboxTypes / ActRange 等参数
	EventData.OptionalObject = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, EventData);

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
	FActionData Out;
	Out.ActDamage     = ActDamage;
	Out.ActRange      = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce  = ActDmgReduce;
	Out.hitboxTypes   = HitboxTypes;
	return Out;
}
