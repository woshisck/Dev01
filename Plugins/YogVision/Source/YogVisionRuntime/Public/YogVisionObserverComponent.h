#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YogVisionSettings.h"
#include "YogVisionObserverComponent.generated.h"

USTRUCT()
struct FYogVisionModifier
{
    GENERATED_BODY()
    float SightBonus = 0.f;
    float SpiritBonus = 0.f;
    float SpiritStrengthBonus = 0.f;
};

UCLASS(ClassGroup=(YogVision), meta=(BlueprintSpawnableComponent, DisplayName="视野观察者"))
class YOGVISIONRUNTIME_API UYogVisionObserverComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYogVisionObserverComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="启用观察者", ToolTip="本地玩家拥有此组件后自动使用基础视野及其装备修正；关闭将恢复当前画面。")) bool bVisionEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="射线起点高度", ToolTip="厘米。以角色位置加此高度发出水平视野遮挡采样。", ClampMin="0", ClampMax="500")) float EyeHeight = 70.f;
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void SetVisionModifier(FName Token, float SightBonus, float SpiritBonus, float SpiritStrengthBonus);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void RemoveVisionModifier(FName Token);
    UFUNCTION(BlueprintCallable, Category="YOG|Vision") void ClearVisionModifiers();
    FYogVisionParameters ResolveParameters(const FYogVisionParameters& Base) const;
private:
    TMap<FName, FYogVisionModifier> Modifiers;
};
