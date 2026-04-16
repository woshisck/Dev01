#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameModes/LevelFlowTypes.h"
#include "RewardPickup.generated.h"

class UBoxComponent;
class APlayerCharacterBase;

/**
 * ARewardPickup — 关卡结算奖励拾取物
 *
 * 在最后一个敌人被击杀处生成，玩家走近触发战利品选择界面。
 * 关卡支持多个拾取物同时存在；每个拾取物在 Spawn 时由 GameMode 预分配
 * 独立的三选一选项（AssignedLoot），互不干扰、不重复。
 */
UCLASS()
class DEVKIT_API ARewardPickup : public AActor
{
	GENERATED_BODY()

public:
	ARewardPickup();

	// 玩家进入范围后按 E 键调用，执行实际拾取逻辑
	void TryPickup(APlayerCharacterBase* Player);

	/**
	 * 由 GameMode 在 Spawn 后立即调用，预分配本拾取物的三选一战利品。
	 * 如果没有调用过（AssignedLoot 为空），TryPickup 会退化为让 GameMode 即时生成。
	 */
	void AssignLoot(const TArray<FLootOption>& InLoot);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionVolume;

private:
	// GameMode 预分配的战利品选项（Spawn 时写入，拾取时广播给 UI）
	TArray<FLootOption> AssignedLoot;

	bool bPickedUp = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
