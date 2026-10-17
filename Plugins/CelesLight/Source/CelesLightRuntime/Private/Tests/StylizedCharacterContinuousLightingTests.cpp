#include "StylizedLightingSettings.h"

#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "RenderStyle/YogRenderStyleSettings.h"
#include "StylizedCharacterLighting.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCharacterContinuousNeutralTest,
	"CelesLight.CharacterLighting.ContinuousNeutralAndDisabled", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCharacterContinuousNeutralTest::RunTest(const FString& Parameters)
{
	FStylizedCharacterLightingProfile Profile;
	Profile.ContinuousDiffuseContrast = 1.0f;
	const FVector4f Neutral = Profile.BuildContinuousLightingParameters(true);
	const float Coordinates[] = {-1.0f, -0.0f, 0.0f, 1.e-12f, 1.e-6f, 0.5f, 1.0f, 2.0f};
	for (const float Coordinate : Coordinates)
	{
		const FVector2f Scale = Profile.EvaluateContinuousLightingScale(Coordinate, Neutral);
		TestEqual(TEXT("Neutral diffuse is exactly native"), Scale.X, 1.0f, 0.0f);
		TestEqual(TEXT("Neutral specular is exactly native"), Scale.Y, 1.0f, 0.0f);
	}
	Profile.ContinuousDiffuseContrast = 2.0f;
	Profile.ContinuousDiffuseIntensity = 4.0f;
	Profile.ContinuousSpecularIntensity = 4.0f;
	const FVector4f Disabled = Profile.BuildContinuousLightingParameters(false);
	TestEqual(TEXT("Disabled profile uses neutral xyz and zero enable"), Disabled, FVector4f(1.0f, 1.0f, 1.0f, 0.0f));
	for (const float Coordinate : Coordinates)
	{
		TestEqual(TEXT("Disabled profile is exactly native"), Profile.EvaluateContinuousLightingScale(Coordinate, Disabled), FVector2f(1.0f, 1.0f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCharacterContinuousStabilityTest,
	"CelesLight.CharacterLighting.ContinuousBoundedAtGrazingAngles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCharacterContinuousStabilityTest::RunTest(const FString& Parameters)
{
	FStylizedCharacterLightingProfile Profile;
	const float Contrasts[] = {0.0f, 1.0f, 1.10f, 2.0f};
	for (const float Contrast : Contrasts)
	{
		Profile.ContinuousDiffuseContrast = Contrast;
		Profile.ContinuousDiffuseIntensity = 4.0f;
		Profile.ContinuousSpecularIntensity = 4.0f;
		const FVector4f Packed = Profile.BuildContinuousLightingParameters(true);
		float PreviousDiffuse = -1.0f;
		for (int32 Index = 0; Index <= 10000; ++Index)
		{
			const float Coordinate = static_cast<float>(Index) / 10000.0f;
			const FVector2f Scale = Profile.EvaluateContinuousLightingScale(Coordinate, Packed);
			if (!FMath::IsFinite(Scale.X) || Scale.X < 0.0f || Scale.X > 8.0f || Scale.Y != 4.0f)
			{
				AddError(TEXT("Continuous response must stay finite and bounded, including NoL=0"));
				return false;
			}
			if (PreviousDiffuse >= 0.0f && FMath::Abs(Scale.X - PreviousDiffuse) > 0.001f)
			{
				AddError(TEXT("Continuous response must not produce lighting steps"));
				return false;
			}
			PreviousDiffuse = Scale.X;
		}
		for (const float Coordinate : {1.e-20f, 1.e-12f, 1.e-6f})
		{
			const FVector2f Scale = Profile.EvaluateContinuousLightingScale(Coordinate, Packed);
			TestTrue(TEXT("Low NoL multiplier is finite"), FMath::IsFinite(Scale.X));
			TestEqual(TEXT("Zero native lighting remains zero; no synthetic floor"), 0.0f * Scale.X, 0.0f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCharacterContinuousSanitizeTest,
	"CelesLight.CharacterLighting.ContinuousInvalidDataAndLegacyIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCharacterContinuousSanitizeTest::RunTest(const FString& Parameters)
{
	FStylizedCharacterLightingProfile Profile;
	Profile.ContinuousDiffuseContrast = std::numeric_limits<float>::quiet_NaN();
	Profile.ContinuousDiffuseIntensity = std::numeric_limits<float>::infinity();
	Profile.ContinuousSpecularIntensity = -std::numeric_limits<float>::infinity();
	TestEqual(TEXT("Invalid authored values fall back to native response"), Profile.BuildContinuousLightingParameters(true), FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
	Profile.ContinuousDiffuseContrast = 30.0f;
	Profile.ContinuousDiffuseIntensity = -2.0f;
	Profile.ContinuousSpecularIntensity = 20.0f;
	TestEqual(TEXT("Authored values are bounded before shader upload"), Profile.BuildContinuousLightingParameters(true), FVector4f(2.0f, 0.0f, 4.0f, 1.0f));
	const FVector4f Expected = Profile.BuildContinuousLightingParameters(true);
	Profile.CharacterExposure = 10.0f;
	Profile.CharacterContrast = 0.0f;
	Profile.CharacterBaseFill = 1.0f;
	Profile.DirectLightIntensityInfluence = 0.0f;
	Profile.DirectLightColorInfluence = 0.0f;
	Profile.IndirectLightingIntensity = 0.0f;
	Profile.ReflectionIntensity = 0.0f;
	TestEqual(TEXT("Legacy exposure, fill, GI and light-normalization data cannot affect new parameters"), Profile.BuildContinuousLightingParameters(true), Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCharacterContinuousTableTest,
	"CelesLight.CharacterLighting.CloudProfileTableAndNativeRenderer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCharacterContinuousTableTest::RunTest(const FString& Parameters)
{
	struct FRestoreCustomCVars
	{
		TMap<IConsoleVariable*, TPair<FString, EConsoleVariableFlags>> Values;
		~FRestoreCustomCVars()
		{
			for (const auto& Pair : Values)
			{
				Pair.Key->ClearFlags(ECVF_SetByMask);
				Pair.Key->Set(*Pair.Value.Key, static_cast<EConsoleVariableFlags>(Pair.Value.Value & ECVF_SetByMask));
				Pair.Key->SetFlags(Pair.Value.Value);
			}
		}
	} RestoreCVars;
	for (const TCHAR* Name : {TEXT("r.StylizedLumenLighting.Enable"), TEXT("r.StylizedLumenLighting.BandCount"),
		TEXT("r.StylizedLumenLighting.BandSoftness"), TEXT("r.StylizedLumenLighting.GlossInfluence"),
		TEXT("r.StylizedLumenLighting.DirectBlend"), TEXT("r.StylizedLumenLighting.IndirectBlend"),
		TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"), TEXT("r.StylizedLumenLighting.SpecularIntensity"),
		TEXT("r.StylizedLumenLighting.SpecularOffset"), TEXT("r.StylizedReflection.Kuwahara.Enable"),
		TEXT("r.StylizedReflection.Kuwahara.Strength"), TEXT("r.StylizedCharacter.SelfShadow.Enable"),
		TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), TEXT("r.StylizedCharacter.SelfShadow.Strength"),
		TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), TEXT("r.StylizedCharacterLighting.DirectColorInfluence"),
		TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence")})
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
			RestoreCVars.Values.Add(CVar, {CVar->GetString(), CVar->GetFlags()});
	TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> PreviousData;
	uint32 PreviousCount = 0;
	GetStylizedCharacterLightingProfiles(PreviousData, PreviousCount);
	struct FRestoreProfileTable
	{
		TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s>& Data;
		uint32 Count;
		~FRestoreProfileTable() { SetStylizedCharacterLightingProfiles(MakeArrayView(Data.GetData(), Data.Num()), Count); }
	} Restore{PreviousData, PreviousCount};
	UYogRenderStyleSettings* RenderSettings = GetMutableDefault<UYogRenderStyleSettings>();
	struct FRestoreGlobalEnable
	{
		UYogRenderStyleSettings* Settings;
		bool Previous;
		~FRestoreGlobalEnable() { Settings->Style.bEnabled = Previous; }
	} RestoreGlobal{RenderSettings, RenderSettings->Style.bEnabled};
	const TCHAR* NativeNames[] = {TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.Lumen.DiffuseIndirect.Allow"), TEXT("r.ReflectionMethod"), TEXT("r.Lumen.Reflections.Allow"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.DefaultFeature.AutoExposure"), TEXT("r.EyeAdaptationQuality")};
	TArray<int32> Before;
	for (const TCHAR* Name : NativeNames)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		Before.Add(Variable ? Variable->GetInt() : INDEX_NONE);
	}
	UStylizedLightingSettings* Settings = NewObject<UStylizedLightingSettings>();
	Settings->CharacterLightingProfiles.SetNum(8);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		FStylizedCharacterLightingProfile& Profile = Settings->CharacterLightingProfiles[Index];
		Profile.ContinuousDiffuseContrast = 1.f + Index * .1f; // Later compatibility data is ignored.
		Profile.DirectDiffuseIntensity = 1.f + Index * .1f;
		Profile.CharacterBaseFill = .2f + Index * .01f;
		Profile.CharacterBaseFillSoftness = .1f;
		Profile.CharacterBaseFillOcclusionInfluence = .25f;
		Profile.SpecularTint = FLinearColor(.7f, .8f, .9f, 1.f);
		Profile.SpecularIntensity = 1.2f;
		Profile.GINormalBlend = .8f;
		Profile.DirectLightColorInfluence = .5f;
		Profile.IndirectLightColorInfluence = 1.f;
		Profile.ReflectionColorInfluence = .5f;
		Profile.IndirectLightingIntensity = .8f;
		Profile.CharacterExposure = 0.f;
		Profile.CharacterContrast = 1.f;
		Profile.ReflectionIntensity = 1.2f;
		Profile.IndirectOcclusionStrength = .35f;
		Profile.DirectLightIntensityInfluence = .5f;
	}
	Settings->bEnableContinuousCharacterLighting = false;
	Settings->bEnableHalfLambertPartition = true;
	Settings->bEnableDarkColorFloor = true;
	Settings->bEnableSceneShadowPartition = true;
	Settings->bEnableGINormal = true;
	Settings->bEnableCharacterTone = false;
	Settings->ApplyToConsoleVariables();
	TStaticArray<FVector4f, StylizedCharacterLighting::TotalFloat4s> Packed;
	uint32 Count = 0;
	GetStylizedCharacterLightingProfiles(Packed, Count);
	TestEqual(TEXT("Cloud profile ABI retains eight profiles"), Count, 8u);
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		const uint32 Base = Index * StylizedCharacterLighting::Float4sPerProfile;
		const FStylizedCharacterLightingProfile& Profile = Settings->CharacterLightingProfiles[Index];
		TestEqual(TEXT("Retired slot zero now carries independently versioned direct transition"), Packed[Base], Settings->SceneShadowTransition.BuildParameters());
		for (uint32 Slot = 4; Slot < 6; ++Slot)
			TestEqual(TEXT("Unused cloud placeholders stay neutral"), Packed[Base + Slot], FVector4f(1.f, 1.f, 1.f, 1.f));
		TestEqual(TEXT("Cloud slot six preserves minimum indirect diffuse and AO response"),
			Packed[Base + 6], FVector4f(.1f, .25f, 0.f, Profile.CharacterBaseFill));
		TestEqual(TEXT("Cloud slot seven preserves authored specular tint and strength"),
			Packed[Base + 7], FVector4f(.7f, .8f, .9f, 1.2f));
		TestEqual(TEXT("Cloud slot eight preserves direct diffuse and GI normal blend"),
			Packed[Base + 8], FVector4f(0.f, 0.f, Profile.DirectDiffuseIntensity, .8f));
		TestEqual(TEXT("Cloud slot nine preserves direct/GI/reflection color and GI energy"),
			Packed[Base + 9], FVector4f(.5f, 1.f, .5f, .8f));
		TestEqual(TEXT("Cloud slot ten preserves authored tone/reflection/AO data"),
			Packed[Base + 10], FVector4f(0.f, 1.f, 1.2f, .35f));
		const uint32 FeatureMask = static_cast<uint32>(Packed[Base + 11].Z);
		const uint32 CloudBits = (1u << 1) | (1u << 3) | (1u << 4) | (1u << 14);
		TestEqual(TEXT("Cloud half Lambert, floor, scene projection and GI normal switches remain active"),
			FeatureMask & CloudBits, CloudBits);
		TestEqual(TEXT("Cloud default tone remains disabled"), FeatureMask & (1u << 17), 0u);
		TestEqual(TEXT("Cloud direct-light intensity response remains authored"), Packed[Base + 11].X, .5f);
	}
	const auto CloudPacked = Packed;
	Settings->bEnableContinuousCharacterLighting = true;
	RenderSettings->Style.bEnabled = false;
	Settings->ApplyToConsoleVariables();
	GetStylizedCharacterLightingProfiles(Packed, Count);
	for (int32 Index = 0; Index < Packed.Num(); ++Index)
		TestEqual(TEXT("Later continuous/global candidate switches cannot rewrite cloud profile slots"), Packed[Index], CloudPacked[Index]);
	Settings->bEnableHalfLambertPartition = false;
	Settings->bEnableDarkColorFloor = false;
	Settings->bEnableGINormal = false;
	Settings->ApplyToConsoleVariables();
	GetStylizedCharacterLightingProfiles(Packed, Count);
	const uint32 DisabledMask = static_cast<uint32>(Packed[11].Z);
	TestEqual(TEXT("Published switches, rather than hardcoded native mask, control their feature bits"),
		DisabledMask & ((1u << 1) | (1u << 3) | (1u << 14)), 0u);
	TestEqual(TEXT("Unchanged scene projection bit remains on"), DisabledMask & (1u << 4), 1u << 4);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(NativeNames); ++Index)
		if (const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(NativeNames[Index]))
			TestEqual(FString::Printf(TEXT("Runtime profile upload does not override native method/quality CVar: %s"), NativeNames[Index]), Variable->GetInt(), Before[Index]);
	return true;
}
#endif
