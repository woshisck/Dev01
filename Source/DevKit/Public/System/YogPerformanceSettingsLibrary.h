#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGame/YogSettingsSave.h"
#include "YogPerformanceSettingsLibrary.generated.h"

USTRUCT(BlueprintType)
struct DEVKIT_API FYogMaterialPerformanceTierInterface
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	EYogPerformanceTargetTier TargetTier = EYogPerformanceTargetTier::Epic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaterialQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 NativeMaterialQualityLevel = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 TextureQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 VTAtlasQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 DynamicOverlayQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaterialLightQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "4"))
	int32 MaterialLightMaxLightInfoCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxUniqueTextureSets = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxRuntimeBlendLayers = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material", meta = (ClampMin = "0", ClampMax = "2"))
	int32 MaxRuntimeOverlayLayers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bPreferSourceMasterMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bPreferBakedMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bUseVTAtlas = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bAllowGroundRuntimeRVTOverlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bAllowWallRuntimeRVTOverlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	bool bBakeStaticDecalsIntoSurface = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	FName SourceMasterMaterialContract = TEXT("M_Env_MasterA_Source");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Performance|Material")
	FName BakedMaterialContract = TEXT("M_Env_Baked_VTAtlas");
};

UCLASS()
class DEVKIT_API UYogPerformanceSettingsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FYogGraphicsSettings MakeGraphicsSettingsForProfile(EYogPerformanceProfile Profile);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FYogGraphicsSettings MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier Tier);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FYogGraphicsSettings MakeCustomGraphicsSettings(
		const FYogGraphicsSettings& BaseSettings,
		float ResolutionScalePercent,
		int32 FrameRateLimit,
		bool bUseLumenLite,
		bool bPreferBatchedGeometryProxies);

	UFUNCTION(BlueprintCallable, Category = "Settings|Performance", meta = (WorldContext = "WorldContextObject"))
	static bool ApplyPerformanceProfile(UObject* WorldContextObject, EYogPerformanceProfile Profile, bool bSaveToDisk = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Performance", meta = (WorldContext = "WorldContextObject"))
	static bool ApplyGraphicsSettings(UObject* WorldContextObject, const FYogGraphicsSettings& Settings, bool bSaveToDisk = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Performance", meta = (WorldContext = "WorldContextObject"))
	static bool ApplySavedGraphicsSettings(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance", meta = (WorldContext = "WorldContextObject"))
	static FYogGraphicsSettings GetSavedGraphicsSettings(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static TArray<EYogPerformanceProfile> GetSelectablePerformanceProfiles();

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static TArray<EYogPerformanceTargetTier> GetSelectablePerformanceTargetTiers();

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FText GetPerformanceProfileDisplayName(EYogPerformanceProfile Profile);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FText GetPerformanceProfileDescription(EYogPerformanceProfile Profile);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FText GetPerformanceTargetTierDisplayName(EYogPerformanceTargetTier Tier);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance")
	static FText GetPerformanceTargetTierDescription(EYogPerformanceTargetTier Tier);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance|Material")
	static int32 GetNativeMaterialQualityLevelForProjectMaterialQuality(int32 ProjectMaterialQuality);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance|Material")
	static FYogMaterialPerformanceTierInterface GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier Tier);

	UFUNCTION(BlueprintPure, Category = "Settings|Performance|Material")
	static FYogMaterialPerformanceTierInterface GetMaterialPerformanceInterfaceForGraphicsSettings(const FYogGraphicsSettings& Settings);
};
