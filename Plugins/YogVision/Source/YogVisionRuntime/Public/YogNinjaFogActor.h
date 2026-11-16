#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "YogNinjaFogActor.generated.h"

class USceneComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UExponentialHeightFogComponent;
class UMaterialInterface;

/** Mounts Ninja's original passive FVOL renderer; it owns no density or fluid solver. */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Ninja 原生体积雾", PrioritizeCategories="Ninja素材 雾体范围 原生雾"))
class YOGVISIONRUNTIME_API AYogNinjaFogActor : public AActor
{
    GENERATED_BODY()
public:
    AYogNinjaFogActor();
    virtual void PostRegisterAllComponents() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    virtual void BeginDestroy() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ninja素材") TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ninja素材") TObjectPtr<UNiagaraComponent> NinjaVolume;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="原生雾") TObjectPtr<UExponentialHeightFogComponent> NativeFog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja素材", meta=(DisplayName="Ninja原始体积Niagara")) TObjectPtr<UNiagaraSystem> NinjaSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ninja素材", meta=(DisplayName="Ninja体积材质",ToolTip="仅接受Volume域材质。此宿主不修改材质资产，不运行NinjaLive流体。")) TObjectPtr<UMaterialInterface> FogMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体范围", meta=(DisplayName="原生EmitterExtents（厘米）")) FVector EmitterExtents=FVector(6200,7200,2800);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="启用体积雾")) bool bFogEnabled=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="请求原生体积雾渲染开关",ToolTip="仅刷新、构造和运行启动时请求；关闭或销毁后有条件恢复原值，不持续抢写外部设置。")) bool bRequestNativeFog=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="原生雾可视距离（厘米）",ClampMin="100",ClampMax="50000")) float NativeViewDistance=12000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="镜头前雾起始距离（厘米）",ClampMin="0",ClampMax="50000")) float FogStartDistance=1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="镜头前渐入距离（厘米）",ClampMin="0",ClampMax="50000")) float FadeDistance=700.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="原生雾", meta=(DisplayName="原生散射方向性",ClampMin="-0.9",ClampMax="0.9")) float ScatteringDistribution=.1f;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="原生雾", meta=(DisplayName="刷新Ninja原生雾")) void RefreshNinjaFog();

private:
    bool HasValidNinjaMaterial() const;
    void RequestNativeFog();
    void ReleaseNativeFogLease();
    void ClearStartupRetry();
    // Deliberately not reflected: a PIE copy must acquire its own lease.
    bool bOwnFogLease=false;
    int32 PreviousFogValue=0;
    uint32 PreviousFogPriority=0;
    FTimerHandle StartupFogRetry;
};
