#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayEffect.h"
#include "BFNode_AreaDamage.generated.h"

class ABFAreaDamageZone;

/**
 * 延迟后在 Buff 拥有者位置生成球形伤害区域，
 * 持续对区域内敌人施加 GE 伤害，持续时间结束后销毁。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Area Damage", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_AreaDamage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 生成延迟（秒）— 触发到实际生成区域之间的等待时间，0 = 立即生成
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.0", DisplayName = "生成延迟（秒）"))
	FFlowDataPinInputProperty_Float Delay;

	// 持续时间（秒）— 伤害区域存在多少秒后销毁
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.01", DisplayName = "持续时间（秒）"))
	FFlowDataPinInputProperty_Float Duration;

	// 球体半径（cm）— 伤害区域的球形半径
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "1.0", DisplayName = "球体半径（cm）"))
	FFlowDataPinInputProperty_Float Radius;

	// 每次伤害量 — 每个伤害间隔对区域内敌人施加的伤害值
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.0", DisplayName = "每次伤害量"))
	FFlowDataPinInputProperty_Float DamageAmount;

	// 伤害间隔（秒）— 多次持续伤害的触发间隔；<= 0 表示只命中一次
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (DisplayName = "伤害间隔（秒）"))
	FFlowDataPinInputProperty_Float DamageInterval;

	// 伤害效果类 — 需配置 SetByCaller Data.Damage Modifier 的 GE
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (DisplayName = "伤害效果类"))
	TSubclassOf<UGameplayEffect> DamageEffect;

	// 区域 Actor 类 — 可用 BP 子类挂载粒子/贴花等视觉效果
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (DisplayName = "区域 Actor 类"))
	TSubclassOf<ABFAreaDamageZone> AreaActorClass;

	// 位置偏移 — 相对于拥有者位置的偏移（世界坐标）
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (DisplayName = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	FTimerHandle DelayTimerHandle;
	TWeakObjectPtr<ABFAreaDamageZone> SpawnedZone;

	float ResolveFloat(const FName& MemberName, const FFlowDataPinInputProperty_Float& Default) const;
	void SpawnArea();

	UFUNCTION()
	void OnAreaExpired();
};
