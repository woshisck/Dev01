#include "UI/RuneInfoCardWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "CommonRichTextBlock.h"
#include "Data/GenericRuneEffectDA.h"
#include "UI/GenericEffectListWidget.h"
#include "Styling/SlateBrush.h"

// ============================================================
//  构造函数
// ============================================================

URuneInfoCardWidget::URuneInfoCardWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 默认背景：深暗灰 #1A1A22 alpha=0.9（用户可在 WBP Class Defaults 覆盖）
    DefaultCardBGBrush.DrawAs    = ESlateBrushDrawType::Box;
    DefaultCardBGBrush.TintColor = FSlateColor(FLinearColor(0.102f, 0.102f, 0.133f, 0.9f));
    TemporaryLockWarningFormat = NSLOCTEXT(
        "RuneInfoCard",
        "TemporaryFinisherLockWarningFmt",
        "<key>终结技未解锁：{0}/{1}</key>\n还需完成 {2} 场战斗。锁定期间消耗此牌不会触发终结技。");
}

// ============================================================
//  点阵颜色常量
// ============================================================

namespace ShapeGridColors
{
    static const FLinearColor Filled (0.20f, 0.60f, 1.00f, 1.0f);  // 亮蓝：已占格
    static const FLinearColor Empty  (0.18f, 0.18f, 0.22f, 0.6f);  // 暗灰：空格
}

// ============================================================
//  生命周期
// ============================================================

void URuneInfoCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 缩放围绕中心展开（默认 pivot 是左上角，会让选中态视觉偏移）
    SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

    // 初始隐藏选中边框（C++ 控制显隐，WBP 不必显式设 Hidden）
    if (SelectionBorder)
        SelectionBorder->SetVisibility(ESlateVisibility::Hidden);
}

// ============================================================
//  显示符文
// ============================================================

