#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GrantGA.generated.h"

/**
 * 向目标授予 GameplayAbility，当 BuffFlow 停止时自动撤销。
 *
 * 使用方式：
 *   在 FA 起点连接此节点，填写 AbilityClass 和 Target。
 *   无需手动连接撤销逻辑——FA 停止（符文卸下/BuffFlow 终止）时
 *   Cleanup() 自动调用 ClearAbility，GA 即被撤销。
 *
 * Out    — GA 授予成功，继续执行后续节点
 * Failed — 目标无效或无 ASC
 *
 * 典型用法（击退符文 FA）：
 *   [Start] → [GrantGA](GA_Knockback, Target=BuffOwner)
 *               └─ Out → [OnDamageDealt] → [PlayNiagara] ...
 *
 *   GA_Knockback 需在 Class Defaults 中配置：
 *     Ability Triggers → Tag = Event.Combat.Knockback, Source = GameplayEvent
 *   然后由攻击 GA 或 [BFNode_SendGameplayEvent] 触发。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Grant GA", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_GrantGA : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要授予的 GA 类（应继承自 RuneGameplayAbility） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** GA 授予目标（通常为 BuffOwner） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** GA 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 AbilityLevel = 1;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	/** FA 停止时自动调用，撤销已授予的 GA */
	virtual void Cleanup() override;

private:
	FGameplayAbilitySpecHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
};
