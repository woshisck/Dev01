#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SpawnRuneAreaProfile.generated.h"

class URuneCardEffectProfileDA;

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Rune Area Profile", Category = "BuffFlow|Profile"))
class DEVKIT_API UBFNode_SpawnRuneAreaProfile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 区域效果配置文件 — 包含范围、效果和表现的 DA 资产
	UPROPERTY(EditAnywhere, Category = "Profile", meta = (DisplayName = "区域配置文件"))
	TObjectPtr<URuneCardEffectProfileDA> Profile = nullptr;

	// 生成位置覆盖（数据引脚）— 连线后使用此世界坐标，覆盖配置文件中的位置计算
	UPROPERTY(EditAnywhere, Category = "Profile|Position", meta = (ToolTip = "Optional data pin. If linked, this world location overrides profile area position calculation.", DisplayName = "生成位置覆盖"))
	FFlowDataPinInputProperty_Vector SpawnLocationOverride;

	// 生成朝向覆盖（数据引脚）— 连线后使用此世界旋转，覆盖配置文件中的朝向计算
	UPROPERTY(EditAnywhere, Category = "Profile|Position", meta = (ToolTip = "Optional data pin. If linked, this world rotation overrides profile area facing.", DisplayName = "生成朝向覆盖"))
	FFlowDataPinInputProperty_Rotator SpawnRotationOverride;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	bool ResolveFacing(AActor* SourceActor, FVector& OutForward, FVector& OutRight) const;
};
