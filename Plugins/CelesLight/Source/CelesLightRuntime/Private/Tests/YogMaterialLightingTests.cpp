#include "RenderStyle/YogRenderStyleSettings.h"
#include "Engine/Scene.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Math/PerspectiveMatrix.h"
#include "SceneView.h"
#include "YogMaterialLighting.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingInputsTest,
	"CelesLight.MaterialPipeline.SafeInputsAndNoDisplayGrading", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingInputsTest::RunTest(const FString&)
{
	FYogMaterialLightingStyle Style;
	Style.DiffuseContrast = std::numeric_limits<float>::quiet_NaN();
	Style.RampBands = 100.f; Style.RampAmount = -1.f; Style.RampSoftness = 0.f;
	Style.IndirectPivot = -1.f; Style.HighlightWidth = 100.f;
	Style.HighlightFalloff = -1.f; Style.ShadowTint = FLinearColor(-1.f, 100.f, std::numeric_limits<float>::infinity());
	const FYogMaterialLightingStyle Safe = Style.Sanitized();
	TestEqual(TEXT("Nonfinite diffuse response is neutral"), Safe.DiffuseContrast, 1.f);
	TestEqual(TEXT("Bands bounded"), Safe.RampBands, 8.f);
	TestEqual(TEXT("Ramp amount bounded"), Safe.RampAmount, 0.f);
	TestEqual(TEXT("Transitions never collapse to hard zero-width"), Safe.RampSoftness, .05f);
	TestEqual(TEXT("GI reference remains positive"), Safe.IndirectPivot, .01f);
	TestEqual(TEXT("GGX width bounded"), Safe.HighlightWidth, 4.f);
	TestEqual(TEXT("GGX falloff lower bound is native"), Safe.HighlightFalloff, 1.f);
	TestEqual(TEXT("Shadow RGB lower bound is nonzero"), Safe.ShadowTint.R, .1f);
	TestEqual(TEXT("Shadow RGB upper bound"), Safe.ShadowTint.G, 2.f);
	TestEqual(TEXT("Invalid shadow tint neutral"), Safe.ShadowTint.B, 1.f);
	FPostProcessSettings Post;
	Post.ColorContrast.W = 1.23f; Post.AutoExposureBias = 2.5f; Post.FilmSlope = .8f;
	FYogNativeRenderStyleSettings Legacy;
	Legacy.GlobalContrast = 1.5f; Legacy.MidtonesContrast = 1.5f; Legacy.ShadowsGamma = 1.2f;
	Legacy.ApplyToPostProcess(Post);
	TestEqual(TEXT("Legacy contrast values do not opt in to display grading"), Post.ColorContrast.W, 1.23);
	TestEqual(TEXT("Native scene exposure remains authored"), Post.AutoExposureBias, 2.5f);
	TestEqual(TEXT("Native film curve remains authored"), Post.FilmSlope, .8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingLiveCVarTest,
	"CelesLight.MaterialPipeline.LiveControlAndNativeIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingLiveCVarTest::RunTest(const FString&)
{
	const TCHAR* Names[] = {
		TEXT("Enable"), TEXT("DefaultLit"), TEXT("Character"), TEXT("DiffuseContrast"), TEXT("Bands"), TEXT("RampAmount"), TEXT("RampSoftness"),
		TEXT("ShadowContrast"), TEXT("IndirectContrast"), TEXT("IndirectPivot"), TEXT("IndirectAmount"), TEXT("HighlightWidth"), TEXT("HighlightFalloff"),
		TEXT("ReflectionContrast"), TEXT("ReflectionAmount"), TEXT("ShadowTintR"), TEXT("ShadowTintG"), TEXT("ShadowTintB"), TEXT("ShadowTintAmount")};
	TMap<FString, FString> Before;
	for (const TCHAR* Name : Names)
	{
		const FString Key = FString(TEXT("r.Yog.MaterialLighting.")) + Name;
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Key);
		if (!TestNotNull(*Key, Variable)) return false;
		Before.Add(Key, Variable->GetString());
	}
	TMap<FString, FString> Native;
	for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"),
		TEXT("r.EyeAdaptationQuality"), TEXT("r.Lumen.ScreenProbeGather.ScreenTraces"), TEXT("r.Lumen.DiffuseIndirect.Allow")})
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) Native.Add(Name, Variable->GetString());
	FYogMaterialLightingStyle Style;
	Style.DiffuseContrast = 1.62f; Style.IndirectAmount = .8f; Style.HighlightWidth = .75f;
	Style.ApplyToRenderer(true);
	auto Value = [](const TCHAR* Name) { return IConsoleManager::Get().FindConsoleVariable(Name)->GetFloat(); };
	TestEqual(TEXT("Retired material pipeline cannot be enabled by an old preset"), Value(TEXT("r.Yog.MaterialLighting.Enable")), 0.f);
	TestEqual(TEXT("Diffuse control reaches renderer immediately"), Value(TEXT("r.Yog.MaterialLighting.DiffuseContrast")), 1.62f);
	TestEqual(TEXT("GI response reaches renderer immediately"), Value(TEXT("r.Yog.MaterialLighting.IndirectAmount")), .8f);
	TestEqual(TEXT("GGX width reaches renderer immediately"), Value(TEXT("r.Yog.MaterialLighting.HighlightWidth")), .75f);
	Style.ApplyToRenderer(false);
	TestEqual(TEXT("Master off returns native shader branch"), Value(TEXT("r.Yog.MaterialLighting.Enable")), 0.f);
	for (const auto& Pair : Native)
		TestEqual(*Pair.Key, IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
	for (const auto& Pair : Before) IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->Set(*Pair.Value, ECVF_SetByCode);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogRetiredMaterialLightingRendererTest,
	"CelesLight.MaterialPipeline.RetiredCVarCannotReactivateRenderer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogRetiredMaterialLightingRendererTest::RunTest(const FString&)
{
	IConsoleVariable* Enable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Yog.MaterialLighting.Enable"));
	if (!TestNotNull(TEXT("Retired enable CVar remains load-compatible"), Enable)) return false;
	struct FRestoreEnable
	{
		IConsoleVariable* Variable;
		FString Value;
		uint32 Flags;
		~FRestoreEnable()
		{
			Variable->SetFlags(static_cast<EConsoleVariableFlags>(Flags));
			Variable->Set(*Value, static_cast<EConsoleVariableFlags>(Flags & ECVF_SetByMask));
		}
	} Restore{Enable, Enable->GetString(), Enable->GetFlags()};
	// Simulate a higher-priority legacy console override, not only saved config.
	Enable->Set(1, ECVF_SetByConsole);
	TestEqual(TEXT("Test really supplied the stale enabled CVar"), Enable->GetInt(), 1);
	FSceneViewInitOptions Options;
	Options.SetViewRectangle(FIntRect(0, 0, 64, 64));
	Options.ViewOrigin = FVector::ZeroVector;
	Options.ViewRotationMatrix = FMatrix::Identity;
	Options.ProjectionMatrix = FReversedZPerspectiveMatrix(PI / 4.0f, 1.0f, 1.0f, 1.0f);
	const FSceneView View(Options);
	const FYogMaterialLightingUniformParameters Parameters = GetYogMaterialLightingParameters(View);
	TestEqual(TEXT("Renderer control stays disabled even with an explicit legacy CVar override"), Parameters.Control.X, 0.0f);
	return true;
}
#endif
