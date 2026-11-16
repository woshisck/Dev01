#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YogVisionTargetComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup=(YogVision), meta=(BlueprintSpawnableComponent, DisplayName="灵视感知目标"))
class YOGVISIONRUNTIME_API UYogVisionTargetComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYogVisionTargetComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="允许被感知", ToolTip="关闭后不显示灵体并立即还原原始身体可见状态；敌人死亡时原生逻辑会关闭。")) bool bTargetEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵体显现半径", ToolTip="厘米。仅控制未知身份灵体的占地显现大小。", ClampMin="1", ClampMax="1000")) float SpiritEchoRadius = 90.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵体显现强度", ToolTip="0至2。乘以玩家灵视强度，0不产生感知灵体。", ClampMin="0", ClampMax="2")) float SpiritIntensity = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="表现", meta=(DisplayName="视野外隐藏原始身体", ToolTip="只隐藏本地渲染的身体、组件血条和特效，不改变AI、碰撞或Actor隐藏状态；关闭预览会还原原始可见状态。")) bool bHideBodyOutsideSight = true;
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsDirectlyVisible() const { return bDirectlyVisible; }
    UFUNCTION(BlueprintPure, Category="YOG|Vision") bool IsSpiritSensed() const { return bSpiritSensed; }
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void SetTargetEnabled(bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void SetSpiritAppearance(float Radius, float Intensity);
    void SetPerception(bool bDirect, bool bSpirit);
    void UpdatePresentation(bool bVisionEnabled, bool bDirect);
    void RestorePresentation();
    bool IsPresentationEligible() const;
    void ApplyRequestedVisibility(UPrimitiveComponent* Component, bool bRequestedVisible);
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
private:
    bool bDirectlyVisible = false;
    bool bSpiritSensed = false;
    bool bPresentationHidden = false;
    TMap<TWeakObjectPtr<UPrimitiveComponent>, bool> VisibilitySnapshot;
};
