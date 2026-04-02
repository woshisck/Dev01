#include "Component/BackpackGridComponent.h"
#include "AbilitySystemComponent.h"

UBackpackGridComponent::UBackpackGridComponent()
{
}

void UBackpackGridComponent::EditorCenterOnGrid()
{
}


bool UBackpackGridComponent::TryPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot)
{
    if (bIsLocked)
    {
        return false;
    }

	if (!CanPlaceRune(Rune, Pivot))
	{
		return false;
	}

    const int NewIndex = PlacedRunes.Num();
    FPlacedRune placed = FPlacedRune();
    PlacedRunes.Add(placed);


    return true;
}

// 移除指定 Guid 的符文，成功返回 true
bool UBackpackGridComponent::RemoveRune(FGuid RuneGuid)
{
    return false;
}

// 将指定符文移动到新位置（内部等价于 Remove + Place）

bool UBackpackGridComponent::MoveRune(FGuid RuneGuid, FIntPoint NewPivot)
{
    return true;
}
// 仅查询是否可放置，不实际放置

bool UBackpackGridComponent::CanPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot) const
{
    for (const FIntPoint cell : GetRuneCells(Rune, Pivot))
    {
        if (!IsCellValid(cell))
        {
            return false;
        }
        if (GridOccupancy[cell.Y * GridWidth + cell.X] != -1)
        {
            return false;
        }
    }
    return true;
}
void UBackpackGridComponent::SetLocked(bool bLocked)
{
}
// 锁定/解锁背包（战斗阶段锁定）


// 由 AttributeSet 的 PostAttributeChange 调用，传入热度百分比 (0~1)

void UBackpackGridComponent::OnHeatPercentChanged(float HeatPercent)
{

	if (HeatPercent >= ActivationZoneConfig.HeatTierThresholds[2])
	{
		CurrentTier = EHeatTier::Tier3;
	}
	else if (HeatPercent >= ActivationZoneConfig.HeatTierThresholds[1])
	{
		CurrentTier = EHeatTier::Tier2;
	}
	else if (HeatPercent >= ActivationZoneConfig.HeatTierThresholds[0])
	{
		CurrentTier = EHeatTier::Tier1;
	}
	else
	{
		CurrentTier = EHeatTier::Tier1;
	}
}


TArray<FIntPoint> UBackpackGridComponent::GetActivationZoneCells() const
{
    TArray<FIntPoint> EmptyArray;
    return EmptyArray;
}

// 查询某格是否被占用，返回 PlacedRunes 下标，-1表示空

int32 UBackpackGridComponent::GetRuneIndexAtCell(FIntPoint Cell) const
{
    return -1;
}
void UBackpackGridComponent::InitWithASC(UAbilitySystemComponent* ASC)
{
}
// 初始化 ASC 引用（在 PlayerCharacterBase::BeginPlay 中调用）


// 装备武器时调用，注入激活区配置（未调用时使用默认矩形配置）

void UBackpackGridComponent::SetActivationZoneConfig(const FActivationZoneConfig& Config)
{
    
}

void UBackpackGridComponent::BeginPlay()
{
    Super::BeginPlay();
}

TSet<FIntPoint> UBackpackGridComponent::ComputeActivationZone() const
{
    return TSet<FIntPoint>();
}

TArray<FIntPoint> UBackpackGridComponent::GetRuneCells(const FRuneInstance& Rune, FIntPoint Pivot) const
{
    return TArray<FIntPoint>();
}

bool UBackpackGridComponent::IsRuneInActivationZone(const FPlacedRune& Placed) const
{
    return false;
}

void UBackpackGridComponent::ActivateRune(FPlacedRune& Placed)
{
}

void UBackpackGridComponent::DeactivateRune(FPlacedRune& Placed)
{
}

void UBackpackGridComponent::RefreshAllActivations()
{
}

int32 UBackpackGridComponent::CellToIndex(FIntPoint Cell) const
{
    return int32();
}

bool UBackpackGridComponent::IsCellValid(FIntPoint Cell) const
{
    return false;
}
