#include "UI/BackpackScreenWidget.h"
#include "UI/RuneDragDropOperation.h"
#include "UI/RuneTooltipWidget.h"
#include "Component/BackpackGridComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "CommonInputSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InputCoreTypes.h"

// ============================================================
//  格子颜色常量
// ============================================================

namespace BackpackColors
{
    static const FLinearColor Empty          (0.12f, 0.12f, 0.12f, 1.f); // 深灰
    static const FLinearColor EmptyActive    (0.05f, 0.18f, 0.45f, 1.f); // 深蓝
    static const FLinearColor OccupiedActive (0.10f, 0.55f, 1.00f, 1.f); // 亮蓝
    static const FLinearColor OccupiedInact  (0.55f, 0.35f, 0.05f, 1.f); // 金橙
    static const FLinearColor Selected       (1.00f, 0.82f, 0.10f, 1.f); // 高亮黄
    static const FLinearColor Hover          (0.10f, 0.80f, 0.20f, 1.f); // 拖拽悬浮绿
    static const FLinearColor GrabbedSource  (0.25f, 0.15f, 0.03f, 1.f); // 暗橙（被抓起的源格）
    static const FLinearColor CursorTarget   (0.05f, 0.60f, 0.55f, 1.f); // 青绿（手柄放置目标格）
}

// ============================================================
//  内部辅助
// ============================================================

UBackpackGridComponent* UBackpackScreenWidget::GetBackpack() const
{
    if (CachedBackpack.IsValid())
        return CachedBackpack.Get();

    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn) return nullptr;

    return Pawn->FindComponentByClass<UBackpackGridComponent>();
}

// ============================================================
//  生命周期
// ============================================================

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 允许接收手柄/键盘输入
    bIsFocusable = true;

    if (APawn* Pawn = GetOwningPlayerPawn())
        CachedBackpack = Pawn->FindComponentByClass<UBackpackGridComponent>();

    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.AddDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.AddDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.AddDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }

    BindGridCellClicks();
    BindPendingRuneSlots();
    RefreshPendingRuneSlots();
}

void UBackpackScreenWidget::NativeDestruct()
{
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.RemoveDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }
    Super::NativeDestruct();
}

// ============================================================
//  委托处理
// ============================================================

