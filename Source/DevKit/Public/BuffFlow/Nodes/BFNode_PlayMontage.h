#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_PlayMontage.generated.h"

class UAnimMontage;

/**
 * 瞬时节点：在目标角色上播放蒙太奇动画
 * 仅支持 ACharacter，非角色类型会直接跳转 Out
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "播放蒙太奇", Category = "BuffFlow|动画"))
class DEVKIT_API UBFNode_PlayMontage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要播放的蒙太奇 */
	UPROPERTY(EditAnywhere, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage;

	/** 播放速率 */
	UPROPERTY(EditAnywhere, Category = "Montage", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	/** 在哪个角色上播放 */
	UPROPERTY(EditAnywhere, Category = "Montage")
	EBFTargetSelector TargetSelector = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
