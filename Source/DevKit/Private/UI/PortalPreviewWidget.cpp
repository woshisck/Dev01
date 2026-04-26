#include "UI/PortalPreviewWidget.h"
#include "Data/RuneDataAsset.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"

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

    // Buff 列表
    if (BuffListBox)
    {
        BuffListBox->ClearChildren();
        for (const FBuffEntry& Entry : Info.PreRolledBuffs)
        {
            if (!Entry.RuneDA) continue;
            const FRuneConfig& Cfg = Entry.RuneDA->RuneInfo.RuneConfig;

            UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

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
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

            UTextBlock* NameTB = NewObject<UTextBlock>(this);
            NameTB->SetText(FText::FromName(Cfg.RuneName));
            UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(NameTB);
            TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TextSlot->SetVerticalAlignment(VAlign_Center);

            UVerticalBoxSlot* RowSlot = BuffListBox->AddChildToVerticalBox(Row);
            RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
        }
    }

    // 战利品摘要
    if (LootSummaryText)
    {
        LootSummaryText->SetText(FText::Format(
            NSLOCTEXT("Portal", "LootSummary", "战利品：符文 ×{0}"),
            FText::AsNumber(Info.LootCount)));
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
