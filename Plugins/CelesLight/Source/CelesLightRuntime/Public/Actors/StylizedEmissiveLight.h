#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StylizedEmissiveLight.generated.h"

class UPointLightComponent;
class USceneComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "0.0"))
	float Intensity = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "1.0"))
	float AttenuationRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light", meta = (ClampMin = "0.0"))
	float SourceRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	bool bUseInverseSquaredFalloff = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive Light")
	bool bCastShadows = false;
};
