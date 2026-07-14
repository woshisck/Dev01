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

USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FStylizedCharacterLightingProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Profile")
	FName ProfileName = TEXT("Default");

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShadowFadeTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShadowTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShallowFadeTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ShallowTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor SSSTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor FrontTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Base Light")
	FLinearColor ForwardTint = FLinearColor::White;

	/** Curve row used by the original seven-region character attenuation ramp. */
	UPROPERTY(EditAnywhere, Category = "Base Light")
	TSoftObjectPtr<UCurveLinearColor> BaseAttenuationRamp;

	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", ClampMax = "0.99", UIMin = "0.0", UIMax = "0.99"))
	float ShadowFadePower = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Base Light", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float DirectDiffuseIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Specular")
	FLinearColor SpecularTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Specular", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectLightColorInfluence = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectLightColorInfluence = 0.10f;

	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionColorInfluence = 0.05f;

	/** 1 uses the smooth vertex normal for GI; 0 retains the normal-mapped surface response. */
	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GINormalBlend = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float IndirectLightingIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Environment", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float ReflectionIntensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Color Fidelity", meta = (UIMin = "-4.0", UIMax = "4.0"))
	float CharacterExposure = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Color Fidelity", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float CharacterContrast = 1.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Stylized Lighting"))
class CELESLIGHTRUNTIME_API UStylizedLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UStylizedLightingSettings();

	/** Shared atlas containing all character attenuation ramp curves. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting")
	TSoftObjectPtr<UCurveLinearColorAtlas> CharacterRampAtlas;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting")
	bool bEnableStylizedLumenLighting = true;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "1.0", ClampMax = "8.0", UIMin = "1.0", UIMax = "8.0"))
	float BandCount = 5.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BandSoftness = 0.18f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GlossInfluence = 0.65f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DirectBlend = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndirectBlend = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (ClampMin = "0.0", UIMax = "4.0"))
	float SpecularIntensity = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category = "Scene Lighting", meta = (UIMin = "-1.0", UIMax = "1.0"))
	float SpecularOffset = 0.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Reflection")
	EStylizedReflectionKuwaharaMode ReflectionKuwaharaMode = EStylizedReflectionKuwaharaMode::Auto;

	UPROPERTY(Config, EditAnywhere, Category = "Character Reflection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionKuwaharaStrength = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow")
	bool bEnableCharacterHalfViewSelfShadow = true;

	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CharacterHalfViewShadowBlend = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CharacterSelfShadowStrength = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Self Shadow", meta = (ClampMin = "1.0", UIMax = "1000.0"))
	float CharacterSelfShadowMaxTraceDistance = 200.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterDirectLightColorInfluence = 0.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterIndirectLightColorInfluence = 0.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CharacterReflectionColorInfluence = 0.0f;

	/** Up to eight reusable lighting looks. Material instances select one through Lighting Profile. */
	UPROPERTY(Config, EditAnywhere, Category = "Character Lighting Profiles", meta = (TitleProperty = "ProfileName", ClampMax = "8"))
	TArray<FStylizedCharacterLightingProfile> CharacterLightingProfiles;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Scene Lighting")
	void ApplyToConsoleVariables() const;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Stylized Lighting"); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
