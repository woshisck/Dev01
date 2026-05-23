#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Water/WaterInteractionReceiver.h"
#include "Water/WaterInteractionTypes.h"
#include "InteractiveWaterVolume.generated.h"

class UBoxComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

UCLASS(Blueprintable)
class DEVKIT_API AInteractiveWaterVolume : public AActor, public IWaterInteractionReceiver
{
	GENERATED_BODY()

public:
	AInteractiveWaterVolume();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanReceiveWaterImpulse_Implementation(FVector WorldLocation, float Radius) const override;
	virtual void AddWaterImpulse_Implementation(
		FVector WorldLocation,
		float Radius,
		float Strength,
		EWaterImpulseType Type,
		FVector Direction,
		float MassScale) override;

	UFUNCTION(BlueprintCallable, Category = "Water")
	void SetPerformanceTier(EWaterPerformanceTier NewTier);

	UFUNCTION(BlueprintCallable, Category = "Water")
	UTextureRenderTarget2D* GetInteractionRenderTarget() const { return InteractionRenderTarget; }

	UFUNCTION(BlueprintCallable, Category = "Water")
	UStaticMeshComponent* GetWaterSurfaceComponent() const { return WaterSurface; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UStaticMeshComponent> WaterSurface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UDecalComponent> CausticsDecal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Quality")
	EWaterPerformanceTier PerformanceTier = EWaterPerformanceTier::Low;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Quality", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 LowTierRenderTargetSize = 512;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Quality", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 HighTierRenderTargetSize = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Material")
	TObjectPtr<UMaterialInterface> WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Material")
	TObjectPtr<UMaterialInterface> CausticsMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Material")
	TObjectPtr<UMaterialInterface> InteractionStampMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Material")
	TObjectPtr<UMaterialInterface> InteractionDecayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Material")
	FWaterVisualTuning VisualTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Interaction", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxMaterialImpulses = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Interaction", meta = (ClampMin = "0.05"))
	float DefaultImpulseLifetime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Interaction")
	bool bSpawnNiagaraOnImpulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Water|Interaction")
	TMap<EWaterImpulseType, TObjectPtr<UNiagaraSystem>> SplashNiagaraByType;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Water|Runtime")
	TObjectPtr<UTextureRenderTarget2D> InteractionRenderTarget;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Water|Runtime")
	TObjectPtr<UTextureRenderTarget2D> ScratchRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CausticsMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StampMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DecayMID;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Water|Runtime")
	TArray<FWaterImpulseRuntimeState> ActiveImpulses;

	void InitializeMaterials();
	void InitializeRenderTargets();
	void ApplyVisualTuningToMaterial(UMaterialInstanceDynamic* TargetMID);
	void TickImpulses(float DeltaSeconds);
	void PushImpulseParametersToMaterial();
	void DrawImpulseToRenderTarget(const FWaterImpulseRuntimeState& Impulse);
	void DecayInteractionRenderTarget(float DeltaSeconds);
	void SwapInteractionRenderTargets();
	void SpawnImpulseNiagara(const FWaterImpulseRuntimeState& Impulse) const;
	float GetLifetimeForImpulse(EWaterImpulseType Type) const;
	float GetFoamStrengthForImpulse(EWaterImpulseType Type, float Strength, float MassScale) const;
	float GetTurbidityStrengthForImpulse(EWaterImpulseType Type, float Strength, float MassScale) const;
	FVector4 GetWaterBoundsVector() const;
};
