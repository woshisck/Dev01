// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ModularPawn.h"
#include "../Character/YogCharacterBase.h"

#include "YogCameraActor.generated.h"






const FVector RightArmRelRotation = FVector(0, 0, 90);
const FVector LeftArmRelRotation = FVector(0, 0, 270);
const FVector UpArmRelRotation = FVector(0, 0, 180);
const FVector DownArmRelRotation = FVector(0, 0, 0);

class AYogPlayerControllerBase;
class AYogCharacterBase;



USTRUCT(BlueprintType)
struct FCameraMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCameraMovementData()
		: MaxSpeed(600.0f), Acceleration(1000.0f), Deceleration(10000.0f), TurningBoost(8.0f), FocusAcc(2048.f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Acceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Deceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TurningBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FocusAcc;

};


UCLASS()
class DEVKIT_API AYogCameraActor : public AModularPawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AYogCameraActor();




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	class UStaticMeshComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	class USpringArmComponent* ScreenArm;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TObjectPtr<UDataTable> CameraMovementDataTable;





	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpeedCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AccelerationCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DecelerationCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TurningBoostCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FocusAccCache;


	


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void CameraMove(EYogCharacterState State);


private:
	TObjectPtr<AYogPlayerControllerBase> PlayerController;
	TObjectPtr<AYogCharacterBase> PlayerCharacter;
};
