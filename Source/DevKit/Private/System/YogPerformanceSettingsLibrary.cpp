#include "System/YogPerformanceSettingsLibrary.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/YogSaveSubsystem.h"

namespace
{
static TAutoConsoleVariable<int32> CVarYogDynamicLightQuality(
	TEXT("r.Yog.DynamicLightQuality"),
	3,
	TEXT("Project runtime dynamic light budget quality: 0 low, 3 epic."));

static TAutoConsoleVariable<int32> CVarYogMaterialQuality(
	TEXT("r.Yog.MaterialQuality"),
	3,
	TEXT("Project runtime material feature quality: 0 low, 3 epic."));

static TAutoConsoleVariable<int32> CVarYogDynamicOverlayQuality(
	TEXT("r.Yog.DynamicOverlayQuality"),
	3,
	TEXT("Project runtime decal/RVT/overlay quality: 0 low, 3 epic."));

static TAutoConsoleVariable<int32> CVarYogMaterialLightQuality(
	TEXT("r.Yog.MaterialLightQuality"),
	3,
	TEXT("Project runtime material light quality: 0 low, 3 epic."));

static TAutoConsoleVariable<int32> CVarYogMaterialLightMaxLightInfoCount(
	TEXT("r.Yog.MaterialLight.MaxLightInfoCount"),
	4,
	TEXT("Maximum material LightInfo entries exposed to material-light systems."));

static TAutoConsoleVariable<int32> CVarYogBatchProxyPreference(
	TEXT("r.Yog.BatchProxyPreference"),
	0,
	TEXT("Project resource selection preference for generated batch proxies: 0 source-biased, 1 proxy-biased."));

static TAutoConsoleVariable<int32> CVarYogTextureCollectionQuality(
	TEXT("r.Yog.TextureCollectionQuality"),
	3,
	TEXT("Project ordinary TextureCollection batching quality: 0 low, 3 epic."));

static TAutoConsoleVariable<int32> CVarYogVTAtlasQuality_DEPRECATED(
	TEXT("r.Yog.VTAtlasQuality"),
	3,
	TEXT("Deprecated alias. Use r.Yog.TextureCollectionQuality; ordinary scene textures stay NoVT and VTAtlas is legacy only."));

int32 ClampQuality(int32 Value)
{
	return FMath::Clamp(Value, 0, 3);
}

int32 NativeMaterialQualityLevelForProjectQuality(int32 ProjectMaterialQuality)
{
	// UE material quality enum order is Low=0, High=1, Medium=2, Epic=3.
	// Project-facing order remains Low=0, Mid=1, High=2, Epic=3.
	switch (ClampQuality(ProjectMaterialQuality))
	{
	case 3:
		return 3; // Epic
	case 2:
		return 1; // High
	case 1:
		return 2; // Medium
	case 0:
	default:
		return 0; // Low
	}
}

EYogPerformanceTargetTier TargetTierForProfile(EYogPerformanceProfile Profile)
{
	switch (Profile)
	{
	case EYogPerformanceProfile::Epic:
		return EYogPerformanceTargetTier::Epic;
	case EYogPerformanceProfile::High:
		return EYogPerformanceTargetTier::High;
	case EYogPerformanceProfile::Mid:
		return EYogPerformanceTargetTier::Mid;
	case EYogPerformanceProfile::Low:
		return EYogPerformanceTargetTier::Low;
	case EYogPerformanceProfile::Custom:
	default:
		return EYogPerformanceTargetTier::Custom;
	}
}

EYogPerformanceProfile ProfileForTargetTier(EYogPerformanceTargetTier Tier)
{
	switch (Tier)
	{
	case EYogPerformanceTargetTier::Epic:
		return EYogPerformanceProfile::Epic;
	case EYogPerformanceTargetTier::High:
		return EYogPerformanceProfile::High;
	case EYogPerformanceTargetTier::Mid:
		return EYogPerformanceProfile::Mid;
	case EYogPerformanceTargetTier::Low:
		return EYogPerformanceProfile::Low;
	case EYogPerformanceTargetTier::Custom:
	default:
		return EYogPerformanceProfile::Custom;
	}
}

void SetCVarInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByConsole);
	}
}

