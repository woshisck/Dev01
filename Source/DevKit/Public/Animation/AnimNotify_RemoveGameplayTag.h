#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_RemoveGameplayTag.generated.h"

/**
 * AnimNotify（点触发）：在蒙太奇指定帧将 ASC 上的 Gameplay Tag 计数强制归零。
 * 只影响 Loose Tag（AddLooseGameplayTag 添加的），不影响 GE-granted tag。
 * 典型用途：在连招最后一招蒙太奇的 CanCombo 窗口结束后挂此 Notify，
 *           确保 PlayerState.AbilityCast.CanCombo 被干净清除。
 */
UCLASS(meta = (DisplayName = "Remove Gameplay Tag"))
class DEVKIT_API UAnimNotify_RemoveGameplayTag : public UAnimNotify
{
    GENERATED_BODY()

public:
    /** 要移除的 Tag（全部计数归零） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTag")
    FGameplayTag TagToRemove;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
