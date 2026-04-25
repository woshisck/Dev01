#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AirWall.generated.h"

class UBoxComponent;

/**
 * 放置于关卡中的空气墙。
 *
 * 默认阻挡所有移动（含冲刺）。
 * 勾选 bAllowDashThrough 后，盒体对象类型切换为 DashThrough 通道：
 *   - 正常移动：Capsule 对 DashThrough 通道 = Block → 依然阻挡
 *   - 冲刺期间：Capsule 对 DashThrough 通道改为 Overlap → 可穿越
 *   - 冲刺步进判断仍正常运行（DashTrace 通道保持 Block），厚度超出步进延伸范围则无法穿越
 */
UCLASS(HideCategories = (Input, LOD, Cooking))
class DEVKIT_API AAirWall : public AActor
{
	GENERATED_BODY()

public:
	AAirWall();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	// 勾选后允许冲刺步进判断穿越此空气墙；取消勾选为实心阻挡
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Wall")
	bool bAllowDashThrough = false;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyCollisionSettings();
};
