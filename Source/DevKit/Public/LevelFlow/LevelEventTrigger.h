#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelEventTrigger.generated.h"

class UBoxComponent;
class UFlowComponent;
class ULevelFlowAsset;
class APlayerCharacterBase;
struct FHitResult;

/**
 * 关卡事件触发器 — 放在关卡中，玩家进入 BoxComponent 后启动指定 LevelFlowAsset。
 *
 * 配置步骤：
 *   1. 放置到关卡，调整 BoxComponent 大小
 *   2. Details → LevelFlow → LevelFlow 字段指定 DA_LevelEvent_XXX
 *   3. DA 里用 LevelEvent 系列节点（LENode_TimeDilation、LENode_ShowTutorial 等）连线
 */
UCLASS(Blueprintable)
class DEVKIT_API ALevelEventTrigger : public AActor
{
	GENERATED_BODY()

public:
	ALevelEventTrigger();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	TObjectPtr<UFlowComponent> LevelFlowComp;

	// 指定要运行的关卡事件 Flow
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	TObjectPtr<ULevelFlowAsset> LevelFlow;

	// 是否只触发一次（默认开启）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelFlow")
	bool bTriggerOnce = true;

public:
	FSimpleMulticastDelegate OnPlayerExited;

private:
	UPROPERTY(VisibleAnywhere, Category = "LevelFlow|Debug")
	bool bTriggered = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
