// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystemBlueprintLibrary.h"

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

	// 将附加 Effect 暂存到角色，GA_MeleeAttack::OnEventReceived 会读取并应用到命中目标
	if (AdditionalTargetEffects.Num() > 0)
	{
		Character->PendingAdditionalHitEffects = AdditionalTargetEffects;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Character;
	EventData.EventTag   = EventTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, EventData);
}