void SetCVarFloat(const TCHAR* Name, float Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByConsole);
	}
}

UGameInstance* GetGameInstanceFromContext(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (UGameInstance* GameInstance = Cast<UGameInstance>(WorldContextObject))
	{
		return GameInstance;
	}

	if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
	{
		return World->GetGameInstance();
	}

	return nullptr;
}

int32 MaxTextureSetsForMaterialQuality(int32 MaterialQuality)
{
	switch (ClampQuality(MaterialQuality))
	{
	case 3:
	case 2:
		return 3;
	case 1:
		return 2;
	case 0:
	default:
		return 1;
	}
}

int32 MaxBlendLayersForMaterialQuality(int32 MaterialQuality)
{
	switch (ClampQuality(MaterialQuality))
	{
	case 3:
	case 2:
		return 3;
	case 1:
		return 2;
	case 0:
	default:
		return 1;
	}
}

int32 MaxOverlayLayersForQuality(int32 DynamicOverlayQuality)
{
	switch (ClampQuality(DynamicOverlayQuality))
	{
	case 3:
		return 2;
	case 2:
	case 1:
		return 1;
	case 0:
	default:
		return 0;
	}
}

UYogSettingsSave* GetSettingsSave(UObject* WorldContextObject)
{
	UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
	UYogSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UYogSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return nullptr;
	}

	if (!SaveSubsystem->GetSettings())
	{
		SaveSubsystem->LoadSettings();
	}

	return SaveSubsystem->GetSettings();
}

void SaveGraphicsSettings(UObject* WorldContextObject, const FYogGraphicsSettings& Settings)
{
	UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
	UYogSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UYogSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	if (!SaveSubsystem->GetSettings())
	{
		SaveSubsystem->LoadSettings();
	}

	if (UYogSettingsSave* SettingsSave = SaveSubsystem->GetSettings())
	{
		SettingsSave->GraphicsSettings = Settings;
		SaveSubsystem->SaveSettings();
	}
}
}

