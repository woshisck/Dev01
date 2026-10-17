#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DeveloperSettings.h"
#include "GameFramework/Actor.h"
#include "YogRenderStyleSettings.generated.h"

struct FPostProcessSettings;
class FSceneViewExtensionBase;

/** Scene-linear diffuse, visibility, GGX and GI response; not post-process contrast. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FYogMaterialLightingStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting") bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting") bool bAffectDefaultLit = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting") bool bAffectCharacters = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.5", ClampMax="2.0")) float DiffuseContrast = 1.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="2", ClampMax="8")) float RampBands = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0", ClampMax="1")) float RampAmount = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.05", ClampMax="0.5")) float RampSoftness = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.5", ClampMax="2")) float ShadowContrast = 1.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.5", ClampMax="2")) float IndirectContrast = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.01", ClampMax="16")) float IndirectPivot = .5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0", ClampMax="1")) float IndirectAmount = .7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.25", ClampMax="4")) float HighlightWidth = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="1", ClampMax="4")) float HighlightFalloff = 1.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0.5", ClampMax="2")) float ReflectionContrast = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0", ClampMax="1")) float ReflectionAmount = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting") FLinearColor ShadowTint = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material Lighting", meta=(ClampMin="0", ClampMax="1")) float ShadowTintAmount = 0.f;
	FYogMaterialLightingStyle Sanitized() const;
	void ApplyToRenderer(bool bGlobalEnabled) const;
};

/** Native color-grading multipliers, not a lighting quantizer or exposure system. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FYogNativeRenderStyleSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="启用全局风格"))
	bool bEnabled = true;

	/** Optional display grading is now separated from the material-lighting pipeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style")
	bool bEnablePostProcessGrading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="全局对比度", ClampMin="0.5", ClampMax="1.5"))
	float GlobalContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="全局饱和度", ClampMin="0.0", ClampMax="2.0"))
	float GlobalSaturation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="暗部对比度", ClampMin="0.5", ClampMax="1.5"))
	float ShadowsContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="明暗分区对比（中间调）", ClampMin="0.5", ClampMax="1.5", ToolTip="UE 原生中间调对比倍率，连续加强亮暗差；不是灯光量化，也不重写阴影或 Lumen。"))
	float MidtonesContrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="暗部细节（Gamma）", ClampMin="0.8", ClampMax="1.2", ToolTip="UE 原生暗部 Gamma 倍率。大于 1 适度保留低亮度细节，不是曝光补偿；过大会冲淡暗部。"))
	float ShadowsGamma = 1.0f;

	FYogNativeRenderStyleSettings Sanitized() const;
	bool IsNeutral() const;
	/** Multiplies native grading after camera/volume blending; never touches GI, shadows, exposure or tone curves. */
	void ApplyToPostProcess(FPostProcessSettings& Settings) const;
};

USTRUCT()
struct CELESLIGHTRUNTIME_API FYogRenderStylePreset
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Preset")
	FName Name;
	UPROPERTY()
	FYogNativeRenderStyleSettings Style;
	UPROPERTY()
	FYogMaterialLightingStyle Lighting;
	/** Legacy presets omit this flag and must never reactivate their old pipeline. */
	UPROPERTY(EditAnywhere, Category="Lumen Layers") bool bIncludesLumenLayers = false;
	UPROPERTY(EditAnywhere, Category="Lumen Layers") bool bEnableLumenLayers = true;
	UPROPERTY(EditAnywhere, Category="Lumen Layers") float LumenLayerBands = 5.f;
	UPROPERTY(EditAnywhere, Category="Lumen Layers") float LumenLayerSoftness = .18f;
	UPROPERTY(EditAnywhere, Category="Lumen Layers") float LumenLayerStrength = 1.f;
	/** Versioned cloud eight-parameter contract; strict four-parameter presets are never implicitly migrated. */
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") bool bIncludesCloudLighting = false;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") int32 CloudLightingVersion = 1;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") bool bEnableCloudLighting = true;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingBands = 5.f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingSoftness = .18f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingGlossInfluence = .65f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingSpecularIntensity = 1.f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingSpecularOffset = 0.f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingIndirectBlend = 1.f;
	UPROPERTY(EditAnywhere, Category="Cloud Lighting") float CloudLightingDirectBlend = 1.f;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="YOG Render Style / 写实风格"))
class CELESLIGHTRUNTIME_API UYogRenderStyleSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config)
	FYogNativeRenderStyleSettings Style;

	UPROPERTY(Config)
	FYogMaterialLightingStyle Lighting;

	UPROPERTY(Config, EditAnywhere, Category="Presets", meta=(DisplayName="预设"))
	TArray<FYogRenderStylePreset> Presets;

	void ApplyToRenderer();
	void ResetToNativeDefaults();
};

/** Explicit new overrides; retired level exposure/layer data never opts into this system. */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class CELESLIGHTRUNTIME_API UYogRenderStyleComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYogRenderStyleComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="覆盖全局风格"))
	bool bOverrideGlobalStyle = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="覆盖优先级"))
	int32 Priority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="关卡调色", EditCondition="bOverrideGlobalStyle"))
	FYogNativeRenderStyleSettings Style;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Native Style", meta=(DisplayName="环境备注", MultiLine="true"))
	FString Notes;

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
};

UCLASS(BlueprintType)
class CELESLIGHTRUNTIME_API AYogRenderStyleActor : public AActor
{
	GENERATED_BODY()
public:
	AYogRenderStyleActor();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Native Style")
	TObjectPtr<UYogRenderStyleComponent> StyleComponent;
};

CELESLIGHTRUNTIME_API FYogNativeRenderStyleSettings ResolveYogNativeRenderStyle(const UWorld* World, FString* OutSource = nullptr);
CELESLIGHTRUNTIME_API TSharedRef<FSceneViewExtensionBase, ESPMode::ThreadSafe> CreateYogNativeRenderStyleExtension();
