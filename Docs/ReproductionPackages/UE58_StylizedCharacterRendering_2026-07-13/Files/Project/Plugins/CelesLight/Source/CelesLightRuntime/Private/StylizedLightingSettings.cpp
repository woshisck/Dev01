#include "StylizedLightingSettings.h"

#include "Curves/CurveLinearColor.h"
#include "Curves/CurveLinearColorAtlas.h"
#include "HAL/IConsoleManager.h"
#include "StylizedCharacterLighting.h"

namespace
{
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
	CharacterRampAtlas = TSoftObjectPtr<UCurveLinearColorAtlas>(FSoftObjectPath(TEXT("/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CA_Ramp.CA_Ramp")));
	CharacterLightingProfiles[0].BaseAttenuationRamp = TSoftObjectPtr<UCurveLinearColor>(FSoftObjectPath(TEXT("/Game/Art/Material/CharacterMaterial/ProxCurve/Ramp/CC_Ramp_01.CC_Ramp_01")));
}

void UStylizedLightingSettings::ApplyToConsoleVariables() const
{
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.Enable"), bEnableStylizedLumenLighting ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandCount"), BandCount);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.BandSoftness"), BandSoftness);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.GlossInfluence"), GlossInfluence);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.DirectBlend"), DirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.IndirectBlend"), IndirectBlend);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularIntensity"), SpecularIntensity);
	SetConsoleVariable(TEXT("r.StylizedLumenLighting.SpecularOffset"), SpecularOffset);
	const int32 KuwaharaMode = ReflectionKuwaharaMode == EStylizedReflectionKuwaharaMode::Auto
		? -1
		: (ReflectionKuwaharaMode == EStylizedReflectionKuwaharaMode::Enabled ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Enable"), KuwaharaMode);
	SetConsoleVariable(TEXT("r.StylizedReflection.Kuwahara.Strength"), ReflectionKuwaharaStrength);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Enable"), bEnableCharacterHalfViewSelfShadow ? 1 : 0);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.HalfViewBlend"), CharacterHalfViewShadowBlend);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.Strength"), CharacterSelfShadowStrength);
	SetConsoleVariable(TEXT("r.StylizedCharacter.SelfShadow.MaxTraceDistance"), CharacterSelfShadowMaxTraceDistance);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.DirectColorInfluence"), CharacterDirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.IndirectColorInfluence"), CharacterIndirectLightColorInfluence);
	SetConsoleVariable(TEXT("r.StylizedCharacterLighting.ReflectionColorInfluence"), CharacterReflectionColorInfluence);

	UCurveLinearColorAtlas* RampAtlas = CharacterRampAtlas.LoadSynchronous();
	SetStylizedCharacterRampAtlas(RampAtlas && RampAtlas->GetResource() ? RampAtlas->GetResource()->TextureRHI : FTextureRHIRef());

	const int32 ProfileCount = FMath::Clamp(CharacterLightingProfiles.Num(), 1, (int32)StylizedCharacterLighting::MaxProfiles);
	TArray<FVector4f> ProfileData;
	ProfileData.Reserve(ProfileCount * StylizedCharacterLighting::Float4sPerProfile);

	for (int32 ProfileIndex = 0; ProfileIndex < ProfileCount; ++ProfileIndex)
	{
		const FStylizedCharacterLightingProfile* Profile = CharacterLightingProfiles.IsValidIndex(ProfileIndex) ? &CharacterLightingProfiles[ProfileIndex] : nullptr;
		const FLinearColor White = FLinearColor::White;
		auto ToVector4f = [](const FLinearColor& Color) { return FVector4f(Color.R, Color.G, Color.B, Color.A); };

		ProfileData.Add(ToVector4f(Profile ? Profile->ShadowFadeTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->ShadowTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->ShallowFadeTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->ShallowTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->SSSTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->FrontTint : White));
		ProfileData.Add(ToVector4f(Profile ? Profile->ForwardTint : White));
		ProfileData.Add(FVector4f(
			Profile ? Profile->SpecularTint.R : 1.0f,
			Profile ? Profile->SpecularTint.G : 1.0f,
			Profile ? Profile->SpecularTint.B : 1.0f,
			Profile ? FMath::Max(Profile->SpecularIntensity, 0.0f) : 1.0f));

		float RampCurveIndex = 0.0f;
		if (RampAtlas && Profile)
		{
			if (UCurveLinearColor* RampCurve = Profile->BaseAttenuationRamp.LoadSynchronous())
			{
				int32 CurveIndex = INDEX_NONE;
				if (RampAtlas->GetCurveIndex(RampCurve, CurveIndex))
				{
					RampCurveIndex = (float)CurveIndex;
				}
			}
		}

		ProfileData.Add(FVector4f(
			RampCurveIndex,
			Profile ? FMath::Clamp(Profile->ShadowFadePower, 0.0f, 0.99f) : 0.5f,
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
			0.0f));
	}

	SetStylizedCharacterLightingProfiles(ProfileData, ProfileCount);
}

#if WITH_EDITOR
void UStylizedLightingSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyToConsoleVariables();
}
#endif
