#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "SacrificeGraceDA.generated.h"

class UFlowAsset;

/**
 * 献祭恩赐：全局被动 Run Buff（不进背包格子）
 *
 * 效果：进关时热度瞬间充满 + BonusEffect GE 生效
 * 代价：热度持续衰退（速率随时间累加），热度降至 Phase 0 时每秒扣血（非致命）
 */
UCLASS(BlueprintType)
class DEVKIT_API USacrificeGraceDA : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** 初始热度衰退速率（每秒扣减的热度点数） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decay", meta = (ClampMin = "0.0"))
    float BaseDecayRate = 0.5f;

    /** 每秒额外累加的衰退速率（关卡内持续加速） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decay", meta = (ClampMin = "0.0"))
    float DecayAccelPerSecond = 0.02f;

    /** 衰退速率上限 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decay", meta = (ClampMin = "0.0"))
    float MaxDecayRate = 3.0f;

    /** Phase 0 时每秒扣除的 HP（不会降至 0） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0.0"))
    float HPDrainPerSecond = 5.0f;

    /** 额外奖励效果 GE（进关时施加，Infinite 类型） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonus")
    TSubclassOf<UGameplayEffect> BonusEffect;

    /** 控制衰退行为的 BuffFlow FA（内含 BFNode_SacrificeDecay） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow")
    TObjectPtr<UFlowAsset> FlowAsset;

    /** 显示名称（用于拾取 UI） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /** 效果描述（用于拾取 UI） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
    FText Description;
};
