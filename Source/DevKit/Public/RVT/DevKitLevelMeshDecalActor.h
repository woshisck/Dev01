#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DevKitLevelMeshDecalActor.generated.h"

struct FPropertyChangedEvent;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;

/**
 * Base actor for level-authored mesh decals that use ISM/HISM components.
 *
 * A Blueprint-supplied box is used as the shared, stable culling bound. The class deliberately
 * creates no native scene component so reparenting an existing Blueprint preserves its SCS root
 * and attachment topology. This class intentionally does not alter RVT or main-pass policy.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Level Mesh Decal Actor"))
class DEVKIT_API ADevKitLevelMeshDecalActor : public AActor
{
	GENERATED_BODY()

public:
	ADevKitLevelMeshDecalActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Reapplies the shared-bounds and no-distance-cull policy to every owned ISM/HISM. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Level Mesh Decal|Culling",
		meta = (DisplayName = "Apply Culling Policy"))
	void ApplyCullingPolicy();

	/** Preferred SCS component name for the Blueprint-supplied bounds box. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Mesh Decal|Culling")
	FName BoundsProxyName = TEXT("BoundsProxy");

	/** Optional component-tag fallback when the bounds box uses a different name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Level Mesh Decal|Culling")
	FName BoundsProxyTag = TEXT("LevelMeshDecalBounds");

	/** Local-space half extent used by the shared culling bound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Mesh Decal|Culling",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector DesiredBoundsExtent = FVector(6500.0, 6500.0, 1500.0);

	/**
	 * Optional safety net for incomplete Blueprints. Prefer adding/tagging a BoxComponent instead;
	 * BoundsScale is less precise and can increase overdraw.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Mesh Decal|Culling|Fallback")
	bool bUseBoundsScaleFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Mesh Decal|Culling|Fallback",
		meta = (EditCondition = "bUseBoundsScaleFallback", ClampMin = "1.0", UIMin = "1.0"))
	float FallbackBoundsScale = 10.0f;

private:
	UBoxComponent* FindBoundsProxy() const;
	void RouteComponentBoundsThroughProxy(
		UInstancedStaticMeshComponent* InstanceComponent,
		UBoxComponent* BoundsProxy);
	void ApplyPolicyToInstanceComponent(
		UInstancedStaticMeshComponent* InstanceComponent,
		UBoxComponent* BoundsProxy);

	bool bLoggedMissingBoundsProxy = false;
};
