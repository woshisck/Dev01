#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "YogLevelEnvironment.generated.h"

/** Retired rendering data, retained for existing level/profile package compatibility. */
USTRUCT(BlueprintType)
struct CELESLIGHTRUNTIME_API FYogLevelEnvironmentSettings
{
	GENERATED_BODY()

	/** Retired brightness bias. Load compatibility only; never applied to lighting. */
	UPROPERTY()
	float CharacterExposureEV = 0.0f;

	UPROPERTY()
	bool bEnableColorLayers = false;

	/** Reference layer count across four stops of native pre-exposed HDR brightness. */
	UPROPERTY()
	int32 ColorLayerCount = 5;

	/** Fraction of one layer used to soften transitions. One is continuous. */
	UPROPERTY()
	float ColorLayerSoftness = 0.18f;

	UPROPERTY()
	float ColorLayerStrength = 1.0f;

	/** Retired absolute exposure range. Serialized only; no HDR range is bypassed. */
	UPROPERTY()
	float ColorLayerMinEV = -8.0f;

	UPROPERTY()
	float ColorLayerMaxEV = 6.0f;

	UPROPERTY()
	bool bColorLayersAffectCharacters = true;

	UPROPERTY()
	bool bColorLayersAffectScene = true;

	FYogLevelEnvironmentSettings Sanitized() const;
	bool HasRenderEffect() const;
	/** Retired rendering API: always returns the input color, independent of old settings. */
	FLinearColor TransformLinearColor(const FLinearColor& Color, bool bCharacter, float PreExposure = 1.0f) const;
};

UCLASS(BlueprintType)
class CELESLIGHTRUNTIME_API UYogLevelEnvironmentProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FYogLevelEnvironmentSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Environment", meta=(MultiLine="true"))
	FString Notes;
};

/** Legacy level metadata only: no view extension, exposure, GI, or color processing. */
UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class CELESLIGHTRUNTIME_API UYogLevelEnvironmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYogLevelEnvironmentComponent();

	UPROPERTY()
	bool bEnabled = true;

	/** Inherit the current project-wide look unless this level explicitly overrides it. */
	UPROPERTY()
	bool bOverrideGlobalSettings = false;

	/** Highest priority enabled, visible override actor wins. Ties use stable actor paths. */
	UPROPERTY()
	int32 Priority = 0;

	UPROPERTY()
	TObjectPtr<UYogLevelEnvironmentProfile> Profile;

	UPROPERTY()
	bool bOverrideProfile = false;

	UPROPERTY()
	FYogLevelEnvironmentSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Environment", meta=(MultiLine="true"))
	FString Notes;

	FYogLevelEnvironmentSettings GetEffectiveSettings() const;
};

UCLASS(BlueprintType)
class CELESLIGHTRUNTIME_API AYogLevelEnvironment : public AActor
{
	GENERATED_BODY()
public:
	AYogLevelEnvironment();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Level Environment")
	TObjectPtr<UYogLevelEnvironmentComponent> EnvironmentComponent;
};

/** Compatibility resolver: always supplies neutral native-rendering data. */
CELESLIGHTRUNTIME_API FYogLevelEnvironmentSettings GetYogGlobalEnvironmentSettings();

/** Highest-priority active level override, or null when the world inherits global settings. */
CELESLIGHTRUNTIME_API const AYogLevelEnvironment* FindYogLevelEnvironmentOverride(const UWorld* World);

/** Compatibility resolver. All world types retain native rendering. */
CELESLIGHTRUNTIME_API FYogLevelEnvironmentSettings ResolveYogLevelEnvironment(const UWorld* World);
