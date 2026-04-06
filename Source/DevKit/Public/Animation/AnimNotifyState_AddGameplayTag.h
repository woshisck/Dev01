#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"
#include "AnimNotifyState_AddGameplayTag.generated.h"

/**
 * AnimNotifyState：在 Notify 区间内为角色 ASC 添加 Gameplay Tag
 * Begin → AddLooseGameplayTag，End / 被打断 → RemoveLooseGameplayTag
 * 典型用途：在连招最后一击的攻击帧窗口上标记 Action.Combo.LastHit
 */
UCLASS(meta = (DisplayName = "Add Gameplay Tag"))
class DEVKIT_API UAnimNotifyState_AddGameplayTag : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    /** 要添加的 Tag（支持多个） */
    UPROPERTY(EditAnywhere, Category = "GameplayTag")
    FGameplayTagContainer Tags;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;

private:
    static UAbilitySystemComponent* GetASC(USkeletalMeshComponent* MeshComp);
};
