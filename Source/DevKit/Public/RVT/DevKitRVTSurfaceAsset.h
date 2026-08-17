#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DevKitRVTSurfaceAsset.generated.h"

class UMaterialInterface;
class UStaticMesh;

/** Distinguishes RVT-only plane decals from visible ground objects. */
UENUM(BlueprintType)
enum class EDevKitRVTSurfaceAssetType : uint8
{
	PlaneDecal UMETA(DisplayName = "RVT Plane Decal"),
	VisibleObject UMETA(DisplayName = "Visible RVT Object")
};

/** Controls whether a visible RVT object keeps its source mesh in the main pass. */
UENUM(BlueprintType)
enum class EDevKitRVTSurfaceGeometryPolicy : uint8
{
	/** Backward-compatible default: planes are RVT-only and visible objects keep their mesh. */
	UseAssetTypeDefault UMETA(DisplayName = "Use Asset Type Default"),
	RVTOnly UMETA(DisplayName = "RVT Projection Only"),
	AlwaysVisible UMETA(DisplayName = "Always Keep Source Mesh"),
	QualityScaled UMETA(DisplayName = "Quality Scaled")
};

/**
 * Reusable artist-authored definition consumed by the RVT surface library and
 * by independently editable RVT surface instance actors.
 */
UCLASS(BlueprintType, meta = (DisplayName = "RVT Surface Asset"))
class DEVKIT_API UDevKitRVTSurfaceAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UDevKitRVTSurfaceAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Asset", AssetRegistrySearchable)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Asset", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Asset", AssetRegistrySearchable)
	EDevKitRVTSurfaceAssetType AssetType = EDevKitRVTSurfaceAssetType::PlaneDecal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Visual")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Visual")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Rendering", meta = (ClampMin = "-1000", ClampMax = "1000"))
	int32 Priority = 0;

	/**
	 * Quality Scaled keeps the source mesh on PC High/Epic and uses only its RVT projection on
	 * Mid/Low. A platform DeviceProfile may force projection without changing the user's quality.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Rendering")
	EDevKitRVTSurfaceGeometryPolicy GeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	FVector DefaultScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	float ZOffset = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	bool bAlignToNormal = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Placement")
	bool bRandomYaw = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Rendering")
	bool bBindWorldHeight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Interaction")
	bool bEnableCollision = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Rendering")
	bool bCastShadow = false;
};
