// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Camera/YogSpringArmComponent.h"
#include "YogCameraActorBase.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCameraActorBase : public ACameraActor
{
	GENERATED_BODY()
public:
	AYogCameraActorBase();


	/** Updates the actor owner and attaches the camera to it */
	void SetupTarget(class APawn& targetPawn);

	/** Updates the relative position of the camera based on the owner's viewpoint */
	virtual void Tick(float deltaSeconds) override;

	/** Shows up the modifiers debug info */
	virtual void DisplayDebug(class UCanvas* canvas, const FDebugDisplayInfo& debugDisplay, float& yL, float& yPos) override;


private:

	/** Updates the camera attachment */
	void UpdateCameraAttachment();

	/** Updates the settings stack */
	void UpdateSettings(float deltaTime);

	UPROPERTY(EditAnywhere)
	TObjectPtr<UYogSpringArmComponent> CameraBoom = nullptr;



};
