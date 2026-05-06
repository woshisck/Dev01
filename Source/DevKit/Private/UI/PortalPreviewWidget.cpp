#include "UI/PortalPreviewWidget.h"
#include "Data/RuneDataAsset.h"
#include "Data/GenericRuneEffectDA.h"
#include "RuneHudTextUtils.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "UI/YogCommonRichTextBlock.h"
#include "CommonInputSubsystem.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
    // 类型徽章颜色映射（v3 决策：.cpp 静态映射，不建 DA）
    // Tag 名 → 文字 + 颜色
    struct FRoomTypeStyle
    {
        FText DisplayName;
        FLinearColor BadgeColor;
    };

    const FRoomTypeStyle& GetRoomTypeStyle(const FGameplayTag& Tag)
    {
        static const FRoomTypeStyle Normal = { NSLOCTEXT("Portal", "RoomTypeNormal", "普通"),  FLinearColor(0.60f, 0.60f, 0.60f, 0.92f) };
        static const FRoomTypeStyle Elite  = { NSLOCTEXT("Portal", "RoomTypeElite",  "精英"),  FLinearColor(0.85f, 0.35f, 0.29f, 0.92f) };
        static const FRoomTypeStyle Shop   = { NSLOCTEXT("Portal", "RoomTypeShop",   "商店"),  FLinearColor(0.85f, 0.69f, 0.28f, 0.92f) };
        static const FRoomTypeStyle Event  = { NSLOCTEXT("Portal", "RoomTypeEvent",  "事件"),  FLinearColor(0.48f, 0.36f, 0.79f, 0.92f) };
        static const FRoomTypeStyle Unknown= { NSLOCTEXT("Portal", "RoomTypeUnknown","未知"),  FLinearColor(0.30f, 0.30f, 0.30f, 0.92f) };

        if (!Tag.IsValid()) return Unknown;
        const FName Name = Tag.GetTagName();
        if (Name == FName("Room.Type.Elite")) return Elite;
        if (Name == FName("Room.Type.Shop"))  return Shop;
        if (Name == FName("Room.Type.Event")) return Event;
        if (Name == FName("Room.Type.Normal"))return Normal;
        return Unknown;
    }

    // ── BuffListBox 行样式（v3 决策：.cpp 静态常量，不建 DA）──
    constexpr int32   PortalBuffNameFontSize     = 13;
    constexpr int32   PortalBuffDescFontSize     = 11;
    constexpr int32   PortalBuffEffectFontSize   = 11;
    constexpr int32   PortalBuffSummaryMaxChars  = 34;
    const FLinearColor PortalBuffNameColor       = FLinearColor(0.93f, 0.93f, 0.93f, 1.0f); // #ECECEC
    const FLinearColor PortalBuffDescColor       = FLinearColor(0.78f, 0.78f, 0.80f, 1.0f); // 次级灰
    const FLinearColor PortalBuffEffectColor     = FLinearColor(0.65f, 0.65f, 0.70f, 1.0f); // 更淡
    const FLinearColor PortalPanelFillColor      = FLinearColor(0.025f, 0.030f, 0.038f, 0.84f);
    const FLinearColor PortalPanelBorderColor    = FLinearColor(0.74f, 0.70f, 0.58f, 0.86f);
    constexpr float   PortalPanelCornerRadius   = 3.f;
    constexpr float   PortalPanelBorderWidth    = 2.f;
    constexpr float   BuffRowSpacing       = 6.f;
    constexpr float   BuffSubLineSpacing   = 2.f;

    // 字号微调：保留原字体 / 字重，只改 Size，避免默认字体被覆盖
    void SetPortalTextSize(UTextBlock* TB, int32 Size)
    {
        if (!TB) return;
        FSlateFontInfo Font = TB->GetFont();
        Font.Size = Size;
        TB->SetFont(Font);
    }

    void ConfigurePortalPanelBorder(UBorder* Border)
    {
        if (!Border)
        {
            return;
        }

        Border->SetBrush(FSlateRoundedBoxBrush(
            PortalPanelFillColor,
            PortalPanelCornerRadius,
            PortalPanelBorderColor,
            PortalPanelBorderWidth));
        Border->SetPadding(FMargin(16.f, 14.f));
    }

    // RuneName 漏配兜底：开发期暴露资产名定位漏配，Shipping 显示"未命名"
    FText ResolvePortalRuneDisplayName(const URuneDataAsset& RuneDA)
    {
        const FName RuneName = RuneDA.RuneInfo.RuneConfig.RuneName;
        if (!RuneName.IsNone())
        {
            return FText::FromName(RuneName);
        }
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Warning,
            TEXT("PortalPreview: RuneDA '%s' 缺少 RuneConfig.RuneName，临时显示资产名"),
            *RuneDA.GetName());
        return FText::FromString(RuneDA.GetName());
