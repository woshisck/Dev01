#include "StylizedLightingSettings.h"

#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "StylizedCharacterLighting.h"
#include "UObject/UnrealType.h"

namespace
{
	enum EStylizedCharacterFeatureBits : uint32
	{
		CharacterDirectLighting = 1u << 0,
		HalfLambertPartition = 1u << 1,
		DarkColorFloor = 1u << 3,
		SceneShadowPartition = 1u << 4,
		LocalMultiLights = 1u << 5,
		WashLights = 1u << 6,
		DirectLightColor = 1u << 7,
		DirectLightIntensity = 1u << 8,
		GGXSpecular = 1u << 9,
		MixMapSpecularControl = 1u << 10,
		SpecularLitSideMask = 1u << 11,
		LumenDiffuseIndirect = 1u << 12,
		LumenIndirectColor = 1u << 13,
		GINormal = 1u << 14,
		EnvironmentReflections = 1u << 15,
		IndirectOcclusion = 1u << 16,
		CharacterTone = 1u << 17,
	};

	uint32 BuildCharacterFeatureMask(const UStylizedLightingSettings& Settings)
	{
		uint32 Mask = 0u;
		auto SetBit = [&Mask](const bool bEnabled, const uint32 Bit)
		{
			if (bEnabled)
			{
				Mask |= Bit;
			}
		};

		SetBit(Settings.bEnableCharacterDirectLighting, CharacterDirectLighting);
		SetBit(Settings.bEnableHalfLambertPartition, HalfLambertPartition);
		SetBit(Settings.bEnableDarkColorFloor, DarkColorFloor);
		SetBit(Settings.bEnableSceneShadowPartition, SceneShadowPartition);
		SetBit(Settings.bEnableLocalMultiLights, LocalMultiLights);
		SetBit(Settings.bEnableWashLights, WashLights);
		SetBit(Settings.bEnableDirectLightColor, DirectLightColor);
		SetBit(Settings.bEnableDirectLightIntensity, DirectLightIntensity);
		SetBit(Settings.bEnableGGXSpecular, GGXSpecular);
		SetBit(Settings.bEnableMixMapSpecularControl, MixMapSpecularControl);
		SetBit(Settings.bEnableSpecularLitSideMask, SpecularLitSideMask);
		SetBit(Settings.bEnableLumenDiffuseIndirect, LumenDiffuseIndirect);
		SetBit(Settings.bEnableLumenIndirectColor, LumenIndirectColor);
		SetBit(Settings.bEnableGINormal, GINormal);
		SetBit(Settings.bEnableEnvironmentReflections, EnvironmentReflections);
		SetBit(Settings.bEnableIndirectOcclusion, IndirectOcclusion);
		SetBit(Settings.bEnableCharacterTone, CharacterTone);
		return Mask;
	}

	void SetConsoleVariable(const TCHAR* Name, const int32 Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByProjectSetting);
		}
	}

	void SetConsoleVariable(const TCHAR* Name, const float Value)
	{
		if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			ConsoleVariable->Set(Value, ECVF_SetByProjectSetting);
		}
	}
}

FYogShadowTransitionSettings FYogShadowTransitionSettings::Sanitized() const
{
	FYogShadowTransitionSettings Result = *this;
	const auto Safe = [](float Value, float Min, float Max, float Fallback)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Fallback;
	};
	Result.Wrap = Safe(Wrap, 0.f, 1.f, .35f);
	Result.Falloff = Safe(Falloff, 1.f, 8.f, 2.f);
	Result.Strength = Safe(Strength, 0.f, 1.f, .35f);
	Result.Tint = FLinearColor(Safe(Tint.R, 0.f, 1.f, 1.f), Safe(Tint.G, 0.f, 1.f, 1.f), Safe(Tint.B, 0.f, 1.f, 1.f), 1.f);
	return Result;
}

FVector4f FYogShadowTransitionSettings::BuildParameters() const
{
	// Retired: keep serialized values loadable, but never enable the old tint ABI.
	return FVector4f(0.f, 0.f, 0.f, 0.f);
}

