// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/YogCursorWidget.h"
#include "PlayerUIStyleDataAsset.generated.h"

/**
 * Centralized player-interface texture theme (DA_PlayerUIStyle).
 *
 * Holds framework/chrome textures for the player HUD: health bar, weapon slot
 * frames, and mouse cursor states. Content-driven images (buff icons, weapon
 * icons) are intentionally NOT stored here — those live on their own assets.
 *
 * Empty texture fields are valid; the consuming widget should fall back to a
 * solid color or hide the element.
 */
UCLASS(BlueprintType)
class DEVKIT_API UPlayerUIStyleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// =========================================================
	// Health bar
	// =========================================================

	/** Filled portion of the health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TObjectPtr<UTexture2D> HealthBarFillTexture;

	/** Empty/depleted track behind the fill. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TObjectPtr<UTexture2D> HealthBarBackgroundTexture;

	/** Decorative frame drawn on top of the bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar")
	TObjectPtr<UTexture2D> HealthBarFrameTexture;

	// =========================================================
	// Weapon slot framework
	// =========================================================

	/** Default (idle) weapon slot frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Slot")
	TObjectPtr<UTexture2D> WeaponSlotFrameTexture;

	/** Frame drawn over the currently selected/active weapon slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Slot")
	TObjectPtr<UTexture2D> WeaponSlotSelectedFrameTexture;

	/** Background fill behind a weapon icon (empty slot look). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Slot")
	TObjectPtr<UTexture2D> WeaponSlotBackgroundTexture;

	// =========================================================
	// Mouse cursor (keyed to EYogCursorState)
	// =========================================================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse Cursor")
	TObjectPtr<UTexture2D> CursorDefaultTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse Cursor")
	TObjectPtr<UTexture2D> CursorInteractTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse Cursor")
	TObjectPtr<UTexture2D> CursorDragTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse Cursor")
	TObjectPtr<UTexture2D> CursorInvalidTexture;

	/** Returns the cursor texture for a state, or CursorDefaultTexture as fallback. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mouse Cursor")
	UTexture2D* GetCursorTexture(EYogCursorState State) const;
};
