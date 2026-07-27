#pragma once

#include "CoreMinimal.h"
#include "StylizedEmissiveTypes.generated.h"

UENUM(BlueprintType)
enum class EStylizedEmissiveVisualSource : uint8
{
	DataOnly UMETA(DisplayName = "Data Only (No Mesh)"),
	ModelLibrary UMETA(DisplayName = "Model Library"),
	CustomMesh UMETA(DisplayName = "Custom Mesh")
};

UENUM(BlueprintType)
enum class EStylizedEmissiveLightingOutput : uint8
{
	StylizedMaterial UMETA(DisplayName = "Stylized Material Lighting"),
	LumenGI UMETA(DisplayName = "Native Lumen GI"),
	Hybrid UMETA(DisplayName = "Hybrid (Material + Lumen)")
};

/**
 * Artist-facing values written to the emissive material.
 *
 * Batched meshes store RGB in PerInstanceCustomData[0..2] and the visible HDR
 * multiplier in PerInstanceCustomData[3]. The same values are used as regular
 * material parameters when an actor is rendered without batching.
 */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedEmissivePerInstanceData
{
	GENERATED_BODY()

	static constexpr int32 ColorDataIndex = 0;
	static constexpr int32 IntensityDataIndex = 3;
	static constexpr int32 NumCustomDataFloats = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive")
	FLinearColor EmissiveColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stylized Emissive", meta = (ClampMin = "0.0", UIMax = "100.0"))
	float EmissiveIntensity = 20.0f;
};
