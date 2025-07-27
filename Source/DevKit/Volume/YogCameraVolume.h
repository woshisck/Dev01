// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Volume.h"

#include "YogCameraVolume.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCameraVolume : public AVolume
{
	GENERATED_BODY()
public:
	
	AYogCameraVolume(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;


	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	// Overlap end function
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);



};
