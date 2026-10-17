#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogLookDevLightingRig.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;

UENUM(BlueprintType)
enum class EYogLookDevLightingScenario : uint8
{
	LowContrast UMETA(DisplayName = "LowContrast"),
	MidContrast UMETA(DisplayName = "MidContrast"),
	HighContrast UMETA(DisplayName = "HighContrast")
};

USTRUCT(BlueprintType)
struct YOGLOOKDEVEDITOR_API FYogLookDevDirectionalSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Intensity", ClampMin = "0", UIMin = "0", UIMax = "10", Units = "lx"))
	float Intensity = 2.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Light Color"))
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Indirect Lighting Intensity", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float IndirectLightingIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Volumetric Scattering Intensity", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float VolumetricScatteringIntensity = 1.f;

	// Calibrated with the original Blueprint's three directional components. This
	// is persisted, but is not the removed automatic "Turn Lighting" control.
	UPROPERTY()
	FRotator Rotation = FRotator(-45.999348, -49.999786, 0.000088);
};

USTRUCT(BlueprintType)
struct YOGLOOKDEVEDITOR_API FYogLookDevSkySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Intensity", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float Intensity = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Light Color"))
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Indirect Lighting Intensity", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float IndirectLightingIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light", meta = (DisplayName = "Volumetric Scattering Intensity", ClampMin = "0", UIMin = "0", UIMax = "4"))
	float VolumetricScatteringIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field Ambient Occlusion", meta = (DisplayName = "Occlusion Max Distance", ClampMin = "0", UIMin = "200", UIMax = "1500", Units = "cm"))
	float OcclusionMaxDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field Ambient Occlusion", meta = (DisplayName = "Occlusion Exponent", ClampMin = "0", UIMin = "0.6", UIMax = "1.6"))
	float OcclusionExponent = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field Ambient Occlusion", meta = (DisplayName = "Min Occlusion", ClampMin = "0", ClampMax = "1"))
	float MinOcclusion = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distance Field Ambient Occlusion", meta = (DisplayName = "Occlusion Tint"))
	FColor OcclusionTint = FColor::Black;
};

/**
 * Lighting-only behavior for the migrated AssetsPreview_BP. Its authored SCS
 * retains the meshes, material assignments, calibration layout and HDRI.
 */
UCLASS(Blueprintable, hidecategories = (Input, Replication, Networking, Physics, Collision, HLOD, Cooking))
class YOGLOOKDEVEDITOR_API AYogLookDevLightingRig : public AActor
{
	GENERATED_BODY()

public:
	AYogLookDevLightingRig();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookDev", meta = (DisplayName = "Lighting Scenarios"))
	EYogLookDevLightingScenario LightingScenario = EYogLookDevLightingScenario::MidContrast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookDev", meta = (DisplayName = "Background Value Compensation", UIMin = "-5", UIMax = "5", ClampMin = "-20", ClampMax = "20"))
	float BackgroundCompensation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookDev", meta = (DisplayName = "Exposure Compensation", UIMin = "-5", UIMax = "5"))
	float ExposureCompensation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookDev", meta = (DisplayName = "Grounded"))
	bool bGrounded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LookDev", meta = (DisplayName = "Calibrator"))
	bool bCalibrator = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Directional Light", meta = (DisplayName = "Directional Light", ShowOnlyInnerProperties))
	FYogLookDevDirectionalSettings DirectionalSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Light", meta = (DisplayName = "Sky Light", ShowOnlyInnerProperties))
	FYogLookDevSkySettings SkySettings;

	/** Choose the original Low/Mid/High calibration; other light settings persist. */
	UFUNCTION(BlueprintCallable, Category = "LookDev")
	void SetLightingScenario(EYogLookDevLightingScenario NewScenario);

	/** Reapply the saved settings without resetting scene transforms or render globals. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LookDev")
	void RefreshLighting();

	UFUNCTION(BlueprintPure, Category = "LookDev")
	UDirectionalLightComponent* GetActiveDirectionalLight() const;

	UFUNCTION(BlueprintPure, Category = "LookDev")
	USkyLightComponent* GetSkyLight() const;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostRegisterAllComponents() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
#endif

private:
	void ResetScenarioCalibration();
	void ApplyLighting();
};