FVector4f FYogShadowTransitionSettings::BuildTint(bool bOverride) const
{
	return FVector4f(1.f, 1.f, 1.f, 0.f);
}

float FYogShadowTransitionSettings::Evaluate(float SignedNoL, float Visibility) const
{
	return 0.f;
}

FYogShadowTransitionSettings FStylizedCharacterLightingProfile::ResolveShadowTransition(const FYogShadowTransitionSettings& Scene) const
{
	return FYogShadowTransitionSettings();
}

FVector4f FStylizedCharacterLightingProfile::BuildContinuousLightingParameters(const bool bEnabled) const
{
	if (!bEnabled)
	{
		return FVector4f(1.0f, 1.0f, 1.0f, 0.0f);
	}

	const auto Sanitize = [](const float Value, const float Maximum)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, Maximum) : 1.0f;
	};
	return FVector4f(
		Sanitize(ContinuousDiffuseContrast, 2.0f),
		Sanitize(ContinuousDiffuseIntensity, 4.0f),
		Sanitize(ContinuousSpecularIntensity, 4.0f), 1.0f);
}

FVector2f FStylizedCharacterLightingProfile::EvaluateContinuousLightingScale(const float NoL, const FVector4f& Parameters)
{
	// CPU compatibility math only; the restored cloud shader never samples this candidate.
	// No division or negative power near NoL=0; the neutral path is exact.
	if (Parameters.W <= 0.0f || (Parameters.X == 1.0f && Parameters.Y == 1.0f && Parameters.Z == 1.0f))
	{
		return FVector2f(1.0f, 1.0f);
	}
	const float Coordinate = FMath::IsFinite(NoL) ? FMath::Clamp(NoL, 0.0f, 1.0f) : 0.0f;
	const float DiffuseResponse = FMath::Max(1.0f + (Parameters.X - 1.0f) * (2.0f * Coordinate - 1.0f), 0.0f);
	return FVector2f(
		FMath::Lerp(1.0f, DiffuseResponse * Parameters.Y, FMath::Clamp(Parameters.W, 0.0f, 1.0f)),
		FMath::Lerp(1.0f, Parameters.Z, FMath::Clamp(Parameters.W, 0.0f, 1.0f)));
}

UStylizedLightingSettings::UStylizedLightingSettings()
{
	GlobalEnvironment = GlobalEnvironment.Sanitized();
	CharacterLightingProfiles.AddDefaulted();
	// Only the first default look opts in. Additional and legacy profiles stay
	// neutral unless explicitly authored; never overwrite a saved false value.
	CharacterLightingProfiles[0].bEnableNPRMetal = true;
}

FVector4f UStylizedLightingSettings::BuildLumenLayerParameters() const
{
	// This is only a UI adapter, not the newer Surface Cache float4 renderer ABI.
	constexpr float Bands = 2.f;
	const float Softness = FMath::IsFinite(BandSoftness) ? FMath::Clamp(BandSoftness, 0.f, 1.f) : .18f;
	const float Indirect = FMath::IsFinite(IndirectBlend) ? FMath::Clamp(IndirectBlend, 0.f, 1.f) : .35f;
	return FVector4f(bEnableStylizedLumenLighting ? 1.f : 0.f, Bands, Softness, Indirect);
}

float UStylizedLightingSettings::EvaluateTwoBandNoL(float NoL, float Softness)
{
	const float N = FMath::IsFinite(NoL) ? FMath::Clamp(NoL, 0.f, 1.f) : 0.f;
	if (N >= .5f) return N;
	const float Width = .25f * (FMath::IsFinite(Softness) ? FMath::Clamp(Softness, 0.f, .49f) : .18f);
	if (Width <= 0.f) return N < .25f ? 0.f : .25f;
	const float DarkBand = .25f * FMath::SmoothStep(.25f, .25f + Width, N);
	return FMath::Lerp(DarkBand, N, FMath::SmoothStep(.5f - Width, .5f, N));
}

