#include "LevelEnvironment/YogLevelEnvironment.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "StylizedLightingSettings.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	class FScopedYogGlobalEnvironment final
	{
	public:
		FScopedYogGlobalEnvironment()
			: Settings(GetMutableDefault<UStylizedLightingSettings>()), Previous(Settings->GlobalEnvironment)
		{
			Settings->GlobalEnvironment = FYogLevelEnvironmentSettings();
			// Simulate an old saved configuration that opts into the retired pass.
			Settings->GlobalEnvironment.bEnableColorLayers = true;
			Settings->GlobalEnvironment.CharacterExposureEV = 6.0f;
			Settings->GlobalEnvironment.ColorLayerCount = 3;
		}
		~FScopedYogGlobalEnvironment() { Settings->GlobalEnvironment = Previous; }

		UStylizedLightingSettings* Settings;
	private:
		FYogLevelEnvironmentSettings Previous;
	};

	class FScopedYogEnvironmentWorld final
	{
	public:
		explicit FScopedYogEnvironmentWorld(EWorldType::Type Type)
			: World(UWorld::CreateWorld(Type, false)) {}
		~FScopedYogEnvironmentWorld() { if (World) World->DestroyWorld(false); }
		UWorld* World;
	};

	bool IsExactIdentity(const FLinearColor& Input, const FLinearColor& Output)
	{
		// Includes signed zero, infinities and NaN bit patterns. A retired pass
		// returns native input without arithmetic, clipping, or exposure gain.
		return FMemory::Memcmp(&Input, &Output, sizeof(FLinearColor)) == 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogEnvironmentNativeIdentityTest,
	"CelesLight.LevelEnvironment.NativeIdentityAndRetiredLayers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogEnvironmentNativeIdentityTest::RunTest(const FString& Parameters)
{
	FYogLevelEnvironmentSettings Settings;
	Settings.bEnableColorLayers = true;
	Settings.bColorLayersAffectCharacters = true;
	Settings.bColorLayersAffectScene = true;
	Settings.ColorLayerStrength = 1.0f;
	Settings.ColorLayerSoftness = 0.0f;
	Settings.CharacterExposureEV = 8.0f;
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	const FLinearColor Inputs[] = {
		FLinearColor::Black,
		FLinearColor(0.18f, 0.09f, 0.045f, 0.4f),
		FLinearColor(12345.0f, 6789.0f, 2468.0f, 0.6f),
		FLinearColor(-0.5f, -0.0f, 0.0f, 0.0f),
		FLinearColor(NaN, Infinity, -Infinity, 1.0f)
	};
	TestFalse(TEXT("Retired layering never requests a rendering pass"), Settings.HasRenderEffect());
	for (const FLinearColor& Input : Inputs)
	{
		for (const bool bCharacter : {false, true})
		{
			for (const float PreExposure : {1.0f, 0.000001f, MAX_flt, NaN})
			{
				TestTrue(TEXT("Native scene and character pixels are bit-exact identity regardless of legacy pre-exposure"),
					IsExactIdentity(Input, Settings.TransformLinearColor(Input, bCharacter, PreExposure)));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogEnvironmentRetiredExtremeSettingsTest,
	"CelesLight.LevelEnvironment.LegacyExtremeSettingsCannotReactivateRendering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogEnvironmentRetiredExtremeSettingsTest::RunTest(const FString& Parameters)
{
	FYogLevelEnvironmentSettings Settings;
	Settings.bEnableColorLayers = true;
	Settings.bColorLayersAffectCharacters = true;
	Settings.bColorLayersAffectScene = true;
	const FLinearColor Input(0.113f, 0.077f, 0.024f, 0.8f);
	for (const float LegacyValue : {0.0f, 1.0f, MAX_flt, -MAX_flt, std::numeric_limits<float>::quiet_NaN()})
	{
		Settings.CharacterExposureEV = LegacyValue;
		Settings.ColorLayerMinEV = LegacyValue;
		Settings.ColorLayerMaxEV = -LegacyValue;
		Settings.ColorLayerStrength = LegacyValue;
		Settings.ColorLayerSoftness = LegacyValue;
		Settings.ColorLayerCount = MAX_int32;
		const FYogLevelEnvironmentSettings Sanitized = Settings.Sanitized();
		TestFalse(TEXT("Legacy enabled flag is inert"), Sanitized.bEnableColorLayers);
		TestFalse(TEXT("Legacy character mask is inert"), Sanitized.bColorLayersAffectCharacters);
		TestFalse(TEXT("Legacy scene mask is inert"), Sanitized.bColorLayersAffectScene);
		TestEqual(TEXT("Legacy character exposure is neutral"), Sanitized.CharacterExposureEV, 0.0f);
		TestEqual(TEXT("Legacy strength is neutral"), Sanitized.ColorLayerStrength, 0.0f);
		TestEqual(TEXT("Legacy softness is continuous"), Sanitized.ColorLayerSoftness, 1.0f);
		TestEqual(TEXT("Legacy layer count has a stable compatibility value"), Sanitized.ColorLayerCount, 5);
		TestFalse(TEXT("Raw legacy data cannot request an effect"), Settings.HasRenderEffect());
		TestTrue(TEXT("Raw legacy data cannot alter native scene RGB"), IsExactIdentity(Input, Settings.TransformLinearColor(Input, false)));
		TestTrue(TEXT("Raw legacy data cannot alter native character RGB"), IsExactIdentity(Input, Settings.TransformLinearColor(Input, true)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogEnvironmentProfileMetadataTest,
	"CelesLight.LevelEnvironment.ProfilesRemainLoadCompatibleAndRenderingInert", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogEnvironmentProfileMetadataTest::RunTest(const FString& Parameters)
{
	FScopedYogGlobalEnvironment Globals;
	UYogLevelEnvironmentProfile* Profile = NewObject<UYogLevelEnvironmentProfile>();
	Profile->Settings.bEnableColorLayers = true;
	Profile->Settings.CharacterExposureEV = 4.0f;
	Profile->Settings.ColorLayerCount = 7;
	Profile->Notes = TEXT("Artist-authored environment notes remain intact");
	UYogLevelEnvironmentComponent* Component = NewObject<UYogLevelEnvironmentComponent>();
	Component->Profile = Profile;
	Component->Settings.bEnableColorLayers = true;
	Component->Settings.CharacterExposureEV = -4.0f;
	Component->Settings.ColorLayerCount = 3;
	Component->Notes = TEXT("Level notes remain intact");
	Component->bOverrideGlobalSettings = true;
	TestFalse(TEXT("Referenced legacy profile cannot activate rendering"), Component->GetEffectiveSettings().HasRenderEffect());
	TestTrue(TEXT("Profile reference is retained"), Component->Profile.Get() == Profile);
	TestEqual(TEXT("Reading does not migrate the profile asset"), Profile->Settings.ColorLayerCount, 7);
	TestEqual(TEXT("Reading does not mutate profile notes"), Profile->Notes, FString(TEXT("Artist-authored environment notes remain intact")));
	Component->bOverrideProfile = true;
	TestFalse(TEXT("Legacy inline override cannot activate rendering"), Component->GetEffectiveSettings().HasRenderEffect());
	TestEqual(TEXT("Reading does not mutate inline serialized data"), Component->Settings.ColorLayerCount, 3);
	TestEqual(TEXT("Reading does not mutate component notes"), Component->Notes, FString(TEXT("Level notes remain intact")));
	Component->bOverrideGlobalSettings = false;
	TestFalse(TEXT("Inherited legacy global data is inert"), Component->GetEffectiveSettings().HasRenderEffect());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogEnvironmentAllWorldsNativeTest,
	"CelesLight.LevelEnvironment.AllWorldsRemainNativeWithLegacyOverrides", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogEnvironmentAllWorldsNativeTest::RunTest(const FString& Parameters)
{
	FScopedYogGlobalEnvironment Globals;
	TestFalse(TEXT("Null world is native"), ResolveYogLevelEnvironment(nullptr).HasRenderEffect());
	for (const EWorldType::Type WorldType : {EWorldType::Editor, EWorldType::Game, EWorldType::PIE, EWorldType::EditorPreview, EWorldType::GamePreview})
	{
		FScopedYogEnvironmentWorld ScopedWorld(WorldType);
		if (!TestNotNull(TEXT("Isolated test world"), ScopedWorld.World)) return false;
		TestFalse(TEXT("Untitled, Game, PIE and preview worlds remain native"), ResolveYogLevelEnvironment(ScopedWorld.World).HasRenderEffect());
		AYogLevelEnvironment* Environment = ScopedWorld.World->SpawnActor<AYogLevelEnvironment>();
		if (!TestNotNull(TEXT("Load-compatible legacy environment actor"), Environment)) return false;
		Environment->EnvironmentComponent->bOverrideGlobalSettings = true;
		Environment->EnvironmentComponent->Settings.bEnableColorLayers = true;
		Environment->EnvironmentComponent->Settings.CharacterExposureEV = 12.0f;
		Environment->EnvironmentComponent->Settings.ColorLayerStrength = 1.0f;
		TestFalse(TEXT("Enabled legacy world override cannot reactivate a scene transform"), ResolveYogLevelEnvironment(ScopedWorld.World).HasRenderEffect());
		const FLinearColor Input(0.01f, 0.007f, 0.002f, 1.0f);
		TestTrue(TEXT("Positive native indirect lighting is returned unchanged"),
			IsExactIdentity(Input, ResolveYogLevelEnvironment(ScopedWorld.World).TransformLinearColor(Input, false)));
	}
	TestEqual(TEXT("Reading worlds does not rewrite saved global exposure data"), Globals.Settings->GlobalEnvironment.CharacterExposureEV, 6.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogEnvironmentNativeCVarsUntouchedTest,
	"CelesLight.LevelEnvironment.ResolvingCompatibilityDataPreservesNativeRendererCVars", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogEnvironmentNativeCVarsUntouchedTest::RunTest(const FString& Parameters)
{
	FScopedYogGlobalEnvironment Globals;
	FScopedYogEnvironmentWorld ScopedWorld(EWorldType::Editor);
	if (!TestNotNull(TEXT("Isolated editor world"), ScopedWorld.World)) return false;
	TMap<FString, FString> NativeValues;
	for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"),
		TEXT("r.EyeAdaptationQuality"), TEXT("r.LocalExposure"), TEXT("r.DefaultFeature.AutoExposure"),
		TEXT("r.Lumen.DiffuseIndirect.Allow"), TEXT("r.Lumen.Reflections.Allow")})
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			NativeValues.Add(Name, Variable->GetString());
		}
	}
	TestTrue(TEXT("Native renderer CVars are available"), NativeValues.Num() >= 6);
	const FYogLevelEnvironmentSettings Global = GetYogGlobalEnvironmentSettings();
	const FYogLevelEnvironmentSettings World = ResolveYogLevelEnvironment(ScopedWorld.World);
	TestFalse(TEXT("Global compatibility data cannot request a pass"), Global.HasRenderEffect());
	TestFalse(TEXT("World compatibility data cannot request a pass"), World.HasRenderEffect());
	for (const TPair<FString, FString>& Pair : NativeValues)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Pair.Key);
		TestEqual(*FString::Printf(TEXT("Native CVar %s is untouched"), *Pair.Key), Variable->GetString(), Pair.Value);
	}
	return true;
}
#endif
