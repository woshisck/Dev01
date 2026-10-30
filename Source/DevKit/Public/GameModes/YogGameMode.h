// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "GameplayTagContainer.h"
#include "GameModes/LevelFlowTypes.h"
#include "GameModes/SpawnTypes.h"
#include "GameModes/GameLifecycleTypes.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "Data/SacrificeGraceDA.h"
#include "Containers/Ticker.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class APlayerCharacterBase;
class UYogGameInstanceBase;
class UYogSaveGame;
class AEnemyCharacterBase;
class APortal;
class AYogCharacterBase;
class ULevelFlowAsset;
class UFlowComponent;
class URuneDataAsset;

DECLARE_MULTICAST_DELEGATE_OneParam(FBossRegisteredNativeDelegate, AEnemyCharacterBase*);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyRegistryChangedNativeDelegate, AEnemyCharacterBase*);

/**
 * Per-map game rules: player spawn, death / revive, kill counting, the enemy registry that the camera
 * and HUD read, cross-room transition and the lifecycle event bus.
 *
 * Room flow (mobs, doors) lives on AYogRoomDirector + Content/Script/Maps. Reward pickups are
 * self-contained actors (ARewardPickup). Neither is this class's job.
 */
UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay()override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bAutoSpawnPlayer = false;

	UFUNCTION(BlueprintCallable, Category = "KillCount")
	void UpdateFinishLevel(int count);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillCount")
	int MonsterKillCount;

	// =========================================================
	// Enemy registry (read by camera, AI, HUD)
	// =========================================================

	/** Registers a spawned enemy so the camera can sense combat. */
	void RegisterEnemy(AEnemyCharacterBase* Enemy);

	/** Drops a dead enemy from the registry. */
	void UnregisterEnemy(AEnemyCharacterBase* Enemy);

	/** Fires from RegisterEnemy for Boss-tier enemies only. Drives the HUD boss health bar. */
	FBossRegisteredNativeDelegate OnBossRegisteredNative;

	/** Fires once per enemy actually added to the registry. */
	FEnemyRegistryChangedNativeDelegate OnEnemyRegisteredNative;

	/** Fires once per enemy actually removed from the registry; never for an enemy that was not registered. */
	FEnemyRegistryChangedNativeDelegate OnEnemyUnregisteredNative;

	/** True while any registered enemy is alive. */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	bool HasAliveEnemies() const;

	/**
	 * Centroid of the living enemies within WithinRadius of Origin.
	 * Returns Origin itself when none are in range (zero offset).
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	FVector GetEnemyCentroid(FVector Origin, float WithinRadius) const;

	/**
	 * Unit direction towards the nearest living enemy.
	 * Returns FVector::ZeroVector when none are alive.
	 */
	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	FVector GetNearestEnemyDirection(FVector Origin) const;

	/** Living enemies within WithinRadius of Origin. Never contains dead or destroyed actors. */
	TArray<AEnemyCharacterBase*> GetNearbyEnemies(FVector Origin, float WithinRadius) const;

	/** Every living registered enemy, no distance filter (EnemyArrowWidget). */
	TArray<AEnemyCharacterBase*> GetAllAliveEnemies() const;

	// =========================================================
	// Run state
	// =========================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelFlow|Deprecated")
	int32 CompletedCombatBattleCount = 0;

	UFUNCTION(BlueprintPure, Category = "LevelFlow|Deprecated")
	int32 GetCompletedCombatBattleCount() const { return CompletedCombatBattleCount; }

	// Current floor, 1-based. TransitionToLevel writes CurrentFloor + 1 to the GameInstance.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign")
	int32 CurrentFloor = 1;

	// Portal-triggered room change: snapshots the run state (including the chosen room), then OpenLevel.
	// Called by APortal.
	UFUNCTION(BlueprintCallable, Category = "LevelFlow")
	void TransitionToLevel(FName NextLevel, URoomDataAsset* NextRoom = nullptr);

	// =========================================================
	// Death / revive
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void HandlePlayerDeath(APlayerCharacterBase* Player);

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	bool RevivePlayerFromDeath();

	UFUNCTION(BlueprintPure, Category = "GameOver")
	bool IsPlayerDeathPending() const { return bPlayerDeathPending; }

	UFUNCTION(BlueprintPure, Category = "GameOver")
	bool IsScriptedDefeatGameOver() const { return bScriptedDefeatGameOver; }

	static bool CanOfferPlayerDeathRevive(bool bInGameOverTriggered, bool bInPlayerDeathReviveUsed);
	static bool ShouldBroadcastRunSummaryForPlayerDeathGameOver(bool bCanRevive);
	static float CalculatePlayerReviveHealth(float MaxHealth, float ReviveHealthPercent);

	// =========================================================
	// Lifecycle event bus
	// =========================================================

	/**
	 * Editor-configurable map: event -> LevelFlowAsset to run.
	 * Events without a mapping do nothing.
	 */
	UPROPERTY(Transient)
	TMap<EGameLifecycleEvent, TObjectPtr<ULevelFlowAsset>> LifecycleEventFlows;

	/**
	 * Runs the flow mapped to Event in LifecycleEventFlows.
	 * One-shot events (HeatPhase* / GameStart / PlayerDeath) de-duplicate themselves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lifecycle Events")
	void TriggerLifecycleEvent(EGameLifecycleEvent Event);

	UFUNCTION(BlueprintCallable, Category = "StoryEvent")
	bool RunStoryLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow = true);

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// Enemy registry. Weak pointers; RegisterEnemy / UnregisterEnemy keep it in sync.
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> AliveEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	float PlayerDeathGameOverDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "1.0"))
	float PlayerReviveHealthPercent = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerReviveProtectionDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameOver", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ClampMax = "1.0"))
	float PlayerDeathTimeDilationScale = 0.12f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bGameOverTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bPlayerDeathReviveUsed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bPlayerDeathPending = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameOver", meta = (AllowPrivateAccess = "true"))
	bool bScriptedDefeatGameOver = false;

	UPROPERTY()
	TWeakObjectPtr<APlayerCharacterBase> PendingDeathPlayer;

	FTSTicker::FDelegateHandle PlayerDeathGameOverTickerHandle;

	/** De-dup set for one-shot lifecycle events. Transient, so it resets per PIE session. */
	UPROPERTY(Transient)
	TSet<EGameLifecycleEvent> FiredOnceEvents;

	/** Runs the lifecycle / story flows. Only one flow at a time; a new trigger stops the old one. */
	UPROPERTY()
	TObjectPtr<UFlowComponent> LifecycleFlowComponent;

private:
	void FinishPlayerDeathGameOver();
	void ClearPlayerDeathGameOverTicker();
	void BeginPlayerDeathVisuals(APlayerCharacterBase* Player);
	void EndPlayerDeathVisuals(APlayerCharacterBase* Player);

	/** One-shot events: HeatPhase* / GameStart / PlayerDeath. */
	static bool IsOneShotEvent(EGameLifecycleEvent Event);
};
