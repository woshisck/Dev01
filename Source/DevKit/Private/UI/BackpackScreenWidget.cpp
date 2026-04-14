#include "UI/BackpackScreenWidget.h"
#include "Component/BackpackGridComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameFramework/Pawn.h"

// ============================================================
//  内部辅助
// ============================================================

UBackpackGridComponent* UBackpackScreenWidget::GetBackpack() const
{
    if (CachedBackpack.IsValid())
    {
        return CachedBackpack.Get();
    }
    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUI] GetBackpack: Pawn is null"));
        return nullptr;
    }
    UBackpackGridComponent* Found = Pawn->FindComponentByClass<UBackpackGridComponent>();
    if (!Found)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUI] GetBackpack: BackpackGridComponent NOT found on %s"), *Pawn->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[BackpackUI] GetBackpack: OK, placed runes = %d"), Found->GetAllPlacedRunes().Num());
    }
    return Found;
}

// ============================================================
//  生命周期
// ============================================================

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 找到玩家的背包组件
    if (APawn* Pawn = GetOwningPlayerPawn())
    {
        CachedBackpack = Pawn->FindComponentByClass<UBackpackGridComponent>();
    }

    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.AddDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.AddDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.AddDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }
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
    // 如果移除的是当前选中格子的符文，清除格子选中状态
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        if (SelectedCell != FIntPoint(-1, -1))
        {
            int32 Idx = Backpack->GetRuneIndexAtCell(SelectedCell);
            if (Idx == -1)
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
//  状态查询
// ============================================================

bool UBackpackScreenWidget::IsCellInActivationZone(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;

    TArray<FIntPoint> ZoneCells = Backpack->GetActivationZoneCells();
    return ZoneCells.Contains(FIntPoint(Col, Row));
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
    if (!Placed.IsValidIndex(Idx)) return FPlacedRune();

    return Placed[Idx];
}

EBackpackCellState UBackpackScreenWidget::GetCellVisualState(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return EBackpackCellState::Empty;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    bool bInZone = IsCellInActivationZone(Col, Row);

    if (RuneIdx < 0)
    {
        // 格子为空
        return bInZone ? EBackpackCellState::EmptyActive : EBackpackCellState::Empty;
    }
    else
    {
        // 格子有符文
        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        if (Placed.IsValidIndex(RuneIdx) && Placed[RuneIdx].bIsActivated)
        {
            return EBackpackCellState::OccupiedActive;
        }
        return EBackpackCellState::OccupiedInactive;
    }
}

TArray<FRuneInstance> UBackpackScreenWidget::GetRuneList() const
{
    TArray<FRuneInstance> Result;

    // PendingRunes（三选一获得，放置后消耗）
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
    {
        Result.Append(Player->PendingRunes);
    }

    // AvailableRunes（固定展示库，放置后不消耗）
    for (const TObjectPtr<URuneDataAsset>& DA : AvailableRunes)
    {
        if (DA) Result.Add(DA->RuneInfo);
    }

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
    if (!Backpack) return Empty;

    return Backpack->GetAllPlacedRunes();
}

// ============================================================
//  操作
// ============================================================

void UBackpackScreenWidget::SelectRuneFromList(int32 Index)
{
    // 再次点击同一个 → 取消选中
    if (SelectedRuneIndex == Index)
    {
        SelectedRuneIndex = -1;
    }
    else
    {
        SelectedRuneIndex = Index;
    }

    // 选了新符文，清除格子选中
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
        // 格子有符文 → 选中这个格子（用于后续移动或移除）
        SelectedCell = Cell;
        SelectedRuneIndex = -1; // 清除列表选中，避免误放置
        OnSelectionChanged();
    }
    else if (SelectedCell != FIntPoint(-1, -1))
    {
        // 格子空 + 有选中格子 → 移动该格子的符文到此处
        int32 SrcRuneIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
        if (SrcRuneIdx >= 0)
        {
            const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
            if (Placed.IsValidIndex(SrcRuneIdx))
            {
                FGuid RuneGuid = Placed[SrcRuneIdx].Rune.RuneGuid;
                FName RuneName = Placed[SrcRuneIdx].Rune.RuneConfig.RuneName;
                if (Backpack->MoveRune(RuneGuid, Cell))
                {
                    SelectedCell = FIntPoint(-1, -1);
                    OnSelectionChanged();
                    OnStatusMessage(FText::Format(
                        NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
                        FText::FromName(RuneName)));
                }
                else
                {
                    OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法移动：目标位置被占用"));
                }
            }
        }
    }
    else if (SelectedRuneIndex >= 0)
    {
        // 格子空 + 有选中列表符文 → 放置符文
        // 判断来源：PendingRunes（消耗）还是 AvailableRunes（不消耗）
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        const int32 PendingCount = Player ? Player->PendingRunes.Num() : 0;
        const bool bFromPending = SelectedRuneIndex < PendingCount;

        FRuneInstance Instance;
        if (bFromPending)
        {
            // PendingRune：直接使用现有实例（已有有效 Guid）
            Instance = Player->PendingRunes[SelectedRuneIndex];
        }
        else
        {
            // AvailableRune：从 DA 创建新实例（生成新 Guid）
            const int32 AvailableIdx = SelectedRuneIndex - PendingCount;
            if (!AvailableRunes.IsValidIndex(AvailableIdx) || !AvailableRunes[AvailableIdx])
                return;
            Instance = AvailableRunes[AvailableIdx]->CreateInstance();
        }

        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            // 放置成功：如果是 Pending 来源，消耗（从列表移除）
            if (bFromPending && Player)
            {
                Player->PendingRunes.RemoveAt(SelectedRuneIndex);
                OnRuneListChanged();
            }
            SelectedRuneIndex = -1;
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PlaceOK", "已放置：{0}"),
                FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack", "PlaceFail", "无法放置：位置被占用"));
        }
    }
    // 格子空 + 无任何选中 → 无操作
}

void UBackpackScreenWidget::RemoveRuneAtSelectedCell()
{
    if (SelectedCell == FIntPoint(-1, -1)) return;

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
    if (RuneIdx < 0)
    {
        OnStatusMessage(NSLOCTEXT("Backpack", "RemoveEmpty", "该格子没有符文"));
        return;
    }

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(RuneIdx)) return;

    FGuid RuneGuid = Placed[RuneIdx].Rune.RuneGuid;
    FName RuneName = Placed[RuneIdx].Rune.RuneConfig.RuneName;

    if (Backpack->RemoveRune(RuneGuid))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "RemoveOK", "已移除：{0}"),
            FText::FromName(RuneName)));
    }
}

void UBackpackScreenWidget::ClearSelection()
{
    SelectedRuneIndex = -1;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}
