#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalLinkedDoor.generated.h"

class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class DEVKIT_API APortalLinkedDoor : public AActor
{
	GENERATED_BODY()

public:
	APortalLinkedDoor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Portal|Door")
	void OpenDoor();

	UFUNCTION(BlueprintCallable, Category = "Portal|Door")
	void CloseDoor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Door")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Door")
	TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal|Door")
	TObjectPtr<UStaticMeshComponent> RightDoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	FRotator LeftClosedRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	FRotator RightClosedRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	FRotator LeftOpenRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	FRotator RightOpenRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door", meta = (ClampMin = "0.0"))
	float OpenDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door", meta = (ClampMin = "0.01"))
	float OpenDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal|Door")
	bool bCloseOnBeginPlay = true;

private:
	void StartOpenAnimation();
	void ApplyClosedPose();
	void ApplyOpenPose(float Alpha);

	FTimerHandle OpenDelayTimer;
	float OpenElapsed = 0.0f;
	bool bOpening = false;
	bool bDoorOpen = false;
};
