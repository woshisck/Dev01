// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "YogSpringArmComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DEVKIT_API UYogSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bReverseLag : 1;

	FVector OwnerVelocity;

	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;
};
