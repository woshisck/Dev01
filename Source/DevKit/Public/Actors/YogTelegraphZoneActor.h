#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogTelegraphZoneActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

/**
 * Ground-projected pre-attack warning zone (telegraph).
 *
 * Spawned by UYogAnimNotifyState_Telegraph over the windup section of an attack montage,
 * then destroyed when the window ends. Parallel to AYogAimArcActor but general-purpose:
 * the material params are driven by the BP subclass via BP_OnShow so any "alert" shader can
 * be plugged in. As a convenience the base also sets the common scalar/vector params
 * ("ArcRadius", "HalfAngle", "Color") when they exist on the material.
 *
 * Usage:
 *   1. Make a BP subclass, assign ZoneMaterial (your warning-zone decal material).
 *   2. Reference that BP class on the notify state's TelegraphClass.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API AYogTelegraphZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AYogTelegraphZoneActor();

	/** Show the zone and push size/shape params to the material. */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void Show(float RadiusCm, float HalfAngleDeg, FLinearColor Color);

	/** Hide the zone (destroy is handled by the notify state). */
	UFUNCTION(BlueprintCallable, Category = "Telegraph")
	void Hide();

protected:
	virtual void BeginPlay() override;

	/** BP hook: drive your warning-zone material params (fill/pulse/color) here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Telegraph")
	void BP_OnShow(float RadiusCm, float HalfAngleDeg, FLinearColor Color);

	UFUNCTION(BlueprintImplementableEvent, Category = "Telegraph")
	void BP_OnHide();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> ZoneDecal;

	/** Warning-zone decal material; set on the BP subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInterface> ZoneMaterial;

	UPROPERTY(BlueprintReadOnly, Category = "Telegraph")
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;
};
