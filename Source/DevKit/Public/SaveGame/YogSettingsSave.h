#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YogSettingsSave.generated.h"

UENUM(BlueprintType)
enum class EYogPerformanceProfile : uint8
{
	Low    = 0 UMETA(DisplayName = "Low"),
	Mid    = 1 UMETA(DisplayName = "Mid"),
	High   = 2 UMETA(DisplayName = "High"),
	Epic   = 3 UMETA(DisplayName = "Epic"),
	Custom = 255 UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EYogPerformanceTargetTier : uint8
{
	Low    = 0 UMETA(DisplayName = "Low"),
	Mid    = 1 UMETA(DisplayName = "Mid"),
	High   = 2 UMETA(DisplayName = "High"),
	Epic   = 3 UMETA(DisplayName = "Epic"),
	Custom = 255 UMETA(DisplayName = "Custom")
};

USTRUCT(BlueprintType)
struct FYogGraphicsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	EYogPerformanceProfile PerformanceProfile = EYogPerformanceProfile::Epic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	EYogPerformanceTargetTier SelectedTargetTier = EYogPerformanceTargetTier::Epic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "25.0", ClampMax = "100.0"))
	float ResolutionScalePercent = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "240"))
	int32 FrameRateLimit = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ViewDistanceQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ShadowQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 GlobalIlluminationQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ReflectionQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 PostProcessQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 TextureQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 EffectsQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 FoliageQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ShadingQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaterialQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 DynamicOverlayQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 VTAtlasQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 DynamicLightQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaterialLightQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MaterialLightMaxLightInfoCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bUseLumenLite = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bUseNanite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bUseVirtualShadowMaps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bPreferBatchedGeometryProxies = false;
};

UCLASS(BlueprintType)
class DEVKIT_API UYogSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MusicVolume = 0.8f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float SFXVolume = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Graphics")
	FYogGraphicsSettings GraphicsSettings;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Slot")
	int32 LastActiveSlot = 0;
};
