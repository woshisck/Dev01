#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StylizedLightingSettings.generated.h"

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

	/** Multiplier for direct diffuse lighting after the two-tone half-Lambert partition is evaluated. */
	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float DirectDiffuseIntensity = 1.0f;

	/** How much physical direct-light intensity affects the character. Zero normalizes intensity while retaining hue and attenuation. */
	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DirectLightIntensityInfluence = 0.25f;

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

	/** Minimum neutral indirect diffuse-lighting level. Uses PBR DiffuseColor, so metallic surfaces receive no fake diffuse fill. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFill = 0.20f;

	/** Soft transition width around the minimum-brightness threshold. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFillSoftness = 0.10f;

	/** How strongly SSAO/Lumen occlusion is allowed to darken the minimum-brightness floor. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFillOcclusionInfluence = 0.25f;

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

	/** Master switch for the custom direct-light response of MSM_StylizedCharacterLit. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableCharacterDirectLighting = true;

	/** Uses the authored half-Lambert mapping (N dot L plus Diffuse Bias) as the direct-light coordinate. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableHalfLambertPartition = true;

	/** Keeps non-metal character diffuse lighting above a neutral, PBR-aware minimum. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableDarkColorFloor = true;

	/** Lets shadows cast by the scene move the character into the dark side of the two-tone partition. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection")
	bool bEnableSceneShadowPartition = true;

	/** Enables point, spot and rect lights in addition to directional lights for character direct lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|03 Multiple Lights")
	bool bEnableLocalMultiLights = true;

	/** Enables lights whose Stylized Character Light Mode is Wash. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|03 Multiple Lights")
	bool bEnableWashLights = true;

	/** Enables scene-light RGB influence on the character. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|03 Multiple Lights")
	bool bEnableDirectLightColor = true;

	/** Enables physical scene-light intensity influence on the character. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|03 Multiple Lights")
	bool bEnableDirectLightIntensity = true;

	/** Enables the native energy-conserving GGX direct specular lobe. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|04 Specular")
	bool bEnableGGXSpecular = true;

	/** Enables MixMap.R signed highlight suppression/advance around the neutral value 0.5. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|04 Specular")
	bool bEnableMixMapSpecularControl = true;

	/** Restricts direct specular to the lit side of the two-tone partition. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|04 Specular")
	bool bEnableSpecularLitSideMask = true;

	/** Enables diffuse global illumination on stylized characters. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableLumenDiffuseIndirect = true;

	/** Enables environment color in diffuse global illumination; disabled keeps luminance only. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableLumenIndirectColor = true;

	/** Enables the smooth vertex-normal override used for GI response and occlusion. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableGINormal = true;

	/** Enables Lumen/SSR environment reflections on stylized characters. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableEnvironmentReflections = true;

	/** Enables SSAO and Lumen short-range occlusion on stylized characters. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableIndirectOcclusion = true;

	/** Enables character-only exposure and contrast processing. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|06 Color Fidelity")
	bool bEnableCharacterTone = false;

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

	/** Scene-referred GI luminance represented by the brightest indirect-light band. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "8.0"))
	float IndirectMaxLuminance = 2.0f;

	/** Global strength of stylized scene specular highlights. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.0f;

	/** Offset added to N dot H before evaluating stylized scene specular highlights. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (UIMin = "-1.0", UIMax = "1.0"))
	float SpecularOffset = 0.0f;

	/** Controls whether reflection-only Kuwahara filtering is disabled, forced, or quality-tier driven. */
	UPROPERTY(Config)
	EStylizedReflectionKuwaharaMode ReflectionKuwaharaMode = EStylizedReflectionKuwaharaMode::Disabled;

	/** Blend strength of the reflection-only Kuwahara filter. */
	UPROPERTY(Config)
	float ReflectionKuwaharaStrength = 0.0f;

	/** Optional screen-space self shadow for stylized characters. Keep disabled when native character self projection must be removed. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "Enable Character Half-View Self Shadow"))
	bool bEnableCharacterHalfViewSelfShadow = false;

	/** Blends the shadow direction from the light direction toward the camera half-view direction. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterHalfViewShadowBlend = 0.5f;

	/** Opacity of the optional screen-space character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterSelfShadowStrength = 1.0f;

	/** Maximum screen-space trace distance in world units for optional character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (ClampMin = "1.0", UIMax = "1000.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterSelfShadowMaxTraceDistance = 200.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|03 Multiple Lights", meta = (DisplayName = "Direct Light Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterDirectLightColorInfluence = 1.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen", meta = (DisplayName = "Indirect Light Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterIndirectLightColorInfluence = 1.0f;

	/** Global master multiplier. The final value is this setting multiplied by the selected profile value. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen", meta = (DisplayName = "Reflection Color Influence Master", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
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
