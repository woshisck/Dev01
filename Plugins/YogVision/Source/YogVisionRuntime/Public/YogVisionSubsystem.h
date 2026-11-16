#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YogVisionSettings.h"
#include "YogVisionSubsystem.generated.h"

class APostProcessVolume;
class UMaterialInstanceDynamic;
class UTexture2D;
class UYogVisionSourceComponent;
class UYogVisionTargetComponent;
class UYogVisionObserverComponent;

UCLASS()
class YOGVISIONRUNTIME_API UYogVisionSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual bool IsTickableInEditor() const override { return true; }
    virtual bool IsTickableWhenPaused() const override { return true; }
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void SetPreviewObserver(AActor* Observer);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void ClearPreview();
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void ApplyParameters(FYogVisionParameters NewParameters);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void SetEnabled(bool bNewEnabled);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision", meta=(DisplayName="设置视野表现淡出")) void SetPresentationFade(float Fade);
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsEnabled() const { return bEnabled; }
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsPresentationActive() const;
    UFUNCTION(BlueprintPure, Category="YOG|Vision") FString GetStatusText() const;
    UFUNCTION(BlueprintPure, Category="YOG|Vision") UMaterialInstanceDynamic* GetMaterialInstance() const { return MaterialInstance; }
    UFUNCTION(BlueprintPure, Category="YOG|Vision") FYogVisionParameters GetParameters() const { return Parameters; }
    // Editor save scopes temporarily disable presentation without ending the owned preview.
    bool HasPreviewSnapshot() const { return bHasPreviewSnapshot; }
    AActor* GetPreviewObserver() const { return PreviewObserver.Get(); }
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsDirectlyVisible(AActor* Actor) const;
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsSpiritSensed(AActor* Actor) const;
    void RegisterSource(UYogVisionSourceComponent* Source);
    void UnregisterSource(UYogVisionSourceComponent* Source);
    void RegisterTarget(UYogVisionTargetComponent* Target);
    void UnregisterTarget(UYogVisionTargetComponent* Target);

private:
    struct FVisionSource
    {
        FVector Center = FVector::ZeroVector;
        float Radius = 0.f;
        float Strength = 1.f;
        float TraceHeight = 70.f;
        TWeakObjectPtr<AActor> Owner;
    };
    bool CreateRenderObjects();
    void RemoveRenderObjects();
    AActor* ResolveObserver() const;
    void UpdatePerception(AActor* Observer, float UpdateDelta);
    void PushParameters(const FYogVisionParameters& Effective);
    bool HasLineOfSight(const FVisionSource& Source, const FVector& Target, AActor* TargetActor) const;
    void UploadVisibility(const TArray<FColor>& Pixels);

    UPROPERTY(Transient) TObjectPtr<APostProcessVolume> Volume;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> VisibilityTexture;
    TWeakObjectPtr<AActor> PreviewObserver;
    TSet<TWeakObjectPtr<UYogVisionSourceComponent>> Sources;
    TSet<TWeakObjectPtr<UYogVisionTargetComponent>> Targets;
    TMap<TWeakObjectPtr<UYogVisionSourceComponent>, float> SmoothedSourceRadii;
    TMap<TWeakObjectPtr<UYogVisionTargetComponent>, FVector> PreviousTargetPositions;
    FYogVisionParameters Parameters;
    FYogVisionParameters SavedPreviewParameters;
    TWeakObjectPtr<AActor> SavedPreviewObserver;
    bool bHasPreviewSnapshot = false;
    bool bSavedEnabled = false;
    bool bEnabled = false;
    bool bMaterialLoadAttempted = false;
    float UpdateAccumulator = 0.f;
    float SmoothedSightRadius = 0.f;
    float SmoothedSpiritStrength = 0.f;
    float PresentationFade = 1.f;
    int32 ActiveSourceCount = 0;
    int32 ActiveEchoCount = 0;
};
