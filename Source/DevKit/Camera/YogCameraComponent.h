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


	virtual AActor* GetTargetActor() const { return GetOwner(); }


};
