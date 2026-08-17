#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DevKitRVTSurfaceInstanceActor.generated.h"

struct FPropertyChangedEvent;
enum class EDevKitRVTSurfaceAssetType : uint8;
enum class EDevKitRVTSurfaceGeometryPolicy : uint8;
class UDevKitRVTSurfaceAsset;
class UInstancedStaticMeshComponent;
class URuntimeVirtualTexture;
class USceneComponent;

/**
 * An RVT surface container whose ISM instances remain individually selectable
 * and transformable in the level editor.
 */
UCLASS(BlueprintType, meta = (DisplayName = "RVT Surface Instance Actor"))
class DEVKIT_API ADevKitRVTSurfaceInstanceActor : public AActor
{
	GENERATED_BODY()

public:
	ADevKitRVTSurfaceInstanceActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Reapplies rendering and interaction settings without removing instances. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RVT Surface|Instances", meta = (DisplayName = "Apply Surface Asset"))
	void ApplySurfaceAsset();

	/**
	 * Initializes an empty controller, or reapplies the same asset to an existing controller.
	 * Rebinding populated instances to a different asset is rejected so transforms cannot silently
	 * change mesh/material semantics. This is the supported editor/Python migration entry point.
	 */
	UFUNCTION(BlueprintCallable, Category = "RVT Surface|Instances", meta = (DisplayName = "Initialize Surface Asset"))
	bool InitializeSurfaceAsset(UDevKitRVTSurfaceAsset* InSurfaceAsset);

	/** Adds one exact world-space transform. Placement defaults are applied by the calling placement tool. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RVT Surface|Instances", meta = (DisplayName = "Add Surface Instance"))
	int32 AddSurfaceInstance(const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RVT Surface|Instances", meta = (DisplayName = "Clear Surface Instances"))
	void ClearSurfaceInstances();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "RVT Surface|Instances", meta = (DisplayName = "Get Surface Instance Count"))
	int32 GetSurfaceInstanceCount() const;

	/** Pure policy resolver used by runtime application and automation tests. */
	static bool ShouldRenderSourceGeometry(
		EDevKitRVTSurfaceAssetType AssetType,
		EDevKitRVTSurfaceGeometryPolicy GeometryPolicy,
		bool bPreferBatchedGeometryProxy,
		bool bForceProjectionOnly,
		bool bRequiresGameplayCollision = false);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> InstanceComponent;

	/** Managed by the RVT surface library. Rebinding an actor with existing instances is intentionally disallowed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RVT Surface")
	TObjectPtr<UDevKitRVTSurfaceAsset> SurfaceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVT Surface|Virtual Texture")
	TObjectPtr<URuntimeVirtualTexture> SurfaceRVT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RVT Surface|Virtual Texture")
	TObjectPtr<URuntimeVirtualTexture> HeightRVT;
};