void UBackpackScreenWidget::HandleRunePlaced(const FRuneInstance& Rune)
{
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneRemoved(FGuid RuneGuid)
{
    if (SelectedCell != FIntPoint(-1, -1))
    {
        if (UBackpackGridComponent* Backpack = GetBackpack())
        {
            if (Backpack->GetRuneIndexAtCell(SelectedCell) == -1)
            {
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
            }
        }
    }
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated)
{
    OnGridNeedsRefresh();
}

// ============================================================
//  格子刷新（BlueprintNativeEvent 默认实现）
// ============================================================

void UBackpackScreenWidget::OnGridNeedsRefresh_Implementation()
{
    const int32 CellCount = CachedCellButtons.Num();
    if (CellCount == 0) return;

    UBackpackGridComponent* BackpackComp = GetBackpack();
    const int32 GW = BackpackComp ? BackpackComp->GridWidth : 5;

    for (int32 i = 0; i < CellCount; i++)
    {
        UButton* Btn = CachedCellButtons[i];
        if (!Btn) continue;

        const int32 Col = i % GW;
        const int32 Row = i / GW;

        // 颜色优先级（手柄抓取中）：目标格 > 源格 > 选中 > 拖拽悬浮 > 状态色
        const FIntPoint Cell(Col, Row);
        FLinearColor Color;
        if (bGrabbingRune && Cell == GamepadCursorCell)
        {
            Color = BackpackColors::CursorTarget;   // 青绿：将要放置的目标格
        }
        else if (bGrabbingRune && Cell == GrabbedFromCell)
        {
            Color = BackpackColors::GrabbedSource;  // 暗橙：符文被抓起的源格
        }
        else if (IsCellSelected(Col, Row))
        {
            Color = BackpackColors::Selected;
        }
        else if (Col == HoverCol && Row == HoverRow)
        {
            Color = BackpackColors::Hover;
        }
        else
        {
            switch (GetCellVisualState(Col, Row))
            {
                case EBackpackCellState::EmptyActive:      Color = BackpackColors::EmptyActive;    break;
                case EBackpackCellState::OccupiedActive:   Color = BackpackColors::OccupiedActive; break;
                case EBackpackCellState::OccupiedInactive: Color = BackpackColors::OccupiedInact;  break;
                default:                                   Color = BackpackColors::Empty;           break;
            }
        }
        Btn->SetBackgroundColor(Color);

        // 图标：有符文则显示；抓取中源格图标半透明（视觉上"已被拿起"）
        if (CachedCellIcons.IsValidIndex(i) && CachedCellIcons[i])
        {
            UImage* Icon = CachedCellIcons[i];
            UTexture2D* Tex = GetRuneIconAtCell(Col, Row);
            if (Tex)
            {
                Icon->SetBrushFromTexture(Tex, /*bMatchSize=*/false);
                const float Opacity = (bGrabbingRune && Cell == GrabbedFromCell) ? 0.30f : 1.f;
                Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Opacity));
                Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else
            {
                Icon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

// ============================================================
//  详情面板刷新（BlueprintNativeEvent 默认实现）
// ============================================================

void UBackpackScreenWidget::OnSelectionChanged_Implementation()
{
    FRuneInstance Info = GetFocusedRuneInfo();
    const bool bHasSelection = Info.RuneGuid.IsValid();

    // 详情面板容器：有选中时显示，没有时隐藏
    if (DetailPanel)
        DetailPanel->SetVisibility(bHasSelection ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

    if (bHasSelection)
    {
        if (DetailName)
            DetailName->SetText(FText::FromName(Info.RuneConfig.RuneName));

        if (DetailDesc)
            DetailDesc->SetText(Info.RuneConfig.RuneDescription);

        if (DetailIcon)
        {
            if (Info.RuneConfig.RuneIcon)
            {
                DetailIcon->SetBrushFromTexture(Info.RuneConfig.RuneIcon, false);
                DetailIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            }
            else
            {
                DetailIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // 操作提示
    if (HintText)
    {
        if (SelectedCell != FIntPoint(-1, -1))
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintMove", "已选中：{0}\n拖拽格子 → 移动\n「移除选中」→ 移除"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else if (SelectedRuneIndex >= 0)
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintPlace", "已选中：{0}\n点击背包格子 → 放置"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else
        {
            HintText->SetText(NSLOCTEXT("Backpack", "HintIdle", "拖拽格子中的符文可移动位置\n点击左侧列表选择待放置符文"));
        }
    }

    // ── 右侧信息卡 ─────────────────────────────────────────────────────────
    if (RuneInfoCard)
    {
        RuneInfoCard->SetVisibility(bHasSelection
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);

        if (bHasSelection)
        {
            if (CardName)  CardName->SetText(FText::FromName(Info.RuneConfig.RuneName));
            if (CardDesc)  CardDesc->SetText(Info.RuneConfig.RuneDescription);

            if (CardIcon)
            {
                if (Info.RuneConfig.RuneIcon)
                {
                    CardIcon->SetBrushFromTexture(Info.RuneConfig.RuneIcon, false);
                    CardIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
                }
                else { CardIcon->SetVisibility(ESlateVisibility::Collapsed); }
            }

            if (CardUpgrade)
            {
                if (Info.UpgradeLevel > 0)
                {
                    CardUpgrade->SetText(FText::Format(
                        NSLOCTEXT("Backpack", "CardLv", "Lv.{0}"),
                        FText::AsNumber(Info.UpgradeLevel + 1)));
                    CardUpgrade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
                }
                else { CardUpgrade->SetVisibility(ESlateVisibility::Collapsed); }
            }
        }
    }

    // 同时刷新格子高亮（让选中格子变黄）
    OnGridNeedsRefresh();
}

// ============================================================
//  状态查询
// ============================================================

bool UBackpackScreenWidget::IsCellInActivationZone(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetActivationZoneCells().Contains(FIntPoint(Col, Row));
}

bool UBackpackScreenWidget::IsCellOccupied(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row)) >= 0;
}

FPlacedRune UBackpackScreenWidget::GetRuneAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return FPlacedRune();

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return FPlacedRune();

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    return Placed.IsValidIndex(Idx) ? Placed[Idx] : FPlacedRune();
}

FRuneInstance UBackpackScreenWidget::GetFocusedRuneInfo() const
{
    if (SelectedCell != FIntPoint(-1, -1))
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            return PR.Rune;
    }
    return GetSelectedRuneInfo();
}

UTexture2D* UBackpackScreenWidget::GetRuneIconAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return nullptr;

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return nullptr;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(Idx)) return nullptr;

    return Placed[Idx].Rune.RuneConfig.RuneIcon;
}

EBackpackCellState UBackpackScreenWidget::GetCellVisualState(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return EBackpackCellState::Empty;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    bool bInZone  = IsCellInActivationZone(Col, Row);

    if (RuneIdx < 0)
        return bInZone ? EBackpackCellState::EmptyActive : EBackpackCellState::Empty;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (Placed.IsValidIndex(RuneIdx) && Placed[RuneIdx].bIsActivated)
        return EBackpackCellState::OccupiedActive;

    return EBackpackCellState::OccupiedInactive;
}

TArray<FRuneInstance> UBackpackScreenWidget::GetRuneList() const
{
    TArray<FRuneInstance> Result;
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
        Result.Append(Player->PendingRunes);

    for (const TObjectPtr<URuneDataAsset>& DA : AvailableRunes)
        if (DA) Result.Add(DA->RuneInfo);

    return Result;
}

int32 UBackpackScreenWidget::GetPendingRuneCount() const
{
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    return Player ? Player->PendingRunes.Num() : 0;
}

FRuneInstance UBackpackScreenWidget::GetSelectedRuneInfo() const
{
    TArray<FRuneInstance> RuneList = GetRuneList();
    if (!RuneList.IsValidIndex(SelectedRuneIndex)) return FRuneInstance();
    return RuneList[SelectedRuneIndex];
}

const TArray<FPlacedRune>& UBackpackScreenWidget::GetAllPlacedRunes() const
{
    static TArray<FPlacedRune> Empty;
    UBackpackGridComponent* Backpack = GetBackpack();
    return Backpack ? Backpack->GetAllPlacedRunes() : Empty;
}

// ============================================================
//  操作
// ============================================================

void UBackpackScreenWidget::SelectRuneFromList(int32 Index)
{
    SelectedRuneIndex = (SelectedRuneIndex == Index) ? -1 : Index;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

void UBackpackScreenWidget::ClickCell(int32 Col, int32 Row)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    FIntPoint Cell(Col, Row);
    int32 RuneIdx = Backpack->GetRuneIndexAtCell(Cell);

    if (RuneIdx >= 0)
    {
        // 已有符文 → 选中（拖拽是主要移动方式，这里保留点选作为备用）
        SelectedCell = Cell;
        SelectedRuneIndex = -1;
        OnSelectionChanged();
    }
    else if (SelectedCell != FIntPoint(-1, -1))
    {
        // 空格 + 已选中格子 → 点击移动（拖拽的备用方式）
        int32 SrcIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
        if (SrcIdx >= 0)
        {
            const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
            if (Placed.IsValidIndex(SrcIdx))
            {
                FGuid RuneGuid = Placed[SrcIdx].Rune.RuneGuid;
                FName RuneName = Placed[SrcIdx].Rune.RuneConfig.RuneName;
                if (Backpack->MoveRune(RuneGuid, Cell))
                {
                    SelectedCell = FIntPoint(-1, -1);
                    OnSelectionChanged();
                    OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","MoveOK","已移动：{0}"), FText::FromName(RuneName)));
                }
                else
                {
                    OnStatusMessage(NSLOCTEXT("Backpack","MoveFail","无法移动：目标位置被占用"));
                }
            }
        }
    }
    else if (SelectedRuneIndex >= 0)
    {
        // 空格 + 已选中列表符文 → 放置
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        const int32 PendingCount = Player ? Player->PendingRunes.Num() : 0;
        const bool bFromPending  = SelectedRuneIndex < PendingCount;

        FRuneInstance Instance;
        if (bFromPending)
        {
            Instance = Player->PendingRunes[SelectedRuneIndex];
        }
        else
        {
            const int32 AvIdx = SelectedRuneIndex - PendingCount;
            if (!AvailableRunes.IsValidIndex(AvIdx) || !AvailableRunes[AvIdx]) return;
            Instance = AvailableRunes[AvIdx]->CreateInstance();
        }

        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            if (bFromPending && Player)
            {
                Player->PendingRunes.RemoveAt(SelectedRuneIndex);
                OnRuneListChanged();
            }
            SelectedRuneIndex = -1;
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","PlaceOK","已放置：{0}"), FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack","PlaceFail","无法放置：位置被占用"));
        }
    }
}

