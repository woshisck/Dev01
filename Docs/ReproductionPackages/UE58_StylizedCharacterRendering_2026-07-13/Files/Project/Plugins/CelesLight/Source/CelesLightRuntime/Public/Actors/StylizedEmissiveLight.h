#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedEmissiveLight.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(BlueprintType)
class CELESLIGHTRUNTIME_API AStylizedEmissiveLight : public AActor
{
	GENERATED_BODY()

public:
	AStylizedEmissiveLight();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stylized Emissive Light")
	void RefreshLight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive Light")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive Light")
	TObjectPtr<UPointLightComponent> LightComponent = nullptr;

	/** Hidden during play, but retained in the Lumen scene as an emissive surface. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive Light")
	TObjectPtr<UStaticMeshComponent> EmissiveSource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	TObjectPtr<UStaticMesh> SourceMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	TObjectPtr<UMaterialInterface> EmissiveMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "0.0"))
	float Intensity = 5000.0f;

	/** HDR multiplier used by the hidden Lumen emissive surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "0.0", UIMax = "100.0"))
	float EmissiveIntensity = 20.0f;

	/** Optional analytic proxy. Disabled by default so the source is true emissive-only lighting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	bool bUsePointLightProxy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "1.0"))
	float AttenuationRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "0.0"))
	float SourceRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	bool bUseInverseSquaredFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	bool bCastShadows = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EmissiveMaterialInstance = nullptr;
};
