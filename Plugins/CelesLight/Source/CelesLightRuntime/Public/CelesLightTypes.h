#pragma once

#include "CoreMinimal.h"
#include "CelesLightTypes.generated.h"

namespace CelesLight
{
	static constexpr int32 LightInfoTextureWidth = 4;
	static constexpr int32 DefaultLightInfoCount = 4;
	static constexpr int32 MaxLightInfoCount = 16;

	static const FName ReceiveActorTag(TEXT("ReceiveCelesLight"));
	static const FName ReceiveMeshTag(TEXT("ReceiveCelesLightMesh"));
	static const FName LightInfoTextureParameterName(TEXT("Tex_LightInfo"));
	static const FName LightInfoCountParameterName(TEXT("LightInfoCount"));
}

UENUM(BlueprintType)
enum class ECelesLightSelectionMode : uint8
{
	NearestToBoxCenter UMETA(DisplayName = "Nearest To Box Center"),
	HighestIntensity UMETA(DisplayName = "Highest Intensity")
};

USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FCelesLightSourceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	FVector WorldPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0"))
	float Radius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0"))
	float Intensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	bool bFillLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothStepMax = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	float SpecularOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light")
	int32 EffectType = 0;
};
