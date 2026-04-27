// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "Data/AbilityData.h"
#include "AN_MeleeDamage.generated.h"

class URuneDataAsset;

UENUM(BlueprintType)
enum class EHitStopMode : uint8
{
	None    UMETA(DisplayName = "None"),
	Freeze  UMETA(DisplayName = "Freeze"),
	Slow    UMETA(DisplayName = "Slow"),
};

/**
 * C++ AnimNotify：向角色 ASC 发送 GameplayEvent，触发 GA 里的 PlayMontageAndWaitForEvent。
 * 所有攻击参数（伤害、范围、命中框等）均在此处配置，不再依赖 AbilityData.FActionData。
 *
 * 工作流：
 *   1. 在蒙太奇攻击帧放置此 Notify
 *   2. 配置 EventTag（GA 需监听同一 Tag）
 *   3. 填写 ActDamage / ActRange / HitboxTypes 等攻击参数
 *   4. 可选：填写 AdditionalRuneEffects（命中目标时额外触发的 Rune DA）
 */
UCLASS(meta = (DisplayName = "AN Melee Damage"))
class DEVKIT_API UAN_MeleeDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_MeleeDamage();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	// ── 事件 ──────────────────────────────────────────────────────────────

	/** 发送给 ASC 的事件 Tag，GA 的 PlayMontageAndWaitForEvent 需监听相同 Tag。*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag EventTag;

	// ── 攻击参数（原 AbilityData.FActionData 迁移至此）─────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActRange = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActResilience = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDmgReduce = 0.f;

	// ── 命中框 ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	TArray<FYogHitboxType> HitboxTypes;

	// ── 攻击停顿 ─────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	EHitStopMode HitStopMode = EHitStopMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Freeze", EditConditionHides, ClampMin = 0.0f, ClampMax = 0.3f))
	float HitStopFrozenDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.0f, ClampMax = 0.5f))
	float HitStopSlowDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.01f, ClampMax = 1.0f))
	float HitStopSlowRate = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 1.01f, ClampMax = 5.0f))
	float HitStopCatchUpRate = 2.0f;

	// ── 命中事件 ─────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	TArray<FGameplayTag> OnHitEventTags;

	// ── 附加效果 ───────────────────────────────────────────────────────────

	/**
	 * 命中目标时额外触发的符文效果列表（施加到目标）。
	 * 每个 Rune DA 的 FlowAsset 会在命中目标上执行；敌人连招各节可分别配置。
	 * 跳帧/Freeze 等自伤效果同样可创建对应 Rune DA 并填入此列表。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<TObjectPtr<URuneDataAsset>> AdditionalRuneEffects;

	// ── 工具 ───────────────────────────────────────────────────────────────

	/** 将本 Notify 的攻击参数打包成 FActionData，供 TargetType 命中框检测使用。*/
	FActionData BuildActionData() const;
};