void UBackpackScreenWidget::RemoveRuneAtSelectedCell()
{
    if (SelectedCell == FIntPoint(-1, -1)) return;

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
    if (RuneIdx < 0) { OnStatusMessage(NSLOCTEXT("Backpack","RemoveEmpty","该格子没有符文")); return; }

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(RuneIdx)) return;

    FGuid RuneGuid = Placed[RuneIdx].Rune.RuneGuid;
    FName RuneName = Placed[RuneIdx].Rune.RuneConfig.RuneName;

    if (Backpack->RemoveRune(RuneGuid))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","RemoveOK","已移除：{0}"), FText::FromName(RuneName)));
    }
}

void UBackpackScreenWidget::ClearSelection()
{
    SelectedRuneIndex = -1;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

void UBackpackScreenWidget::OpenBackpack()
{
    SetVisibility(ESlateVisibility::Visible);

    if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(GetOwningPlayer()))
    {
        PC->SetPause(true);
        PC->SetBlockGameInput(true, true);  // UIOnly 模式，显示鼠标
    }

    // 请求焦点，使手柄输入生效
    SetUserFocus(GetOwningPlayer());
}

void UBackpackScreenWidget::CloseBackpack()
{
    SetVisibility(ESlateVisibility::Collapsed);

    // 清空持有状态
    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    ClearSelection();

    if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(GetOwningPlayer()))
    {
        PC->SetPause(false);
        PC->SetBlockGameInput(false);  // 恢复 Game 模式
    }
}

// ============================================================
//  格子绑定（拖拽模式：按钮设为 HitTestInvisible，不绑 OnClicked）
// ============================================================