#else
        UE_LOG(LogTemp, Warning, TEXT("PortalPreview: RuneDA 缺少 RuneConfig.RuneName"));
        return NSLOCTEXT("Portal", "UnnamedRune", "未命名符文");
#endif
    }

    // GenericEffect 单条文本格式化（4 种情况降级，避免空冒号 / 空行）
    FText FormatPortalGenericEffectLine(const UGenericRuneEffectDA& Effect)
    {
        const bool bHasName = !Effect.DisplayName.IsEmptyOrWhitespace();
        const bool bHasDesc = !Effect.Description.IsEmptyOrWhitespace();
        if (bHasName && bHasDesc)
        {
            return FText::Format(
                NSLOCTEXT("Portal", "BulletNameDesc", "• {0}：{1}"),
                Effect.DisplayName, Effect.Description);
        }
        if (bHasName)
        {
            return FText::Format(
                NSLOCTEXT("Portal", "BulletNameOnly", "• {0}"),
                Effect.DisplayName);
        }
        if (bHasDesc)
        {
            return FText::Format(
                NSLOCTEXT("Portal", "BulletDescOnly", "• {0}"),
                Effect.Description);
        }
        return FText::GetEmpty();
    }
}

void UPortalPreviewWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ConfigurePortalPanelBorder(BG);

    // 写入初始提示文字
    RefreshHintText(ECommonInputType::MouseAndKeyboard);

    // 订阅输入设备切换，实时刷新图标（手柄↔键鼠）
    if (UCommonInputSubsystem* InputSub =
        ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
    {
        InputSub->OnInputMethodChangedNative.AddUObject(
            this, &UPortalPreviewWidget::RefreshHintText);
    }

    SetInteractHintVisible(false);
}

