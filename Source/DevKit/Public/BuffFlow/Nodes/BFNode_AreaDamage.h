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

	/** 生成区域前的延迟（秒），0 = 立即 */
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.0"))
	FFlowDataPinInputProperty_Float Delay;

	/** 区域持续时间（秒） */
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.01"))
	FFlowDataPinInputProperty_Float Duration;

	/** 球体半径（cm） */
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "1.0"))
	FFlowDataPinInputProperty_Float Radius;

	/** 每次伤害量 */
	UPROPERTY(EditAnywhere, Category = "AreaDamage", meta = (ClampMin = "0.0"))
	FFlowDataPinInputProperty_Float DamageAmount;

	/** 伤害间隔（秒），<= 0 表示仅命中一次 */
	UPROPERTY(EditAnywhere, Category = "AreaDamage")
	FFlowDataPinInputProperty_Float DamageInterval;

	/** 伤害 GE（需配置 SetByCaller Data.Damage Modifier） */
	UPROPERTY(EditAnywhere, Category = "AreaDamage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 区域 Actor 类（可用 BP 子类挂载粒子/贴花） */
	UPROPERTY(EditAnywhere, Category = "AreaDamage")
	TSubclassOf<ABFAreaDamageZone> AreaActorClass;

	/** 相对于拥有者位置的偏移 */
	UPROPERTY(EditAnywhere, Category = "AreaDamage")
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
