#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameModes/LevelFlowTypes.h"
#include "RewardPickup.generated.h"

class UBoxComponent;
class UWidgetComponent;
class URuneRewardFloatWidget;
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

	// 玩家进入范围后按 E 键调用，触发战利品 UI（不再立刻销毁自己）
	void TryPickup(APlayerCharacterBase* Player);

	/** 选符文确认后由 LootSelectionWidget 调用：销毁本拾取物 */
	void ConsumeAndDestroy();

	/** 跳过选择后由 LootSelectionWidget 调用：复位状态使玩家可再次按 E 重开 */
	void ResetForSkip(APlayerCharacterBase* Player);

	/**
	 * 由 GameMode 在 Spawn 后立即调用，预分配本拾取物的三选一战利品。
	 * 如果没有调用过（AssignedLoot 为空），TryPickup 会退化为让 GameMode 即时生成。
	 */
	void AssignLoot(const TArray<FLootOption>& InLoot);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UBoxComponent> CollisionVolume;

	// 符文奖励浮窗 WidgetComponent（Screen Space）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "浮窗")
	TObjectPtr<UWidgetComponent> RuneInfoWidgetComp;

	// 在 BP_RewardPickup 里指定浮窗 WBP 类
	UPROPERTY(EditDefaultsOnly, Category = "浮窗")
	TSubclassOf<URuneRewardFloatWidget> RuneFloatWidgetClass;

	// 浮窗侧向偏移（摄像机 Right 方向，cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "浮窗")
	float WidgetSideOffset = 300.f;

	// 浮窗垂直偏移（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "浮窗")
	float WidgetZOffset = 50.f;

private:
	// GameMode 预分配的战利品选项（Spawn 时写入，拾取时广播给 UI）
	TArray<FLootOption> AssignedLoot;

	bool bPickedUp = false;
	bool bPlayerInRange = false;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
