#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortalDirectionWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTextBlock;
class UHorizontalBox;
class APortal;

/**
 * 关卡结算后的传送门方位指引 HUD overlay。
 *
 * 仿 UEnemyArrowWidget 实现：在屏幕边缘对所有"开启 + 屏幕外"的 Portal 画箭头 + 房间名标签。
 *
 * 由 AYogHUD 通过 SetActive(true) 在 EnterArrangementPhase 末尾启用；
 * 玩家进入任意门 Box（PendingPortal != null）→ 全部箭头隐藏。
 *
 * Designer：
 *   Canvas Panel "RootCanvas"（全屏，Anchor 全屏）
 *
 * 箭头单元由 C++ 在 SetActive 时根据传入的 Portal 列表动态创建。
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UPortalDirectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * 启用/禁用方位指引。启用时根据传入 Portal 列表重建箭头池。
     * 由 AYogHUD::ShowPortalGuidance / HidePortalGuidance 调用。
     */
    UFUNCTION(BlueprintCallable, Category = "PortalDirection")
    void SetActive(bool bInActive, const TArray<APortal*>& OpenPortals);

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCanvasPanel> RootCanvas;

    /** 箭头贴图（约定顶点朝上=-Y，0° 基准） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TObjectPtr<UTexture2D> ArrowTexture;

    /** 箭头图标像素尺寸 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "8"))
    float ArrowSize = 36.f;

    /** 距屏幕边缘的留白（像素） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0"))
    float EdgeMargin = 80.f;

    /** 屏幕内可视判定向内缩减（像素）。值越大越早显示箭头 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0"))
    float OnScreenShrink = 100.f;

    /** 投影点相对门 Pivot 的 Z 抬升（cm），与门视觉中心对齐 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    float ArrowProjectionZOffset = 80.f;

    /** 贴图朝向补偿角（顶点朝上时填 90°） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    float ArrowAngleOffset = 90.f;

    /** 箭头颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    FLinearColor ArrowColor = FLinearColor(0.95f, 0.85f, 0.55f, 0.92f);

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    // 箭头单元：每个 Portal 对应一份（CanvasPanel 的子）
    struct FArrowUnit
    {
        TWeakObjectPtr<APortal> Portal;
        TObjectPtr<UHorizontalBox> Container;
        TObjectPtr<UImage>         ArrowImage;
        TObjectPtr<UTextBlock>     LabelText;
    };

    UPROPERTY()
    TArray<TWeakObjectPtr<APortal>> TrackedPortals;

    TArray<FArrowUnit> Units;
    bool bActive = false;

    void RebuildArrowUnits();
    void ClearArrowUnits();

    bool IsOnScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;
    FVector2D ClampToScreenEdge(const FVector2D& ScreenPos,
                                const FVector2D& Center,
                                const FVector2D& ViewportSize) const;
    float CalcArrowAngle(const FVector2D& TargetScreenPos,
                         const FVector2D& Center) const;
};