void UBackpackScreenWidget::BindGridCellClicks()
{
    if (!BackpackGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUI] BackpackGrid 未找到，检查 Designer 中面板名称"));
        return;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 GW = Backpack ? Backpack->GridWidth  : 5;
    const int32 GH = Backpack ? Backpack->GridHeight : 5;

    // 清空旧子节点（每次 NativeConstruct 重新创建，支持运行时尺寸变化）
    BackpackGrid->ClearChildren();
    CachedCellButtons.Empty();
    CachedCellIcons.Empty();

    BackpackGrid->SetVisibility(ESlateVisibility::HitTestInvisible);

    for (int32 Row = 0; Row < GH; Row++)
    {
        for (int32 Col = 0; Col < GW; Col++)
        {
            UButton* Btn = NewObject<UButton>(this);
            Btn->SetVisibility(ESlateVisibility::HitTestInvisible);

            UUniformGridSlot* GridSlot = BackpackGrid->AddChildToUniformGrid(Btn, Row, Col);

            UImage* Icon = NewObject<UImage>(this);
            Icon->SetVisibility(ESlateVisibility::Collapsed);
            Btn->SetContent(Icon);

            CachedCellButtons.Add(Btn);
            CachedCellIcons.Add(Icon);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] 动态创建主格子 %d×%d = %d 个"), GW, GH, GW * GH);
}

// ============================================================
//  左侧待放置符文槽
// ============================================================

void UBackpackScreenWidget::BindPendingRuneSlots()
{
    if (!PendingRuneGrid) return;

    PendingRuneGrid->ClearChildren();
    CachedPendingButtons.Empty();
    CachedPendingIcons.Empty();

    PendingRuneGrid->SetVisibility(ESlateVisibility::HitTestInvisible);

    const int32 TotalSlots = FMath::Max(1, PendingGridCols) * FMath::Max(1, PendingGridRows);
    for (int32 i = 0; i < TotalSlots; i++)
    {
        const int32 Col = i % PendingGridCols;
        const int32 Row = i / PendingGridCols;

        UButton* Btn = NewObject<UButton>(this);
        Btn->SetVisibility(ESlateVisibility::HitTestInvisible);

        PendingRuneGrid->AddChildToUniformGrid(Btn, Row, Col);

        UImage* Icon = NewObject<UImage>(this);
        Icon->SetVisibility(ESlateVisibility::Collapsed);
        Btn->SetContent(Icon);

        CachedPendingButtons.Add(Btn);
        CachedPendingIcons.Add(Icon);
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] 动态创建待放置槽 %d×%d = %d 个"), PendingGridCols, PendingGridRows, TotalSlots);
}

void UBackpackScreenWidget::RefreshPendingRuneSlots()
{
    if (CachedPendingButtons.Num() == 0) return;

    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    const int32 PendingCount = Player ? Player->PendingRunes.Num() : 0;

    for (int32 i = 0; i < CachedPendingButtons.Num(); i++)
    {
        UButton* Btn = CachedPendingButtons[i];
        if (!Btn) continue;

        if (i < PendingCount)
        {
            // 有符文：显示图标
            Btn->SetBackgroundColor(FLinearColor(0.12f, 0.08f, 0.22f, 1.f)); // 深紫
            if (CachedPendingIcons.IsValidIndex(i) && CachedPendingIcons[i])
            {
                UImage* Icon = CachedPendingIcons[i];
                UTexture2D* Tex = Player->PendingRunes[i].RuneConfig.RuneIcon;
                if (Tex)
                {
                    Icon->SetBrushFromTexture(Tex, false);
                    Icon->SetColorAndOpacity(FLinearColor::White);
                    Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
                }
                else { Icon->SetVisibility(ESlateVisibility::Collapsed); }
            }
        }
        else
        {
            // 空槽：灰色
            Btn->SetBackgroundColor(FLinearColor(0.06f, 0.06f, 0.06f, 1.f));
            if (CachedPendingIcons.IsValidIndex(i) && CachedPendingIcons[i])
                CachedPendingIcons[i]->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

bool UBackpackScreenWidget::GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const
{
    // 逐个检测 Button 几何体，兼容任意 Panel 布局
    for (int32 i = 0; i < CachedPendingButtons.Num(); i++)
    {
        UButton* Btn = CachedPendingButtons[i];
        if (!Btn) continue;

        const FGeometry BtnGeo  = Btn->GetCachedGeometry();
        const FVector2D Local   = BtnGeo.AbsoluteToLocal(AbsPos);
        const FVector2D BtnSize = BtnGeo.GetLocalSize();

        if (Local.X >= 0.f && Local.X < BtnSize.X && Local.Y >= 0.f && Local.Y < BtnSize.Y)
        {
            OutIndex = i;
            return true;
        }
    }
    return false;
}

// ============================================================
//  坐标辅助
// ============================================================

bool UBackpackScreenWidget::GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const
{
    if (!BackpackGrid) return false;

    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 GW = Backpack ? Backpack->GridWidth  : 5;
    const int32 GH = Backpack ? Backpack->GridHeight : 5;

    const FGeometry& GridGeo = BackpackGrid->GetCachedGeometry();
    const FVector2D  GridSize = GridGeo.GetLocalSize();
    if (GridSize.X <= 0.f || GridSize.Y <= 0.f) return false;

    const FVector2D LocalPos = GridGeo.AbsoluteToLocal(AbsolutePos);
    const float CellW = GridSize.X / GW;
    const float CellH = GridSize.Y / GH;

    const int32 Col = FMath::FloorToInt(LocalPos.X / CellW);
    const int32 Row = FMath::FloorToInt(LocalPos.Y / CellH);

    if (Col < 0 || Col >= GW || Row < 0 || Row >= GH) return false;

    OutCol = Col;
    OutRow = Row;
    return true;
}

// ============================================================
//  拖拽事件实现
// ============================================================

FReply UBackpackScreenWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);

    // ── 手柄A键检测：用自跟踪的 bIsGamepadInputMode 标志 ────────────────────
    // NativeOnKeyDown 任意手柄键按下时置 true，NativeOnMouseMove 鼠标移动时置 false
    // 比 IsInputKeyDown 可靠（IsInputKeyDown 在 PreviewMouseDown 阶段还未注册）
    if (bIsGamepadInputMode)
    {
        UE_LOG(LogTemp, Log, TEXT("[BackpackUI] PreviewMouseDown 手柄A键(bIsGamepadInputMode) → 直接调 GamepadConfirm"));
        GamepadConfirm();
        return FReply::Handled();
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] PreviewMouseDown  真实鼠标点击"));

    // ── 左侧待放置槽检测：优先于主格子 ──────────────────────────────────
    {
        int32 PendingIdx;
        if (GetPendingSlotAtScreenPos(InMouseEvent.GetScreenSpacePosition(), PendingIdx))
        {
            APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
            if (Player && PendingIdx < Player->PendingRunes.Num())
            {
                PendingDragIndex = PendingIdx;
                return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
            }
            return FReply::Handled(); // 空槽，吃掉事件
        }
    }

    int32 Col, Row;
    if (!GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);

    if (IsCellOccupied(Col, Row))
    {
        // 立即选中格子（详情面板即时响应）
        SelectedCell      = FIntPoint(Col, Row);
        SelectedRuneIndex = -1;
        // 真实鼠标点击时同步手柄光标
        GamepadCursorCell = FIntPoint(Col, Row);
        OnSelectionChanged();

        // 记录拖拽源，等待 NativeOnDragDetected
        PendingDragCol = Col;
        PendingDragRow = Row;

        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    else
    {
        // 空格子：放置列表符文 / 点击移动已选符文
        PendingDragCol = -1;
        PendingDragRow = -1;
        ClickCell(Col, Row);
        return FReply::Handled();
    }
}

void UBackpackScreenWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    // ── 左侧待放置符文拖拽 ────────────────────────────────────────────────
    if (PendingDragIndex >= 0)
    {
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        if (Player && PendingDragIndex < Player->PendingRunes.Num())
        {
            const FRuneInstance& PendingRune = Player->PendingRunes[PendingDragIndex];

            URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
            DragOp->SrcCol            = -1;
            DragOp->SrcRow            = -1;
            DragOp->SrcPivot          = FIntPoint(-1, -1);
            DragOp->DraggedRune       = PendingRune;
            DragOp->PendingSourceIndex = PendingDragIndex;

            UImage* DragVisual = NewObject<UImage>(this);
            if (UTexture2D* Tex = PendingRune.RuneConfig.RuneIcon)
                DragVisual->SetBrushFromTexture(Tex, false);
            DragVisual->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f));

            FWidgetTransform T;
            T.Scale       = FVector2D(1.08f, 1.08f);
            T.Translation = FVector2D(0.f, -8.f);
            DragVisual->SetRenderTransform(T);

            DragOp->DefaultDragVisual = DragVisual;
            DragOp->Pivot             = EDragPivot::MouseDown;

            PendingDragIndex = -1;
            OutOperation = DragOp;
            return;
        }
        PendingDragIndex = -1;
        return;
    }

    // ── 主格子符文拖拽 ────────────────────────────────────────────────────
    if (PendingDragCol < 0 || PendingDragRow < 0)
        return;

    FPlacedRune PR = GetRuneAtCell(PendingDragCol, PendingDragRow);
    if (!PR.Rune.RuneGuid.IsValid())
    {
        PendingDragCol = PendingDragRow = -1;
        return;
    }

    URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
    DragOp->SrcCol      = PendingDragCol;
    DragOp->SrcRow      = PendingDragRow;
    DragOp->SrcPivot    = PR.Pivot;   // 记录原始 Pivot，用于多格形状的落点偏移计算
    DragOp->DraggedRune = PR.Rune;

    // 拖拽视觉：符文图标（半透明）
    UImage* DragVisual = NewObject<UImage>(this);
    if (UTexture2D* Tex = PR.Rune.RuneConfig.RuneIcon)
        DragVisual->SetBrushFromTexture(Tex, /*bMatchSize=*/false);
    DragVisual->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f));

    // 浮空效果：略微放大 + 向上偏移
    FWidgetTransform DragTransform;
    DragTransform.Scale       = FVector2D(1.08f, 1.08f);
    DragTransform.Translation = FVector2D(0.f, -8.f);
    DragVisual->SetRenderTransform(DragTransform);

    DragOp->DefaultDragVisual = DragVisual;
    DragOp->Pivot             = EDragPivot::MouseDown;

    PendingDragCol = PendingDragRow = -1;
    OutOperation = DragOp;
}

