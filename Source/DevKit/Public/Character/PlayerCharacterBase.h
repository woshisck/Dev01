// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Data/RuneDataAsset.h"
#include "Data/AltarDataAsset.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "Containers/Ticker.h"

#include "PlayerCharacterBase.generated.h"

/**
 * 
 */
//class AAuraBase;
class ARewardPickup;
class APlayerCharacterBase;
class ASacrificeGracePickup;
class AAltarActor;
class AShopActor;
class AWeaponSpawner;
class APortal;
class AWeaponInstance;
class UYogSaveGame;
class UBackpackStyleDataAsset;
class UBackpackGridComponent;
class UCombatDeckComponent;
class UCombatItemComponent;
class UComboRuntimeComponent;
class UBuffFlowComponent;
class USacrificeRuneComponent;
class USkillChargeComponent;
class UWeaponDefinition;
class USacrificeGraceDA;
class UYogAbilitySystemComponent;
class UDamageEdgeFlashWidget;
UENUM()
enum class EPlayerState : uint8
{
	OnMove			UMETA(DisplayName = "OnMove"),
	OnAction		UMETA(DisplayName = "Action")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerStateDelegate, EPlayerState, State);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractEndDelegate, APlayerCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemInteractStartDelegate, APlayerCharacterBase*, Character, AActor*, actor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatUpdateDelegate, const float, HeatPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMaxHeatUpdateDelegate, const float, MaxHeatValue);
// 热度阶段变化（0=无热度, 1=白光, 2=绿光, 3=橙黄, 4=过热红光）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeatPhaseDelegate, int32, Phase);

