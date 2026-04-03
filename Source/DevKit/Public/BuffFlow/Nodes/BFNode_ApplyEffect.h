#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "BFNode_ApplyEffect.generated.h"

/**
 * 实现效果节点：向指定目标施加一个 GameplayEffect
 * Target 默认为"上次伤害目标"，适合配合伤害触发器使用
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "实现效果", Category = "BuffFlow|效果"))
class DEVKIT_API UBFNode_ApplyEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要施加的 GameplayEffect 类 */
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSubclassOf<UGameplayEffect> Effect;

	/** 效果等级 */
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "1"))
	float Level = 1.f;

	/** 施加目标（默认：上次伤害目标） */
	UPROPERTY(EditAnywhere, Category = "Effect")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
