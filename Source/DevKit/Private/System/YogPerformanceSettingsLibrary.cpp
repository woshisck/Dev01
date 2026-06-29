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
	TEXT("Project runtime dynamic light budget quality: 0 low, 3 high."));

static TAutoConsoleVariable<int32> CVarYogMaterialLightQuality(
	TEXT("r.Yog.MaterialLightQuality"),
	3,
	TEXT("Project runtime material/cel light quality: 0 low, 3 high."));

static TAutoConsoleVariable<int32> CVarYogMaterialLightMaxLightInfoCount(
	TEXT("r.Yog.MaterialLight.MaxLightInfoCount"),
	4,
	TEXT("Maximum material LightInfo entries exposed to material-light systems."));

int32 ClampQuality(int32 Value)
{
	return FMath::Clamp(Value, 0, 3);
}

int32 LightInfoCountForMaterialLightQuality(int32 Quality)
{
	switch (ClampQuality(Quality))
	{
	case 0:
		return 0;
	case 1:
		return 1;
	case 2:
		return 2;
	case 3:
	default:
		return 4;
	}
}

void SetCVarInt(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByGameSetting);
	}
}

void SetCVarFloat(const TCHAR* Name, float Value)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		CVar->Set(Value, ECVF_SetByGameSetting);
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
	Settings.SelectedTargetTier = EYogPerformanceTargetTier::Custom;
	Settings.bUseNanite = false;
	Settings.bUseVirtualShadowMaps = false;

	switch (Profile)
	{
	case EYogPerformanceProfile::Ultra:
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
		Settings.DynamicLightQuality = 2;
		Settings.MaterialLightQuality = 2;
		Settings.MaterialLightMaxLightInfoCount = 2;
		Settings.bUseLumenLite = true;
		Settings.bPreferBatchedGeometryProxies = false;
		break;

	case EYogPerformanceProfile::Medium:
		Settings.ResolutionScalePercent = 70.f;
		Settings.FrameRateLimit = 0;
		Settings.ViewDistanceQuality = 1;
		Settings.ShadowQuality = 1;
		Settings.GlobalIlluminationQuality = 1;
		Settings.ReflectionQuality = 1;
		Settings.PostProcessQuality = 1;
		Settings.TextureQuality = 2;
		Settings.EffectsQuality = 1;
		Settings.FoliageQuality = 1;
		Settings.ShadingQuality = 1;
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
		Settings.GlobalIlluminationQuality = 0;
		Settings.ReflectionQuality = 0;
		Settings.PostProcessQuality = 0;
		Settings.TextureQuality = 1;
		Settings.EffectsQuality = 0;
		Settings.FoliageQuality = 0;
		Settings.ShadingQuality = 0;
		Settings.DynamicLightQuality = 0;
		Settings.MaterialLightQuality = 0;
		Settings.MaterialLightMaxLightInfoCount = 0;
		Settings.bUseLumenLite = false;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceProfile::Custom:
	default:
		Settings.PerformanceProfile = EYogPerformanceProfile::Custom;
		break;
	}

	return Settings;
}

