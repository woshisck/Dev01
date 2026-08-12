#include "StylizedLightingSettings.h"

#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "StylizedCharacterLighting.h"

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

UStylizedLightingSettings::UStylizedLightingSettings()
{
	CharacterLightingProfiles.AddDefaulted();
}

void UStylizedLightingSettings::ApplyToConsoleVariables() const
{
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"), bEnableStylizedLumenLighting ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandCount"), BandCount);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandSoftness"), BandSoftness);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.GlossInfluence"), GlossInfluence);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.DirectBlend"), DirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.IndirectBlend"), IndirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.IndirectMaxLuminance"), IndirectMaxLuminance);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularIntensity"), SpecularIntensity);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularOffset"), SpecularOffset);
	// Kuwahara is intentionally disabled for character rendering. Keep the
	// serialized settings for compatibility with existing projects only.
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Enable"), 0);
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Strength"), 0.0f);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Enable"), bEnableCharacterHalfViewSelfShadow ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), CharacterHalfViewShadowBlend);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Strength"), CharacterSelfShadowStrength);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), CharacterSelfShadowMaxTraceDistance);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.DirectColorInfluence"), CharacterDirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), CharacterIndirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence"), CharacterReflectionColorInfluence);

	const int32 ProfileCount = FMath::Clamp(CharacterLightingProfiles.Num(), 1, (int32)StylizedCharacterLighting::MaxProfiles);
	const uint32 CharacterFeatureMask = BuildCharacterFeatureMask(*this);
	TArray<FVector4f> ProfileData;
	ProfileData.Reserve(ProfileCount * StylizedCharacterLighting::Float4sPerProfile);

	for (int32 ProfileIndex = 0; ProfileIndex < ProfileCount; ++ProfileIndex)
	{
		const FStylizedCharacterLightingProfile* Profile = CharacterLightingProfiles.IsValidIndex(ProfileIndex) ? &CharacterLightingProfiles[ProfileIndex] : nullptr;
		const FLinearColor White = FLinearColor::White;
		auto ToVector4f = [](const FLinearColor& Color) { return FVector4f(Color.R, Color.G, Color.B, Color.A); };

		// Slots 0-5 remain neutral to preserve the existing renderer table ABI
		// while the removed seven-zone Ramp path is no longer sampled.
		for (int32 NeutralSlot = 0; NeutralSlot < 6; ++NeutralSlot)
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
			0.0f));
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
	ApplyToConsoleVariables();
}
#endif