void URuneInfoCardWidget::ShowRune(const FRuneInstance& Rune)
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (CardBG)
    {
        if (Rune.RuneConfig.CardBackground)
        {
            CardBG->SetBrushFromTexture(Rune.RuneConfig.CardBackground, false);
        }
        else
        {
            CardBG->SetBrush(DefaultCardBGBrush);
        }
        CardBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (CardName)
        CardName->SetText(FText::FromName(Rune.RuneConfig.RuneName));

    if (CardDesc)
        CardDesc->SetText(Rune.RuneConfig.RuneDescription);

    if (CardEffect)
    {
        const FText Keywords = BuildEffectKeywords(Rune.RuneConfig.GenericEffects);
        if (Keywords.IsEmpty())
        {
            CardEffect->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            CardEffect->SetText(Keywords);
            CardEffect->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }

    if (CardCombatInfo)
    {
        const FText CombatInfo = BuildCombatCardInfo(Rune.CombatCard);
        if (CombatInfo.IsEmpty())
        {
            CardCombatInfo->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            CardCombatInfo->SetText(CombatInfo);
            CardCombatInfo->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }

    if (GoldCostText)
    {
        if (Rune.RuneConfig.GoldCost > 0)
        {
            GoldCostText->SetText(FText::Format(
                NSLOCTEXT("RuneInfoCard", "GoldCostFmt", "{0} G"),
                FText::AsNumber(Rune.RuneConfig.GoldCost)));
            GoldCostText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            GoldCostText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (GoldCostIcon)
    {
        GoldCostIcon->SetVisibility(Rune.RuneConfig.GoldCost > 0
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }

    if (CardIcon)
    {
        if (Rune.RuneConfig.RuneIcon)
        {
            CardIcon->SetBrushFromTexture(Rune.RuneConfig.RuneIcon, false);
            CardIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (CardUpgrade)
    {
        if (Rune.UpgradeLevel > 0)
        {
            CardUpgrade->SetText(FText::Format(
                NSLOCTEXT("RuneInfoCard", "LvText", "Lv.{0}"),
                FText::AsNumber(Rune.UpgradeLevel + 1)));
            CardUpgrade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardUpgrade->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    BuildShapeGrid(Rune.Shape);

    // 缓存通用效果到 CachedEffects，供 Expanded 切换时复用
    CachedEffects.Reset();
    for (const TObjectPtr<UGenericRuneEffectDA>& E : Rune.RuneConfig.GenericEffects)
    {
        if (E) CachedEffects.Add(E);
    }
    SyncGenericEffectListVisibility();

    // 触发淡入动画
    FadeAlpha = 0.f;
    bFading   = true;
    SetRenderOpacity(0.f);
}

void URuneInfoCardWidget::ShowCombatCard(const FCombatCardInstance& Card)
{
    if (!Card.SourceData)
    {
        HideCard();
        return;
    }

    ShowRune(Card.SourceData->RuneInfo);

    const FText LockWarning = BuildTemporaryLockWarning(Card);
    if (CardEffect && !LockWarning.IsEmpty())
    {
        const FText Keywords = BuildEffectKeywords(Card.SourceData->RuneInfo.RuneConfig.GenericEffects);
        CardEffect->SetText(Keywords.IsEmpty()
            ? LockWarning
            : FText::Format(
                NSLOCTEXT("RuneInfoCard", "TemporaryLockWarningWithKeywordsFmt", "{0}\n\n{1}"),
                LockWarning,
                Keywords));
        CardEffect->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (CardCombatInfo)
    {
        const FText CombatInfo = BuildCombatCardInfo(Card);
        if (CombatInfo.IsEmpty())
        {
            CardCombatInfo->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            CardCombatInfo->SetText(CombatInfo);
            CardCombatInfo->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }
}

void URuneInfoCardWidget::SetGenericEffectsExpanded(bool bExpanded)
{
    if (bGenericEffectsExpanded == bExpanded) return;
    bGenericEffectsExpanded = bExpanded;
    SyncGenericEffectListVisibility();
}

// ============================================================
//  选中态高亮
// ============================================================

void URuneInfoCardWidget::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    if (SelectionBorder)
        SelectionBorder->SetVisibility(bInSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void URuneInfoCardWidget::SyncGenericEffectListVisibility()
{
    if (!GenericEffectList) return;

    if (bGenericEffectsExpanded)
    {
        TArray<UGenericRuneEffectDA*> Effects;
        Effects.Reserve(CachedEffects.Num());
        for (const TObjectPtr<UGenericRuneEffectDA>& E : CachedEffects)
        {
            if (E) Effects.Add(E.Get());
        }
        // SetEffects 内部会按数组空/非空自动 Collapse / SelfHitTestInvisible
        GenericEffectList->SetEffects(Effects);
    }
    else
    {
        // 折叠态：直接清空 + 隐藏，无视 CachedEffects
        GenericEffectList->SetEffects(TArray<UGenericRuneEffectDA*>());
    }
}

// ============================================================
//  Tick：淡入 + 选中缩放插值
// ============================================================

void URuneInfoCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 淡入（一次性，结束自动停止）
    if (bFading)
    {
        FadeAlpha = FMath::Min(FadeAlpha + InDeltaTime / FadeDuration, 1.f);
        SetRenderOpacity(FadeAlpha);
        if (FadeAlpha >= 1.f) bFading = false;
    }

    // 选中缩放插值（持续 Tick；近似收敛后无明显开销）
    const float TargetScale = bSelected ? SelectedRenderScale : 1.0f;
    if (!FMath::IsNearlyEqual(CurrentRenderScale, TargetScale, 0.001f))
    {
        CurrentRenderScale = FMath::FInterpTo(CurrentRenderScale, TargetScale, InDeltaTime, ScaleInterpSpeed);
        SetRenderScale(FVector2D(CurrentRenderScale));
    }
}

// ============================================================
//  隐藏卡片
// ============================================================

void URuneInfoCardWidget::HideCard()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

// ============================================================
//  通用效果关键词拼接
// ============================================================

FText URuneInfoCardWidget::BuildEffectKeywords(const TArray<TObjectPtr<UGenericRuneEffectDA>>& Effects) const
{
    TArray<FText> Names;
    Names.Reserve(Effects.Num());
    for (const TObjectPtr<UGenericRuneEffectDA>& E : Effects)
    {
        if (E)
        {
            Names.Add(FText::Format(
                NSLOCTEXT("RuneInfoCard", "EffectKeywordMarkupFmt", "<key>{0}</key>"),
                E->DisplayName));
        }
    }
    if (Names.IsEmpty()) return FText::GetEmpty();
    return FText::Join(NSLOCTEXT("RuneInfoCard", "EffectKeywordSeparator", " · "), Names);
}

FText URuneInfoCardWidget::BuildCombatCardInfo(const FCombatCardConfig& Config) const
{
    FCombatCardInstance Card;
    Card.Config = Config;
    Card.LinkOrientation = Config.DefaultLinkOrientation;
    return BuildCombatCardInfo(Card);
}

FText URuneInfoCardWidget::BuildCombatCardInfo(const FCombatCardInstance& Card) const
{
    const FCombatCardConfig& Config = Card.Config;
    if (!Config.bIsCombatCard)
    {
        return FText::GetEmpty();
    }

    auto CardTypeToString = [](ECombatCardType Type) -> FString
    {
        switch (Type)
        {
        case ECombatCardType::Normal:
        case ECombatCardType::Attack:
            return TEXT("普通卡牌");
        case ECombatCardType::Link:
            return TEXT("连携卡牌");
        case ECombatCardType::Finisher:
            return TEXT("终结技卡牌");
        case ECombatCardType::Passive:
            return TEXT("被动卡牌");
        default:
            return TEXT("未知");
        }
    };

    auto OrientationToString = [](ECombatCardLinkOrientation Orientation) -> FString
    {
        return Orientation == ECombatCardLinkOrientation::Reversed ? TEXT("反向") : TEXT("正向");
    };

    auto TagsToString = [](const FGameplayTagContainer& Tags) -> FString
    {
        TArray<FGameplayTag> TagArray;
        Tags.GetGameplayTagArray(TagArray);
        TArray<FString> Parts;
        Parts.Reserve(TagArray.Num());
        for (const FGameplayTag& Tag : TagArray)
        {
            Parts.Add(Tag.ToString());
        }
        return Parts.IsEmpty() ? TEXT("无") : FString::Join(Parts, TEXT(" / "));
    };

    TArray<FString> Lines;
    if (Card.bTemporarilyLocked)
    {
        Lines.Add(FString::Printf(
            TEXT("锁定状态：未解锁（%d/%d）"),
            FMath::Max(0, Card.TemporaryUnlockCurrentCompletedBattles),
            FMath::Max(0, Card.TemporaryUnlockRequiredCompletedBattles)));
    }
    Lines.Add(FString::Printf(TEXT("分类：%s"), *CardTypeToString(Config.CardType)));
    Lines.Add(FString::Printf(TEXT("CardId：%s"), Config.CardIdTag.IsValid() ? *Config.CardIdTag.ToString() : TEXT("未配置")));
    Lines.Add(FString::Printf(TEXT("效果Tag：%s"), *TagsToString(Config.CardEffectTags)));

    if (Config.CardType == ECombatCardType::Link)
    {
        Lines.Add(FString::Printf(TEXT("当前方向：%s"), *OrientationToString(Card.LinkOrientation)));
        Lines.Add(FString::Printf(TEXT("连携配方：%d 条"), Config.LinkRecipes.Num()));
    }

    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText URuneInfoCardWidget::BuildTemporaryLockWarning(const FCombatCardInstance& Card) const
{
    if (!bShowTemporaryLockWarning || !Card.bTemporarilyLocked || Card.TemporaryUnlockRequiredCompletedBattles <= 0)
    {
        return FText::GetEmpty();
    }

    const int32 CurrentBattles = FMath::Max(0, Card.TemporaryUnlockCurrentCompletedBattles);
    const int32 RequiredBattles = FMath::Max(0, Card.TemporaryUnlockRequiredCompletedBattles);
    const int32 RemainingBattles = FMath::Max(0, RequiredBattles - CurrentBattles);

    return FText::Format(
        TemporaryLockWarningFormat,
        FText::AsNumber(CurrentBattles),
        FText::AsNumber(RequiredBattles),
        FText::AsNumber(RemainingBattles));
}

// ============================================================
//  点阵生成
// ============================================================

void URuneInfoCardWidget::BuildShapeGrid(const FRuneShape& Shape)
{
    if (!ShapeGrid) return;

    ShapeGrid->ClearChildren();

    if (Shape.Cells.IsEmpty()) return;

    // ── 计算包围盒 ───────────────────────────────────────────────
    int32 MinX = INT_MAX, MaxX = INT_MIN;
    int32 MinY = INT_MAX, MaxY = INT_MIN;
    for (const FIntPoint& Cell : Shape.Cells)
    {
        MinX = FMath::Min(MinX, Cell.X);
        MaxX = FMath::Max(MaxX, Cell.X);
        MinY = FMath::Min(MinY, Cell.Y);
        MaxY = FMath::Max(MaxY, Cell.Y);
    }

    const int32 Cols   = MaxX - MinX + 1;
    const int32 Rows   = MaxY - MinY + 1;
    const int32 MaxDim = FMath::Max(Cols, Rows);

    TSet<FIntPoint> Occupied(Shape.Cells);

    // ── 固定点大小 8px，间隔 2px，居中到 64×64 区域 ────────────
    constexpr float AreaSize = 64.f;
    constexpr float DotSize  = 8.f;
    constexpr float Gap      = 2.f;
    constexpr float StepSize = DotSize + Gap;

    // 整个点阵实际宽高，用于居中
    const float GridW = Cols * StepSize - Gap;
    const float GridH = Rows * StepSize - Gap;
    const float OffX  = (AreaSize - GridW) * 0.5f;
    const float OffY  = (AreaSize - GridH) * 0.5f;

    // ── 逐格创建绝对定位 UImage ─────────────────────────────────
    for (int32 Row = 0; Row < Rows; Row++)
    {
        for (int32 Col = 0; Col < Cols; Col++)
        {
            const FIntPoint AbsCell(Col + MinX, Row + MinY);
            const bool bFilled = Occupied.Contains(AbsCell);

            UImage* Dot = WidgetTree
                ? WidgetTree->ConstructWidget<UImage>(UImage::StaticClass())
                : NewObject<UImage>(this);

            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
            Brush.TintColor = FSlateColor(FLinearColor::White);
            Brush.OutlineSettings.CornerRadii  = FVector4(3.f, 3.f, 3.f, 3.f);
            Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            Dot->SetBrush(Brush);
            Dot->SetColorAndOpacity(bFilled ? ShapeGridColors::Filled : ShapeGridColors::Empty);

            UCanvasPanelSlot* DotSlot = ShapeGrid->AddChildToCanvas(Dot);
            DotSlot->SetAutoSize(false);
            DotSlot->SetPosition(FVector2D(OffX + Col * StepSize, OffY + Row * StepSize));
            DotSlot->SetSize(FVector2D(DotSize, DotSize));
        }
    }
}

// ============================================================
//  Wrapper button 桥接（多卡场景，给外部的点击/悬停事件携带 VisibleIndex）
// ============================================================

void URuneInfoCardWidget::BindToWrapperButton(UButton* InWrapper)
{
    if (!InWrapper) return;

    if (UButton* Prev = BoundWrapperButton.Get())
    {
        Prev->OnClicked.RemoveDynamic(this, &URuneInfoCardWidget::HandleWrapperClicked);
        Prev->OnHovered.RemoveDynamic(this, &URuneInfoCardWidget::HandleWrapperHovered);
    }

    BoundWrapperButton = InWrapper;
    InWrapper->OnClicked.AddDynamic(this, &URuneInfoCardWidget::HandleWrapperClicked);
    InWrapper->OnHovered.AddDynamic(this, &URuneInfoCardWidget::HandleWrapperHovered);
}

void URuneInfoCardWidget::HandleWrapperClicked()
{
    OnRuneCardClickedNative.Broadcast(VisibleIndex);
}

void URuneInfoCardWidget::HandleWrapperHovered()
{
    OnRuneCardHoveredNative.Broadcast(VisibleIndex);
}
