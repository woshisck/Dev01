#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "RuneInfoCardWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanel;
class UCanvasPanelSlot;
class UCommonRichTextBlock;
class UGenericEffectListWidget;
class UBorder;
class UGenericRuneEffectDA;

/**
 * 符文信息卡 Widget
 *
 * 蓝图制作步骤：
 *   1. 新建 Widget Blueprint，父类选 RuneInfoCardWidget
 *   2. Designer 里放以下控件（名称必须完全一致）：
 *        CardIcon    Image     符文图标
 *        CardName    TextBlock 符文名
 *        CardDesc    TextBlock 符文描述
 *        CardUpgrade TextBlock 升级等级（满级时自动隐藏）
 *   3. 在 WBP_BackpackScreen 的 Canvas Panel 里放本 Widget 实例，
 *      命名为 RuneInfoCard，Visibility 设为 Collapsed
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneInfoCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URuneInfoCardWidget(const FObjectInitializer& ObjectInitializer);

    // =========================================================
    // Designer 绑定（名称必须完全一致）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CardBG;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CardIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonRichTextBlock> CardDesc;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardUpgrade;

    /**
     * 符文占格点阵（UniformGridPanel，命名 "ShapeGrid"）。
     * C++ 根据 FRuneShape 动态生成点阵子节点，无需在 Designer 里手工填充内容。
     * 建议在 Designer 里给它设一个固定大小（如 120×120）。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> ShapeGrid;

    /** 效果描述（命名 "CardEffect"），与 CardDesc 区分显示 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonRichTextBlock> CardEffect;

    /** 战斗卡牌信息（命名 "CardCombatInfo"），显示卡牌分类、ID、效果标签、方向和配方。 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonRichTextBlock> CardCombatInfo;

    /** 通用效果列表小窗（命名 "GenericEffectList"），FRuneConfig::GenericEffects 非空时自动显示 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UGenericEffectListWidget> GenericEffectList;

    /**
     * 卡片背景暗化遮罩（命名 "ContentBlocker"，UBorder 类型，半透明黑）。
     * 叠在 CardBG 之上、CardContent 之下，确保文字可读。
     * 在 Overlay 中通过子控件添加顺序控制 Z-order：CardBG → ContentBlocker → CardContent → SelectionBorder。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBorder> ContentBlocker;

    /**
     * 选中态高亮边框（命名 "SelectionBorder"，UBorder 类型）。
     * 默认 Hidden，由 SetSelected(true) 自动显示。
     * WBP 端建议：UBorder 包卡片或叠在卡片之上，Brush.DrawAs=Box，Brush.Margin 用于 9-slice 描边效果。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBorder> SelectionBorder;

    /** 金币图标（命名 "GoldCostIcon"）。GoldCost > 0 时可见，否则 Collapsed。 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> GoldCostIcon;

    /** 金币数量文字（命名 "GoldCostText"）。GoldCost > 0 时显示 "N G"，否则 Collapsed。 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GoldCostText;

    // =========================================================
    // 对外接口（BackpackScreenWidget 调用）
    // =========================================================

    /** 显示指定符文信息 */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void ShowRune(const FRuneInstance& Rune);

    /** 隐藏卡片 */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void HideCard();

    /**
     * 切换右侧通用效果子窗的展开状态。
     * - 默认 true（兼容背包等单卡场景，无需调用即可展开）
     * - LootSelection 等多卡场景：RebuildCards 时全设 false，FocusCard 时只对聚焦卡设 true
     */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void SetGenericEffectsExpanded(bool bExpanded);

    /**
     * 设置选中态：显示 SelectionBorder + 缩放到 SelectedRenderScale。
     * 由 LootSelection.FocusCard / 背包详情页等调用方主动管理；ShowRune 不会自动清除选中态，
     * 调用方负责在重建/换卡时显式 SetSelected(false)。
     */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void SetSelected(bool bInSelected);

    UFUNCTION(BlueprintPure, Category = "RuneInfoCard")
    bool IsSelected() const { return bSelected; }

    /**
     * CardBackground 留空时使用的默认背景笔刷。
     * 默认值：DrawAs=Box, Color=#1A1A22 alpha=0.9（深暗灰）。
     * 可在 WBP Class Defaults 中覆盖。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuneInfoCard|Visual")
    FSlateBrush DefaultCardBGBrush;

    /** 选中时缩放比例（默认 1.06） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuneInfoCard|Visual")
    float SelectedRenderScale = 1.06f;

    /** 缩放插值速度（FInterpTo Speed） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuneInfoCard|Visual")
    float ScaleInterpSpeed = 12.f;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    /** 根据 FRuneShape 重建 ShapeGrid 子节点（点阵） */
    void BuildShapeGrid(const FRuneShape& Shape);

    /** 将 GenericEffects 的 DisplayName 拼成 "击退 · 燃烧" 格式，供 CardEffect 显示 */
    FText BuildEffectKeywords(const TArray<TObjectPtr<UGenericRuneEffectDA>>& Effects) const;

    FText BuildCombatCardInfo(const FCombatCardConfig& Config) const;

    /** 按 bGenericEffectsExpanded + CachedEffects 同步子窗显示状态 */
    void SyncGenericEffectListVisibility();

    float FadeAlpha = 1.f;
    bool  bFading   = false;
    static constexpr float FadeDuration = 0.15f;

    // 默认 true：单卡场景（背包）开箱即用；多卡场景外部显式 SetGenericEffectsExpanded(false)
    bool bGenericEffectsExpanded = true;

    UPROPERTY()
    TArray<TObjectPtr<UGenericRuneEffectDA>> CachedEffects;

    // 选中态 + 缩放插值
    bool  bSelected          = false;
    float CurrentRenderScale = 1.0f;
};
