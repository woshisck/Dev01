#include "UI/BackpackScreenWidget.h"
#include "UI/RuneDragDropOperation.h"
#include "UI/RuneTooltipWidget.h"
#include "Component/BackpackGridComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameFramework/Pawn.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/PanelWidget.h"
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

    for (int32 i = 0; i < CellCount; i++)
    {
        UButton* Btn = CachedCellButtons[i];
        if (!Btn) continue;

        const int32 Col = i % 5;
        const int32 Row = i / 5;

        // 颜色优先级：选中 > 拖拽悬浮 > 状态色
        FLinearColor Color;
        if (IsCellSelected(Col, Row))
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

        // 图标：有符文则显示，没有则隐藏
        if (CachedCellIcons.IsValidIndex(i) && CachedCellIcons[i])
        {
            UImage* Icon = CachedCellIcons[i];
            UTexture2D* Tex = GetRuneIconAtCell(Col, Row);
            if (Tex)
            {
                Icon->SetBrushFromTexture(Tex, /*bMatchSize=*/false);
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

// ============================================================
//  格子绑定（拖拽模式：按钮设为 HitTestInvisible，不绑 OnClicked）
// ============================================================

void UBackpackScreenWidget::BindGridCellClicks()
{
    UPanelWidget* Grid = Cast<UPanelWidget>(GetWidgetFromName(TEXT("BackpackGrid")));
    if (!Grid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUI] BackpackGrid 面板未找到，检查蓝图中面板名称"));
        return;
    }

    // 面板本身也设为 HitTestInvisible，确保 BackpackScreenWidget 接管所有输入
    Grid->SetVisibility(ESlateVisibility::HitTestInvisible);

    CachedCellButtons.Empty();
    CachedCellIcons.Empty();

    for (int32 i = 0; i < Grid->GetChildrenCount(); i++)
    {
        // 支持 BackpackGrid→Button 和 BackpackGrid→SizeBox→Button 两种结构
        UWidget* Child = Grid->GetChildAt(i);
        UButton* Btn   = Cast<UButton>(Child);
        if (!Btn)
        {
            if (UPanelWidget* Container = Cast<UPanelWidget>(Child))
                Btn = Cast<UButton>(Container->GetChildAt(0));
        }
        if (!Btn) { CachedCellButtons.Add(nullptr); CachedCellIcons.Add(nullptr); continue; }

        // 按钮设为 HitTestInvisible：正常渲染颜色，但不拦截鼠标事件
        Btn->SetVisibility(ESlateVisibility::HitTestInvisible);

        // 在 Button 内部创建 Icon Image
        UImage* Icon = NewObject<UImage>(this);
        Icon->SetVisibility(ESlateVisibility::Collapsed);
        Btn->SetContent(Icon);

        CachedCellButtons.Add(Btn);
        CachedCellIcons.Add(Icon);
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] 已绑定 %d 个格子（拖拽模式）"), CachedCellButtons.Num());
}

// ============================================================
//  坐标辅助
// ============================================================

bool UBackpackScreenWidget::GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const
{
    UWidget* GridWidget = GetWidgetFromName(TEXT("BackpackGrid"));
    if (!GridWidget) return false;

    const FGeometry& GridGeo = GridWidget->GetCachedGeometry();
    const FVector2D GridSize = GridGeo.GetLocalSize();
    if (GridSize.X <= 0.f || GridSize.Y <= 0.f) return false;

    const FVector2D LocalPos = GridGeo.AbsoluteToLocal(AbsolutePos);

    const float CellW = GridSize.X / 5.f;
    const float CellH = GridSize.Y / 5.f;

    const int32 Col = FMath::FloorToInt(LocalPos.X / CellW);
    const int32 Row = FMath::FloorToInt(LocalPos.Y / CellH);

    if (Col < 0 || Col >= 5 || Row < 0 || Row >= 5) return false;

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

    int32 Col, Row;
    if (!GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);

    if (IsCellOccupied(Col, Row))
    {
        // 立即选中格子（详情面板即时响应）
        SelectedCell      = FIntPoint(Col, Row);
        SelectedRuneIndex = -1;
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

    // D-Pad 移动光标
    if (Key == EKeys::Gamepad_DPad_Up)    { MoveGamepadCursor( 0, -1); return FReply::Handled(); }
    if (Key == EKeys::Gamepad_DPad_Down)  { MoveGamepadCursor( 0,  1); return FReply::Handled(); }
    if (Key == EKeys::Gamepad_DPad_Left)  { MoveGamepadCursor(-1,  0); return FReply::Handled(); }
    if (Key == EKeys::Gamepad_DPad_Right) { MoveGamepadCursor( 1,  0); return FReply::Handled(); }

    // A 键：抓取 / 放置
    if (Key == EKeys::Gamepad_FaceButton_Bottom) { GamepadConfirm(); return FReply::Handled(); }

    // B 键：取消
    if (Key == EKeys::Gamepad_FaceButton_Right) { GamepadCancel(); return FReply::Handled(); }

    // Y 键：移除当前光标格子的符文
    if (Key == EKeys::Gamepad_FaceButton_Top)
    {
        SelectedCell = GamepadCursorCell;
        RemoveRuneAtSelectedCell();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 W = Backpack ? Backpack->GridWidth  : 5;
    const int32 H = Backpack ? Backpack->GridHeight : 5;

    GamepadCursorCell.X = FMath::Clamp(GamepadCursorCell.X + DCol, 0, W - 1);
    GamepadCursorCell.Y = FMath::Clamp(GamepadCursorCell.Y + DRow, 0, H - 1);

    // 更新选中高亮（视觉反馈）
    SelectedCell = GamepadCursorCell;
    OnSelectionChanged();

    // 更新 Tooltip（传入虚拟本地坐标，Tooltip 显示在光标格右侧）
    UpdateTooltipForCell(GamepadCursorCell.X, GamepadCursorCell.Y, FVector2D::ZeroVector);
}

void UBackpackScreenWidget::GamepadConfirm()
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    if (!bGrabbingRune)
    {
        // 第一步：抓取
        int32 RuneIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);
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
            // 放回原位 = 取消
            GamepadCancel();
            return;
        }

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        int32 DstIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);

        if (SrcIdx < 0)
        {
            // 源格子符文已不存在（被移除），取消抓取
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

            if (Backpack->MoveRune(RuneA.RuneGuid, NewPivot))
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
