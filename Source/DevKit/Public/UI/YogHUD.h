#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EnemyArrowWidget.h"
#include "YogHUD.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class UYogSaveGame;
class APostProcessVolume;
class UWeaponFloatWidget;
class UWeaponGlassIconWidget;
class UWeaponGlassAnimDA;
class UWeaponDefinition;
class UBackpackScreenWidget;

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
	//  EnemyArrow
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "EnemyArrow")
	TSubclassOf<UEnemyArrowWidget> EnemyArrowWidgetClass;

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
	//  背包
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Backpack")
	TSubclassOf<UBackpackScreenWidget> BackpackScreenClass;

	/** 三选一结束后自动打开背包 */
	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void OpenBackpack();

	// ─────────────────────────────────────────
	//  武器浮窗 → 玻璃图标
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponFloatWidget> WeaponFloatClass;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponGlassIconWidget> WeaponGlassIconClass;

	/** 动画时序 DA（折叠/缩小/飞行/消失参数） */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TObjectPtr<UWeaponGlassAnimDA> WeaponGlassAnimDA;

	/**
	 * 拾取武器时调用：显示 WeaponFloat，自动延迟折叠→飞向左下角玻璃图标
	 * （也可在蓝图中手动调用 TriggerWeaponCollapse 提前折叠）
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void TriggerWeaponPickup(const UWeaponDefinition* Def);

	/** 手动触发折叠（跳过 AutoCollapseDelay） */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void TriggerWeaponCollapse();

	/**
	 * 开背包前调用：玻璃图标播放放大→消失动画
	 * WeaponFloat 会先收缩，背包应在此之后激活
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void NotifyBackpackOpening();

	/** 获取左下角玻璃图标的屏幕中心坐标（供外部查询） */
	UFUNCTION(BlueprintPure, Category = "WeaponGlass")
	FVector2D GetWeaponGlassIconScreenCenter() const;

	/** 直接获取 WeaponFloat Widget 实例（供蓝图调用 SetWeaponDefinition 等） */
	UFUNCTION(BlueprintPure, Category = "WeaponGlass")
	UWeaponFloatWidget* GetWeaponFloatWidget() const { return WeaponFloatWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UTutorialPopupWidget> TutorialPopupWidget;

	UPROPERTY()
	TObjectPtr<UEnemyArrowWidget> EnemyArrowWidget;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> PausePPVolume;

	int32 PausePopupCount  = 0;
	float PauseEffectAlpha = 0.f;

	UPROPERTY()
	TObjectPtr<UBackpackScreenWidget> BackpackWidget;

	// ── WeaponGlass ──
	UPROPERTY()
	TObjectPtr<UWeaponFloatWidget> WeaponFloatWidget;

	UPROPERTY()
	TObjectPtr<UWeaponGlassIconWidget> WeaponGlassIconWidget;

	FTimerHandle CollapseTimerHandle;

	UFUNCTION()
	void OnSaveGameLoaded(UYogSaveGame* SaveGame);

	UFUNCTION()
	void OnWeaponFlyComplete(UTexture2D* Thumbnail);
};
