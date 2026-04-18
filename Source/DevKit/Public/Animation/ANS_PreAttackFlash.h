#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_PreAttackFlash.generated.h"

/**
 * 攻击前摇闪红 AnimNotifyState。
 * 放在攻击蒙太奇的"前摇预警"区间，BeginNotify 开始脉冲，EndNotify 自动停止。
 * 驱动 AYogCharacterBase::StartPreAttackFlash / StopPreAttackFlash。
 * 适用于玩家和敌人（任何继承 AYogCharacterBase 的角色）。
 */
UCLASS(meta = (DisplayName = "ANS Pre Attack Flash"))
class DEVKIT_API UANS_PreAttackFlash : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
