#include "StylizedLightingSettings.h"

#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "RenderStyle/YogRenderStyleSettings.h"
#include "StylizedCharacterLighting.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLumenLayerSettingRangeTest,
	"CelesLight.LumenLayers.SettingsFiniteRanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogLumenLayerSettingRangeTest::RunTest(const FString& Parameters)
{
	UStylizedLightingSettings* Settings = NewObject<UStylizedLightingSettings>();
	Settings->bEnableStylizedLumenLighting = true;
	Settings->BandCount = 5.f; Settings->BandSoftness = .18f; Settings->IndirectBlend = 1.f;
	TestEqual(TEXT("Legacy five-band data resolves to exactly two shadow bands"),
		Settings->BuildLumenLayerParameters(), FVector4f(1.f, 2.f, .18f, 1.f));
	Settings->BandCount = std::numeric_limits<float>::quiet_NaN();
	Settings->BandSoftness = std::numeric_limits<float>::infinity();
	Settings->IndirectBlend = -std::numeric_limits<float>::infinity();
	TestEqual(TEXT("UI adapter has finite fallback data without redefining renderer energy"),
		Settings->BuildLumenLayerParameters(), FVector4f(1.f, 2.f, .18f, .35f));
	Settings->BandCount = 50.f; Settings->BandSoftness = -1.f; Settings->IndirectBlend = 20.f;
	TestEqual(TEXT("Arbitrary old band counts cannot re-enable multibanding"),
		Settings->BuildLumenLayerParameters(), FVector4f(1.f, 2.f, 0.f, 1.f));
	Settings->BandCount = 3.6f; Settings->BandSoftness = 2.f; Settings->IndirectBlend = -2.f;
	TestEqual(TEXT("Fractional old band counts also resolve to two shadow bands"),
		Settings->BuildLumenLayerParameters(), FVector4f(1.f, 2.f, 1.f, 0.f));
	const FVector4f BeforeCompatibility = Settings->BuildLumenLayerParameters();
	Settings->GlobalEnvironment.CharacterExposureEV = 20.f;
	Settings->bEnableContinuousCharacterLighting = true;
	TestEqual(TEXT("Later global exposure and continuous candidate cannot change cloud UI adapter values"),
		Settings->BuildLumenLayerParameters(), BeforeCompatibility);
	TestFalse(TEXT("Later level exposure system remains inactive"), Settings->GlobalEnvironment.Sanitized().HasRenderEffect());
	Settings->bEnableStylizedLumenLighting = false;
	TestEqual(TEXT("Scene stylization disable retains authored legacy data"),
		Settings->BuildLumenLayerParameters(), FVector4f(0.f, 2.f, 1.f, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogLumenLayerSettingRoutingTest,
	"CelesLight.LumenLayers.SettingsIndependentRouting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogLumenLayerSettingRoutingTest::RunTest(const FString& Parameters)
{
	UStylizedLightingSettings* Settings = NewObject<UStylizedLightingSettings>();
	UYogRenderStyleSettings* Global = GetMutableDefault<UYogRenderStyleSettings>();
	struct FRestore
	{
		UYogRenderStyleSettings* Global;
		bool bStyle;
		TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
		uint32 Count = 0;
		TMap<IConsoleVariable*, TPair<FString, EConsoleVariableFlags>> CVars;
		~FRestore()
		{
			Global->Style.bEnabled = bStyle;
			SetStylizedCharacterLightingProfiles(MakeArrayView(Packed.GetData(), Packed.Num()), Count);
			for (const auto& Pair : CVars)
			{
				Pair.Key->ClearFlags(ECVF_SetByMask);
				Pair.Key->Set(*Pair.Value.Key, static_cast<EConsoleVariableFlags>(Pair.Value.Value & ECVF_SetByMask));
				Pair.Key->SetFlags(Pair.Value.Value);
			}
		}
	} Restore{Global, Global->Style.bEnabled};
	GetStylizedCharacterLightingProfiles(Restore.Packed, Restore.Count);
	const TCHAR* ActiveNames[] = {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
		TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
		TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
		TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"), TEXT("r.StylizedLumenLighting.SpecularIntensity"),
		TEXT("r.StylizedLumenLighting.SpecularOffset"), TEXT("r.StylizedReflection.Kuwahara.Enable"),
		TEXT("r.StylizedReflection.Kuwahara.Strength"), TEXT("r.StylizedCharacter.SelfShadow.Enable"),
		TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), TEXT("r.StylizedCharacter.SelfShadow.Strength"),
		TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), TEXT("r.StylizedCharacterLighting.DirectColorInfluence"),
		TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence")};
	const TSet<FString> CloudUnimplementedNames = {TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"),
		TEXT("r.StylizedReflection.Kuwahara.Enable"), TEXT("r.StylizedReflection.Kuwahara.Strength")};
	TSet<FString> InitiallyUnregistered;
	for (const TCHAR* Name : ActiveNames)
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!CVar && CloudUnimplementedNames.Contains(Name))
		{
			InitiallyUnregistered.Add(Name);
			AddInfo(FString::Printf(TEXT("Historical setting is serialized but has no implementation in published Engine76: %s"), Name));
			continue;
		}
		if (!TestNotNull(*FString::Printf(TEXT("Published cloud CVar is registered: %s"), Name), CVar)) return false;
		Restore.CVars.Add(CVar, {CVar->GetString(), CVar->GetFlags()});
		CVar->ClearFlags(ECVF_SetByMask);
	}
	IConsoleVariable* RetiredStrength = IConsoleManager::Get().FindConsoleVariable(TEXT("r.StylizedLumenLighting.Strength"));
	const FString BeforeRetiredStrength = RetiredStrength ? RetiredStrength->GetString() : FString();
	TMap<FString, FString> NativeBefore;
	for (const TCHAR* Name : {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.DiffuseIndirect.Allow"),
		TEXT("r.ReflectionMethod"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.Shadow.Virtual.Enable"),
		TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality"), TEXT("r.TonemapperFilm")})
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name)) NativeBefore.Add(Name, CVar->GetString());

	Settings->bEnableStylizedLumenLighting = true;
	Settings->BandCount = 6.f; Settings->BandSoftness = .23f;
	Settings->GlossInfluence = .65f; Settings->DirectBlend = .72f; Settings->IndirectBlend = .78f;
	Settings->IndirectMaxLuminance = 2.4f; Settings->SpecularIntensity = 1.3f; Settings->SpecularOffset = -.1f;
	Settings->ReflectionKuwaharaMode = EStylizedReflectionKuwaharaMode::Enabled;
	Settings->ReflectionKuwaharaStrength = 1.f; // Published implementation intentionally disables this filter.
	Settings->bEnableCharacterHalfViewSelfShadow = true;
	Settings->CharacterHalfViewShadowBlend = .4f;
	Settings->CharacterSelfShadowStrength = .8f;
	Settings->CharacterSelfShadowMaxTraceDistance = 10.f;
	Settings->CharacterDirectLightColorInfluence = .7f;
	Settings->CharacterIndirectLightColorInfluence = .6f;
	Settings->CharacterReflectionColorInfluence = .5f;
	Global->Style.bEnabled = false;
	Settings->bEnableContinuousCharacterLighting = true;
	Settings->ApplyToConsoleVariables();
	const float Expected[] = {1.f, 2.f, .23f, .65f, .72f, .78f, 2.4f, 1.3f, -.1f, 0.f, 0.f, 1.f, .4f, .8f, 10.f, .7f, .6f, .5f};
	static_assert(UE_ARRAY_COUNT(ActiveNames) == UE_ARRAY_COUNT(Expected), "Complete cloud CVar contract must be tested.");
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ActiveNames); ++Index)
	{
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(ActiveNames[Index]))
			TestEqual(*FString::Printf(TEXT("Original cloud value routes without retired Style enable: %s"), ActiveNames[Index]),
				CVar->GetFloat(), Expected[Index]);
		else TestTrue(*FString::Printf(TEXT("Only exact unimplemented historical names may be absent: %s"), ActiveNames[Index]),
			CloudUnimplementedNames.Contains(ActiveNames[Index]));
	}
	for (const FString& Name : InitiallyUnregistered)
		TestNull(*FString::Printf(TEXT("Cloud upload does not register or invent an unimplemented effect: %s"), *Name),
			IConsoleManager::Get().FindConsoleVariable(*Name));
	TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
	uint32 Count = 0; GetStylizedCharacterLightingProfiles(Packed, Count);
	for (uint32 Index = 0; Index < Count; ++Index)
		TestEqual(TEXT("Continuous compatibility fields cannot replace the versioned transition slot"),
			Packed[Index * StylizedCharacterLighting::Float4sPerProfile], Settings->SceneShadowTransition.BuildParameters());

	Settings->bEnableStylizedLumenLighting = false;
	Global->Style.bEnabled = true;
	for (int32 Refresh = 0; Refresh < 3; ++Refresh)
	{
		Settings->ApplyToConsoleVariables();
		TestEqual(TEXT("Disabled cloud scene stylization survives repeated upload regardless of new Style switch"),
			IConsoleManager::Get().FindConsoleVariable(ActiveNames[0])->GetInt(), 0);
	}
	if (RetiredStrength)
		TestEqual(TEXT("Published upload never touches the newer strict Surface Cache Strength CVar"), RetiredStrength->GetString(), BeforeRetiredStrength);
	for (const auto& Pair : NativeBefore)
		TestEqual(*FString::Printf(TEXT("Cloud settings do not override native method, quality or exposure CVar: %s"), *Pair.Key),
			IConsoleManager::Get().FindConsoleVariable(*Pair.Key)->GetString(), Pair.Value);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogTwoBandShadowCurveTest,
	"CelesLight.LumenLayers.TwoShadowBandsContinuousBrightSide", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogTwoBandShadowCurveTest::RunTest(const FString&)
{
	using S = UStylizedLightingSettings;
	TestEqual(TEXT("Deep shadow is the first band"), S::EvaluateTwoBandNoL(.1f, 0.f), 0.f);
	TestEqual(TEXT("Upper dark side is the second band"), S::EvaluateTwoBandNoL(.35f, 0.f), .25f);
	TestEqual(TEXT("Exact hard boundary begins the second band"), S::EvaluateTwoBandNoL(.25f, 0.f), .25f);
	for (float Softness : {0.f, .18f, .49f, 1.f})
	{
		float Previous = 0.f;
		for (int32 Index = 0; Index <= 10000; ++Index)
		{
			const float NoL = Index / 10000.f;
			const float Result = S::EvaluateTwoBandNoL(NoL, Softness);
			if (!FMath::IsFinite(Result) || Result < Previous - 1.e-6f || Result > NoL + 1.e-6f)
				return TestTrue(TEXT("Two shadow bands stay finite, monotonic and never create boundary glow"), false);
			if (NoL >= .5f && Result != NoL)
				return TestTrue(TEXT("Bright side is exactly native, not another lighting band"), false);
			if (Softness == 0.f && NoL < .5f && Result != 0.f && Result != .25f)
				return TestTrue(TEXT("Hard dark side has only two discrete values"), false);
			Previous = Result;
		}
	}
	TestEqual(TEXT("Invalid NoL is finite dark"), S::EvaluateTwoBandNoL(std::numeric_limits<float>::quiet_NaN(), .18f), 0.f);
	TestEqual(TEXT("Invalid softness uses default"), S::EvaluateTwoBandNoL(.3f, std::numeric_limits<float>::infinity()), S::EvaluateTwoBandNoL(.3f, .18f));
	return true;
}
#endif