void UStylizedLightingSettings::ApplyToConsoleVariables() const
{
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"), bEnableStylizedLumenLighting ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandCount"), 2.f);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandSoftness"), FMath::Clamp(BandSoftness, 0.f, .49f));
	// Preserve legacy ABI/readback only; this slot has no current shader effect.
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.GlossInfluence"), GlossInfluence);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.DirectBlend"), DirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.IndirectBlend"), IndirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularIntensity"), SpecularIntensity);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularOffset"), SpecularOffset);
	// Kuwahara is intentionally disabled for character rendering. Keep the
	// serialized settings for compatibility with existing projects only.
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Enable"), 0);
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Strength"), 0.0f);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Enable"), bEnableCharacterHalfViewSelfShadow ? 1 : 0);
	// Confirmed production look: native VSM. Retain the experiment in renderer
	// for explicit internal validation, not as artist-facing saved controls.
	SetConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.Enable"), 0);
	SetConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.Self"), bEnableCharacterIndependentSelfShadow ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedCharacter.Shadow.Split.HalfVector"), 1);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), CharacterHalfViewShadowBlend);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Strength"), CharacterSelfShadowStrength);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), CharacterSelfShadowMaxTraceDistance);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.DirectColorInfluence"), CharacterDirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), CharacterIndirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence"), CharacterReflectionColorInfluence);

	ApplyShadowTransitionParameters();
}

void UStylizedLightingSettings::ApplyShadowTransitionParameters() const
{
	const int32 ProfileCount = FMath::Clamp(CharacterLightingProfiles.Num(), 1, (int32)StylizedCharacterLighting::MaxProfiles);
	const uint32 CharacterFeatureMask = BuildCharacterFeatureMask(*this);
	TArray<FVector4f> ProfileData;
	ProfileData.Reserve(ProfileCount * StylizedCharacterLighting::Float4sPerProfile);

	for (int32 ProfileIndex = 0; ProfileIndex < ProfileCount; ++ProfileIndex)
	{
		const FStylizedCharacterLightingProfile* Profile = CharacterLightingProfiles.IsValidIndex(ProfileIndex) ? &CharacterLightingProfiles[ProfileIndex] : nullptr;
		const FLinearColor White = FLinearColor::White;
		auto ToVector4f = [](const FLinearColor& Color) { return FVector4f(Color.R, Color.G, Color.B, Color.A); };

		// Reuse retired tint slots for explicit NPR controls; never consume legacy tint data.
		const auto& Art = Profile && Profile->bOverrideMetalArt ? Profile->MetalArt : MetalArt;
		auto Safe = [](float V, float Lo, float Hi, float Fallback)
		{ return FMath::IsFinite(V) ? FMath::Clamp(V, Lo, Hi) : Fallback; };
		ProfileData.Add(FVector4f(Safe(Art.LightFace,0,2,.18f), Safe(Art.DarkFace,0,1,.012f), Safe(Art.HighlightWidth,.1f,3,1.5f), Safe(Art.BaseBlockSize,0,1,.30f)));
		// Reuse retired environment-band lanes for explicit mode and blend.
		// The twelve-vector ABI and material-profile opt-in remain unchanged.
		ProfileData.Add(FVector4f(Safe(Art.HighlightStrength,0,10,3.2f), Safe(Art.CoreStrength,0,20,4.5f),
			Art.HighlightMode == EStylizedMetalHighlightMode::Stylized ? 1.f : 0.f,
			Safe(Art.Stylization,0,1,.5f)));
		ProfileData.Add(FVector4f(Safe(Art.DarkSideStrength,0,1,.35f),0.f,0.f,Safe(NonMetalReflectionAlpha,0,1,.3f)));
		ProfileData.Add(FVector4f(Safe(Art.HighlightTint.R,0,1,1),Safe(Art.HighlightTint.G,0,1,1),Safe(Art.HighlightTint.B,0,1,1),Safe(MetalReflectionAlpha,0,1,.7f)));
		for (int32 NeutralSlot = 4; NeutralSlot < 6; ++NeutralSlot)
		{
			ProfileData.Add(ToVector4f(White));
		}
		ProfileData.Add(FVector4f(
			Profile ? FMath::Clamp(Profile->CharacterBaseFillSoftness, 0.0f, 1.0f) : 0.10f,
			Profile ? FMath::Clamp(Profile->CharacterBaseFillOcclusionInfluence, 0.0f, 1.0f) : 0.25f,
			0.0f,
			Profile ? FMath::Clamp(Profile->CharacterBaseFill, 0.0f, 1.0f) : 0.20f));
		ProfileData.Add(FVector4f(
			Profile ? Profile->SpecularTint.R : 1.0f,
			Profile ? Profile->SpecularTint.G : 1.0f,
			Profile ? Profile->SpecularTint.B : 1.0f,
			Profile ? FMath::Max(Profile->SpecularIntensity, 0.0f) : 1.0f));

		ProfileData.Add(FVector4f(
			0.0f,
			0.0f,
			Profile ? FMath::Max(Profile->DirectDiffuseIntensity, 0.0f) : 1.0f,
			Profile ? FMath::Clamp(Profile->GINormalBlend, 0.0f, 1.0f) : 1.0f));
		ProfileData.Add(FVector4f(
			Profile ? FMath::Clamp(Profile->DirectLightColorInfluence, 0.0f, 1.0f) : 0.25f,
			Profile ? FMath::Clamp(Profile->IndirectLightColorInfluence, 0.0f, 1.0f) : 0.10f,
			Profile ? FMath::Clamp(Profile->ReflectionColorInfluence, 0.0f, 1.0f) : 0.05f,
			Profile ? FMath::Max(Profile->IndirectLightingIntensity, 0.0f) : 1.0f));
		ProfileData.Add(FVector4f(
			Profile ? Profile->CharacterExposure : 0.0f,
			Profile ? FMath::Max(Profile->CharacterContrast, 0.0f) : 1.0f,
			Profile ? FMath::Max(Profile->ReflectionIntensity, 0.0f) : 1.0f,
			Profile ? FMath::Clamp(Profile->IndirectOcclusionStrength, 0.0f, 1.0f) : 1.0f));
		ProfileData.Add(FVector4f(
			Profile ? FMath::Clamp(Profile->DirectLightIntensityInfluence, 0.0f, 1.0f) : 0.25f,
			0.0f,
			static_cast<float>(CharacterFeatureMask),
			Profile && Profile->bEnableNPRMetal ? 1.0f : 0.0f));
	}

	SetStylizedCharacterLightingProfiles(ProfileData, ProfileCount);
}

