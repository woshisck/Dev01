#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LevelEnvironment/YogLevelEnvironment.h"
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

/** Retired shadow tint data, kept only so existing assets and config still load. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FYogShadowTransitionSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow Transition")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow Transition", meta = (ClampMin = "0", ClampMax = "1"))
	float Wrap = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow Transition", meta = (ClampMin = "1", ClampMax = "8"))
	float Falloff = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow Transition", meta = (ClampMin = "0", ClampMax = "1"))
	float Strength = .35f;
	/** Linear multiplier of shadow-side diffuse. White is neutral; RGB cannot amplify light. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shadow Transition")
	FLinearColor Tint = FLinearColor::White;

	FYogShadowTransitionSettings Sanitized() const;
	/** Compatibility builders always return inactive/neutral GPU data. */
	FVector4f BuildParameters() const;
	FVector4f BuildTint(bool bOverride = false) const;
	/** Retired response: always zero. */
	float Evaluate(float SignedNoL, float Visibility = 1.f) const;
};

UENUM(BlueprintType)
enum class EStylizedMetalHighlightMode : uint8
{
	PBR UMETA(DisplayName = "PBR 响应"),
	Stylized UMETA(DisplayName = "风格化块面")
};

/** Native GGX response and optional art-directed highlight block shaping. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedMetalArtSettings
{
	GENERATED_BODY()
	/** PBR ignores Stylization but retains its last value for mode switching. */
	UPROPERTY(EditAnywhere, Category="Metal")
	EStylizedMetalHighlightMode HighlightMode = EStylizedMetalHighlightMode::Stylized;
	/** 0 is native GGX contrast; 1 is full block shaping. No independent fill light. */
	UPROPERTY(EditAnywhere, Category="Metal", meta=(ClampMin="0", ClampMax="1"))
	float Stylization = .5f;
	UPROPERTY(EditAnywhere, Category="Metal") float LightFace = .18f;
	/** Legacy dark-face reflection gain, not the artist's darkening control. */
	UPROPERTY() float DarkFace = .012f;
	/** Broadens the stylized carrier in microfacet-slope space, independently of the PBR endpoint. */
	UPROPERTY(EditAnywhere, Category="Metal", meta=(ClampMin="0", ClampMax="1")) float BaseBlockSize = .30f;
	/** Suppresses the real specular tail between blocks. Zero is neutral; never adds light. */
	UPROPERTY(EditAnywhere, Category="Metal", meta=(ClampMin="0", ClampMax="1")) float DarkSideStrength = .35f;
	/** Scales the roughness-derived default width; an explicit material width overrides it. */
	UPROPERTY(EditAnywhere, Category="Metal") float HighlightWidth = 1.5f;
	/** Retired compatibility data. Highlight edge softness is now material Roughness. */
	UPROPERTY() float EdgeSoftness = .008f;
	UPROPERTY(EditAnywhere, Category="Metal") float HighlightStrength = 3.2f;
	UPROPERTY(EditAnywhere, Category="Metal") float CoreStrength = 4.5f;
	/** Retired load-only compatibility data; artificial environment bands no longer render. */
	UPROPERTY() float EnvironmentStrength = 1.f;
	UPROPERTY() float EnvironmentWidth = 1.f;
	UPROPERTY() FVector BandDirection = FVector(-.35, .2, .92);
	UPROPERTY(EditAnywhere, Category="Metal") FLinearColor HighlightTint = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedCharacterLightingProfile
{
	GENERATED_BODY()

	/** Artist-facing name used to identify this reusable character lighting profile. */
	UPROPERTY(EditAnywhere, Category = "Profile")
	FName ProfileName = TEXT("Default");

	/** Artist-authored metal response for this profile only. Material instances select the profile through Lighting Profile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Specular", meta = (DisplayName = "NPR 金属", ToolTip = "只对当前 Lighting Profile 的金属区域启用风格化高光与反射；关闭后回到原 PBR 增强路径，不关闭材质 Metallic 或 VSM。"))
	bool bEnableNPRMetal = false;
	UPROPERTY(EditAnywhere, Category="Specular") bool bOverrideMetalArt = false;
	UPROPERTY(EditAnywhere, Category="Specular") FStylizedMetalArtSettings MetalArt;

	/** Existing materials select this profile through their Lighting Profile parameter. */
	UPROPERTY()
	bool bOverrideShadowTransition = false;
	/** When overriding, bEnabled=false explicitly disables the transition for this character profile. */
	UPROPERTY()
	FYogShadowTransitionSettings ShadowTransition;
	FYogShadowTransitionSettings ResolveShadowTransition(const FYogShadowTransitionSettings& Scene) const;

	/** Continuous direct-diffuse response around native Default Lit. One is exactly native; no steps or added dark fill. */
	UPROPERTY()
	float ContinuousDiffuseContrast = 1.10f;

	/** Multiplier of native direct diffuse only. Scene-light color, intensity, attenuation and shadow visibility are preserved. */
	UPROPERTY()
	float ContinuousDiffuseIntensity = 1.0f;

	/** Multiplier of the native direct GGX lobe; roughness, Fresnel and environment reflections stay native. */
	UPROPERTY()
	float ContinuousSpecularIntensity = 1.0f;

	/** Compatibility-only CPU data; never uploaded to the restored cloud profile table. */
	FVector4f BuildContinuousLightingParameters(bool bEnabled) const;

	/** Compatibility-only CPU reference; not sampled by the restored cloud shaders. */
	static FVector2f EvaluateContinuousLightingScale(float NoL, const FVector4f& Parameters);

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

	/** Minimum indirect diffuse level, preserving sampled GI hue. Uses PBR DiffuseColor so fully metallic surfaces receive no fake diffuse fill. Zero retains native GI. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFill = 0.20f;

	/** Soft transition width around the minimum-brightness threshold. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFillSoftness = 0.10f;

	/** How strongly SSAO/Lumen occlusion is allowed to darken the minimum-brightness floor. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterBaseFillOcclusionInfluence = 0.25f;

	/** Exposure in stops for this profile. +1 doubles character lighting; -1 halves it. */
	UPROPERTY()
	float CharacterExposure = 0.0f;

	/** Luminance contrast applied only to MSM_StylizedCharacterLit lighting. One is neutral. */
	UPROPERTY()
	float CharacterContrast = 1.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Stylized Lighting"))
class CELESLIGHTRUNTIME_API UStylizedLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UStylizedLightingSettings();

	/** Retired data, no longer uploaded or exposed as an artist control. */
	UPROPERTY(Config)
	FYogShadowTransitionSettings SceneShadowTransition;

	/** Language used by the Stylized Lighting project settings panel. Auto follows the editor culture. */
	UPROPERTY(Config, EditAnywhere, Category = "Interface")
	EStylizedLightingEditorLanguage EditorLanguage = EStylizedLightingEditorLanguage::Auto;

	/** Abandoned level exposure/color-layer data remains load-compatible and inactive. */
	UPROPERTY(Config)
	FYogLevelEnvironmentSettings GlobalEnvironment;

	/** Compatibility-only switch; cloud rendering never reads the later continuous candidate. */
	UPROPERTY(Config)
	bool bEnableContinuousCharacterLighting = false;

	/** Master switch for the custom direct-light response of MSM_StylizedCharacterLit. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableCharacterDirectLighting = true;

	/** Uses the authored half-Lambert mapping (N dot L plus Diffuse Bias) as the direct-light coordinate. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableHalfLambertPartition = true;

	/** Legacy additive diffuse floor. Disabled by default; native GI supplies the shadow-side environment lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|01 Basic Half Lambert")
	bool bEnableDarkColorFloor = false;

	/** Lets native VSM received shadows, including self projection, move the character into the dark partition. GI is composed separately and remains visible. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "角色接收原生投影（包含自投影）"))
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

	UPROPERTY(Config, EditAnywhere, Category="Metal Art") FStylizedMetalArtSettings MetalArt;
	UPROPERTY(Config, EditAnywhere, Category="Metal Art", meta=(ClampMin="0", ClampMax="1")) float NonMetalReflectionAlpha = .3f;
	UPROPERTY(Config, EditAnywhere, Category="Metal Art", meta=(ClampMin="0", ClampMax="1")) float MetalReflectionAlpha = .7f;
	/** 0 off, 1 four quadrant regions, 2 eight overlapping angular sectors. */

	/** Enables SSAO and Lumen short-range occlusion on stylized characters. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|05 Global Illumination Lumen")
	bool bEnableIndirectOcclusion = false;

	/** Enables character-only exposure and contrast processing. */
	UPROPERTY(Config)
	bool bEnableCharacterTone = false;

	/** Enables banded stylization for scene direct and Lumen indirect lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting")
	bool bEnableStylizedLumenLighting = true;

	/** Serialized compatibility value. Rendering always uses two shadow bands and continuous highlights. */
	UPROPERTY(Config)
	float BandCount = 2.0f;

	/** Width of the transition between lighting bands. Zero produces hard steps. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "0.49"))
	float BandSoftness = 0.18f;

	/** Retired compatibility value; roughness no longer shifts shadow bands. */
	UPROPERTY(Config)
	float GlossInfluence = 0.65f;

	/** Blend between native and banded direct lighting. Zero is native and one is fully stylized. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectBlend = 1.0f;

	/** Blend between native and banded Lumen diffuse indirect lighting. */
	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectBlend = 0.35f;

	/** Retired compatibility data: the current shader has no such parameter. */
	UPROPERTY(Config)
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

	/** Compatibility only; artist path is fixed to native VSM (separation off). */
	UPROPERTY(Config)
	bool bEnableCharacterShadowSeparation = false;
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection")
	bool bEnableCharacterIndependentSelfShadow = true;
	/** Compatibility only; half-vector preference is inert while separation is off. */
	UPROPERTY(Config)
	bool bEnableCharacterHalfVectorShadow = true;

	/** Additional HZB screen-space occlusion for Toon lights. Not a native VSM self-shadow switch; offscreen occluders cannot be traced. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "附加屏幕空间遮挡（非 VSM 自投影开关）"))
	bool bEnableCharacterHalfViewSelfShadow = false;

	/** Blends the shadow direction from the light direction toward the camera half-view direction. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "遮挡方向向视角偏移", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterHalfViewShadowBlend = 0.5f;

	/** Opacity of the optional screen-space character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "附加屏幕空间遮挡强度", ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
	float CharacterSelfShadowStrength = 1.0f;

	/** Maximum screen-space trace distance in world units for optional character self shadow. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Features|02 Scene Projection", meta = (DisplayName = "附加遮挡最大追踪距离 / cm", ClampMin = "1.0", UIMax = "1000.0", EditCondition = "bEnableCharacterHalfViewSelfShadow", EditConditionHides))
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

	/** UI compatibility tuple: enabled, authored bands, softness, legacy indirect blend.
	 * Actual cloud rendering uses the complete old CVar and profile contract. */
	FVector4f BuildLumenLayerParameters() const;
	/** CPU reference of the shadow-only two-band shader response. NoL >= .5 is exactly native. */
	static float EvaluateTwoBandNoL(float NoL, float Softness);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Scene Lighting")
	void ApplyToConsoleVariables() const;

	/** Upload character profiles without resetting unrelated CVars; retired tint slots are neutral. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Shadow Transition")
	void ApplyShadowTransitionParameters() const;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Stylized Lighting"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
