#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "BFNode_ApplyModifierDA.generated.h"

class UGEConfigDataAsset;

/**
 * 从 GEConfigDataAsset 读取配置并施加属性修改 GE。
 *
 * 对比 BFNode_ApplyAttributeModifier：
 *   · 参数集中在 DA 资产中，多个符文 / 节点可共享同一配置
 *   · 节点本身只保留 Target 和 Value 覆盖引脚，更轻量
 *
 * Value 数据引脚（可选）：
 *   有连线 → 覆盖 DA.DefaultValue
 *   无连线 → 使用 DA.DefaultValue
 *
 * 输出引脚：
 *   Out    — 施加成功
 *   Failed — Config 未设置、目标无效或无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Modifier (DA)", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyModifierDA : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** GE 配置资产 */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UGEConfigDataAsset> Config;

	/**
	 * 数值覆盖（数据引脚，可选）
	 * 有连线时覆盖 Config.DefaultValue；无连线时忽略，直接使用 Config.DefaultValue。
	 */
	UPROPERTY(EditAnywhere, Category = "Config")
	FFlowDataPinInputProperty_Float Value;

	/** 施加目标 */
	UPROPERTY(EditAnywhere, Category = "Config")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FActiveGameplayEffectHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;

	/** 缓存 GE 对象供复用（GAS 堆叠规则依赖相同 Def 指针） */
	UPROPERTY()
	TObjectPtr<UGameplayEffect> CachedGE;
};
