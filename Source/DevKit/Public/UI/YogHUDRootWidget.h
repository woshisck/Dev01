#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YogHUDRootWidget.generated.h"

class ULiquidHealthBarWidget;
class UEnemyArrowWidget;
class UWeaponGlassIconWidget;
class UInfoPopupWidget;
class UCombatDeckBarWidget;
class UPlayerCommonInfoWidget;
class UCurrentRoomBuffWidget;
class UOverlay;

/**
 * Main HUD container widget.
 *
 * WBP_HUDRoot keeps stable named regions so gameplay widgets can be placed
 * without hard-coding screen positions in gameplay code.
 */
UCLASS()
class DEVKIT_API UYogHUDRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Stable layout regions used by WBP_HUDRoot. These are optional so older
	// HUD blueprints keep loading while generated layouts catch up.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> TopLeftPlayerInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> TopRightPlayerInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> BossInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> LeftLevelInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> RightLevelInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> BottomLeftPlayerInfoRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> BottomCenterCombatRegion;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> BottomRightPlayerInfoRegion;

	/** WBP control variable name: PlayerHealthBar */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULiquidHealthBarWidget> PlayerHealthBar;

	/** WBP control variable name: EnemyArrow */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEnemyArrowWidget> EnemyArrow;

	/** WBP control variable name: WeaponGlassIcon */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWeaponGlassIconWidget> WeaponGlassIcon;

	/** WBP control variable name: InfoPopup */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UInfoPopupWidget> InfoPopup;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCombatDeckBarWidget> CombatDeckBar;

	/** Player common resource display. WBP control variable name: PlayerCommonInfoHud */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPlayerCommonInfoWidget> PlayerCommonInfoHud;

	/** Current room enemy rune/buff panel. WBP control variable name: CurrentRoomBuffPanel */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCurrentRoomBuffWidget> CurrentRoomBuffPanel;
};
