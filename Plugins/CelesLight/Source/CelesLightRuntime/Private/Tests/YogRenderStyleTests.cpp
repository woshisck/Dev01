#include "RenderStyle/YogRenderStyleSettings.h"

#include "Engine/Level.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	struct FScopedRenderStyle
	{
		UYogRenderStyleSettings* Settings = GetMutableDefault<UYogRenderStyleSettings>();
		FYogNativeRenderStyleSettings Previous = Settings->Style;
		~FScopedRenderStyle() { Settings->Style = Previous; }
	};
	struct FScopedStyleWorld
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
		~FScopedStyleWorld() { if (World) World->DestroyWorld(false); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleIdentityTest,
	"CelesLight.RenderStyle.NativeAndDisabledIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleIdentityTest::RunTest(const FString& Parameters)
{
	FPostProcessSettings Settings;
	Settings.ColorContrast = FVector4(1.1f, .9f, .8f, 1.2f);
	Settings.ColorSaturation = FVector4(.8f, .7f, .6f, 1.3f);
	Settings.ColorContrastShadows = FVector4(.6f, .7f, .8f, .9f);
	Settings.ColorContrastMidtones = FVector4(.7f, .8f, .9f, 1.1f);
	Settings.ColorGammaShadows = FVector4(.9f, 1.1f, 1.2f, .95f);
	const FVector4 Contrast = Settings.ColorContrast;
	const FVector4 Saturation = Settings.ColorSaturation;
	const FVector4 Shadows = Settings.ColorContrastShadows;
	const FVector4 Midtones = Settings.ColorContrastMidtones;
	const FVector4 ShadowsGamma = Settings.ColorGammaShadows;
	FYogNativeRenderStyleSettings Style;
	TestTrue(TEXT("Default settings are neutral"), Style.IsNeutral());
	TestEqual(TEXT("Default midtone contrast is native"), Style.MidtonesContrast, 1.f);
	TestEqual(TEXT("Default shadow gamma is native"), Style.ShadowsGamma, 1.f);
	Style.ApplyToPostProcess(Settings);
	TestTrue(TEXT("Native contrast is exact identity"), Settings.ColorContrast == Contrast);
	TestTrue(TEXT("Native saturation is exact identity"), Settings.ColorSaturation == Saturation);
	TestTrue(TEXT("Native shadow contrast is exact identity"), Settings.ColorContrastShadows == Shadows);
	TestTrue(TEXT("Native midtone contrast is exact identity"), Settings.ColorContrastMidtones == Midtones);
	TestTrue(TEXT("Native shadow gamma is exact identity"), Settings.ColorGammaShadows == ShadowsGamma);
	Style.bEnabled = false; Style.GlobalContrast = 1.5f; Style.GlobalSaturation = 0.f; Style.ShadowsContrast = .5f;
	Style.MidtonesContrast = 1.5f; Style.ShadowsGamma = 1.2f;
	Style.ApplyToPostProcess(Settings);
	TestTrue(TEXT("Disabled contrast is exact identity"), Settings.ColorContrast == Contrast);
	TestTrue(TEXT("Disabled saturation is exact identity"), Settings.ColorSaturation == Saturation);
	TestTrue(TEXT("Disabled shadow contrast is exact identity"), Settings.ColorContrastShadows == Shadows);
	TestTrue(TEXT("Disabled midtone contrast is exact identity"), Settings.ColorContrastMidtones == Midtones);
	TestTrue(TEXT("Disabled shadow gamma is exact identity"), Settings.ColorGammaShadows == ShadowsGamma);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleNativeGradingTest,
	"CelesLight.RenderStyle.NativeGradingPreservesLightingExposureAndCurve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleNativeGradingTest::RunTest(const FString& Parameters)
{
	FPostProcessSettings Settings;
	Settings.ColorContrast = FVector4(1.1f, .9f, .8f, 1.2f);
	Settings.ColorSaturation = FVector4(.8f, .7f, .6f, 1.3f);
	Settings.ColorContrastShadows = FVector4(.6f, .7f, .8f, .9f);
	Settings.ColorContrastMidtones = FVector4(.7f, .8f, .9f, 1.1f);
	Settings.ColorGammaShadows = FVector4(.9f, 1.1f, 1.2f, .95f);
	Settings.AutoExposureBias = 2.5f; Settings.FilmSlope = .8f;
	Settings.BloomIntensity = .4f; Settings.LumenSceneDetail = 2.f;
	const FPostProcessSettings Before = Settings;
	FYogNativeRenderStyleSettings Style;
	Style.bEnablePostProcessGrading = true;
	Style.GlobalContrast = 1.06f; Style.GlobalSaturation = .95f; Style.ShadowsContrast = 1.02f;
	Style.MidtonesContrast = 1.12f; Style.ShadowsGamma = 1.08f;
	Style.ApplyToPostProcess(Settings);
	TestEqual(TEXT("All-channel contrast multiplier"), Settings.ColorContrast.W, Before.ColorContrast.W * 1.06f);
	TestEqual(TEXT("All-channel saturation multiplier"), Settings.ColorSaturation.W, Before.ColorSaturation.W * .95f);
	TestEqual(TEXT("All-channel shadow multiplier"), Settings.ColorContrastShadows.W, Before.ColorContrastShadows.W * 1.02f);
	TestEqual(TEXT("All-channel midtone contrast multiplier"), Settings.ColorContrastMidtones.W, Before.ColorContrastMidtones.W * 1.12f);
	TestEqual(TEXT("All-channel shadow gamma multiplier"), Settings.ColorGammaShadows.W, Before.ColorGammaShadows.W * 1.08f);
	TestTrue(TEXT("Authored contrast tint is preserved"), Settings.ColorContrast.X == Before.ColorContrast.X && Settings.ColorContrast.Y == Before.ColorContrast.Y && Settings.ColorContrast.Z == Before.ColorContrast.Z);
	TestTrue(TEXT("Authored saturation tint is preserved"), Settings.ColorSaturation.X == Before.ColorSaturation.X && Settings.ColorSaturation.Y == Before.ColorSaturation.Y && Settings.ColorSaturation.Z == Before.ColorSaturation.Z);
	TestTrue(TEXT("Authored shadow contrast tint is preserved"), Settings.ColorContrastShadows.X == Before.ColorContrastShadows.X && Settings.ColorContrastShadows.Y == Before.ColorContrastShadows.Y && Settings.ColorContrastShadows.Z == Before.ColorContrastShadows.Z);
	TestTrue(TEXT("Authored midtone contrast tint is preserved"), Settings.ColorContrastMidtones.X == Before.ColorContrastMidtones.X && Settings.ColorContrastMidtones.Y == Before.ColorContrastMidtones.Y && Settings.ColorContrastMidtones.Z == Before.ColorContrastMidtones.Z);
	TestTrue(TEXT("Authored shadow gamma tint is preserved"), Settings.ColorGammaShadows.X == Before.ColorGammaShadows.X && Settings.ColorGammaShadows.Y == Before.ColorGammaShadows.Y && Settings.ColorGammaShadows.Z == Before.ColorGammaShadows.Z);
	TestEqual(TEXT("Native exposure is unchanged"), Settings.AutoExposureBias, Before.AutoExposureBias);
	TestEqual(TEXT("Native film curve is unchanged"), Settings.FilmSlope, Before.FilmSlope);
	TestEqual(TEXT("Native bloom is unchanged"), Settings.BloomIntensity, Before.BloomIntensity);
	TestEqual(TEXT("Native Lumen detail is unchanged"), Settings.LumenSceneDetail, Before.LumenSceneDetail);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleSanitizeTest,
	"CelesLight.RenderStyle.SafeFiniteInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleSanitizeTest::RunTest(const FString& Parameters)
{
	FYogNativeRenderStyleSettings Style;
	Style.GlobalContrast = -100.f; Style.GlobalSaturation = 100.f;
	Style.ShadowsContrast = std::numeric_limits<float>::quiet_NaN();
	Style.MidtonesContrast = -100.f; Style.ShadowsGamma = 100.f;
	const FYogNativeRenderStyleSettings Safe = Style.Sanitized();
	TestEqual(TEXT("Contrast lower bound"), Safe.GlobalContrast, .5f);
	TestEqual(TEXT("Saturation upper bound"), Safe.GlobalSaturation, 2.f);
	TestEqual(TEXT("Nonfinite input is native"), Safe.ShadowsContrast, 1.f);
	TestEqual(TEXT("Midtone contrast lower bound"), Safe.MidtonesContrast, .5f);
	TestEqual(TEXT("Shadow gamma upper bound"), Safe.ShadowsGamma, 1.2f);
	Style.MidtonesContrast = 100.f; Style.ShadowsGamma = -100.f;
	const FYogNativeRenderStyleSettings OppositeBounds = Style.Sanitized();
	TestEqual(TEXT("Midtone contrast upper bound"), OppositeBounds.MidtonesContrast, 1.5f);
	TestEqual(TEXT("Shadow gamma lower bound"), OppositeBounds.ShadowsGamma, .8f);
	Style.MidtonesContrast = std::numeric_limits<float>::infinity();
	Style.ShadowsGamma = std::numeric_limits<float>::quiet_NaN();
	const FYogNativeRenderStyleSettings Nonfinite = Style.Sanitized();
	TestEqual(TEXT("Nonfinite midtone contrast is native"), Nonfinite.MidtonesContrast, 1.f);
	TestEqual(TEXT("Nonfinite shadow gamma is native"), Nonfinite.ShadowsGamma, 1.f);
	Style.MidtonesContrast = std::numeric_limits<float>::quiet_NaN();
	Style.ShadowsGamma = -std::numeric_limits<float>::infinity();
	const FYogNativeRenderStyleSettings OtherNonfinite = Style.Sanitized();
	TestEqual(TEXT("NaN midtone contrast is native"), OtherNonfinite.MidtonesContrast, 1.f);
	TestEqual(TEXT("Infinite shadow gamma is native"), OtherNonfinite.ShadowsGamma, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleToneRangeControlsTest,
	"CelesLight.RenderStyle.ToneRangeControlsAreIndependentlyNonNeutral", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleToneRangeControlsTest::RunTest(const FString& Parameters)
{
	FPostProcessSettings MidtoneSettings;
	const FPostProcessSettings Native = MidtoneSettings;
	FYogNativeRenderStyleSettings MidtoneStyle;
	MidtoneStyle.bEnablePostProcessGrading = true;
	MidtoneStyle.MidtonesContrast = 1.12f;
	TestFalse(TEXT("Midtone-only change is not skipped as neutral"), MidtoneStyle.IsNeutral());
	MidtoneStyle.ApplyToPostProcess(MidtoneSettings);
	TestEqual(TEXT("Midtone-only change reaches native field"), MidtoneSettings.ColorContrastMidtones.W, Native.ColorContrastMidtones.W * 1.12f);
	TestTrue(TEXT("Midtone-only change leaves global contrast untouched"), MidtoneSettings.ColorContrast == Native.ColorContrast);
	TestTrue(TEXT("Midtone-only change leaves shadow contrast untouched"), MidtoneSettings.ColorContrastShadows == Native.ColorContrastShadows);
	TestTrue(TEXT("Midtone-only change leaves shadow gamma untouched"), MidtoneSettings.ColorGammaShadows == Native.ColorGammaShadows);
	FPostProcessSettings ShadowSettings;
	FYogNativeRenderStyleSettings ShadowStyle;
	ShadowStyle.bEnablePostProcessGrading = true;
	ShadowStyle.ShadowsGamma = 1.08f;
	TestFalse(TEXT("Shadow-gamma-only change is not skipped as neutral"), ShadowStyle.IsNeutral());
	ShadowStyle.ApplyToPostProcess(ShadowSettings);
	TestEqual(TEXT("Shadow-gamma-only change reaches native field"), ShadowSettings.ColorGammaShadows.W, Native.ColorGammaShadows.W * 1.08f);
	TestTrue(TEXT("Shadow-gamma-only change leaves global gamma untouched"), ShadowSettings.ColorGamma == Native.ColorGamma);
	TestTrue(TEXT("Shadow-gamma-only change leaves midtone contrast untouched"), ShadowSettings.ColorContrastMidtones == Native.ColorContrastMidtones);
	TestEqual(TEXT("Shadow-gamma-only change leaves native exposure untouched"), ShadowSettings.AutoExposureBias, Native.AutoExposureBias);
	TestEqual(TEXT("Shadow-gamma-only change leaves native film curve untouched"), ShadowSettings.FilmSlope, Native.FilmSlope);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleInheritanceTest,
	"CelesLight.RenderStyle.GlobalInheritanceExplicitOverrideAndMasterSwitch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleInheritanceTest::RunTest(const FString& Parameters)
{
	FScopedRenderStyle Globals; FScopedStyleWorld Scope;
	if (!TestNotNull(TEXT("Isolated world"), Scope.World)) return false;
	Globals.Settings->Style = FYogNativeRenderStyleSettings();
	Globals.Settings->Style.GlobalContrast = 1.06f;
	Globals.Settings->Style.MidtonesContrast = 1.12f;
	Globals.Settings->Style.ShadowsGamma = 1.08f;
	TestEqual(TEXT("Untitled world inherits global style"), ResolveYogNativeRenderStyle(Scope.World).GlobalContrast, 1.06f);
	TestEqual(TEXT("Untitled world inherits global midtone contrast"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, 1.12f);
	TestEqual(TEXT("Untitled world inherits global shadow gamma"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, 1.08f);
	AYogRenderStyleActor* First = Scope.World->SpawnActor<AYogRenderStyleActor>();
	AYogRenderStyleActor* Second = Scope.World->SpawnActor<AYogRenderStyleActor>();
	if (!TestNotNull(TEXT("First override actor"), First) || !TestNotNull(TEXT("Second override actor"), Second)) return false;
	First->StyleComponent->RegisterComponent(); Second->StyleComponent->RegisterComponent();
	First->StyleComponent->Style.MidtonesContrast = .9f;
	First->StyleComponent->Style.ShadowsGamma = .95f;
	TestEqual(TEXT("New component does not opt in by default"), ResolveYogNativeRenderStyle(Scope.World).GlobalContrast, 1.06f);
	TestEqual(TEXT("Inactive local midtone contrast cannot replace global"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, 1.12f);
	TestEqual(TEXT("Inactive local shadow gamma cannot replace global"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, 1.08f);
	First->StyleComponent->bOverrideGlobalStyle = true;
	First->StyleComponent->Style.GlobalContrast = 1.1f;
	Second->StyleComponent->bOverrideGlobalStyle = true;
	Second->StyleComponent->Priority = 10;
	Second->StyleComponent->Style.GlobalContrast = .95f;
	Second->StyleComponent->Style.MidtonesContrast = 1.2f;
	Second->StyleComponent->Style.ShadowsGamma = 1.1f;
	TestEqual(TEXT("Highest priority explicit override wins"), ResolveYogNativeRenderStyle(Scope.World).GlobalContrast, .95f);
	TestEqual(TEXT("Explicit override selects local midtone contrast"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, 1.2f);
	TestEqual(TEXT("Explicit override selects local shadow gamma"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, 1.1f);
	Scope.World->PersistentLevel->bIsVisible = false;
	TestEqual(TEXT("Hidden levels cannot override"), ResolveYogNativeRenderStyle(Scope.World).GlobalContrast, 1.06f);
	TestEqual(TEXT("Hidden levels restore global midtone contrast"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, 1.12f);
	TestEqual(TEXT("Hidden levels restore global shadow gamma"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, 1.08f);
	Scope.World->PersistentLevel->bIsVisible = true;
	Second->StyleComponent->bOverrideGlobalStyle = false;
	TestEqual(TEXT("Disabled override inherits next active one"), ResolveYogNativeRenderStyle(Scope.World).GlobalContrast, 1.1f);
	TestEqual(TEXT("Disabled override selects next active midtone contrast"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, .9f);
	TestEqual(TEXT("Disabled override selects next active shadow gamma"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, .95f);
	Globals.Settings->Style.bEnabled = false;
	TestFalse(TEXT("Level cannot reactivate disabled global master"), ResolveYogNativeRenderStyle(Scope.World).bEnabled);
	TestTrue(TEXT("Master disabled result is native"), ResolveYogNativeRenderStyle(Scope.World).IsNeutral());
	TestEqual(TEXT("Master disabled midtone contrast is native"), ResolveYogNativeRenderStyle(Scope.World).MidtonesContrast, 1.f);
	TestEqual(TEXT("Master disabled shadow gamma is native"), ResolveYogNativeRenderStyle(Scope.World).ShadowsGamma, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRenderStyleCVarIsolationTest,
	"CelesLight.RenderStyle.PreservesNativeRendererCVars", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRenderStyleCVarIsolationTest::RunTest(const FString& Parameters)
{
	FScopedRenderStyle Globals; FScopedStyleWorld Scope;
	TMap<FString, FString> Before;
	for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.EyeAdaptationQuality"), TEXT("r.Lumen.DiffuseIndirect.Allow"), TEXT("r.Lumen.Reflections.Allow")})
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) Before.Add(Name, Variable->GetString());
	}
	TestTrue(TEXT("Native renderer CVars available"), Before.Num() >= 4);
	Globals.Settings->Style.GlobalContrast = 1.06f;
	Globals.Settings->Style.MidtonesContrast = 1.12f;
	Globals.Settings->Style.ShadowsGamma = 1.08f;
	Globals.Settings->ApplyToRenderer();
	ResolveYogNativeRenderStyle(Scope.World);
	for (const auto& Pair : Before)
	{
		TestEqual(*Pair.Key, IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
	}
	Globals.Settings->Style = Globals.Previous; Globals.Settings->ApplyToRenderer();
	return true;
}
#endif
