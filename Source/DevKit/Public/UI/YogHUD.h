#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameModes/LevelFlowTypes.h"
#include "GameplayEffectTypes.h"
#include "YogHUD.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class UYogSaveGame;
class APostProcessVolume;
class UWeaponGlassAnimDA;
class UWeaponDefinition;
class UBackpackScreenWidget;
class UWeaponTrailWidget;
class UWeaponThumbnailFlyWidget;
class ULevelEndEffectDA;
class ULevelEndRevealWidget;
class UMaterialInstanceDynamic;
class ULootSelectionWidget;
class UYogHUDRootWidget;

UCLASS()
class DEVKIT_API AYogHUD : public AHUD
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────
	//  Tutorial
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TSubclassOf<UTutorialPopupWidget> TutorialPopupClass;

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TObjectPtr<UDialogContentDA> DialogContentDA;

	// ─────────────────────────────────────────
	//  主 HUD 容器
	// ─────────────────────────────────────────

	/** 主 HUD 容器 WBP（WBP_HUDRoot，内含血条/箭头/武器图标等常驻元素） */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UYogHUDRootWidget> MainHUDClass;

	// ─────────────────────────────────────────
	//  暂停遮罩后处理
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "PauseEffect")
	float PauseFadeDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "PauseEffect")
	float PauseTargetSaturation = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "PauseEffect")
	float PauseTargetGain = 0.40f;

	void BeginPauseEffect();
	void EndPauseEffect();

	// ─────────────────────────────────────────
	//  关卡结束视觉特效
	// ─────────────────────────────────────────

	// 特效参数 DA（在 BP_YogHUD Details 面板赋值 DA_LevelEndEffect）
	UPROPERTY(EditDefaultsOnly, Category = "LevelEndEffect")
	TObjectPtr<ULevelEndEffectDA> LevelEndEffectDA;

	// 圆形揭幕 Widget 类（WBP 中一个全屏 Image 命名 RevealImage）
	UPROPERTY(EditDefaultsOnly, Category = "LevelEndEffect")
	TSubclassOf<ULevelEndRevealWidget> LevelEndRevealWidgetClass;

	/**
	 * 触发关卡结束特效（由 YogGameMode::EnterArrangementPhase 调用）。
	 * @param LootWorldPos  loot 拾取物的世界坐标，用于圆形揭幕中心
	 */
	UFUNCTION(BlueprintCallable, Category = "LevelEndEffect")
	void TriggerLevelEndEffect(FVector LootWorldPos);

	/**
	 * 揭幕动画完成（slow-mo + 黑屏退场全部走完）后广播一次。
	 * GameMode::BeginPlay 监听此事件 -> TriggerLifecycleEvent(LevelClearRevealed)。
	 * 设计：HUD 只负责"表演"和"广播表演完成"，不直接驱动业务逻辑（教程/UI）。
	 */
	FSimpleMulticastDelegate OnLevelEndEffectFinished;

	// ─────────────────────────────────────────
	//  背包
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Backpack")
	TSubclassOf<UBackpackScreenWidget> BackpackScreenClass;

	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void OpenBackpack();

	// ─────────────────────────────────────────
	//  三选一 Loot
	// ─────────────────────────────────────────

	// 在 BP_YogHUD Details 中指定 WBP_LootSelection（从 HB_PlayerMain 里移除对应的 CommonUI Stack 条目）
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<ULootSelectionWidget> LootSelectionWidgetClass;

	// GameMode 直接调用（不走 delegate），Widget 被销毁时自动重建
	void ShowLootSelectionUI(const TArray<FLootOption>& Options);

	// ─────────────────────────────────────────
	//  武器缩略图飞行 → 玻璃图标
	// ─────────────────────────────────────────

	/** 缩略图飞行 Widget 类（WBP：全屏 CanvasPanel + Image 命名 ThumbnailImage） */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponThumbnailFlyWidget> ThumbnailFlyClass;

	/** 流光拖尾 Widget */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponTrailWidget> TrailWidgetClass;

	/** 动画时序 DA */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TObjectPtr<UWeaponGlassAnimDA> WeaponGlassAnimDA;

	/**
	 * 拾取武器时调用：从 StartScreenPos 飞向左下角玻璃图标
	 * @param Def            武器定义（获取缩略图）
	 * @param StartScreenPos 飞行起点屏幕坐标（通常为 Spawner 投影坐标）
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void TriggerWeaponPickup(const UWeaponDefinition* Def, FVector2D StartScreenPos);

	/** 开背包前调用：玻璃图标播放放大→消失动画 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void NotifyBackpackOpening();

	/** 获取左下角玻璃图标的屏幕中心坐标 */
	UFUNCTION(BlueprintPure, Category = "WeaponGlass")
	FVector2D GetWeaponGlassIconScreenCenter() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UYogHUDRootWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UTutorialPopupWidget> TutorialPopupWidget;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> PausePPVolume;

	int32 PausePopupCount  = 0;
	float PauseEffectAlpha = 0.f;

	bool    bLevelEndEffectActive     = false;
	bool    bSlowMoPhaseEnded         = false;
	float   LevelEndEffectStartRealTime = 0.f;
	FVector CachedLootWorldPos        = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<ULevelEndRevealWidget> ActiveRevealWidget;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RevealDynMat;

	void TickLevelEndEffect();
	void StartRevealAnimation();

	void BindHealthAttributes(APawn* Pawn);

	UFUNCTION()
	void OnPawnPossessed(APawn* OldPawn, APawn* NewPawn);

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);

	bool bHasWeapon = false;

	UPROPERTY()
	TObjectPtr<UBackpackScreenWidget> BackpackWidget;

	UPROPERTY()
	TObjectPtr<ULootSelectionWidget> LootSelectionWidget;

	UPROPERTY()
	TObjectPtr<UWeaponTrailWidget> ActiveTrailWidget;

	void OnFlyProgressUpdate(FVector2D FlyStart, FVector2D CurrentPos, float Alpha);

	UFUNCTION()
	void OnSaveGameLoaded(UYogSaveGame* SaveGame);

	UFUNCTION()
	void OnWeaponFlyComplete(UTexture2D* Thumbnail);
};
