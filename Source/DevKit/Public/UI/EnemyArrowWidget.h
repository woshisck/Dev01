#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyArrowWidget.generated.h"

class UCanvasPanel;
class UImage;
class UYogAbilitySystemComponent;
class AEnemyCharacterBase;

/**
 * 敌人方向箭头 HUD 覆盖层。
 * 当玩家 AppearDelay 秒内未受伤且屏幕内看不到任何存活敌人时，
 * 在屏幕边缘显示最多 MaxArrows 个小三角箭头，分别指向最近的敌人。
 *
 * Designer 层级：
 *   Canvas Panel（命名 "RootCanvas"，全屏，Anchors 全屏 Offset 全 0）
 *
 * Details 配置：ArrowTexture（顶点朝上三角形）、AppearDelay、MaxArrows、ArrowSize、EdgeMargin、ArrowColor
 * HUD 蓝图：Details → Enemy Arrow → Enemy Arrow Widget Class = WBP_EnemyArrow
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UEnemyArrowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCanvasPanel> RootCanvas;

    /** 箭头贴图（顶点朝上/-Y 方向为 0° 基准） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TObjectPtr<UTexture2D> ArrowTexture;

    /** 屏幕内无敌人且无受伤超过该秒数后显示箭头 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0.1"))
    float AppearDelay = 1.5f;

    /** 同时最多显示几个箭头 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxArrows = 3;

    /** 箭头图标像素尺寸 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "8"))
    float ArrowSize = 32.f;

    /** 距屏幕边缘的留白（像素） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0"))
    float EdgeMargin = 60.f;

    /**
     * 判断"敌人在屏幕内"时向内缩减的像素量（屏幕空间）。
     * 值越大 → 敌人越早被判定为"离屏"→ 箭头越早出现。
     * 默认 150px。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0"))
    float OnScreenShrink = 150.f;

    /**
     * 超过此世界距离（cm）的敌人强制视为"离屏"，无论实际投影结果如何。
     * 0 = 不启用（只用屏幕投影判断）。
     * 推荐值：1500~3000（约 15~30 米）。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0"))
    float ForceOffScreenDistance = 0.f;

    /** 投影点相对脚底的 Z 抬升量（cm），修正斜视角下的显示偏差，默认 60 ≈ 角色腰部 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    float ArrowProjectionZOffset = 60.f;

    /**
     * 箭头旋转基准偏移（°）。根据贴图朝向调整：
     *  顶点朝上（-Y）→ 90（默认）
     *  顶点朝右（+X）→ 0
     *  顶点朝下（+Y）→ -90
     *  顶点朝左（-X）→ 180
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    float ArrowAngleOffset = 90.f;

    /** 箭头颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    FLinearColor ArrowColor = FLinearColor(1.f, 0.8f, 0.2f, 0.9f);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY()
    TArray<TObjectPtr<UImage>> ArrowImages;

    TWeakObjectPtr<UYogAbilitySystemComponent> CachedPlayerASC;
    float LastCombatEventTime = -999.f;

    void RebuildArrowPool();

    UFUNCTION()
    void OnPlayerDamageTaken(UYogAbilitySystemComponent* SourceASC, float Damage);

    /** 返回 true 表示在摄像机前方且在视口矩形内；OutScreenPos 为像素坐标（后方敌人已镜像） */
    bool IsOnScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;

    /** 将屏幕坐标钳制到距屏幕边缘 EdgeMargin 的矩形上 */
    FVector2D ClampToScreenEdge(const FVector2D& ScreenPos,
                                const FVector2D& Center,
                                const FVector2D& ViewportSize) const;

    /** 计算箭头旋转角度（°），使贴图顶点指向 EnemyScreenPos 方向 */
    float CalcArrowAngle(const FVector2D& EnemyScreenPos, const FVector2D& Center) const;
};
