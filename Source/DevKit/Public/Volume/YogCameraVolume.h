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

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Volume|Debug")
	bool bShowDebugInGame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Volume|Debug", meta = (EditCondition = "bShowDebugInGame"))
	FColor DebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Volume|Debug", meta = (ClampMin = "0.1", EditCondition = "bShowDebugInGame"))
	float DebugLineThickness = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Volume|Camera", meta = (ClampMin = "0.0"))
	float ExtendedArmLength = 3200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Volume|Camera", meta = (ClampMin = "0.0"))
	float ArmLengthBlendSpeed = 3.f;

protected:
	virtual void BeginPlay() override;


	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	// Overlap end function
	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);



};