FYogGraphicsSettings UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForProfile(EYogPerformanceProfile Profile)
{
	FYogGraphicsSettings Settings;
	Settings.PerformanceProfile = Profile;
	Settings.SelectedTargetTier = TargetTierForProfile(Profile);
	Settings.bUseNanite = false;
	Settings.bUseVirtualShadowMaps = false;

	switch (Profile)
	{
	case EYogPerformanceProfile::Epic:
		Settings.ResolutionScalePercent = 100.f;
		Settings.FrameRateLimit = 0;
		Settings.ViewDistanceQuality = 3;
		Settings.ShadowQuality = 3;
		Settings.GlobalIlluminationQuality = 3;
		Settings.ReflectionQuality = 3;
		Settings.PostProcessQuality = 3;
		Settings.TextureQuality = 3;
		Settings.EffectsQuality = 3;
		Settings.FoliageQuality = 3;
		Settings.ShadingQuality = 3;
		Settings.MaterialQuality = 3;
		Settings.DynamicOverlayQuality = 3;
		Settings.TextureCollectionQuality = 3;
		Settings.VTAtlasQuality = Settings.TextureCollectionQuality;
		Settings.DynamicLightQuality = 3;
		Settings.MaterialLightQuality = 3;
		Settings.MaterialLightMaxLightInfoCount = 4;
		Settings.bUseLumenLite = true;
		Settings.bPreferBatchedGeometryProxies = false;
		break;

	case EYogPerformanceProfile::High:
		Settings.ResolutionScalePercent = 100.f;
		Settings.FrameRateLimit = 0;
		Settings.ViewDistanceQuality = 2;
		Settings.ShadowQuality = 2;
		Settings.GlobalIlluminationQuality = 2;
		Settings.ReflectionQuality = 2;
		Settings.PostProcessQuality = 2;
		Settings.TextureQuality = 2;
		Settings.EffectsQuality = 2;
		Settings.FoliageQuality = 2;
		Settings.ShadingQuality = 2;
		Settings.MaterialQuality = 2;
		Settings.DynamicOverlayQuality = 2;
		Settings.TextureCollectionQuality = 2;
		Settings.VTAtlasQuality = Settings.TextureCollectionQuality;
		Settings.DynamicLightQuality = 2;
		Settings.MaterialLightQuality = 2;
		Settings.MaterialLightMaxLightInfoCount = 2;
		Settings.bUseLumenLite = true;
		Settings.bPreferBatchedGeometryProxies = false;
		break;

	case EYogPerformanceProfile::Mid:
		Settings.ResolutionScalePercent = 70.f;
		Settings.FrameRateLimit = 0;
		Settings.ViewDistanceQuality = 1;
		Settings.ShadowQuality = 1;
		Settings.GlobalIlluminationQuality = 1;
		Settings.ReflectionQuality = 1;
		Settings.PostProcessQuality = 2;
		Settings.TextureQuality = 2;
		Settings.EffectsQuality = 1;
		Settings.FoliageQuality = 1;
		Settings.ShadingQuality = 1;
		Settings.MaterialQuality = 1;
		Settings.DynamicOverlayQuality = 1;
		Settings.TextureCollectionQuality = 1;
		Settings.VTAtlasQuality = Settings.TextureCollectionQuality;
		Settings.DynamicLightQuality = 1;
		Settings.MaterialLightQuality = 1;
		Settings.MaterialLightMaxLightInfoCount = 1;
		Settings.bUseLumenLite = true;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceProfile::Low:
		Settings.ResolutionScalePercent = 55.f;
		Settings.FrameRateLimit = 0;
		Settings.ViewDistanceQuality = 0;
		Settings.ShadowQuality = 0;
		Settings.GlobalIlluminationQuality = 1;
		Settings.ReflectionQuality = 0;
		Settings.PostProcessQuality = 1;
		Settings.TextureQuality = 1;
		Settings.EffectsQuality = 0;
		Settings.FoliageQuality = 0;
		Settings.ShadingQuality = 0;
		Settings.MaterialQuality = 0;
		Settings.DynamicOverlayQuality = 0;
		Settings.TextureCollectionQuality = 0;
		Settings.VTAtlasQuality = Settings.TextureCollectionQuality;
		Settings.DynamicLightQuality = 0;
		Settings.MaterialLightQuality = 1;
		Settings.MaterialLightMaxLightInfoCount = 1;
		Settings.bUseLumenLite = true;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceProfile::Custom:
	default:
		Settings.PerformanceProfile = EYogPerformanceProfile::Custom;
		Settings.SelectedTargetTier = EYogPerformanceTargetTier::Custom;
		break;
	}

	return Settings;
}

FYogGraphicsSettings UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier Tier)
{
	FYogGraphicsSettings Settings = MakeGraphicsSettingsForProfile(ProfileForTargetTier(Tier));
	Settings.SelectedTargetTier = Tier;
	return Settings;
}

FYogGraphicsSettings UYogPerformanceSettingsLibrary::MakeCustomGraphicsSettings(
	const FYogGraphicsSettings& BaseSettings,
	float ResolutionScalePercent,
	int32 FrameRateLimit,
	bool bUseLumenLite,
	bool bPreferBatchedGeometryProxies)
{
	FYogGraphicsSettings Settings = BaseSettings;
	Settings.PerformanceProfile = EYogPerformanceProfile::Custom;
	Settings.SelectedTargetTier = EYogPerformanceTargetTier::Custom;
	Settings.ResolutionScalePercent = FMath::Clamp(ResolutionScalePercent, 25.f, 100.f);
	Settings.FrameRateLimit = FMath::Clamp(FrameRateLimit, 0, 240);
	Settings.bUseLumenLite = bUseLumenLite;
	Settings.bPreferBatchedGeometryProxies = bPreferBatchedGeometryProxies;
	return Settings;
}