bool UBackpackScreenWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (!Cast<URuneDragDropOperation>(InOperation))
        return false;

    int32 Col, Row;
    if (GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), Col, Row))
    {
        if (HoverCol != Col || HoverRow != Row)
        {
            HoverCol = Col;
            HoverRow = Row;
            OnGridNeedsRefresh();
        }
    }
    else if (HoverCol != -1 || HoverRow != -1)
    {
        HoverCol = HoverRow = -1;
        OnGridNeedsRefresh();
    }

    return true;
}

bool UBackpackScreenWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    HoverCol = HoverRow = -1;

    URuneDragDropOperation* RuneOp = Cast<URuneDragDropOperation>(InOperation);
    if (!RuneOp)
    {
        OnGridNeedsRefresh();
        return false;
    }

    // ── 左侧待放置符文 → 主格子放置 ──────────────────────────────────────
    if (RuneOp->PendingSourceIndex >= 0)
    {
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Player || !Backpack || RuneOp->PendingSourceIndex >= Player->PendingRunes.Num())
        {
            OnGridNeedsRefresh();
            return false;
        }

        int32 TargetCol, TargetRow;
        if (!GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
        {
            OnGridNeedsRefresh();
            return false;
        }

        const FRuneInstance PendingRune = Player->PendingRunes[RuneOp->PendingSourceIndex];
        if (Backpack->TryPlaceRune(PendingRune, FIntPoint(TargetCol, TargetRow)))
        {
            Player->PendingRunes.RemoveAt(RuneOp->PendingSourceIndex);
            RefreshPendingRuneSlots();
            OnRuneListChanged();
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PendingPlaceOK", "已放置：{0}"),
                FText::FromName(PendingRune.RuneConfig.RuneName)));
            return true;
        }

        OnStatusMessage(NSLOCTEXT("Backpack", "PendingPlaceFail", "无法放置：位置被占用"));
        OnGridNeedsRefresh();
        return false;
    }

    int32 TargetCol, TargetRow;
    if (!GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
    {
        OnGridNeedsRefresh();
        return false;
    }

    // 拖到原格子，视为取消
    if (TargetCol == RuneOp->SrcCol && TargetRow == RuneOp->SrcRow)
    {
        OnGridNeedsRefresh();
        return false;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack)
    {
        OnGridNeedsRefresh();
        return false;
    }

    // 抓取偏移 = 抓取格 - 原始Pivot；新Pivot = 落点 - 抓取偏移
    // 保证"抓哪格，那格就落在鼠标松开的位置"，多格形状整体跟着移动
    const FIntPoint GrabOffset = FIntPoint(RuneOp->SrcCol, RuneOp->SrcRow) - RuneOp->SrcPivot;
    const FIntPoint NewPivot   = FIntPoint(TargetCol, TargetRow) - GrabOffset;

    if (Backpack->MoveRune(RuneOp->DraggedRune.RuneGuid, NewPivot))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
            FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName)));
        return true;
    }

    // MoveRune 失败：检查目标格是否被另一个符文占用 → 尝试互换
    const int32 DstIdx = Backpack->GetRuneIndexAtCell(FIntPoint(TargetCol, TargetRow));
    if (DstIdx >= 0)
    {
        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        if (Placed.IsValidIndex(DstIdx))
        {
            FRuneInstance RuneB  = Placed[DstIdx].Rune;
            FIntPoint     PivotA = RuneOp->SrcPivot;
            FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);

            const bool bPlaceA = Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotB);
            const bool bPlaceB = Backpack->TryPlaceRune(RuneB, PivotA);

            if (bPlaceA && bPlaceB)
            {
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                    FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName),
                    FText::FromName(RuneB.RuneConfig.RuneName)));
                return true;
            }

            // 互换失败（形状冲突），放回原位
            Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotA);
            Backpack->TryPlaceRune(RuneB, PivotB);
            OnStatusMessage(NSLOCTEXT("Backpack", "SwapFail", "无法互换：形状冲突"));
            OnGridNeedsRefresh();
            return false;
        }
    }

    OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法移动：目标位置被占用"));
    OnGridNeedsRefresh();
    return false;
}

