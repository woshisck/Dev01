#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YogUIRegistry.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EYogUIScreenId : uint8
{
	MainHUD,
	Backpack,
	LootSelection,
	PauseMenu,
	TutorialPopup,
	SacrificeGraceOption,
	InfoPopup,
	PortalPreview,
	PortalDirection,
	CurrentRoomBuff,
	CombatItemBar,
	FinisherQTE,
	LevelEndReveal,
	WeaponFloat,
	WeaponThumbnailFly,
	WeaponTrail,
	DamageEdgeFlash,
	ShopSelection,
	AltarMenu,
	SacrificeSelection,
	RunePurification,
	EntryMenu
};

/**
 * Visual + input layer. Higher value = "on top" semantically.
 * Subsystem uses this to decide input mode + focus when a screen activates.
 *
 *  Game   : passive overlays (HUD, indicators, world-anchored popups). No focus, no input mode change.
 *  Menu   : in-game UI that needs focus but does not pause the game (Backpack, Loot, SacrificeGrace, Tutorial).
 *  Modal  : top-priority dialog/menu that owns input until dismissed (PauseMenu, system popups).
 */
UENUM(BlueprintType)
enum class EYogUILayer : uint8
{
	Game   UMETA(DisplayName = "Game (overlay, no focus)"),
	Menu   UMETA(DisplayName = "Menu (focusable, in-game)"),
	Modal  UMETA(DisplayName = "Modal (top priority)")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FYogUIRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EYogUIScreenId ScreenId = EYogUIScreenId::MainHUD;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UUserWidget> WidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ZOrder = 0;

	/** 该 Screen 的逻辑层级。Subsystem 用它决定激活时是否切 InputMode、是否抢焦点。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EYogUILayer Layer = EYogUILayer::Game;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bCreateOnHUDStart = false;

	/** Optional input policy override. If false, UYogUIManagerSubsystem derives sensible defaults from Layer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bOverrideInputPolicy = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bShowMouseCursor = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bPauseGame = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bDisablePawnInput = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	bool bAffectsMajorUI = false;

	/** 这条 Entry 的用途说明，给后来配 DA 的策划/程序看，不参与运行时逻辑。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (MultiLine = "true"))
	FString Description;
};

UCLASS(BlueprintType)
class DEVKIT_API UYogUIRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "UI")
	bool FindEntry(EYogUIScreenId ScreenId, FYogUIRegistryEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	TSoftClassPtr<UUserWidget> GetWidgetClass(EYogUIScreenId ScreenId) const;

	UFUNCTION(BlueprintPure, Category = "UI")
	int32 GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder = 0) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<FYogUIRegistryEntry> Entries;
};