bool UYogPerformanceSettingsLibrary::ApplyPerformanceProfile(UObject* WorldContextObject, EYogPerformanceProfile Profile, bool bSaveToDisk)
{
	return ApplyGraphicsSettings(WorldContextObject, MakeGraphicsSettingsForProfile(Profile), bSaveToDisk);
}

bool UYogPerformanceSettingsLibrary::ApplyGraphicsSettings(UObject* WorldContextObject, const FYogGraphicsSettings& Settings, bool bSaveToDisk)
{
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!UserSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("[PerfSettings] ApplyGraphicsSettings: GEngine or GameUserSettings is null — settings NOT applied"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[PerfSettings] ApplyGraphicsSettings BEGIN profile=%d GIQ=%d ShadowQ=%d DynLightQ=%d MatLightQ=%d LightCount=%d"),
		static_cast<int32>(Settings.PerformanceProfile),
		Settings.GlobalIlluminationQuality,
		Settings.ShadowQuality,
		Settings.DynamicLightQuality,
		Settings.MaterialLightQuality,
		Settings.MaterialLightMaxLightInfoCount);

	UserSettings->SetViewDistanceQuality(ClampQuality(Settings.ViewDistanceQuality));
	UserSettings->SetShadowQuality(ClampQuality(Settings.ShadowQuality));
	UserSettings->SetGlobalIlluminationQuality(ClampQuality(Settings.GlobalIlluminationQuality));
	UserSettings->SetReflectionQuality(ClampQuality(Settings.ReflectionQuality));
	UserSettings->SetPostProcessingQuality(ClampQuality(Settings.PostProcessQuality));
	UserSettings->SetTextureQuality(ClampQuality(Settings.TextureQuality));
	UserSettings->SetVisualEffectQuality(ClampQuality(Settings.EffectsQuality));
	UserSettings->SetFoliageQuality(ClampQuality(Settings.FoliageQuality));
	UserSettings->SetShadingQuality(ClampQuality(Settings.ShadingQuality));
	UserSettings->SetResolutionScaleValueEx(FMath::Clamp(Settings.ResolutionScalePercent, 25.f, 100.f));
	UserSettings->SetFrameRateLimit(Settings.FrameRateLimit > 0 ? static_cast<float>(Settings.FrameRateLimit) : 0.f);
	UserSettings->ApplySettings(false);
	UserSettings->SaveSettings();

	const int32 ClampedMaterialQuality = ClampQuality(Settings.MaterialQuality);
	const int32 ClampedMaterialLightQuality = ClampQuality(Settings.MaterialLightQuality);
	const int32 ClampedReflectionQuality = ClampQuality(Settings.ReflectionQuality);
	const bool bUseLumenGI = Settings.bUseLumenLite;
	const bool bUseLumenReflections = bUseLumenGI && ClampedReflectionQuality >= 3;

	SetCVarInt(TEXT("r.DynamicGlobalIlluminationMethod"), bUseLumenGI ? 1 : 0);
	SetCVarInt(TEXT("r.Lumen.DiffuseIndirect.Allow"), bUseLumenGI ? 1 : 0);
	SetCVarInt(TEXT("r.ReflectionMethod"), bUseLumenReflections ? 1 : 2);
	SetCVarInt(TEXT("r.Lumen.Reflections.Allow"), bUseLumenReflections ? 1 : 0);
	SetCVarInt(TEXT("r.VolumetricFog"), 0);
	SetCVarInt(TEXT("r.Nanite"), Settings.bUseNanite ? 1 : 0);
	SetCVarInt(TEXT("r.Shadow.Virtual.Enable"), Settings.bUseVirtualShadowMaps ? 1 : 0);
	SetCVarInt(TEXT("r.Yog.DynamicLightQuality"), ClampQuality(Settings.DynamicLightQuality));
	SetCVarInt(TEXT("r.Yog.MaterialQuality"), ClampedMaterialQuality);
	SetCVarInt(TEXT("r.MaterialQualityLevel"), NativeMaterialQualityLevelForProjectQuality(ClampedMaterialQuality));
	SetCVarInt(TEXT("r.Yog.DynamicOverlayQuality"), ClampQuality(Settings.DynamicOverlayQuality));
	SetCVarInt(TEXT("r.Yog.MaterialLightQuality"), ClampedMaterialLightQuality);
	SetCVarInt(TEXT("r.Yog.MaterialLight.MaxLightInfoCount"),
		FMath::Clamp(Settings.MaterialLightMaxLightInfoCount, 0, 4));
	SetCVarInt(TEXT("r.Yog.BatchProxyPreference"), Settings.bPreferBatchedGeometryProxies ? 1 : 0);
	SetCVarInt(TEXT("r.Yog.TextureCollectionQuality"), ClampQuality(Settings.TextureCollectionQuality));
	SetCVarInt(TEXT("r.Yog.VTAtlasQuality"), ClampQuality(Settings.TextureCollectionQuality));
	SetCVarFloat(TEXT("r.ScreenPercentage"), FMath::Clamp(Settings.ResolutionScalePercent, 25.f, 100.f));
	SetCVarFloat(TEXT("t.MaxFPS"), Settings.FrameRateLimit > 0 ? static_cast<float>(Settings.FrameRateLimit) : 0.f);

	UE_LOG(LogTemp, Log, TEXT("[PerfSettings] ApplyGraphicsSettings END — r.Yog.DynamicLightQuality=%d r.Yog.MaterialLightQuality=%d r.Yog.MaterialLight.MaxLightInfoCount=%d"),
		ClampQuality(Settings.DynamicLightQuality),
		ClampQuality(Settings.MaterialLightQuality),
		FMath::Clamp(Settings.MaterialLightMaxLightInfoCount, 0, 4));

	if (bSaveToDisk)
	{
		SaveGraphicsSettings(WorldContextObject, Settings);
	}

	return true;
}

