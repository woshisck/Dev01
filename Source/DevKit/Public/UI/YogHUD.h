#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EnemyArrowWidget.h"
#include "YogHUD.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class UYogSaveGame;
class APostProcessVolume;
class UWeaponGlassIconWidget;
class UWeaponGlassAnimDA;
class UWeaponDefinition;
class UBackpackScreenWidget;
class UWeaponTrailWidget;
class UWeaponThumbnailFlyWidget;

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

	UFUNCTION(BlueprintCallable, Category = "Backpack")
	void OpenBackpack();

	// ─────────────────────────────────────────
	//  武器缩略图飞行 → 玻璃图标
	// ─────────────────────────────────────────

	/** 缩略图飞行 Widget 类（WBP：全屏 CanvasPanel + Image 命名 ThumbnailImage） */
	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponThumbnailFlyWidget> ThumbnailFlyClass;

	UPROPERTY(EditDefaultsOnly, Category = "WeaponGlass")
	TSubclassOf<UWeaponGlassIconWidget> WeaponGlassIconClass;

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
	TObjectPtr<UTutorialPopupWidget> TutorialPopupWidget;

	UPROPERTY()
	TObjectPtr<UEnemyArrowWidget> EnemyArrowWidget;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> PausePPVolume;

	int32 PausePopupCount  = 0;
	float PauseEffectAlpha = 0.f;

	UPROPERTY()
	TObjectPtr<UBackpackScreenWidget> BackpackWidget;

	UPROPERTY()
	TObjectPtr<UWeaponGlassIconWidget> WeaponGlassIconWidget;

	UPROPERTY()
	TObjectPtr<UWeaponTrailWidget> ActiveTrailWidget;

	void OnFlyProgressUpdate(FVector2D FlyStart, FVector2D CurrentPos, float Alpha);

	UFUNCTION()
	void OnSaveGameLoaded(UYogSaveGame* SaveGame);

	UFUNCTION()
	void OnWeaponFlyComplete(UTexture2D* Thumbnail);
};
