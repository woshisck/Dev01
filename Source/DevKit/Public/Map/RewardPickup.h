#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/PickupInteractable.h"
#include "RewardPickup.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UPrimitiveComponent;
class UTexture2D;
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
class DEVKIT_API ARewardPickup : public AActor, public IPickupInteractable
{
	GENERATED_BODY()

public:
	ARewardPickup();

	// ~ IPickupInteractable
	virtual void OnPlayerEnterRange(APlayerCharacterBase* Player) override;
	virtual void OnPlayerLeaveRange(APlayerCharacterBase* Player) override;
	virtual void TryPickup(APlayerCharacterBase* Player) override;

	/** 选符文确认后由 LootSelectionWidget 调用：销毁本拾取物 */
	void ConsumeAndDestroy();

	/** 跳过选择后由 LootSelectionWidget 调用：复位状态使玩家可再次按 E 重开 */
	void ResetForSkip(APlayerCharacterBase* Player);

	/**
	 * 由 GameMode 在 Spawn 后立即调用，预分配本拾取物的三选一战利品。
	 * 如果没有调用过（AssignedLoot 为空），TryPickup 会退化为让 GameMode 即时生成。
	 */
	void AssignLoot(const TArray<FLootOption>& InLoot);

	/** 重新应用当前拾取许可到可见性和 overlap；Story 节点运行时改 bAllowPickupOutsideArrangement 后需要调用。 */
	void RefreshPickupAvailability();

	UFUNCTION(BlueprintCallable, Category = "Reward|Pickup")
	void PlaySpawnFocusCue();

	bool IsAvailableForCameraFocus() const;

	UFUNCTION(BlueprintPure, Category = "Reward|Pickup")
	bool IsSpawnFocusCueActive() const { return bSpawnFocusCueActive; }

	/** Returns true when a pickup can grant all loot directly without opening card selection. */
	static bool ShouldGrantLootImmediatelyForOptions(const TArray<FLootOption>& Options);

	/** Returns the configured material currency tag, falling back to the HUD's primary material currency. */
	static FGameplayTag ResolveMaterialCurrencyTag(FGameplayTag ConfiguredTag);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Fixed")
	bool bUseFixedLootOptions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Fixed", meta = (EditCondition = "bUseFixedLootOptions"))
	TArray<FLootOption> FixedLootOptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Story")
	bool bAllowPickupOutsideArrangement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Spawn Focus")
	bool bEnableSpawnFocusCue = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Spawn Focus", meta = (ClampMin = "0.01", ClampMax = "1.0", EditCondition = "bEnableSpawnFocusCue"))
	float SpawnFocusDilationScale = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Spawn Focus", meta = (ClampMin = "0.05", EditCondition = "bEnableSpawnFocusCue"))
	float SpawnFocusDilationDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Spawn Focus", meta = (ClampMin = "0.05", EditCondition = "bEnableSpawnFocusCue"))
	float SpawnFocusHighlightDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Spawn Focus", meta = (ClampMin = "0", ClampMax = "255", EditCondition = "bEnableSpawnFocusCue"))
	int32 SpawnFocusStencilValue = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Audio")
	bool bStopAutoActivatedAudioOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Immediate Loot FX")
	TObjectPtr<UNiagaraSystem> GoldImmediateGrantFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Immediate Loot FX")
	TObjectPtr<UTexture2D> GoldImmediateGrantIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Immediate Loot FX")
	TObjectPtr<UNiagaraSystem> MaterialImmediateGrantFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Immediate Loot FX")
	TObjectPtr<UTexture2D> MaterialImmediateGrantIcon;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Pickup")
	void K2_OnImmediateLootGranted(ELootType LootType, int32 Amount, FGameplayTag MetaCurrencyTag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Pickup")
	void K2_OnGoldLootGranted(int32 Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Pickup")
	void K2_OnMaterialLootGranted(FGameplayTag MetaCurrencyTag, int32 Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Pickup")
	void K2_OnSpawnFocusCueStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Reward|Pickup")
	void K2_OnSpawnFocusCueEnded();

private:
	struct FPrimitiveHighlightState
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		bool bRenderCustomDepth = false;
		int32 CustomDepthStencilValue = 0;
	};
	// GameMode 预分配的战利品选项（Spawn 时写入，拾取时广播给 UI）
	UPROPERTY(Transient)
	TArray<FLootOption> AssignedLoot;

	bool bPickedUp = false;
	bool bPlayerInRange = false;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;
	bool bSpawnFocusCueActive = false;
	bool bSpawnFocusTimeDilationActive = false;
	float SpawnFocusHighlightElapsed = 0.f;
	float PreviousSpawnFocusGlobalTimeDilation = 1.f;
	FTSTicker::FDelegateHandle SpawnFocusDilationTickerHandle;
	TArray<FPrimitiveHighlightState> SpawnFocusPrimitiveStates;

	bool IsPickupAllowed() const;
	bool ShouldGrantImmediately(const TArray<FLootOption>& Options) const;
	bool GrantImmediateLoot(APlayerCharacterBase* Player, const TArray<FLootOption>& Options);
	void ClearNearbyPlayer();
	void SetSpawnFocusHighlightActive(bool bActive);
	void TickSpawnFocusCue(float DeltaTime);
	void RestoreSpawnFocusTimeDilation();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
