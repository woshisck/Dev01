#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StylizedLightingSettings.generated.h"

class UCurveLinearColor;
class UCurveLinearColorAtlas;

UENUM(BlueprintType)
enum class EStylizedReflectionKuwaharaMode : uint8
{
	Auto UMETA(DisplayName = "Auto (Epic and Cinematic)"),
	Disabled UMETA(DisplayName = "Disabled"),
	Enabled UMETA(DisplayName = "Enabled")
};

UENUM()
enum class EStylizedLightingEditorLanguage : uint8
{
	Auto UMETA(DisplayName = "Auto / 自动"),
	English UMETA(DisplayName = "English"),
	SimplifiedChinese UMETA(DisplayName = "简体中文")
};

USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedCharacterLightingProfile
{
	GENERATED_BODY()

	/** Artist-facing name used to identify this reusable character lighting profile. */
	UPROPERTY(EditAnywhere, Category = "Profile")
	FName ProfileName = TEXT("Default");

	/** Tint for the transition between the deepest shadow and shadow region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShadowFadeTint = FLinearColor::White;

	/** Tint for the deepest shadow region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShadowTint = FLinearColor::White;

	/** Tint for the transition between shadow and shallow-shadow regions. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShallowFadeTint = FLinearColor::White;

	/** Tint for the shallow-shadow region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShallowTint = FLinearColor::White;

	/** Tint for the soft subsurface-style transition region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor SSSTint = FLinearColor::White;

	/** Tint for the front-lit region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor FrontTint = FLinearColor::White;

	/** Tint for the strongest forward-facing light region. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ForwardTint = FLinearColor::White;

	/** Curve row used by the original seven-region character attenuation ramp. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	TSoftObjectPtr<UCurveLinearColor> BaseAttenuationRamp;

	/** Original material parameter used by the seven-region ramp. Zero matches M_MasterCharacterMAT. */
	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", ClampMax = "0.99", UIMin = "0.0", UIMax = "0.99"))
	float ShadowFadePower = 0.0f;

	/** Multiplier for direct diffuse lighting after the attenuation ramp is evaluated. */
	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float DirectDiffuseIntensity = 1.0f;

	/** Color tint applied to the character specular highlight. */
	UPROPERTY(EditAnywhere, Category = "Specular")
	FLinearColor SpecularTint = FLinearColor::White;

	/** Per-profile strength of character specular highlights. */
	UPROPERTY(EditAnywhere, Category = "Specular", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.0f;

	/** Per-profile RGB influence. Multiplied by the project-wide direct color influence master. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectLightColorInfluence = 0.25f;

	/** Per-profile RGB influence. Multiplied by the project-wide indirect color influence master. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectLightColorInfluence = 0.10f;

	/** Per-profile RGB influence. Multiplied by the project-wide reflection color influence master. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionColorInfluence = 0.05f;

	/** 1 uses the smooth vertex normal for GI; 0 retains the normal-mapped surface response. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GINormalBlend = 1.0f;

	/** Multiplier for Lumen diffuse indirect lighting on the character. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float IndirectLightingIntensity = 1.0f;

	/** Multiplier for environment reflections on the character. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float ReflectionIntensity = 1.0f;

	/** Strength of SSAO and Lumen short-range occlusion on the character. Zero removes self-occlusion darkening without disabling projected scene shadows. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float IndirectOcclusionStrength = 1.0f;

	/** Minimum unlit-style BaseColor contribution used when scene direct and indirect lighting are too weak. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFill = 0.20f;

	/** Exposure in stops for this profile. +1 doubles character lighting; -1 halves it. */
	UPROPERTY(EditAnywhere, Category = "Color Fidelity", meta = (UIMin = "-4.0", UIMax = "4.0"))
	float CharacterExposure = 0.0f;

	/** Luminance contrast applied only to MSM_StylizedCharacterLit lighting. One is neutral. */
	UPROPERTY(EditAnywhere, Category = "Color Fidelity", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float CharacterContrast = 1.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Stylized Lighting"))
class CELESLIGHTRUNTIME_API UStylizedLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UStylizedLightingSettings();

	/** Language used by the Stylized Lighting project settings panel. Auto follows the editor culture. */
	UPROPERTY(Config, EditAnywhere, Category = "Interface")
	EStylizedLightingEditorLanguage EditorLanguage = EStylizedLightingEditorLanguage::Auto;

	/** Shared atlas containing all character attenuation ramp curves. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting")
	TSoftObjectPtr<UCurveLinearColorAtlas> CharacterRampAtlas;

	/** Enables banded stylization for scene direct and Lumen indirect lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting")
	bool bEnableStylizedLumenLighting = true;

	/** Number of discrete lighting bands. Higher values preserve more continuous shading. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "1.0", ClampMax = "8.0", UIMin = "1.0", UIMax = "8.0"))
	float BandCount = 5.0f;

	/** Width of the transition between lighting bands. Zero produces hard steps. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BandSoftness = 0.18f;

	/** Amount that surface glossiness shifts the band response. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlossInfluence = 0.65f;

	/** Blend between native and banded direct lighting. Zero is native and one is fully stylized. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DirectBlend = 1.0f;

	/** Blend between native and banded Lumen diffuse indirect lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectBlend = 0.35f;

	/** Global strength of stylized scene specular highlights. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.2f;

	/** Offset added to N dot H before evaluating stylized scene specular highlights. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (UIMin = "-1.0", UIMax = "1.0"))
	float SpecularOffset = 0.0f;

	/** Controls whether reflection-only Kuwahara filtering is disabled, forced, or quality-tier driven. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Reflection")
	EStylizedReflectionKuwaharaMode ReflectionKuwaharaMode = EStylizedReflectionKuwaharaMode::Auto;

	/** Blend strength of the reflection-only Kuwahara filter. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Reflection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionKuwaharaStrength = 1.0f;

	/** Optional screen-space self shadow for stylized characters. Keep disabled when native character self projection must be removed. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (DisplayName = "Enable Character Half-View Self Shadow"))
	bool bEnableCharacterHalfViewSelfShadow = false;

	/** Blends the shadow direction from the light direction toward the camera half-view direction. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterHalfViewShadowBlend = 0.5f;

	/** Opacity of the optional screen-space character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterSelfShadowStrength = 1.0f;

	/** Maximum screen-space trace distance in world units for optional character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "1.0", UIMax = "1000.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterSelfShadowMaxTraceDistance = 200.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting|Global Multipliers", meta = (DisplayName = "Direct Light Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterDirectLightColorInfluence = 1.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting|Global Multipliers", meta = (DisplayName = "Indirect Light Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterIndirectLightColorInfluence = 1.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting|Global Multipliers", meta = (DisplayName = "Reflection Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterReflectionColorInfluence = 1.0f;

	/** Up to eight reusable lighting looks. Material instances select one through Lighting Profile. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting Profiles", meta = (TitleProperty = "ProfileName", ClampMax = "8"))
	TArray<FStylizedCharacterLightingProfile> CharacterLightingProfiles;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Scene Lighting")
	void ApplyToConsoleVariables() const;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Stylized Lighting"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