FYogGraphicsSettings UYogPerformanceSettingsLibrary::MakeGraphicsSettingsForTargetTier(EYogPerformanceTargetTier Tier)
{
	FYogGraphicsSettings Settings;

	switch (Tier)
	{
	case EYogPerformanceTargetTier::PCUltra:
		Settings = MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Ultra);
		Settings.SelectedTargetTier = Tier;
		Settings.FrameRateLimit = 0;
		Settings.bUseLumenLite = true;
		Settings.bUseNanite = false;
		Settings.bUseVirtualShadowMaps = false;
		Settings.bPreferBatchedGeometryProxies = false;
		break;

	case EYogPerformanceTargetTier::SteamDeck15W:
		Settings = MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Medium);
		Settings.SelectedTargetTier = Tier;
		Settings.ResolutionScalePercent = 70.f;
		Settings.FrameRateLimit = 0;
		Settings.bUseLumenLite = true;
		Settings.bUseNanite = false;
		Settings.bUseVirtualShadowMaps = false;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceTargetTier::Switch2Candidate:
		Settings = MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Medium);
		Settings.SelectedTargetTier = Tier;
		Settings.ResolutionScalePercent = 75.f;
		Settings.FrameRateLimit = 0;
		Settings.bUseLumenLite = true;
		Settings.bUseNanite = false;
		Settings.bUseVirtualShadowMaps = false;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceTargetTier::SteamDeck5W:
		Settings = MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Low);
		Settings.SelectedTargetTier = Tier;
		Settings.ResolutionScalePercent = 55.f;
		Settings.FrameRateLimit = 0;
		Settings.bUseLumenLite = false;
		Settings.bUseNanite = false;
		Settings.bUseVirtualShadowMaps = false;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceTargetTier::FallbackLow:
		Settings = MakeGraphicsSettingsForProfile(EYogPerformanceProfile::Low);
		Settings.SelectedTargetTier = Tier;
		Settings.ResolutionScalePercent = 50.f;
		Settings.FrameRateLimit = 0;
		Settings.TextureQuality = 0;
		Settings.bUseLumenLite = false;
		Settings.bUseNanite = false;
		Settings.bUseVirtualShadowMaps = false;
		Settings.bPreferBatchedGeometryProxies = true;
		break;

	case EYogPerformanceTargetTier::Custom:
	default:
		Settings.PerformanceProfile = EYogPerformanceProfile::Custom;
		Settings.SelectedTargetTier = EYogPerformanceTargetTier::Custom;
		break;
	}

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
		return false;
	}

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

	SetCVarInt(TEXT("r.DynamicGlobalIlluminationMethod"), Settings.bUseLumenLite ? 1 : 0);
	SetCVarInt(TEXT("r.ReflectionMethod"), Settings.bUseLumenLite ? 1 : 2);
	SetCVarInt(TEXT("r.Nanite"), Settings.bUseNanite ? 1 : 0);
	SetCVarInt(TEXT("r.Shadow.Virtual.Enable"), Settings.bUseVirtualShadowMaps ? 1 : 0);
	SetCVarInt(TEXT("r.Yog.DynamicLightQuality"), ClampQuality(Settings.DynamicLightQuality));
	SetCVarInt(TEXT("r.Yog.MaterialLightQuality"), ClampQuality(Settings.MaterialLightQuality));
	SetCVarInt(TEXT("r.Yog.MaterialLight.MaxLightInfoCount"), FMath::Clamp(Settings.MaterialLightMaxLightInfoCount, 0, 4));
	SetCVarFloat(TEXT("r.ScreenPercentage"), FMath::Clamp(Settings.ResolutionScalePercent, 25.f, 100.f));
	SetCVarFloat(TEXT("t.MaxFPS"), Settings.FrameRateLimit > 0 ? static_cast<float>(Settings.FrameRateLimit) : 0.f);

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
		return false;
	}

	FYogGraphicsSettings Settings = SettingsSave->GraphicsSettings;
	Settings.FrameRateLimit = 0;
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
		EYogPerformanceProfile::Low,
		EYogPerformanceProfile::Medium,
		EYogPerformanceProfile::High,
		EYogPerformanceProfile::Ultra
	};
}

TArray<EYogPerformanceTargetTier> UYogPerformanceSettingsLibrary::GetSelectablePerformanceTargetTiers()
{
	return {
		EYogPerformanceTargetTier::PCUltra,
		EYogPerformanceTargetTier::SteamDeck15W,
		EYogPerformanceTargetTier::Switch2Candidate,
		EYogPerformanceTargetTier::SteamDeck5W,
		EYogPerformanceTargetTier::FallbackLow
	};
}

