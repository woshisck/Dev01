#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_GetProjectileModule.generated.h"

/**
 * 读取符文数据资产（DA）的飞行物模块配置，将各字段输出为数据引脚。
 * 在符文 DA 上启用飞行物模块（ActiveModules.bProjectile = true）后，
 * 此节点可将 Count、ConeAngle、bSweep 等值传递到下游节点（SpawnRangedProjectiles 等）。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Get Projectile Module", Category = "BuffFlow|Module"))
class DEVKIT_API UBFNode_GetProjectileModule : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ---- 数据输出引脚 ----

	// 飞行速度（cm/s）— 从 ProjectileModule.Speed 读取
	UPROPERTY(EditAnywhere, Category = "Output|Projectile", meta = (DisplayName = "飞行速度（cm/s）"))
	FFlowDataPinOutputProperty_Float Speed;

	// 发射数量 — 从 ProjectileModule.Count 读取
	UPROPERTY(EditAnywhere, Category = "Output|Projectile", meta = (DisplayName = "发射数量"))
	FFlowDataPinOutputProperty_Int32 Count;

	// 散布锥角（度）— 从 ProjectileModule.ConeAngleDegrees 读取
	UPROPERTY(EditAnywhere, Category = "Output|Projectile", meta = (DisplayName = "散布锥角（度）"))
	FFlowDataPinOutputProperty_Float ConeAngleDegrees;

	// 穿透飞行 — 从 ProjectileModule.bSweepCollision 读取
	UPROPERTY(EditAnywhere, Category = "Output|Projectile", meta = (DisplayName = "穿透飞行"))
	FFlowDataPinOutputProperty_Bool bSweepCollision;

	// 模块已启用 — 符文 DA 上 ActiveModules.bProjectile 是否为 true
	UPROPERTY(EditAnywhere, Category = "Output|Projectile", meta = (DisplayName = "模块已启用"))
	FFlowDataPinOutputProperty_Bool bModuleEnabled;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
