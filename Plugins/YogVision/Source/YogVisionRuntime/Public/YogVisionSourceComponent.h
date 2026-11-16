#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "YogVisionSourceComponent.generated.h"

class ULocalLightComponent;

UCLASS(ClassGroup=(YogVision), meta=(BlueprintSpawnableComponent, DisplayName="火光视野源"))
class YOGVISIONRUNTIME_API UYogVisionSourceComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UYogVisionSourceComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="启用视野源", ToolTip="此组件显式授权光照揭示视野；场景其他灯光不会自动获得视野能力。")) bool bVisionEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="光照揭示半径", ToolTip="厘米。圆形视野范围；绑定点光后还受光照衰减半径限制。", ClampMin="0", ClampMax="20000")) float Radius = 650.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="揭示强度", ToolTip="0至1。光源揭开迷雾的强度，0等同熄灭。", ClampMin="0", ClampMax="1")) float Strength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="自动绑定同Actor点光", ToolTip="按同一Actor的点光开关与衰减范围揭示视野。基础版本只支持点光圆形视野。")) bool bBindOwnerLight = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="射线起点高度", ToolTip="厘米。水平墙体遮挡采样相对于组件位置的高度。", ClampMin="0", ClampMax="500")) float TraceHeight = 70.f;
    UFUNCTION(BlueprintCallable, Category="YOG|Vision", meta=(ToolTip="绑定点光组件：灯光熄灭、隐藏或销毁时停止揭示。基础版本不支持聚光锥体；传空可解除显式绑定。")) void BindLight(ULocalLightComponent* Light);
    float GetEffectiveRadius() const;
    float GetEffectiveStrength() const;
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
private:
    ULocalLightComponent* ResolveBoundLight() const;
    UPROPERTY(Transient) TWeakObjectPtr<ULocalLightComponent> BoundLight;
    bool bHadExplicitLightBinding = false;
};