#if WITH_EDITOR
FText UStylizedLightingSettings::GetSectionText() const
{
	const bool bUseChinese = EditorLanguage == EStylizedLightingEditorLanguage::SimplifiedChinese
		|| (EditorLanguage == EStylizedLightingEditorLanguage::Auto
			&& FInternationalization::Get().GetCurrentCulture()->GetName().StartsWith(TEXT("zh")));
	return FText::FromString(bUseChinese ? TEXT("风格化灯光") : TEXT("Stylized Lighting"));
}

FText UStylizedLightingSettings::GetSectionDescription() const
{
	const bool bUseChinese = EditorLanguage == EStylizedLightingEditorLanguage::SimplifiedChinese
		|| (EditorLanguage == EStylizedLightingEditorLanguage::Auto
			&& FInternationalization::Get().GetCurrentCulture()->GetName().StartsWith(TEXT("zh")));
	return FText::FromString(bUseChinese
		? TEXT("角色分层光照、场景风格化 Lumen、反射滤镜与可选角色自阴影的美术设置。")
		: TEXT("Artist controls for character banded lighting, stylized scene Lumen, reflection filtering, and optional character self shadow."));
}

void UStylizedLightingSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UStylizedLightingSettings, GlobalEnvironment))
	{
		GlobalEnvironment = GlobalEnvironment.Sanitized();
		return; // Keep the later level exposure/layer system inactive.
	}
	if (HasAnyFlags(RF_ClassDefaultObject)) ApplyToConsoleVariables();
}
#endif
