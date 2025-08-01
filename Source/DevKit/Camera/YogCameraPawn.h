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


UENUM()
enum class ECameraRelatedPosition : uint8
{
	OnTarget_Top			UMETA(DisplayName = "Top"),
	OnTarget_Down			UMETA(DisplayName = "Down"),
	OnTarget_Right			UMETA(DisplayName = "Right"),
	OnTarget_Left			UMETA(DisplayName = "Left")
};


UENUM(BlueprintType)
enum class EYogCameraStates : uint8
{
	FocusCharacter		UMETA(DisplayName = "FocusCharacter"),
	FollowMove			UMETA(DisplayName = "FollowMove"),
	BlockVolume			UMETA(DisplayName = "BlockVolume"),
	Idle				UMETA(DisplayName = "Idle")

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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> FloatingMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AYogCharacterBase> FocusCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AAIController> CameraController;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TObjectPtr<UDataTable> CameraMovementDataTable;




	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EYogCameraStates CameraStatus;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	EYogCameraStates PrevStatus;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ECameraRelatedPosition CurrentRaltedPosition;


	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector VolumeOverlapLoc;

	UFUNCTION()
	void SetVolumeOverlapLoc(FVector loc);


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


	UFUNCTION()
	void InitDataTable(UDataTable* camera_DT);


	UFUNCTION()
	void UpdateCameraLoc(float DeltaTime);

private:
	TObjectPtr<AYogPlayerControllerBase> PlayerController;
};
