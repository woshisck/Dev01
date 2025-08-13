// Copyright 2021, Project Zero, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TopDownPhysicsCameraActor.generated.h"

UCLASS()
class TOPDOWNPHYSICSCAMERA_API ATopDownPhysicsCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
	//Set default values
	ATopDownPhysicsCameraActor();
	UPROPERTY(BlueprintReadWrite, Category = Camera)
		class UStaticMeshComponent* RootSceneComponent;

	UPROPERTY()
		class USpringArmComponent* ScreenArm;

	//Arms
	UPROPERTY()
		class USpringArmComponent* RightArm;
	UPROPERTY()
		class USpringArmComponent* LeftArm;
	UPROPERTY()
		class USpringArmComponent* UpArm;
	UPROPERTY()
		class USpringArmComponent* DownArm;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		class UCameraComponent* CameraComponent;

	UPROPERTY(BlueprintReadWrite, Category = Camera)
		TArray<USpringArmComponent*> AllArms = { RightArm, LeftArm, UpArm, DownArm };
	UPROPERTY(BlueprintReadWrite, Category = Camera)
		TArray<USpringArmComponent*> HorizontalArms = { RightArm, LeftArm};
	UPROPERTY(BlueprintReadWrite, Category = Camera)
		TArray<USpringArmComponent*> VerticalArms = { UpArm, DownArm };

	//Arrows for debug purposes
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		class UArrowComponent* RightArrow;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		class UArrowComponent* LeftArrow;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		class UArrowComponent* UpArrow;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		class UArrowComponent* DownArrow;

	/** If true, will show arrows indicating where the collision probes are headed. */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		bool bShowDebug = false;

	/** False: Set each probes’ distances individually. True: set them all equally in one go. */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		bool bUseUniformCollisionDistance = true;

	/** Distance from character, smaller value means the camera is closer to the character. */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera)
		float DistanceFromCharacter = 1200.f;

	/** If you wish to give your character an offset when moving (e.g. camera trailing behind the character), adjust here. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		FVector CameraAdditionalLocalOffset = FVector::ZeroVector;

	/** If you wish to change the rotation, the local rotator will allow the camera angle to change. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		FRotator CameraRotator = { -60,0,0 }; //Y, Z, X because sure, why not UE4...



	/** Probe distance in all directions. */
	UPROPERTY(EditDefaultsOnly, Category = Camera, meta = (EditCondition =
		"bUseUniformCollisionDistance"))
		float UniformCollisionDistance = 600.f;

	/** Probe distance in right direction. */
	UPROPERTY(EditDefaultsOnly, Category = Camera, meta = (EditCondition =
		"!bUseUniformCollisionDistance"))
		float RightCollisionDistance = 600.f;

	/** Probe distance in left direction. */
	UPROPERTY(EditDefaultsOnly, Category = Camera, meta = (EditCondition =
		"!bUseUniformCollisionDistance"))
		float LeftCollisionDistance = 600.f;

	/** Probe distance in up direction. */
	UPROPERTY(EditDefaultsOnly, Category = Camera, meta = (EditCondition =
		"!bUseUniformCollisionDistance"))
		float UpCollisionDistance = 600.f;

	/** Probe distance in down direction. */
	UPROPERTY(EditDefaultsOnly, Category = Camera, meta = (EditCondition =
		"!bUseUniformCollisionDistance"))
		float DownCollisionDistance = 600.f;

	/** Whether collision is enabled. Set to false if you want to turn off camera bounds */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		bool bDoCollisionTest = true;

	/** The radius of the collision probe at the end of an arrow. The bigger the value, the farther it probes. Set between 5 to 12 for optimal results. */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera, meta = (EditCondition = "bDoCollisionTest"))
		float CameraCollisionProbeSize = 5.f;

	/**  This is the channel the probes will use, make sure your camera blocking volumes are set up accordingly. Whatever the camera shouldn't collide with should overlap or ignore this channel. */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = Camera, meta = (EditCondition = "bDoCollisionTest"))
		TEnumAsByte<ECollisionChannel> CameraCollisionProbeChannel = ECollisionChannel::ECC_Camera;

	/** Enable if you want to give the camera lag or “inertia” when moving. */
	UPROPERTY(BlueprintReadWrite, Category = Camera, EditDefaultsOnly)
		bool bEnablePlayerCameraLag = true;

	/** Determines how quickly the camera will follow the player when moving around. Higher value -> “snappier” follow. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Camera)
		float PlayerCameraLagSpeed = 0.f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& e) override;
#endif

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void ToggleDirectionalArmCollision(bool DoCollisionTest);

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void SetDirectionalArmCameraLag(bool NewEnableCameraLag, float CameraLagSpeed);

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void SetVerticalArmCollision(bool bEnableCollisionTest);

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void SetHorizontalArmCollision(bool bEnableCollisionTest);
	
	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void SetDirectionalArmCameraLagVertical(bool NewEnableCameraLag, float CameraLagSpeed);

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void SetDirectionalArmCameraLagHorizontal(bool NewEnableCameraLag, float CameraLagSpeed);

	UFUNCTION(BlueprintCallable, Category = Camera)
		virtual void TakeOverView();
	

private:
	virtual void ScreenArmSetup();
	virtual void DirectionalArmSetup();
	virtual void ArrowSetup();


};