UCLASS()
class DEVKIT_API APlayerCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRep_PlayerState() override;


	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetOwnCamera(AYogCameraPawn* cameraActor);

	UFUNCTION(BlueprintCallable)
	AYogCameraPawn* GetOwnCamera();

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;

	virtual void FinishDying() override;

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void PrepareForDeathReviveChoice();

	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void ReviveFromDeath(float ReviveHealthPercent, float ProtectionDuration);

	UFUNCTION(BlueprintPure, Category = "GameOver")
	bool IsWaitingForDeathReviveChoice() const { return bWaitingForDeathReviveChoice; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	void ItemInteract(const AItemSpawner* item);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TObjectPtr<UBackpackGridComponent> BackpackGridComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TObjectPtr<UCombatDeckComponent> CombatDeckComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Item")
	TObjectPtr<UCombatItemComponent> CombatItemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Combo")
	TObjectPtr<UComboRuntimeComponent> ComboRuntimeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuffFlow")
	TObjectPtr<UBuffFlowComponent> BuffFlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sacrifice Rune")
	TObjectPtr<USacrificeRuneComponent> SacrificeRuneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SkillCharge")
	TObjectPtr<USkillChargeComponent> SkillChargeComponent;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TObjectPtr<UPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FItemInteractStartDelegate OnItemInterActionStart;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FItemInteractEndDelegate OnItemInterActionEnd;

	UPROPERTY(BlueprintAssignable, Category = "Character")
	FPlayerStateDelegate OnPlayerStateUpdate;

	UPROPERTY(BlueprintAssignable, Category = "State")
	FPlayerStateDelegate OnFPlayerStateDeleg;

	// 热度阶段广播（由热度系统调用，武器 Instance 订阅以更新发光颜色）
	UPROPERTY(BlueprintAssignable, Category = "Heat")
	FHeatPhaseDelegate OnHeatPhaseChanged;

	/** 热度升阶时的手柄震动效果（在角色蓝图 Details 中填入 ForceFeedbackEffect 资产） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Heat|Feedback")
	TObjectPtr<UForceFeedbackEffect> PhaseUpForceFeedback;

	// ─── 热度升阶视觉光效（玩家身体）─────────────────────────────────────

	/** 玩家骨骼网格 Overlay 材质（需含 SweepProgress / GlowAlpha / EmissiveColor 三个参数） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat|Visual")
	TObjectPtr<UMaterialInterface> PhaseUpPlayerOverlayMaterial;

	/** 热度颜色配置（与背包格子颜色统一来源，填入 DA_BackpackStyle） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heat|Visual")
	TObjectPtr<UBackpackStyleDataAsset> HeatStyleDA;

	/** 扫射动画时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.1"))
	float GlowSweepDuration = 0.5f;

	/** 扫射结束后边缘光保持时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.0"))
	float GlowHoldDuration = 3.0f;

	/** 边缘光淡出时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0.1"))
	float GlowFadeDuration = 0.5f;

	/** 炫彩强度（玩家边缘 Fresnel 极强处的彩虹光，0=关闭，推荐 0.2–0.35） */
	UPROPERTY(EditDefaultsOnly, Category = "Heat|Visual", meta = (ClampMin = "0", ClampMax = "1"))
	float GlowIridIntensity = 0.28f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback")
	bool bEnableDamageReceivedFeedback = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Screen")
	FLinearColor DamageScreenFlashColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Screen", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageScreenFlashAlpha = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Screen", meta = (ClampMin = "0.01"))
	float DamageScreenFlashDuration = 0.32f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Screen", meta = (ClampMin = "0.01", ClampMax = "0.45"))
	float DamageScreenEdgeWidthRatio = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Player Glow")
	TObjectPtr<UMaterialInterface> DamagePlayerOverlayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Player Glow")
	FLinearColor DamagePlayerGlowColor = FLinearColor(6.f, 6.f, 6.f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Player Glow", meta = (ClampMin = "0.01"))
	float DamagePlayerGlowDuration = 0.16f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Player Glow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamagePlayerGlowIridIntensity = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Controller")
	TObjectPtr<UForceFeedbackEffect> DamageReceivedForceFeedback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Controller", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageDynamicForceFeedbackIntensity = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Controller", meta = (ClampMin = "0.0"))
	float DamageDynamicForceFeedbackDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Time Dilation")
	bool bEnableDamageTimeDilation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Time Dilation", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float DamageTimeDilationScale = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage Feedback|Time Dilation", meta = (ClampMin = "0.0"))
	float DamageTimeDilationDuration = 0.15f;

	UPROPERTY()
	TObjectPtr<AItemSpawner> OverlappingSpawner;

	// 当前在拾取范围内的 RewardPickup（按 E 键时触发拾取）
	UPROPERTY()
	TObjectPtr<ARewardPickup> PendingPickup;

	// 当前在拾取范围内的 SacrificeGracePickup（按 E 键时触发献祭恩赐获取）
	UPROPERTY()
	TObjectPtr<ASacrificeGracePickup> PendingSacrificePickup;

	UPROPERTY()
	TObjectPtr<AAltarActor> PendingAltar;

	UPROPERTY()
	TObjectPtr<AShopActor> PendingShop;

	// 当前在拾取范围内的 WeaponSpawner（按 E 键时触发武器拾取）
	UPROPERTY()
	TObjectPtr<AWeaponSpawner> PendingWeaponSpawner;

	// 当前在交互范围内的传送门（按 E 键时触发 TryEnter）
	// 设计约束保证多门 Box 不重叠，单值实现足够（v3 决策表）
	UPROPERTY()
	TObjectPtr<APortal> PendingPortal;

	// 当前装备的武器 Actor（换武器时 Destroy）
	UPROPERTY()
	TObjectPtr<AWeaponInstance> EquippedWeaponInstance;

	// 提供当前武器的 Spawner（换武器时恢复其展示网格颜色）
	UPROPERTY()
	TObjectPtr<AWeaponSpawner> EquippedFromSpawner;

	UFUNCTION(BlueprintPure, Category = "Backpack")
	UBackpackGridComponent* GetBackpackGridComponent();

	// 将符文加入待放置列表（由 GameMode 的 SelectLoot 调用）
	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void AddRuneToInventory(const FRuneInstance& Rune);

	// 待放置符文列表（整理阶段从此处拖放到格子）
	UPROPERTY(BlueprintReadOnly, Category = "Backpack")
	TArray<FRuneInstance> PendingRunes;

	// 切关后从 GameInstance.PendingRunState 恢复 HP / 金币 / 符文 / 热度阶段
	void RestoreRunStateFromGI();

	// BeginPlay 末尾重新 Link 武器动画层（GAS 授能可能覆盖切关时已 Link 的层）
	void RelinkWeaponAnimLayer();

	// 当前装备的武器定义（切关时写入 RunState，由 RestoreRunStateFromGI 重新装备）
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> EquippedWeaponDef;

	// ─── 献祭恩赐（全局 Run Buff）────────────────────────────────────

	/** 当前生效的献祭恩赐 DA（None = 未获得） */
	UPROPERTY(BlueprintReadOnly, Category = "SacrificeGrace")
	TObjectPtr<USacrificeGraceDA> ActiveSacrificeGrace;

	/**
	 * 获取献祭恩赐：施加满热度 GE + BonusEffect + 启动衰退 FA
	 * 由拾取物触发（接受确认后调用）
	 */
	UFUNCTION(BlueprintCallable, Category = "SacrificeGrace")
	void AcquireSacrificeGrace(USacrificeGraceDA* DA);

	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	bool ApplySacrificeOfferingCost(const FAltarSacrificeEntry& CostEntry, int32 DeckCardIndex = -1);

	void RestoreSacrificeOfferingCosts(const TArray<FSacrificeOfferingCostState>& Costs);

	const TArray<FSacrificeOfferingCostState>& GetSacrificeOfferingCosts() const { return ActiveSacrificeOfferingCosts; }

	// ─── 最后输入方向（冲刺朝向使用）────────────────────────────────
	// 由 Controller.Move() 在每次非零输入时更新，世界空间单位向量
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FVector LastInputDirection = FVector::ForwardVector;

	friend UPlayerAttributeSet;

	UFUNCTION(BlueprintPure, Category = "Heat")
	int32 GetCurrentHeatPhase() const { return CurrentHeatPhase; }

protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;

private:

	void SetupHeatPhaseTagListeners();
	void OnHeatPhaseTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnHeatPhaseParentTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 触发玩家身体扫射+边缘光效果 */
	void StartPlayerPhaseGlow(int32 Phase);
	/** Tick 内逐帧更新材质参数 */
	void TickPlayerPhaseGlow(float DeltaTime);

	UFUNCTION()
	void HandleDamageReceivedFeedback(UYogAbilitySystemComponent* SourceASC, float Damage);

	void PlayDamageScreenFlash();
	void StartDamagePlayerGlow();
	void TickDamagePlayerGlow(float DeltaTime);
	void StartDamageTimeDilation();
	void RestoreDamageTimeDilation();
	void EndReviveProtection();

	int32 CurrentHeatPhase = 0;
	UPROPERTY()
	TArray<FSacrificeOfferingCostState> ActiveSacrificeOfferingCosts;
	float PhaseGlowElapsed = -1.f;
	float DamageGlowElapsed = -1.f;
	float PreviousDamageGlobalTimeDilation = 1.f;
	FTSTicker::FDelegateHandle DamageTimeDilationTickerHandle;
	bool bDamageTimeDilationVisualActive = false;
	bool bWaitingForDeathReviveChoice = false;
	FTimerHandle ReviveProtectionTimerHandle;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PlayerOverlayDynMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> DamageOverlayDynMat;
	UPROPERTY() TObjectPtr<UDamageEdgeFlashWidget> DamageEdgeFlashWidget;

};