FReply UBackpackScreenWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == HeldDirKey)
    {
        bDirKeyHeld    = false;
        HeldKeyTime    = 0.f;
        LastRepeatCount = 0;
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // ── 浮空抓取图标（每帧更新位置 + 上下浮动动画） ──────────────────────────
    if (GrabbedRuneIcon)
    {
        if (bGrabbingRune)
        {
            UTexture2D* Tex = GetRuneIconAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            if (Tex)
            {
                GrabbedRuneIcon->SetBrushFromTexture(Tex, false);
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

                // 计算 GamepadCursorCell 格子中心在本 Widget 局部坐标下的位置
                UWidget* GridWidget = GetWidgetFromName(TEXT("BackpackGrid"));
                if (GridWidget)
                {
                    const FGeometry& GridGeo = GridWidget->GetCachedGeometry();
                    const FVector2D  GridSize = GridGeo.GetLocalSize();
                    if (GridSize.X > 0.f && GridSize.Y > 0.f)
                    {
                        UBackpackGridComponent* BGComp = GetBackpack();
                        const int32 TGW = BGComp ? BGComp->GridWidth  : 5;
                        const int32 TGH = BGComp ? BGComp->GridHeight : 5;
                        const float CellW = GridSize.X / TGW;
                        const float CellH = GridSize.Y / TGH;
                        const FVector2D CellCenter(
                            (GamepadCursorCell.X + 0.5f) * CellW,
                            (GamepadCursorCell.Y + 0.5f) * CellH);

                        const FVector2D AbsPos   = GridGeo.LocalToAbsolute(CellCenter);
                        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(AbsPos);

                        // 上下浮动动画（sin 波形，振幅 5px，频率 3Hz）
                        const float FloatY = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 5.f;

                        // 图标中心对准格子中心（图标尺寸由 Designer 决定，用 CellW*0.7 估算半径）
                        const float HalfSize = CellW * 0.35f;
                        FWidgetTransform T;
                        T.Translation = LocalPos + FVector2D(-HalfSize, -HalfSize + FloatY);
                        T.Scale       = FVector2D(0.7f, 0.7f); // 略小于一格，防止遮挡格子颜色
                        GrabbedRuneIcon->SetRenderTransform(T);
                    }
                }
            }
            else
            {
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        else
        {
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (!bDirKeyHeld) return;

    // 焦点丢失时 NativeOnKeyUp 可能未触发，双重确认按键实际还在被按住
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (!PC->IsInputKeyDown(HeldDirKey))
        {
            bDirKeyHeld    = false;
            HeldKeyTime    = 0.f;
            LastRepeatCount = 0;
            return;
        }
    }

    HeldKeyTime += InDeltaTime;

    // 初始延迟未到，不重复
    if (HeldKeyTime < DirRepeatInitial) return;

    // 计算应当已发生的重复次数
    const int32 TargetCount = FMath::FloorToInt((HeldKeyTime - DirRepeatInitial) / DirRepeatRate);
    if (TargetCount <= LastRepeatCount) return;

    LastRepeatCount = TargetCount;

    if      (HeldDirKey == EKeys::Gamepad_DPad_Up)    MoveGamepadCursor( 0, -1);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Down)  MoveGamepadCursor( 0,  1);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MoveGamepadCursor(-1,  0);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MoveGamepadCursor( 1,  0);
}

void UBackpackScreenWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    HoverCol       = HoverRow       = -1;
    PendingDragCol = PendingDragRow = -1;
    OnGridNeedsRefresh();
}

// ============================================================
//  手柄输入
// ============================================================

FReply UBackpackScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] KeyDown: %s  IsRepeat=%d  CursorCell=(%d,%d)  bGrabbing=%d"),
        *Key.ToString(), InKeyEvent.IsRepeat() ? 1 : 0,
        GamepadCursorCell.X, GamepadCursorCell.Y, bGrabbingRune ? 1 : 0);

    // 任意手柄键按下 → 标记为手柄模式（鼠标移动时会重置为 false）
    bIsGamepadInputMode = true;

    // 屏蔽摇杆数字键：防止传给 Super 触发 Slate 焦点导航
    if (Key == EKeys::Gamepad_RightStick_Up   || Key == EKeys::Gamepad_RightStick_Down  ||
        Key == EKeys::Gamepad_RightStick_Left  || Key == EKeys::Gamepad_RightStick_Right ||
        Key == EKeys::Gamepad_LeftStick_Up     || Key == EKeys::Gamepad_LeftStick_Down   ||
        Key == EKeys::Gamepad_LeftStick_Left   || Key == EKeys::Gamepad_LeftStick_Right)
    {
        return FReply::Handled();
    }

    // Special Left / Tab：关闭背包（绕过 Enhanced Input 的暂停屏蔽）
    if (Key == EKeys::Gamepad_Special_Left || Key == EKeys::Tab)
    {
        CloseBackpack();
        return FReply::Handled();
    }

    // D-Pad 移动光标：立即移动一格，同时启动重复计时
    auto StartDirRepeat = [&](int32 DC, int32 DR)
    {
        MoveGamepadCursor(DC, DR);
        HeldDirKey     = Key;
        bDirKeyHeld    = true;
        HeldKeyTime    = 0.f;
        LastRepeatCount = 0;
        return FReply::Handled();
    };

    if (Key == EKeys::Gamepad_DPad_Up)    return StartDirRepeat( 0, -1);
    if (Key == EKeys::Gamepad_DPad_Down)  return StartDirRepeat( 0,  1);
    if (Key == EKeys::Gamepad_DPad_Left)  return StartDirRepeat(-1,  0);
    if (Key == EKeys::Gamepad_DPad_Right) return StartDirRepeat( 1,  0);

    // A / B / Y 不允许 OS 重复触发（防止按住 A 导致抓取后立即放置）
    if (!InKeyEvent.IsRepeat())
    {
        if (Key == EKeys::Gamepad_FaceButton_Bottom) { GamepadConfirm(); return FReply::Handled(); }
        if (Key == EKeys::Gamepad_FaceButton_Right)  { GamepadCancel();  return FReply::Handled(); }
        if (Key == EKeys::Gamepad_FaceButton_Top)
        {
            SelectedCell = GamepadCursorCell;
            RemoveRuneAtSelectedCell();
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 W = Backpack ? Backpack->GridWidth  : 5;
    const int32 H = Backpack ? Backpack->GridHeight : 5;

    const FIntPoint Before = GamepadCursorCell;
    GamepadCursorCell.X = FMath::Clamp(GamepadCursorCell.X + DCol, 0, W - 1);
    GamepadCursorCell.Y = FMath::Clamp(GamepadCursorCell.Y + DRow, 0, H - 1);
    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] MoveGamepadCursor (%d,%d)+(%d,%d) → (%d,%d)"),
        Before.X, Before.Y, DCol, DRow, GamepadCursorCell.X, GamepadCursorCell.Y);

    // 抓取中：SelectedCell 保持在抓取源格（黄色高亮 = "被抓的符文在哪"）
    // 未抓取：SelectedCell 跟随光标（黄色高亮 = "光标在哪"）
    if (!bGrabbingRune)
    {
        SelectedCell = GamepadCursorCell;
        OnSelectionChanged();
    }
    else
    {
        // 只刷新格子颜色（让悬浮目标格也能看到），不改变选中状态
        OnGridNeedsRefresh();
    }

    // 更新 Tooltip（传入虚拟本地坐标，Tooltip 显示在光标格右侧）
    UpdateTooltipForCell(GamepadCursorCell.X, GamepadCursorCell.Y, FVector2D::ZeroVector);
}

