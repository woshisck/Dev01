#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_GetRuneTuningValue.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Get Rune Tuning Value", Category = "BuffFlow|Rune"))
class DEVKIT_API UBFNode_GetRuneTuningValue : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 打开后可手动输入任意 Key；关闭则从预设下拉选择
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "自定义Key"))
	bool bCustomKey = false;

	// 预设下拉（bCustomKey = false 时显示）
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "数值表 Key", GetOptions = "GetPresetKeyNames", EditCondition = "!bCustomKey", EditConditionHides))
	FName Key;

	// 自由输入（bCustomKey = true 时显示）
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "数值表 Key（自定义）", EditCondition = "bCustomKey", EditConditionHides))
	FName CustomKey;

	UFUNCTION()
	static TArray<FString> GetPresetKeyNames();

	FName GetActiveKey() const { return bCustomKey ? CustomKey : Key; }

	// 默认值 — Key 不存在时返回的兜底值
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "默认值"))
	float DefaultValue = 0.f;

	// 查询结果（数据输出引脚）— 可连线到 ApplyAttributeModifier.Value 等
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "数值（输出）"))
	FFlowDataPinOutputProperty_Float Value;

	// 是否找到（数据输出引脚）— Key 在调参表中存在时为 true
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "是否找到（输出）"))
	FFlowDataPinOutputProperty_Bool bFound;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
