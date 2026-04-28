#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameModes/LevelFlowTypes.h"
#include "GameplayEffectTypes.h"
#include "YogHUD.generated.h"

class UTutorialPopupWidget;
class UTutorialRegistryDA;
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
class UInfoPopupWidget;
class ULevelInfoPopupDA;
class UPortalPreviewWidget;
class UPortalDirectionWidget;
class APortal;
class ARewardPickup;
class ASacrificeGracePickup;
class USacrificeGraceDA;
class USacrificeGraceOptionWidget;
class APlayerCharacterBase;


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

	/** 教程注册表（项目级唯一，配置一次永不再动）。
	 *  里面的 TMap<FName, UDialogContentDA*> 管理所有 EventID → 弹窗内容的映射。 */
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TObjectPtr<UTutorialRegistryDA> TutorialRegistry;

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

	/**
	 * 只读模式打开背包，关闭时回调 OnClosed。
	 * 背包内拖拽/旋转等修改操作被禁用，玩家仅能查看。
	 * 用于 LootSelection 期间的"预览"功能。
	 */
	void OpenBackpackForPreview(FSimpleDelegate OnClosed);

	// ─────────────────────────────────────────
	//  三选一 Loot
	// ─────────────────────────────────────────

	// 在 BP_YogHUD Details 中指定 WBP_LootSelection（从 HB_PlayerMain 里移除对应的 CommonUI Stack 条目）
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<ULootSelectionWidget> LootSelectionWidgetClass;

	// GameMode 直接调用（不走 delegate），Widget 被销毁时自动重建
	void ShowLootSelectionUI(const TArray<FLootOption>& Options);

	/**
	 * 由 RewardPickup 调用入队。当前没活跃选择 → 立即弹；否则排队。
	 * @param SourcePickup 选符文确认时由 LootSelection 销毁；跳过时复位
	 */
	void QueueLootSelection(const TArray<FLootOption>& Options, ARewardPickup* SourcePickup);

	/** 由 LootSelectionWidget 在 Skip / Select 后调用，弹队列下一项 */
	void OnLootSelectionFinished();

	// ─────────────────────────────────────────
	//  献祭恩赐确认弹窗
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "SacrificeGrace")
	TSubclassOf<USacrificeGraceOptionWidget> SacrificeGraceOptionWidgetClass;

	/**
	 * 由 SacrificeGracePickup::TryPickup 调用：显示 Yes/No 确认弹窗。
	 * Yes → AcquireSacrificeGrace → 销毁拾取物；No → 复位拾取物。
	 */
	void ShowSacrificeGraceOption(USacrificeGraceDA* DA, APlayerCharacterBase* Player, ASacrificeGracePickup* Pickup);

	// ─────────────────────────────────────────
	//  轻量信息提示浮窗（不暂停游戏，放在 WBP_HUDRoot 内）
	// ─────────────────────────────────────────

	void ShowInfoPopup(const ULevelInfoPopupDA* DA);

	UInfoPopupWidget* GetInfoPopupWidget() const;

	// ─────────────────────────────────────────
	//  Portal 引导（v3：单例浮窗 + 屏幕边缘方位箭头 + 进入过场 Blackout）
	// ─────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	TSubclassOf<UPortalPreviewWidget> PortalPreviewClass;

	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	TSubclassOf<UPortalDirectionWidget> PortalDirectionClass;

	/** 浮窗距门屏幕投影的水平避让偏移（仿 RewardPickup） */
	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	float PortalWidgetSideOffset = 32.f;

	/** 浮窗距门屏幕投影的 Z 偏移（向上抬，避免遮门） */
	UPROPERTY(EditDefaultsOnly, Category = "Portal")
	float PortalWidgetZOffset = 80.f;

	/** 即使门在屏幕外，距玩家小于此距离也会被选为 Target */
	UPROPERTY(EditDefaultsOnly, Category = "Portal", meta = (ClampMin = "0"))
	float PortalForceShowDistance = 800.f;

	/** Target 切换距离滞回（cm），防中点抖动 */
	UPROPERTY(EditDefaultsOnly, Category = "Portal", meta = (ClampMin = "0"))
	float PortalSwitchHysteresis = 100.f;

	/** 进入过场渐黑时长（线性，秒）。应与 Portal::PortalEntryFailSafeBuffer 对齐 */
	UPROPERTY(EditDefaultsOnly, Category = "Portal", meta = (ClampMin = "0.05"))
	float PortalBlackoutDuration = 0.3f;

	/** 关卡结算后启用引导（EnterArrangementPhase 末尾调；主城传送门跳过此调用） */
	void ShowPortalGuidance();

	/** 切关前 / 战斗开始时关闭引导 */
	void HidePortalGuidance();

	/** Portal 玩家进/出范围回调（v3 决策：当前实现仅由 TickPortalPreview 读 PendingPortal，
	    本接口保留作为 BP 扩展或后续高级行为占位） */
	void NotifyPlayerInPortalRange(APortal* Portal);
	void NotifyPlayerExitedPortalRange(APortal* Portal);

	/** 由 APortal::TryEnter 调用：线性渐黑（PostProcess Saturation/Gain 朝 LevelEndEffectDA 目标插值） */
	void BeginBlackoutFade(float Duration);

	/** 反向：从 Blackout 线性插回正常。下一关 BeginPlay 检测 GI->bPlayLevelIntroFadeIn 时调 */
	void EndBlackoutFade(float Duration);

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
	TObjectPtr<USacrificeGraceOptionWidget> SacrificeGraceOptionWidget;

	// === LootSelection 队列管理（多个 RewardPickup 同时触发时按 FIFO 排队） ===
	struct FQueuedLootRequest
	{
		TArray<FLootOption> Options;
		TWeakObjectPtr<ARewardPickup> SourcePickup;
	};
	TArray<FQueuedLootRequest> LootQueue;
	bool bLootSelectionActive = false;

	// === 背包预览模式：关闭时触发的一次性回调 ===
	FSimpleDelegate BackpackPreviewClosedCallback;
	FDelegateHandle BackpackPreviewDeactivatedHandle;
	void OnBackpackPreviewClosed();

	UPROPERTY()
	TObjectPtr<UWeaponTrailWidget> ActiveTrailWidget;

	// === Portal 引导（私有运行时状态） ===
	UPROPERTY()
	TObjectPtr<UPortalPreviewWidget>   PortalPreviewWidget;

	UPROPERTY()
	TObjectPtr<UPortalDirectionWidget> PortalDirectionWidget;

	TArray<TWeakObjectPtr<APortal>> CachedOpenPortals;
	TWeakObjectPtr<APortal>         CurrentPreviewTarget;
	bool bShowPortalGuidance = false;

	// === Portal 进入过场 Blackout（独立 PP Volume，不与 Pause/LevelEnd 互扰） ===
	UPROPERTY()
	TObjectPtr<APostProcessVolume> BlackoutPPVolume;

	float BlackoutAlpha          = 0.f;
	float BlackoutTargetAlpha    = 0.f;
	float BlackoutActiveDuration = 0.5f;

	void TickPortalPreview(float DeltaSeconds);
	void TickBlackoutFade(float DeltaSeconds);
	void ApplyBlackoutPP();

	void OnFlyProgressUpdate(FVector2D FlyStart, FVector2D CurrentPos, float Alpha);

	UFUNCTION()
	void OnSaveGameLoaded(UYogSaveGame* SaveGame);

	UFUNCTION()
	void OnWeaponFlyComplete(UTexture2D* Thumbnail);
};
