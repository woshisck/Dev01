#pragma once

#include "CoreMinimal.h"
#include "CelesLightSourceInterface.h"
#include "GameFramework/Actor.h"
#include "StylizedEmissiveTypes.h"
#include "StylizedEmissiveLight.generated.h"

class UBillboardComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStylizedEmissiveModelLibrary;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A stylized emissive source that can be completely data-only or use a visible
 * model selected from a reusable library. No analytic LightComponent is used.
 */
UCLASS(BlueprintType, DisplayName = "Stylized Emissive Source")
class CELESLIGHTRUNTIME_API AStylizedEmissiveLight : public AActor, public ICelesLightSourceInterface
{
	GENERATED_BODY()

public:
	AStylizedEmissiveLight();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual UPointLightComponent* GetLight_Implementation() const override;
	virtual AActor* GetActor_Implementation() const override;
	virtual void GetCelesLightData_Implementation(FCelesLightSourceData& OutData) const override;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stylized Emissive")
	void RefreshEmissiveSource();

	/** Applies one complete artist preset from the hidden model-library backend. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stylized Emissive")
	bool ApplyLibraryPreset(UStylizedEmissiveModelLibrary* InLibrary, FName InModelId);

	/** List of model identifiers from the selected library. */
	UFUNCTION()
	TArray<FString> GetLibraryModelOptions() const;

	/** Material values used both by the individual mesh and by an automatic HISM batch. */
	UFUNCTION(BlueprintPure, Category = "Stylized Emissive|Performance")
	FStylizedEmissivePerInstanceData GetPerInstanceMaterialData() const;

	/**
	 * Returns render data only when this actor can be represented safely by the
	 * automatic per-level batcher. Intended for the runtime subsystem.
	 */
	bool GetAutomaticBatchRenderData(
		UStaticMesh*& OutMesh,
		UMaterialInterface*& OutMaterial,
		FTransform& OutWorldTransform,
		bool& bOutUseLumenGI) const;

	/** Hides only the individual source mesh; light data remains available. */
	void SetRuntimeBatched(bool bInRuntimeBatched);
	bool IsRuntimeBatched() const { return bRuntimeBatched; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive|Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive|Components")
	TObjectPtr<UStaticMeshComponent> EmissiveSource = nullptr;

#if WITH_EDITORONLY_DATA
	/** Selection icon used when the source has no model. It is never cooked. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stylized Emissive|Components")
	TObjectPtr<UBillboardComponent> EditorSprite = nullptr;
#endif

	/** Choose no runtime mesh, a reusable library model, or a per-instance mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Visual")
	EStylizedEmissiveVisualSource VisualSource = EStylizedEmissiveVisualSource::DataOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Visual", meta = (EditCondition = "VisualSource == EStylizedEmissiveVisualSource::ModelLibrary", EditConditionHides))
	TObjectPtr<UStylizedEmissiveModelLibrary> ModelLibrary = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Visual", meta = (GetOptions = "GetLibraryModelOptions", EditCondition = "VisualSource == EStylizedEmissiveVisualSource::ModelLibrary", EditConditionHides))
	FName LibraryModel = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Visual", meta = (DisplayName = "Custom Mesh", EditCondition = "VisualSource == EStylizedEmissiveVisualSource::CustomMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> CustomMesh = nullptr;

	/** Compatibility data for actors saved by the previous implementation. */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use CustomMesh or a model-library entry."))
	TObjectPtr<UStaticMesh> SourceMesh = nullptr;

	/** Used by custom meshes and by library entries that do not provide a material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Visual", meta = (DisplayName = "Fallback Emissive Material", EditCondition = "VisualSource != EStylizedEmissiveVisualSource::DataOnly", EditConditionHides))
	TObjectPtr<UMaterialInterface> EmissiveMaterial = nullptr;

	/** Data-only is always material lighting; Native Lumen requires a visible mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Lighting")
	EStylizedEmissiveLightingOutput LightingOutput = EStylizedEmissiveLightingOutput::StylizedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Lighting")
	FLinearColor LightColor = FLinearColor::White;

	/** Intensity written to the Celes material-light data texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Lighting", meta = (ClampMin = "0.0", UIMax = "10000.0"))
	float Intensity = 5000.0f;

	/** HDR multiplier written to the visible model's emissive material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Lighting", meta = (ClampMin = "0.0", UIMax = "100.0", EditCondition = "VisualSource != EStylizedEmissiveVisualSource::DataOnly", EditConditionHides))
	float EmissiveIntensity = 20.0f;

	/**
	 * In Game and PIE, compatible sources in the same level are rendered by
	 * HISM batches. Disable this for sources that must move or animate at runtime.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Performance", meta = (DisplayName = "Allow Automatic Level Batching"))
	bool bAllowAutomaticBatching = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive|Lighting", meta = (ClampMin = "1.0", UIMax = "5000.0"))
	float AttenuationRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Stylized Emissive|Lighting")
	bool bFillLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Stylized Emissive|Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Stylized Emissive|Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Stylized Emissive|Lighting")
	float SpecularOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Stylized Emissive|Lighting")
	int32 EffectType = 0;

private:
	bool UsesStylizedMaterialLighting() const;
	bool UsesLumenGI() const;
	void ResolveVisualModel(UStaticMesh*& OutMesh, UMaterialInterface*& OutMaterial, FTransform& OutRelativeTransform) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EmissiveMaterialInstance = nullptr;

	UPROPERTY(Transient)
	bool bRuntimeBatched = false;
};
