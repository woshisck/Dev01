#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_FinisherTimeDilation.generated.h"

/**
 * 区间 AnimNotifyState：子弹时间效果。
 *
 * Begin：全局时间膨胀 × SlowDilation（默认0.15），玩家抵消保持正常速度，显示输入提示UI。
 * End：  若时间膨胀未被GA提前恢复，则在此处恢复，隐藏UI。
 *
 * 放置位置：终结技蒙太奇中"输入窗口"的起止区间。
 */
UCLASS(meta = (DisplayName = "ANS Finisher Time Dilation"))
class DEVKIT_API UANS_FinisherTimeDilation : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    /** 全局慢动作倍率（0.15 = 全局减速到1/6.6，玩家用CustomTimeDilation抵消）*/
    UPROPERTY(EditAnywhere, Category = "TimeDilation")
    float SlowDilation = 0.15f;

    /** 显示/隐藏提示UI用的GameplayEvent Tag（广播给PlayerController）*/
    UPROPERTY(EditAnywhere, Category = "UI")
    FGameplayTag PromptShowEventTag;

    UPROPERTY(EditAnywhere, Category = "UI")
    FGameplayTag PromptHideEventTag;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
