// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectKey.h"
#include "YogCameraOcclusionFadeComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class APlayerCameraManager;
class FReferenceCollector;

USTRUCT()
struct DEVKIT_API FYogOcclusionFadeTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	float CurrentAlpha = 1.0f;
	float TargetAlpha = 1.0f;
	bool bHitThisTrace = false;
};

UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UYogCameraOcclusionFadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYogCameraOcclusionFadeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	bool bEnableOcclusionFade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0"))
	float TraceInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0"))
	float TraceRadius = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	FVector PlayerTargetOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	bool bOnlyFadeTaggedOccluders = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	FName OccluderTag = TEXT("CameraOccluder");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinVisibleAlpha = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.01"))
	float FadeOutSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion", meta = (ClampMin = "0.01"))
	float FadeInSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion")
	FName FadeScalarParameterName = TEXT("CameraOcclusionAlpha");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion|Material")
	TObjectPtr<UMaterialInterface> OcclusionFadeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion|Debug")
	bool bDrawDebugOcclusionTrace = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Occlusion|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawDuration = 0.08f;

	bool IsFadeAllowedForComponent(const UPrimitiveComponent* Component) const;

private:
	void RunOcclusionTrace();
	void UpdateFadeInterpolation(float DeltaTime);
	void MarkAllTargetsVisible();
	void AddOrUpdateFadeTarget(UPrimitiveComponent* Component);
	void CacheMaterialsForTarget(FYogOcclusionFadeTarget& Target, UPrimitiveComponent* Component);
	void ApplyAlphaToTarget(FYogOcclusionFadeTarget& Target);
	void RestoreTargetMaterials(FYogOcclusionFadeTarget& Target);
	void RestoreAllMaterials();
	APlayerCameraManager* GetOwnerCameraManager() const;

	TMap<TObjectKey<UPrimitiveComponent>, FYogOcclusionFadeTarget> FadeTargets;

	float TimeSinceLastTrace = 0.0f;
};
