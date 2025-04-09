// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "Camera/CameraComponent.h"


#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"

#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

#include "Kismet/GameplayStatics.h"
#include "ModularPawn.h"
#include "../Character/YogCharacterBase.h"


#include "YogCameraPawn.generated.h"



//D:\Epic Library\UE_5.4\Engine\Source\Runtime\Engine\Classes\GameFramework\FloatingPawnMovement.h
//D:\Epic Library\UE_5.4\Engine\Source\Runtime\Engine\Classes\GameFramework\SpringArmComponent.h

const FVector RightArmRelRotation = FVector(0, 0, 90);
const FVector LeftArmRelRotation = FVector(0, 0, 270);
const FVector UpArmRelRotation = FVector(0, 0, 180);
const FVector DownArmRelRotation = FVector(0, 0, 0);

class AYogPlayerControllerBase;
class AYogCharacterBase;
class UFloatingPawnMovement;


class AAIController;


UENUM(BlueprintType)
enum class EYogCameraStates : uint8
{
	FocusCharacter		UMETA(DisplayName = "FocusCharacter"),
	FollowMove			UMETA(DisplayName = "FollowMove"),
	Idle				UMETA(DisplayName = "Idle")
};



USTRUCT(BlueprintType)
struct FCameraMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCameraMovementData()
		: FocusSpeed(0.5f), FollowSpeed(0.5f), DistFromCharacter(100.f)
	{
	}

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float MaxSpeed;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Acceleration;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float Deceleration;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float TurningBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FocusSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FollowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistFromCharacter;

};


UCLASS()
class DEVKIT_API AYogCameraPawn : public AModularPawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AYogCameraPawn(const FObjectInitializer& ObjectInitializer);

	virtual void PostActorCreated();

	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	class UStaticMeshComponent* RootSceneComponent;


	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> FloatingMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AYogCharacterBase> FocusCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AAIController> CameraController;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TObjectPtr<UDataTable> CameraMovementDataTable;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EYogCameraStates CameraStatus;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float MaxSpeedCache;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float AccelerationCache;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float DecelerationCache;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//float TurningBoostCache;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float cache_followSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float cache_focusSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float cache_distFromCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector cache_playerMovementInput;


	//VOID UCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
	UFUNCTION(BlueprintCallable)
	void SetCameraStates(EYogCameraStates NewMovementMode);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PlayerTargetLoc;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UFUNCTION(BlueprintCallable)
	void OnCameraStatesChanged(EYogCameraStates PreviousMovementMode, EYogCameraStates NextMovementMode);


private:
	TObjectPtr<AYogPlayerControllerBase> PlayerController;
};
