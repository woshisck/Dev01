#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StylizedEmissiveTypes.h"
#include "StylizedEmissiveModelLibrary.generated.h"

class UMaterialInterface;
class UStaticMesh;

/** A reusable visible model preset for a stylized emissive source. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedEmissiveModelEntry
{
	GENERATED_BODY()

	/** Stable internal identifier used by placed actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FName ModelId = NAME_None;

	/** Artist-facing name displayed in the tool. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FText DisplayName;

	/** Optional artist-facing explanation of the intended use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset", meta = (MultiLine = true))
	FText Description;

	/** Disabled means this preset creates a data-only source with no runtime mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	bool bUseMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (EditCondition = "bUseMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Optional preset material. The actor material is used when this is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (EditCondition = "bUseMesh", EditConditionHides))
	TObjectPtr<UMaterialInterface> Material = nullptr;

	/** Local transform applied to the selected visual model. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual", meta = (EditCondition = "bUseMesh", EditConditionHides))
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	EStylizedEmissiveLightingOutput LightingOutput = EStylizedEmissiveLightingOutput::StylizedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", UIMax = "10000.0"))
	float Intensity = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "0.0", UIMax = "100.0", EditCondition = "bUseMesh", EditConditionHides))
	float EmissiveIntensity = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMin = "1.0", UIMax = "5000.0"))
	float AttenuationRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Lighting")
	bool bFillLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Lighting")
	float SpecularOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Lighting")
	int32 EffectType = 0;
};

/**
 * Persistence backend for the artist-facing Stylized Emissive Library tool.
 * Artists configure and place presets through YogTool rather than this raw array.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Stylized Emissive Model Library"))
class CELESLIGHTRUNTIME_API UStylizedEmissiveModelLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Models", meta = (TitleProperty = "ModelId"))
	TArray<FStylizedEmissiveModelEntry> Models;

	const FStylizedEmissiveModelEntry* FindModel(FName ModelId) const;
	TArray<FString> GetModelOptions() const;
};