bool UYogPerformanceSettingsLibrary::ApplySavedGraphicsSettings(UObject* WorldContextObject)
{
	const UYogSettingsSave* SettingsSave = GetSettingsSave(WorldContextObject);
	if (!SettingsSave)
	{
		// No settings file — apply Mid as a safe default for first-time users.
		UE_LOG(LogTemp, Warning, TEXT("[PerfSettings] ApplySavedGraphicsSettings: no settings save; applying Mid default"));
		return ApplyGraphicsSettings(WorldContextObject, MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Mid), false);
	}

	FYogGraphicsSettings Settings = SettingsSave->GraphicsSettings;

	// For preset profiles, always regenerate from current code definitions so stale saved
	// values (from an older build or a corrupted save) never produce wrong quality levels.
	if (Settings.PerformanceProfile != EYogPerformanceProfile::Custom)
	{
		const EYogPerformanceProfile SavedProfile = Settings.PerformanceProfile;
		Settings = MakeGraphicsSettingsForProfile(SavedProfile);
	}

	Settings.FrameRateLimit = 0;

	UE_LOG(LogTemp, Log, TEXT("[PerfSettings] ApplySavedGraphicsSettings: profile=%d DynLightQ=%d MatLightQ=%d GIQ=%d ShadowQ=%d"),
		static_cast<int32>(Settings.PerformanceProfile),
		Settings.DynamicLightQuality,
		Settings.MaterialLightQuality,
		Settings.GlobalIlluminationQuality,
		Settings.ShadowQuality);

	return ApplyGraphicsSettings(WorldContextObject, Settings, false);
}