void UBackpackScreenWidget::GamepadConfirm()
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] GamepadConfirm  bGrabbing=%d  CursorCell=(%d,%d)  GrabbedFrom=(%d,%d)"),
        bGrabbingRune ? 1 : 0,
        GamepadCursorCell.X, GamepadCursorCell.Y,
        GrabbedFromCell.X, GrabbedFromCell.Y);

    if (!bGrabbingRune)
    {
        // 第一步：抓取
        int32 RuneIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);
        UE_LOG(LogTemp, Log, TEXT("[BackpackUI]   → 尝试抓取 (%d,%d)  RuneIdx=%d"),
            GamepadCursorCell.X, GamepadCursorCell.Y, RuneIdx);
        if (RuneIdx >= 0)
        {
            bGrabbingRune    = true;
            GrabbedFromCell  = GamepadCursorCell;
            SelectedCell     = GamepadCursorCell;
            OnSelectionChanged();
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabOK", "已抓取符文，移动光标后按 A 放置"));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabEmpty", "该格子没有符文"));
        }
    }
    else
    {
        // 第二步：放置或互换
        if (GamepadCursorCell == GrabbedFromCell)
        {
            UE_LOG(LogTemp, Log, TEXT("[BackpackUI]   → 光标未移动，取消抓取"));
            // 放回原位 = 取消
            GamepadCancel();
            return;
        }

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        int32 DstIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);

        UE_LOG(LogTemp, Log, TEXT("[BackpackUI]   → 放置 GrabbedFrom=(%d,%d) SrcIdx=%d → Target=(%d,%d) DstIdx=%d"),
            GrabbedFromCell.X, GrabbedFromCell.Y, SrcIdx,
            GamepadCursorCell.X, GamepadCursorCell.Y, DstIdx);

        if (SrcIdx < 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BackpackUI]   → 源格子符文不存在，取消"));
            bGrabbingRune = false;
            GrabbedFromCell = FIntPoint(-1, -1);
            return;
        }

        FRuneInstance RuneA = Placed[SrcIdx].Rune;

        if (DstIdx >= 0)
        {
            // 目标有符文 → 互换
            FRuneInstance RuneB     = Placed[DstIdx].Rune;
            FIntPoint     PivotA    = Placed[SrcIdx].Pivot;
            FIntPoint     PivotB    = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
        }
        else
        {
            // 目标空格 → 移动
            FIntPoint SrcPivot = Placed[SrcIdx].Pivot;
            FIntPoint Offset   = GrabbedFromCell - SrcPivot;
            FIntPoint NewPivot = GamepadCursorCell - Offset;

            UE_LOG(LogTemp, Log, TEXT("[BackpackUI]   → 移动计算: SrcPivot=(%d,%d)  GrabbedFrom=(%d,%d)  Offset=(%d,%d)  Cursor=(%d,%d)  NewPivot=(%d,%d)"),
                SrcPivot.X, SrcPivot.Y,
                GrabbedFromCell.X, GrabbedFromCell.Y,
                Offset.X, Offset.Y,
                GamepadCursorCell.X, GamepadCursorCell.Y,
                NewPivot.X, NewPivot.Y);

            const bool bMoved = Backpack->MoveRune(RuneA.RuneGuid, NewPivot);
            UE_LOG(LogTemp, Log, TEXT("[BackpackUI]   → MoveRune 结果: %s"), bMoved ? TEXT("成功") : TEXT("失败"));

            if (bMoved)
            {
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法放置：目标位置被占用"));
            }
        }

        bGrabbingRune   = false;
        GrabbedFromCell = FIntPoint(-1, -1);
        SelectedCell    = GamepadCursorCell;
        OnGridNeedsRefresh();
    }
}

