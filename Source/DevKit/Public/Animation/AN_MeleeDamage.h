// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
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

	/**
	 * 命中目标时额外施加的 GameplayEffect 列表（施加到目标，不是自身）。
	 * 留空 = 不附加额外 Effect（默认行为）。
	 * 敌人连招：在每一节的 AN_MeleeDamage 上分别配置，实现逐节不同效果。
	 * 玩家：同样支持，填写此处配置的 GE 会在命中时附加到被打角色上。
	 * 实际应用由 GA_MeleeAttack::OnEventReceived 完成。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> AdditionalTargetEffects;
};
