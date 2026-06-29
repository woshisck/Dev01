#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YogSettingsSave.generated.h"

UENUM(BlueprintType)
enum class EYogPerformanceProfile : uint8
{
	Low    UMETA(DisplayName = "低"),
	Medium UMETA(DisplayName = "中"),
	High   UMETA(DisplayName = "高"),
	Ultra  UMETA(DisplayName = "超高"),
	Custom UMETA(DisplayName = "自定义")
};

UENUM(BlueprintType)
enum class EYogPerformanceTargetTier : uint8
{
	PCUltra UMETA(DisplayName = "PC Ultra"),
	SteamDeck15W UMETA(DisplayName = "Steam Deck 15W"),
	Switch2Candidate UMETA(DisplayName = "Switch 2 Candidate"),
	SteamDeck5W UMETA(DisplayName = "Steam Deck 5W"),
	FallbackLow UMETA(DisplayName = "Fallback Low"),
	Custom UMETA(DisplayName = "Custom")
};

USTRUCT(BlueprintType)
struct FYogGraphicsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	EYogPerformanceProfile PerformanceProfile = EYogPerformanceProfile::Ultra;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	EYogPerformanceTargetTier SelectedTargetTier = EYogPerformanceTargetTier::PCUltra;

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

// ============================================================
//  全局设置存档（跨槽位，独立文件 "Settings" 槽位）
//  不绑定任何槽位，所有槽位共享同一份设置。
// ============================================================
UCLASS(BlueprintType)
class DEVKIT_API UYogSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:

	// ── 音频 ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MusicVolume = 0.8f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float SFXVolume = 1.f;

	// Runtime graphics and performance profile.
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Graphics")
	FYogGraphicsSettings GraphicsSettings;

	// ── 上次选择的槽位（启动时自动高亮）────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Slot")
	int32 LastActiveSlot = 0;
};