void UPortalPreviewWidget::NativeDestruct()
{
    if (UCommonInputSubsystem* InputSub =
        ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
    {
        InputSub->OnInputMethodChangedNative.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UPortalPreviewWidget::RefreshHintText(ECommonInputType /*NewInputType*/)
{
    if (InteractHintRoot)
    {
        // SetText 触发 RichText 重建，装饰器重新读 CommonInputSubsystem 获取当前设备图标
        InteractHintRoot->SetText(
            FText::FromString(TEXT("<input action=\"Interact\"/> 进入")));
    }
}

void UPortalPreviewWidget::SetPreviewInfo(const FPortalPreviewInfo& Info)
{
    // 房间名
    if (RoomNameText)
    {
        RoomNameText->SetText(Info.RoomDisplayName);
    }

    // 类型徽章 + 文字
    const FRoomTypeStyle& Style = GetRoomTypeStyle(Info.RoomTypeTag);
    if (RoomTypeBadge)
    {
        RoomTypeBadge->SetBrushColor(Style.BadgeColor);
    }
    if (RoomTypeText)
    {
        RoomTypeText->SetText(Style.DisplayName);
    }

    // Buff 列表（每行 = HBox[Icon | VBox{Name / Desc / GenericEffects×N}]）
    if (BuffListBox)
    {
        BuffListBox->ClearChildren();
        for (const FBuffEntry& Entry : Info.PreRolledBuffs)
        {
            if (!Entry.RuneDA) continue;
            const FRuneConfig& Cfg = Entry.RuneDA->RuneInfo.RuneConfig;

            UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

            // ── 左：Icon 28×28，顶端对齐（多行时图标对第一行）──
            USizeBox* IconBox = NewObject<USizeBox>(this);
            IconBox->SetWidthOverride(28.f);
            IconBox->SetHeightOverride(28.f);
            UImage* Icon = NewObject<UImage>(this);
            if (Cfg.RuneIcon)
                Icon->SetBrushFromTexture(Cfg.RuneIcon, true);
            else
                Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
            IconBox->AddChild(Icon);

            UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
            IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
            IconSlot->SetVerticalAlignment(VAlign_Top);
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

            // ── 右：VBox 名称 / 描述 / 通用效果列表 ──
            UVerticalBox* RightVBox = NewObject<UVerticalBox>(this);

            // 名称（漏配兜底见 ResolveRuneDisplayName；AutoWrap 防本地化/长资产名溢出）
            UTextBlock* NameTB = NewObject<UTextBlock>(this);
            NameTB->SetText(ResolvePortalRuneDisplayName(*Entry.RuneDA));
            NameTB->SetColorAndOpacity(FSlateColor(PortalBuffNameColor));
            NameTB->SetAutoWrapText(true);
            SetPortalTextSize(NameTB, PortalBuffNameFontSize);
            RightVBox->AddChildToVerticalBox(NameTB);

            // HUD 摘要优先，未配置时从完整描述自动压成 1-2 行。
            const FText SummaryText = RuneHudTextUtils::GetRuneHudSummary(Cfg, PortalBuffSummaryMaxChars);
            if (!SummaryText.IsEmptyOrWhitespace())
            {
                UTextBlock* DescTB = NewObject<UTextBlock>(this);
                DescTB->SetText(SummaryText);
                DescTB->SetColorAndOpacity(FSlateColor(PortalBuffDescColor));
                DescTB->SetAutoWrapText(true);
                SetPortalTextSize(DescTB, PortalBuffDescFontSize);
                UVerticalBoxSlot* DescSlot = RightVBox->AddChildToVerticalBox(DescTB);
                DescSlot->SetPadding(FMargin(0.f, BuffSubLineSpacing, 0.f, 0.f));
            }

            // GenericEffects（每条按 4 种情况降级，全空则跳过）
            for (const TObjectPtr<UGenericRuneEffectDA>& EffectPtr : Cfg.GenericEffects)
            {
                const UGenericRuneEffectDA* Effect = EffectPtr.Get();
                if (!Effect) continue;
                const FText Line = FormatPortalGenericEffectLine(*Effect);
                if (Line.IsEmptyOrWhitespace()) continue;

                UTextBlock* EffTB = NewObject<UTextBlock>(this);
                EffTB->SetText(Line);
                EffTB->SetColorAndOpacity(FSlateColor(PortalBuffEffectColor));
                EffTB->SetAutoWrapText(true);
                SetPortalTextSize(EffTB, PortalBuffEffectFontSize);
                UVerticalBoxSlot* EffSlot = RightVBox->AddChildToVerticalBox(EffTB);
                EffSlot->SetPadding(FMargin(0.f, BuffSubLineSpacing, 0.f, 0.f));
            }

            UHorizontalBoxSlot* RightSlot = Row->AddChildToHorizontalBox(RightVBox);
            RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            RightSlot->SetVerticalAlignment(VAlign_Center);

            UVerticalBoxSlot* RowSlot = BuffListBox->AddChildToVerticalBox(Row);
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, BuffRowSpacing));
        }
    }

    // 战利品摘要（仅列类型，不带数量）
    if (LootSummaryText)
    {
        LootSummaryText->SetText(
            NSLOCTEXT("Portal", "LootSummary", "战利品：符文、金币"));
    }

    // 浮窗刚显示时默认隐藏交互提示，待 HUD 检测到 PendingPortal == this 再调 SetInteractHintVisible(true)
    SetInteractHintVisible(false);

    K2_OnPreviewShown();
}

void UPortalPreviewWidget::SetInteractHintVisible(bool bVisible)
{
    if (InteractHintRoot)
    {
        InteractHintRoot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (bVisible && !bInteractHintVisible)
    {
        K2_OnInteractHintShown();
    }
    bInteractHintVisible = bVisible;
}