FYogGraphicsSettings UYogPerformanceSettingsLibrary::GetSavedGraphicsSettings(UObject* WorldContextObject)
{
	if (const UYogSettingsSave* SettingsSave = GetSettingsSave(WorldContextObject))
	{
		FYogGraphicsSettings Settings = SettingsSave->GraphicsSettings;
		Settings.FrameRateLimit = 0;
		return Settings;
	}

	return FYogGraphicsSettings();
}

TArray<EYogPerformanceProfile> UYogPerformanceSettingsLibrary::GetSelectablePerformanceProfiles()
{
	return {
		EYogPerformanceProfile::Epic,
		EYogPerformanceProfile::High,
		EYogPerformanceProfile::Mid,
		EYogPerformanceProfile::Low
	};
}

TArray<EYogPerformanceTargetTier> UYogPerformanceSettingsLibrary::GetSelectablePerformanceTargetTiers()
{
	return {
		EYogPerformanceTargetTier::Epic,
		EYogPerformanceTargetTier::High,
		EYogPerformanceTargetTier::Mid,
		EYogPerformanceTargetTier::Low
	};
}

FText UYogPerformanceSettingsLibrary::GetPerformanceProfileDisplayName(EYogPerformanceProfile Profile)
{
	switch (Profile)
	{
	case EYogPerformanceProfile::Epic:
		return NSLOCTEXT("DevKitPerformance", "ProfileEpic", "Epic");
	case EYogPerformanceProfile::High:
		return NSLOCTEXT("DevKitPerformance", "ProfileHigh", "High");
	case EYogPerformanceProfile::Mid:
		return NSLOCTEXT("DevKitPerformance", "ProfileMid", "Mid");
	case EYogPerformanceProfile::Low:
		return NSLOCTEXT("DevKitPerformance", "ProfileLow", "Low");
	case EYogPerformanceProfile::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "ProfileCustom", "Custom");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceTargetTierDisplayName(EYogPerformanceTargetTier Tier)
{
	switch (Tier)
	{
	case EYogPerformanceTargetTier::Epic:
		return NSLOCTEXT("DevKitPerformance", "TargetEpic", "Epic");
	case EYogPerformanceTargetTier::High:
		return NSLOCTEXT("DevKitPerformance", "TargetHigh", "High");
	case EYogPerformanceTargetTier::Mid:
		return NSLOCTEXT("DevKitPerformance", "TargetMid", "Mid");
	case EYogPerformanceTargetTier::Low:
		return NSLOCTEXT("DevKitPerformance", "TargetLow", "Low");
	case EYogPerformanceTargetTier::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "TargetCustom", "Custom");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceProfileDescription(EYogPerformanceProfile Profile)
{
	switch (Profile)
	{
	case EYogPerformanceProfile::Epic:
		return NSLOCTEXT("DevKitPerformance", "ProfileEpicDesc",
			"Highest PC quality. Source assets are preferred, with full VT atlas and material-light budgets.");
	case EYogPerformanceProfile::High:
		return NSLOCTEXT("DevKitPerformance", "ProfileHighDesc",
			"High PC quality. Source assets remain common, with selective proxies and high VT atlas budgets.");
	case EYogPerformanceProfile::Mid:
		return NSLOCTEXT("DevKitPerformance", "ProfileMidDesc",
			"Default recommended tier. Batch proxies and baked surfaces are preferred where validated.");
	case EYogPerformanceProfile::Low:
		return NSLOCTEXT("DevKitPerformance", "ProfileLowDesc",
			"Low-power tier. Batch proxies, baked surfaces, low material budgets, and minimal dynamic overlay.");
	case EYogPerformanceProfile::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "ProfileCustomDesc",
			"User-controlled custom graphics settings.");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceTargetTierDescription(EYogPerformanceTargetTier Tier)
{
	switch (Tier)
	{
	case EYogPerformanceTargetTier::Epic:
		return NSLOCTEXT("DevKitPerformance", "TargetEpicDesc",
			"Epic target. Highest visual budget and source-biased environment selection.");
	case EYogPerformanceTargetTier::High:
		return NSLOCTEXT("DevKitPerformance", "TargetHighDesc",
			"High target. High visual budget with selective generated proxy use.");
	case EYogPerformanceTargetTier::Mid:
		return NSLOCTEXT("DevKitPerformance", "TargetMidDesc",
			"Mid target. Recommended default with proxy and baked-surface preference.");
	case EYogPerformanceTargetTier::Low:
		return NSLOCTEXT("DevKitPerformance", "TargetLowDesc",
			"Low target. Lowest runtime budget with generated proxy and baked-surface preference.");
	case EYogPerformanceTargetTier::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "TargetCustomDesc",
			"User-controlled custom graphics settings.");
	}
}

int32 UYogPerformanceSettingsLibrary::GetNativeMaterialQualityLevelForProjectMaterialQuality(int32 ProjectMaterialQuality)
{
	return NativeMaterialQualityLevelForProjectQuality(ProjectMaterialQuality);
}

FYogMaterialPerformanceTierInterface UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForTargetTier(EYogPerformanceTargetTier Tier)
{
	return GetMaterialPerformanceInterfaceForGraphicsSettings(MakeGraphicsSettingsForTargetTier(Tier));
}

FYogMaterialPerformanceTierInterface UYogPerformanceSettingsLibrary::GetMaterialPerformanceInterfaceForGraphicsSettings(const FYogGraphicsSettings& Settings)
{
	FYogMaterialPerformanceTierInterface MaterialInterface;
	MaterialInterface.TargetTier = Settings.SelectedTargetTier;
	MaterialInterface.MaterialQuality = ClampQuality(Settings.MaterialQuality);
	MaterialInterface.NativeMaterialQualityLevel = NativeMaterialQualityLevelForProjectQuality(Settings.MaterialQuality);
	MaterialInterface.TextureQuality = ClampQuality(Settings.TextureQuality);
	MaterialInterface.TextureCollectionQuality = ClampQuality(Settings.TextureCollectionQuality);
	MaterialInterface.VTAtlasQuality = MaterialInterface.TextureCollectionQuality;
	MaterialInterface.DynamicOverlayQuality = ClampQuality(Settings.DynamicOverlayQuality);
	MaterialInterface.MaterialLightQuality = ClampQuality(Settings.MaterialLightQuality);
	MaterialInterface.MaterialLightMaxLightInfoCount = FMath::Clamp(Settings.MaterialLightMaxLightInfoCount, 0, 4);
	MaterialInterface.MaxUniqueTextureSets = MaxTextureSetsForMaterialQuality(Settings.MaterialQuality);
	MaterialInterface.MaxRuntimeBlendLayers = MaxBlendLayersForMaterialQuality(Settings.MaterialQuality);
	MaterialInterface.MaxRuntimeOverlayLayers = MaxOverlayLayersForQuality(Settings.DynamicOverlayQuality);
	MaterialInterface.bPreferSourceMasterMaterial = !Settings.bPreferBatchedGeometryProxies && MaterialInterface.MaterialQuality >= 2;
	MaterialInterface.bPreferBakedMaterial = Settings.bPreferBatchedGeometryProxies || MaterialInterface.MaterialQuality <= 1;
	MaterialInterface.bUseTextureCollection = true;
	MaterialInterface.bUseVTAtlas = false;
	MaterialInterface.bAllowGroundRuntimeRVTOverlay = MaterialInterface.DynamicOverlayQuality >= 1;
	MaterialInterface.bAllowWallRuntimeRVTOverlay = false;
	MaterialInterface.bBakeStaticDecalsIntoSurface = true;
	MaterialInterface.SourceMasterMaterialContract = TEXT("M_Env_MasterA_Source");
	MaterialInterface.BakedMaterialContract = TEXT("M_Env_Baked_TextureCollection");
	return MaterialInterface;
}
