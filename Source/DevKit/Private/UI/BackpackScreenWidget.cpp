#include "UI/BackpackScreenWidget.h"
#include "Component/BackpackGridComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameFramework/Pawn.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/PanelWidget.h"

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

    UBackpackGridComponent* Found = Pawn->FindComponentByClass<UBackpackGridComponent>();
    return Found;
}

// ============================================================
//  生命周期
// ============================================================

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

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

        // 颜色：选中格子用高亮黄，其余按状态设置
        FLinearColor Color;
        if (IsCellSelected(Col, Row))
        {
            Color = BackpackColors::Selected;
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
            // 已选中格子：提示下一步
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintMove", "已选中：{0}\n点击空格 → 移动\n「移除选中」→ 移除"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else if (SelectedRuneIndex >= 0)
        {
            // 已选中列表符文：提示放置
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintPlace", "已选中：{0}\n点击背包格子 → 放置"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else
        {
            HintText->SetText(NSLOCTEXT("Backpack", "HintIdle", "点击背包格子选中符文\n点击左侧列表选择待放置符文"));
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
        SelectedCell = Cell;
        SelectedRuneIndex = -1;
        OnSelectionChanged();
    }
    else if (SelectedCell != FIntPoint(-1, -1))
    {
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
//  格子按钮绑定 + Icon Image 自动创建
// ============================================================

void UBackpackScreenWidget::BindGridCellClicks()
{
    UPanelWidget* Grid = Cast<UPanelWidget>(GetWidgetFromName(TEXT("BackpackGrid")));
    if (!Grid)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUI] BackpackGrid 面板未找到，检查蓝图中面板名称"));
        return;
    }

    CachedCellButtons.Empty();
    CachedCellIcons.Empty();
    CellClickHandlers.Empty();

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

        // 绑定点击
        const int32 Col = i % 5;
        const int32 Row = i / 5;
        UGridCellClickHandler* Handler = NewObject<UGridCellClickHandler>(this);
        Handler->Owner = this;
        Handler->Col   = Col;
        Handler->Row   = Row;
        CellClickHandlers.Add(Handler);
        Btn->OnClicked.AddDynamic(Handler, &UGridCellClickHandler::HandleClick);

        // 在 Button 内部创建 Icon Image（覆盖整个按钮，不拦截点击）
        UImage* Icon = NewObject<UImage>(this);
        Icon->SetVisibility(ESlateVisibility::Collapsed);
        Btn->SetContent(Icon);

        CachedCellButtons.Add(Btn);
        CachedCellIcons.Add(Icon);
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackUI] 已绑定 %d 个格子（Button + Icon）"), CachedCellButtons.Num());
}

void UGridCellClickHandler::HandleClick()
{
    if (UBackpackScreenWidget* W = Owner.Get())
        W->ClickCell(Col, Row);
}
