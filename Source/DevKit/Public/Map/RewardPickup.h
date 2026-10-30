#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "Character/YogInteractable.h"
#include "GameModes/LevelFlowTypes.h"
#include "RewardPickup.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UInteractPromptComponent;
class UPrimitiveComponent;
class UTexture2D;
class UWidgetComponent;
class URuneRewardFloatWidget;
class ULootTypeChoiceWidget;
class URewardLootTableDataAsset;
class APlayerCharacterBase;

/**
 * Self-contained reward pickup. It is visible and interactable as soon as it exists, and it never
 * reads the GameMode. The owner (a map script, a story node, a dying enemy) decides when it spawns.
 *
 * Loot comes from, in order: options handed over by AssignLoot, FixedLootOptions, or a rune roll from
 * LootTable through UYogLootSubsystem.
 */
UCLASS()
class DEVKIT_API ARewardPickup : public AActor, public IYogInteractable
{
	GENERATED_BODY()

public:
	ARewardPickup();

	void OnPlayerEnterRange(APlayerCharacterBase* Player);
	void OnPlayerLeaveRange(APlayerCharacterBase* Player);
	void TryPickup(APlayerCharacterBase* Player);

	// ~ IYogInteractable
	virtual void TryInteract(APlayerCharacterBase* Player) override { TryPickup(Player); }
	virtual int32 GetInteractPriority() const override { return YogInteractPriority::Pickup; }
	virtual bool CanInteract(const APlayerCharacterBase* Player) const override { return !bPickedUp && bPickupEnabled; }

	/**
	 * Called by LootTypeChoiceWidget when the player picks one of LootTypeChoices.
	 * Rune opens the rune card selection; other types are granted immediately.
	 */
	void ResolveLootTypeChoice(int32 Index, APlayerCharacterBase* Player);

	/** 选符文确认后由 LootSelectionWidget 调用：销毁本拾取物 */
	void ConsumeAndDestroy();

	/** 跳过选择后由 LootSelectionWidget 调用：复位状态使玩家可再次按 E 重开 */
	void ResetForSkip(APlayerCharacterBase* Player);

	/** Hands this pickup its loot at spawn time. Without it the pickup rolls runes when interacted with. */
	void AssignLoot(const TArray<FLootOption>& InLoot);

	/** Shows or hides the pickup and its overlap. A map script uses this to gate a pickup until a room is cleared. */
	UFUNCTION(BlueprintCallable, Category = "Reward|Pickup")
	void SetPickupEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Reward|Pickup")
	bool IsPickupEnabled() const { return bPickupEnabled; }

	/** Re-applies bPickupEnabled to visibility and overlap. */
	void RefreshPickupAvailability();

	/**
	 * Spawns a pickup carrying Options. A null PickupClass falls back to UYogSettings::DefaultRewardPickupClass.
	 * Returns null when no class resolves or Options is empty.
	 */
	static ARewardPickup* SpawnWithLoot(UObject* WorldContext, TSubclassOf<ARewardPickup> PickupClass,
		const FVector& Location, const TArray<FLootOption>& Options, bool bPlayFocusCue = true);

	UFUNCTION(BlueprintCallable, Category = "Reward|Pickup")
	void PlaySpawnFocusCue();

	bool IsAvailableForCameraFocus() const;

	UFUNCTION(BlueprintPure, Category = "Reward|Pickup")
	bool IsSpawnFocusCueActive() const { return bSpawnFocusCueActive; }

	/** Returns true when a pickup can grant all loot directly without opening card selection. */
	static bool ShouldGrantLootImmediatelyForOptions(const TArray<FLootOption>& Options);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Fixed")
	bool bUseFixedLootOptions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Fixed", meta = (EditCondition = "bUseFixedLootOptions"))
	TArray<FLootOption> FixedLootOptions;

	/** When on, interacting opens a popup to choose the loot type before anything is granted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Type Choice")
	bool bOfferLootTypeChoice = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Type Choice", meta = (EditCondition = "bOfferLootTypeChoice"))
	TArray<FLootTypeChoice> LootTypeChoices;

	// Untick to spawn the pickup hidden until SetPickupEnabled(true) is called.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward|Pickup")
	bool bPickupEnabled = true;

	// Rune pool and card count for the rune roll. Empty uses UYogSettings::DefaultRewardLootTable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward|Pickup")
	TObjectPtr<URewardLootTableDataAsset> LootTable;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Floating Panel")
	TObjectPtr<UWidgetComponent> RuneInfoWidgetComp;

	// 按住 E 拾取的提示（按键图标 + 蓄力环）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact Prompt")
	TObjectPtr<UInteractPromptComponent> InteractPromptComp;

	// 在 BP_RewardPickup 里指定浮窗 WBP 类
	UPROPERTY(EditDefaultsOnly, Category = "Floating Panel")
	TSubclassOf<URuneRewardFloatWidget> RuneFloatWidgetClass;

	// 浮窗侧向偏移（摄像机 Right 方向，cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Panel")
	float WidgetSideOffset = 300.f;

	// 浮窗垂直偏移（cm）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating Panel")
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
	// Loot handed over at spawn (see AssignLoot); shown on the floating panel and offered on interact.
	UPROPERTY(Transient)
	TArray<FLootOption> AssignedLoot;

	bool bPickedUp = false;
	bool bPlayerInRange = false;
	TWeakObjectPtr<ULootTypeChoiceWidget> OpenLootTypeChoiceWidget;
	TWeakObjectPtr<APlayerCharacterBase> NearbyPlayer;
	bool bSpawnFocusCueActive = false;
	bool bSpawnFocusTimeDilationActive = false;
	float SpawnFocusHighlightElapsed = 0.f;
	float PreviousSpawnFocusGlobalTimeDilation = 1.f;
	FTSTicker::FDelegateHandle SpawnFocusDilationTickerHandle;
	TArray<FPrimitiveHighlightState> SpawnFocusPrimitiveStates;

	bool ShouldGrantImmediately(const TArray<FLootOption>& Options) const;
	bool GrantImmediateLoot(APlayerCharacterBase* Player, const TArray<FLootOption>& Options);
	bool TryOpenLootTypeChoice(APlayerCharacterBase* Player, APlayerController* PC);
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
