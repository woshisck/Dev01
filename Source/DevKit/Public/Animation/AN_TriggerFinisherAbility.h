#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_TriggerFinisherAbility.generated.h"

/**
 * 单帧 AnimNotify：检测玩家是否处于终结技充能窗口，若是则自动触发终结技 GA。
 *
 * 放置位置：H3 蒙太奇伤害判定帧（AN_MeleeDamage）之后的回收帧。
 * 用法：将本 Notify 拖入 H3 蒙太奇 Notifies 轨道，无需额外配置。
 */
UCLASS(meta = (DisplayName = "AN Trigger Finisher Ability"))
class DEVKIT_API UAN_TriggerFinisherAbility : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
