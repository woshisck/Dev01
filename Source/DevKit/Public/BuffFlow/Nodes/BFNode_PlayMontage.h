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
UCLASS(NotBlueprintable, meta = (DisplayName = "Play Montage", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_PlayMontage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 蒙太奇资产 — 要播放的动画 Montage
	UPROPERTY(EditAnywhere, Category = "Montage", meta = (DisplayName = "蒙太奇资产"))
	TObjectPtr<UAnimMontage> Montage;

	// 播放速率 — 蒙太奇播放速率倍数（1.0 = 正常速度）
	UPROPERTY(EditAnywhere, Category = "Montage", meta = (ClampMin = "0.1", DisplayName = "播放速率"))
	float PlayRate = 1.f;

	// 播放目标 — 在哪个角色上播放此蒙太奇，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "Montage", meta = (DisplayName = "播放目标"))
	EBFTargetSelector TargetSelector = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
