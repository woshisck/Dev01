#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_PhaseDecayTimer.generated.h"

/**
 * 阶段衰减计时器（替代 Flow 内置 Timer 节点）
 *
 * In     → 启动计时器；若已在运行则静默忽略（不报错，与原始 C++ 行为一致）
 * Cancel → 取消计时器（在 OnHeatAboveZero 时调用）
 * Out    → 计时结束后触发（连接 BFNode_DecrementPhase）
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Phase Decay Timer", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_PhaseDecayTimer : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

    /** 计时时长（秒） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (ClampMin = "0.1", DisplayName = "Duration"))
    float Duration = 10.f;

protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;

private:
    UFUNCTION()
    void OnTimerComplete();

    FTimerHandle TimerHandle;
};
