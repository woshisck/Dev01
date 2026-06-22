#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CelesLightSourceInterface.h"
#include "CelesPointLight.generated.h"

class UCelesLightReceiveComponent;
class UPointLightComponent;
class USphereComponent;

UCLASS()
class CELESLIGHTRUNTIME_API ACelesPointLight : public AActor, public ICelesLightSourceInterface
{
	GENERATED_BODY()

public:
	ACelesPointLight(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void ConstructionUpdate();

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void RegisterOverlappingReceivers(bool bInEditor = false);

	virtual UPointLightComponent* GetLight_Implementation() const override;
	virtual AActor* GetActor_Implementation() const override;
	virtual void GetCelesLightData_Implementation(FCelesLightSourceData& OutData) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light")
	TObjectPtr<UPointLightComponent> PointLightComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light")
	TObjectPtr<USphereComponent> SphereVolume = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	bool bEnableOriginLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	bool bFillLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0"))
	float LightIntensityMultiply = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightSmoothStepMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LightSmoothStepMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	float LightSpecOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	int32 LightEffectType = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	TArray<FName> LightEffectTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	TArray<FName> FilterActorTags;

private:
	UFUNCTION()
	void HandleSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RegisterReceiverActor(AActor* Actor, bool bInEditor);
	void DeregisterReceiverActor(AActor* Actor, bool bInEditor);
	float GetCelesRadius() const;
};
