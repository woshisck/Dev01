#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SacrificeDecay.generated.h"

/**
 * 献祭恩赐衰退节点
 *
 * In   → 启动衰退计时器（若已在运行则静默忽略）
 * Stop → 停止衰退（FA 停止时也会自动调用 Cleanup）
 *
 * 每秒：
 *   1. 对 Heat 施加 -CurrentDecayRate Instant GE
 *   2. CurrentDecayRate = Min(CurrentDecayRate + DecayAccelPerSecond, MaxDecayRate)
 *   3. 若 CurrentPhase == 0 且 Health > HPDrainPerSecond，施加 -HPDrainPerSecond Instant GE
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Sacrifice Decay", Category = "BuffFlow|Sacrifice"))
class DEVKIT_API UBFNode_SacrificeDecay : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

    // 初始衰退速率 — 每秒热度扣减量的起始值
    UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0.0", DisplayName = "初始衰退速率"))
    float BaseDecayRate = 0.5f;

    // 每秒加速量 — 衰退速率每秒累加的额外值（越来越快）
    UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0.0", DisplayName = "每秒加速量"))
    float DecayAccelPerSecond = 0.02f;

    // 衰退速率上限 — 衰退速率的最大值，防止无限加速
    UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0.0", DisplayName = "衰退速率上限"))
    float MaxDecayRate = 3.0f;

    // Phase 0 掉血量 — Phase 0 时每秒扣除的 HP（非致命）
    UPROPERTY(EditAnywhere, Category = "Penalty", meta = (ClampMin = "0.0", DisplayName = "Phase0 每秒扣血量"))
    float HPDrainPerSecond = 5.0f;

protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;

private:
    UFUNCTION()
    void OnDecayTick();

    FTimerHandle DecayTimerHandle;
    float CurrentDecayRate = 0.f;
};
