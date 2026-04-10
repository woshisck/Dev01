// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AN_MeleeDamage.generated.h"

/**
 * C++ AnimNotify：向角色 ASC 发送 GameplayEvent，触发 GA 里的 Play Montage and Wait for Event。
 * 替代 Blueprint AN_Dmg_GeneralAttack。
 * EventTag 默认为 GameplayEffect.DamageType.GeneralAttack，可在蒙太奇中单独覆盖。
 */
UCLASS(meta = (DisplayName = "AN Melee Damage"))
class DEVKIT_API UAN_MeleeDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_MeleeDamage();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	/** 发送给 ASC 的事件 Tag，GA 的 Play Montage and Wait for Event 需监听相同 Tag。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag EventTag;
};
