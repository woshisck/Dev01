#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/AbilityData.h"
#include "YogTelegraphZoneActor.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Resolved footprint of a telegraph zone.
 *
 * Mirrors the geometry of the FYogHitboxType that the matching AN_MeleeDamage will use, so the
 * warning zone and the actual hit describe the same area. UYogAnimNotifyState_Telegraph fills
 * this either from that notify or from its own hand-set fields.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FYogTelegraphShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	EHitBoxType ShapeType = EHitBoxType::Annulus;

	/** Outer reach in cm: fan/circle radius, or forward length for Square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float Radius = 300.f;

	/** Sector half-angle in degrees. Unused by Square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float HalfAngle = 45.f;

	/** Inner cull radius in cm; the fan's inner edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float InnerRadius = 0.f;

	/** Lateral half-extent in cm for Square shapes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph", meta = (ClampMin = "0.0"))
	float HalfWidth = 0.f;

	/** Footprint placement relative to the owner (+X forward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telegraph")
	FVector LocalOffset = FVector::ZeroVector;
};

/**
 * Ground pre-attack warning zone (telegraph).
 *
 * Spawned by UYogAnimNotifyState_Telegraph over the windup section of an attack montage,
 * then destroyed when the window ends. The footprint is a flat unlit plane laid on the ground,
 * not a deferred decal - matching UMontageVFXBindingComponent's annulus indicator. A decal
 * would need every floor surface to have Receives Decals enabled, which is not guaranteed here.
 *
 * The plane's local +X is the owner's forward, +Y lateral, and the mask materials read that
 * from UV0. The base sets the common params ("ArcRadius", "HalfAngle", "InnerRadiusRatio",
 * "Color") when they exist on the material; BP_OnShow is the hook for anything else.
 *
 * Usage:
 *   1. Make a BP subclass per shape, assign ZoneMaterial (Surface / Unlit / Translucent).
 *   2. Reference those BP classes on the notify state's ZoneClassByShape / TelegraphClass.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API AYogTelegraphZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AYogTelegraphZoneActor();

	/** Show the zone and push size/shape params to the material. */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void Show(const FYogTelegraphShape& Shape, FLinearColor Color);

	/** Hide the zone (destroy is handled by the notify state). */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void Hide();

	/**
	 * Begin the 0 -> 1 outer-edge fill that runs over the telegraph window.
	 * Duration is in animation time, so it already accounts for montage play rate.
	 * A non-positive duration shows the zone fully filled, since there is no timeline to run.
	 */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void StartProgress(float Duration);

	/** Advance the fill by one animation-time delta. No-op until StartProgress is called. */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void AdvanceProgress(float DeltaSeconds);

protected:
	/** Pushes "OuterRadiusRatio"; materials without the param ignore it. */
	void PushProgressRatio(float Ratio);

	virtual void BeginPlay() override;

	/** Creates the MID on first use and warns if ZoneMaterial was never assigned. */
	UMaterialInstanceDynamic* EnsureDynamicMaterial();

	/** BP hook: drive your warning-zone material params (fill/pulse/color) here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Telegraph")
	void BP_OnShow(const FYogTelegraphShape& Shape, FLinearColor Color);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telegraph")
	void BP_OnHide();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ZonePlane;

	/** Flat plane lying in XY. Defaults to the engine plane (100 cm across). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UStaticMesh> ZoneMesh;

	/** Warning-zone material; set on the BP subclass. Surface / Unlit / Translucent. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInterface> ZoneMaterial;

	/** Base width of ZoneMesh in cm, used to convert the zone diameter into a scale. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph", meta = (ClampMin = "1.0"))
	float MeshSize = 100.f;

	/** Lift above the owner's feet, to avoid z-fighting with the floor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	float GroundZOffset = 8.f;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

private:
	/** Length of the telegraph window in animation time; 0 means no fill is running. */
	float ProgressDuration = 0.f;
	float ProgressElapsed = 0.f;
};
