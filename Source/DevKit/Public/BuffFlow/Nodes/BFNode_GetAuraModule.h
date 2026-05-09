#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_GetAuraModule.generated.h"

/**
 * 读取符文 DA 的光环/场地模块配置，将各尺寸和时间字段输出为数据引脚。
 * 下游的 SpawnRuneGroundPathEffect 节点可通过数据引脚连线直接使用这些值，
 * 无需在节点上手动重复填写数值。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Get Aura Module", Category = "BuffFlow|Module"))
class DEVKIT_API UBFNode_GetAuraModule : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ---- 数据输出引脚 ----

	// 区域长度（cm）— 从 AuraModule.Length 读取
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "区域长度（cm）"))
	FFlowDataPinOutputProperty_Float Length;

	// 区域宽度（cm）— 从 AuraModule.Width 读取
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "区域宽度（cm）"))
	FFlowDataPinOutputProperty_Float Width;

	// 区域高度（cm）— 从 AuraModule.Height 读取
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "区域高度（cm）"))
	FFlowDataPinOutputProperty_Float Height;

	// 持续时间（秒）— 从 AuraModule.Duration 读取
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "持续时间（秒）"))
	FFlowDataPinOutputProperty_Float Duration;

	// 触发间隔（秒）— 从 AuraModule.TickInterval 读取
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "触发间隔（秒）"))
	FFlowDataPinOutputProperty_Float TickInterval;

	// 模块已启用 — 符文 DA 上 ActiveModules.bAura 是否为 true
	UPROPERTY(EditAnywhere, Category = "Output|Aura", meta = (DisplayName = "模块已启用"))
	FFlowDataPinOutputProperty_Bool bModuleEnabled;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
