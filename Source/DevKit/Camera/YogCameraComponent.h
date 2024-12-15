// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "YogCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UYogCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
public:
	UYogCameraComponent(const FObjectInitializer& ObjectInitializer);



	UFUNCTION(BlueprintPure, Category = "Character Camera")
	static UYogCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UYogCameraComponent>() : nullptr); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Camera")
	float CameraHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Camera")
	float CameraLerpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Camera")
	bool bFollowPlayer;

	UFUNCTION(BlueprintPure, Category = "Character Camera")
	FVector GetDesiredLocation();

	void InitCamera();

	void UpdatePosition();
	FVector GetOffsetDirection();
};
