// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GenericPlatform/ICursor.h"
#include "YogCursorStyleDataAsset.generated.h"

UENUM(BlueprintType)
enum class EYogCursorState : uint8
{
	Default,
	Interact,
	Drag,
	Invalid
};

USTRUCT(BlueprintType)
struct FYogCursorArt
{
	GENERATED_BODY()

	/**
	 * Content-relative path with NO file extension. "Slate/Cursors/Pointer" resolves
	 * to Content/Slate/Cursors/Pointer.png. Hardware cursors are loaded from loose files
	 * on disk by the platform, so an imported UTexture2D asset cannot be used here.
	 * Per-DPI variants are picked up automatically (Pointer@1.25x.png, Pointer@2x.png).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cursor")
	FName CursorPath;

	/** Click point, normalized 0..1 across the image, not pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cursor", meta = (ClampMin = 0, ClampMax = 1))
	FVector2D HotSpot = FVector2D::ZeroVector;
};

/**
 * Designer-facing hardware cursor art (DA_YogCursorStyle), resolved from DefaultGame.ini
 * through UDevAssetManager::GetCursorStyle() and applied by AYogPlayerControllerBase.
 *
 * Entries with an empty CursorPath are skipped, leaving that shape as the OS default.
 */
UCLASS(BlueprintType)
class DEVKIT_API UYogCursorStyleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mouse Cursor")
	TMap<EYogCursorState, FYogCursorArt> CursorArt;

	/**
	 * Maps a game state onto a stock engine cursor shape. Reusing the stock shapes means
	 * UMG widgets that already request Hand or GrabHandClosed inherit this art for free.
	 */
	static EMouseCursor::Type StateToSlot(EYogCursorState State);
};
