#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryAssetLabel.h"
#include "GameplayTagContainer.h"
#include "RVT/DevKitRVTSurfaceAsset.h"
#include "DevKitDecalAsset.generated.h"

class URuntimeVirtualTexture;

/** Rendering backends supported by the unified decal editor. */
UENUM(BlueprintType)
enum class EDevKitDecalBackend : uint8
{
	RVTPlane UMETA(DisplayName = "RVT Plane"),
	RVTVisibleMesh UMETA(DisplayName = "RVT Ground Object"),
	MeshDecal UMETA(DisplayName = "Static Mesh Decal"),
	StaticMeshOverlay UMETA(DisplayName = "Static Mesh Overlay"),
	DeferredProjection UMETA(DisplayName = "Deferred Decal"),
	Unsupported UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EDevKitDecalUsage : uint8
{
	StaticAuthoring,
	RuntimeDynamic,
	Generated,
	GameplayCritical
};

USTRUCT(BlueprintType)
struct DEVKIT_API FDevKitDecalRepresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation")
	FName Platform = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MinQuality = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MaxQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation")
	EDevKitDecalBackend Backend = EDevKitDecalBackend::RVTPlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation")
	bool bKeepSourceGeometry = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Representation")
	bool bRuntimeSwitchable = false;
};

/** Unified artist-facing definition. Existing RVT assets remain valid through LegacyRVTAsset. */
UCLASS(BlueprintType, meta = (DisplayName = "Decal Asset"))
class DEVKIT_API UDevKitDecalAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDevKitDecalAsset();
	virtual void PostInitProperties() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity", AssetRegistrySearchable)
	FGuid DefinitionGuid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity", AssetRegistrySearchable)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity", meta = (Categories = "DevKit.Decal"))
	FGameplayTagContainer UsageTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Rendering", AssetRegistrySearchable)
	EDevKitDecalBackend Backend = EDevKitDecalBackend::RVTPlane;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Rendering")
	TArray<FDevKitDecalRepresentation> Representations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Rendering")
	EDevKitRVTSurfaceGeometryPolicy GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	/** Default projector extents for DeferredProjection records. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source", meta = (EditCondition = "Backend == EDevKitDecalBackend::DeferredProjection"))
	FVector DefaultDecalSize = FVector(64.f, 64.f, 64.f);

	/** Deferred decals are individual proxies; this is intentionally not used as an ISM batch key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source", meta = (EditCondition = "Backend == EDevKitDecalBackend::DeferredProjection"))
	int32 DeferredSortOrder = 0;

	/** Gameplay-critical geometry must keep a source representation on every quality tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Rendering")
	bool bEnableCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Rendering")
	bool bCastShadow = false;

	/** Compatibility link to the existing RVT surface library asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source")
	TObjectPtr<UDevKitRVTSurfaceAsset> LegacyRVTAsset = nullptr;

	/** Resolved RVT targets copied from the legacy surface controller during migration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source")
	TObjectPtr<URuntimeVirtualTexture> SurfaceRVT = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Source")
	TObjectPtr<URuntimeVirtualTexture> HeightRVT = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	FTransform DefaultTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement", meta = (ClampMin = "0"))
	float MinSpacing = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	bool bAllowOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Validation")
	bool bRequiresSurfaceRVT = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Validation")
	bool bRequiresWorldHeightRVT = false;

	UFUNCTION(BlueprintPure, Category = "Decal Asset")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "Decal Asset")
	bool IsRuntimeAddressable() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
