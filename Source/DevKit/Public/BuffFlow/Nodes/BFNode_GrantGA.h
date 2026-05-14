#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Types/FlowDataPinProperties.h"
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
 * 输出数据引脚（授予时写入）：
 *   bGAGranted — 是否成功授予
 *   GALevel    — 授予时的等级
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Grant GA", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_GrantGA : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要授予的 GA 类 — 应继承自 RuneGameplayAbility
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "GA 类"))
	TSubclassOf<UGameplayAbility> AbilityClass;

	// 授予目标 — 将 GA 授予给哪个 Actor 的 ASC，通常为 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "授予目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	// GA 等级 — 可直接填写固定值，也可连接上游数据引脚动态驱动
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "GA 等级"))
	FFlowDataPinInputProperty_Int32 AbilityLevel;

	// 是否成功授予（数据输出引脚）
	UPROPERTY(EditAnywhere, Category = "Output|GAInfo", meta = (DisplayName = "是否成功授予"))
	FFlowDataPinOutputProperty_Bool bGAGranted;

	// 授予时的 GA 等级（数据输出引脚）
	UPROPERTY(EditAnywhere, Category = "Output|GAInfo", meta = (DisplayName = "GA 等级（输出）"))
	FFlowDataPinOutputProperty_Int32 GALevel;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;

	/** FA 停止时自动调用，撤销已授予的 GA */
	virtual void Cleanup() override;

private:
	FGameplayAbilitySpecHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
};
