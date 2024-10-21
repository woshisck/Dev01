// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "YogPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	AYogPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

protected:
	// virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;



};