FText UYogPerformanceSettingsLibrary::GetPerformanceProfileDisplayName(EYogPerformanceProfile Profile)
{
	switch (Profile)
	{
	case EYogPerformanceProfile::Ultra:
		return NSLOCTEXT("DevKitPerformance", "ProfileUltra", "超高");
	case EYogPerformanceProfile::High:
		return NSLOCTEXT("DevKitPerformance", "ProfileHigh", "高");
	case EYogPerformanceProfile::Medium:
		return NSLOCTEXT("DevKitPerformance", "ProfileMedium", "中");
	case EYogPerformanceProfile::Low:
		return NSLOCTEXT("DevKitPerformance", "ProfileLow", "低");
	case EYogPerformanceProfile::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "ProfileCustom", "自定义");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceTargetTierDisplayName(EYogPerformanceTargetTier Tier)
{
	switch (Tier)
	{
	case EYogPerformanceTargetTier::PCUltra:
		return NSLOCTEXT("DevKitPerformance", "TargetPCUltra", "PC Ultra");
	case EYogPerformanceTargetTier::SteamDeck15W:
		return NSLOCTEXT("DevKitPerformance", "TargetSteamDeck15W", "Steam Deck 15W");
	case EYogPerformanceTargetTier::Switch2Candidate:
		return NSLOCTEXT("DevKitPerformance", "TargetSwitch2Candidate", "Switch 2 Candidate");
	case EYogPerformanceTargetTier::SteamDeck5W:
		return NSLOCTEXT("DevKitPerformance", "TargetSteamDeck5W", "Steam Deck 5W");
	case EYogPerformanceTargetTier::FallbackLow:
		return NSLOCTEXT("DevKitPerformance", "TargetFallbackLow", "Fallback Low");
	case EYogPerformanceTargetTier::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "TargetCustom", "Custom");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceProfileDescription(EYogPerformanceProfile Profile)
{
	switch (Profile)
	{
	case EYogPerformanceProfile::Ultra:
		return NSLOCTEXT("DevKitPerformance", "ProfileUltraDesc", "PC最高画质。全质量等级，Lumen全效，无Draw Call优化。");
	case EYogPerformanceProfile::High:
		return NSLOCTEXT("DevKitPerformance", "ProfileHighDesc", "PC高画质。Lumen高质量，全分辨率，无合批代理。");
	case EYogPerformanceProfile::Medium:
		return NSLOCTEXT("DevKitPerformance", "ProfileMediumDesc", "掌机中档（15W）。Lumen Lite，70%分辨率，开启合批代理，60帧目标。");
	case EYogPerformanceProfile::Low:
		return NSLOCTEXT("DevKitPerformance", "ProfileLowDesc", "掌机低档（5W）。无Lumen，55%分辨率，强制合批代理，30帧目标。");
	case EYogPerformanceProfile::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "ProfileCustomDesc", "用户自定义画质参数。");
	}
}

FText UYogPerformanceSettingsLibrary::GetPerformanceTargetTierDescription(EYogPerformanceTargetTier Tier)
{
	switch (Tier)
	{
	case EYogPerformanceTargetTier::PCUltra:
		return NSLOCTEXT("DevKitPerformance", "TargetPCUltraDesc", "Highest PC quality. Lumen enabled, Nanite and VSM disabled by project policy.");
	case EYogPerformanceTargetTier::SteamDeck15W:
		return NSLOCTEXT("DevKitPerformance", "TargetSteamDeck15WDesc", "Handheld 10-15W candidate. Lumen Lite enabled for testing, batch proxies preferred.");
	case EYogPerformanceTargetTier::Switch2Candidate:
		return NSLOCTEXT("DevKitPerformance", "TargetSwitch2CandidateDesc", "Switch 2 candidate profile. Conservative handheld tier until device profiling is available.");
	case EYogPerformanceTargetTier::SteamDeck5W:
		return NSLOCTEXT("DevKitPerformance", "TargetSteamDeck5WDesc", "Handheld 5W tier. Lumen disabled, batch proxies preferred, frame rate unlocked for profiling.");
	case EYogPerformanceTargetTier::FallbackLow:
		return NSLOCTEXT("DevKitPerformance", "TargetFallbackLowDesc", "Fallback low tier. Lumen disabled with the lowest runtime budget.");
	case EYogPerformanceTargetTier::Custom:
	default:
		return NSLOCTEXT("DevKitPerformance", "TargetCustomDesc", "User-controlled custom graphics settings.");
	}
}
