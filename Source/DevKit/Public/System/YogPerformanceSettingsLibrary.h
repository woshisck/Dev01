#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGame/YogSettingsSave.h"
#include "YogPerformanceSettingsLibrary.generated.h"

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
};
