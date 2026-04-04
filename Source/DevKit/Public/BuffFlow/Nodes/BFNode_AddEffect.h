#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayTagContainer.h"
#include "BFNode_AddEffect.generated.h"

class UEffectRegistry;

/**
 * 通过 Tag 添加效果节点（策划向）
 * 策划只需填 EffectTag（如 Buff.Bleed），节点查询 EffectRegistry 自动找到对应 DA 并施加
 * 完全无需接触 GE 类，实现与程序的解耦
 *
 * Out    — 成功施加
 * Failed — Tag 未在 Registry 中注册，或目标无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Add Effect (Tag)", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_AddEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 效果 Tag（须在 Registry 中注册，例如 Buff.Bleed / Buff.Burn） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag EffectTag;

	/** 效果施加目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 效果注册表（创建一份 DA_EffectRegistry 并拖入） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<UEffectRegistry> Registry;

	/** GE 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 Level = 1;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