void UBackpackScreenWidget::GamepadCancel()
{
    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    SelectedCell    = FIntPoint(-1, -1);
    OnSelectionChanged();
    OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "已取消"));
}

void UBackpackScreenWidget::UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos)
{
    if (!RuneTooltip) return;

    if (IsCellOccupied(Col, Row))
    {
        FPlacedRune PR = GetRuneAtCell(Col, Row);
        RuneTooltip->ShowRuneInfo(PR.Rune);

        // 定位：偏移到鼠标/光标右下方
        const FVector2D Offset(16.f, -8.f);
        RuneTooltip->SetRenderTranslation(LocalPos + Offset);
    }
    else
    {
        RuneTooltip->HideTooltip();
    }
}

// ============================================================
//  鼠标移动：Tooltip 跟随
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 只有超过阈值的移动才切回鼠标模式
    // 防止：① D-Pad 触发 Slate 虚拟光标重定位产生的小 delta ② 手无意碰到鼠标
    const FVector2D Delta = InMouseEvent.GetCursorDelta();
    if (Delta.SizeSquared() > 64.f)   // 8px 以上才视为真实鼠标操作
    {
        bIsGamepadInputMode = false;
        UE_LOG(LogTemp, Verbose, TEXT("[BackpackUI] MouseMove delta=%.1f → 切换鼠标模式"), FMath::Sqrt(Delta.SizeSquared()));
    }

    int32 Col, Row;
    if (GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
    {
        const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        UpdateTooltipForCell(Col, Row, LocalPos);
    }
    else if (RuneTooltip)
    {
        RuneTooltip->HideTooltip();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}
