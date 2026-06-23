// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YogCursorWidget.generated.h"

class UImage;
class UTexture2D;

UENUM(BlueprintType)
enum class EYogCursorState : uint8
{
	Default,
	Interact,
	Drag,
	Invalid
};

UCLASS(Abstract)
class DEVKIT_API UYogCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Applies the cursor image for a state.
	 * Default C++ impl pulls the texture from the UI style asset (with logging
	 * fallback). Override in BP only if you need custom behavior.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Cursor")
	void OnCursorStateChanged(EYogCursorState NewState);

protected:
	virtual void NativeConstruct() override;

	/** WBP must contain an Image named exactly "Img_Cursor". */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Cursor;

	/** Fallback brush used when the style asset has no texture for a state. */
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TObjectPtr<UTexture2D> DefaultCursorTexture;
};
