#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_PostAtkWindow.generated.h"

/**
 * AnimNotifyState：攻击后摇快速混出窗口
 *
 * 放在蒙太奇的 Recovery 区段（或整个后摇段），每 Tick 检测两类输入：
 *
 *   1. 攻击输入（LightAtk / HeavyAtk）：
 *      清除 TagToClearOnActionInput（默认 CanCombo），蒙太奇本身由
 *      新激活的 GA 通过 GAS EndAbility 流程停止，无需此处强制打断。
 *
 *   2. 移动输入：
 *      直接调用 Montage_Stop(MoveBlendOutTime) 混出当前蒙太奇，
 *      触发 GA_PlayMontage::OnMontageBlendOut → EndAbility，恢复移动。
 *
 * 参数：
 *   MoveBlendOutTime  — 移动输入触发时的 Blend 时间（秒）；0 = 立即停止
 *   TagToClearOnActionInput — 检测到攻击输入时要归零的 Loose Tag
 */
UCLASS(meta = (DisplayName = "Post Attack Window"))
class DEVKIT_API UAnimNotifyState_PostAtkWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_PostAtkWindow();

    /** 移动输入触发蒙太奇混出时的 BlendOut 时间（秒）；0 = 立即停止 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostAtkWindow")
    float MoveBlendOutTime = 0.15f;

    /**
     * 检测到攻击输入（LightAtk/HeavyAtk）时要清零的 Loose Tag。
     * 默认填写 PlayerState.AbilityCast.CanCombo。
     * 清空后新 Ability 激活，由其内部 EndAbility 流程停止蒙太奇。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostAtkWindow")
    FGameplayTag TagToClearOnActionInput;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;

private:
    // 记录本次 Notify 开始时间，用于过滤旧的预输入（单玩家安全）
    float BeginTime = 0.0f;

    // 防止混出已发起时重复调用 Montage_Stop
    bool bStopRequested = false;
};
