#include "Component/BackpackGridComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

// =========================================================
// FActivationZoneConfig
// =========================================================

FActivationZoneConfig FActivationZoneConfig::MakeDefault()
{
	FActivationZoneConfig Config;
	Config.HeatTierThresholds = { 0.33f, 0.66f };

	// Tier1：中心 1×1，格子 (2,2)
	FRuneShape Tier1Shape;
	Tier1Shape.Cells = { FIntPoint(2, 2) };

	// Tier2：2×2，格子 (2,2)-(3,3)
	FRuneShape Tier2Shape;
	for (int32 Y = 2; Y <= 3; Y++)
		for (int32 X = 2; X <= 3; X++)
			Tier2Shape.Cells.Add(FIntPoint(X, Y));

	// Tier3：4×4，格子 (1,1)-(4,4)
	FRuneShape Tier3Shape;
	for (int32 Y = 1; Y <= 4; Y++)
		for (int32 X = 1; X <= 4; X++)
			Tier3Shape.Cells.Add(FIntPoint(X, Y));

	Config.ZoneShapes = { Tier1Shape, Tier2Shape, Tier3Shape };
	return Config;
}

// =========================================================
// UBackpackGridComponent
// =========================================================

UBackpackGridComponent::UBackpackGridComponent()
{
}

void UBackpackGridComponent::EditorCenterOnGrid()
{
}

void UBackpackGridComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化占用图，所有格子填 -1（空）
	GridOccupancy.Init(-1, GridWidth * GridHeight);

	// 若未手动配置激活区，使用默认矩形配置
	if (ActivationZoneConfig.ZoneShapes.IsEmpty())
	{
		ActivationZoneConfig = FActivationZoneConfig::MakeDefault();
	}
}

// =========================================================
// 公开接口
// =========================================================

bool UBackpackGridComponent::TryPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot)
{
	if (bIsLocked)
		return false;

	if (!CanPlaceRune(Rune, Pivot))
		return false;

	const int32 NewIndex = PlacedRunes.Num();

	FPlacedRune Placed;
	Placed.Rune = Rune;
	Placed.Pivot = Pivot;
	Placed.bIsActivated = false;
	PlacedRunes.Add(Placed);

	// 更新占用图
	for (const FIntPoint Cell : GetRuneCells(Rune, Pivot))
	{
		GridOccupancy[CellToIndex(Cell)] = NewIndex;
	}

	// 重算激活状态（新放置的符文可能已在激活区内）
	RefreshAllActivations();

	OnRunePlaced.Broadcast(Rune);
	return true;
}

bool UBackpackGridComponent::RemoveRune(FGuid RuneGuid)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < PlacedRunes.Num(); i++)
	{
		if (PlacedRunes[i].Rune.RuneGuid == RuneGuid)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
		return false;

	// 先取消激活（移除 GE）
	DeactivateRune(PlacedRunes[FoundIndex]);

	// 清空占用图
	for (const FIntPoint Cell : GetRuneCells(PlacedRunes[FoundIndex].Rune, PlacedRunes[FoundIndex].Pivot))
	{
		GridOccupancy[CellToIndex(Cell)] = -1;
	}

	PlacedRunes.RemoveAt(FoundIndex);

	// 更新后续符文在占用图中的下标（RemoveAt 导致下标偏移）
	for (int32 i = FoundIndex; i < PlacedRunes.Num(); i++)
	{
		for (const FIntPoint Cell : GetRuneCells(PlacedRunes[i].Rune, PlacedRunes[i].Pivot))
		{
			GridOccupancy[CellToIndex(Cell)] = i;
		}
	}

	RefreshAllActivations();
	OnRuneRemoved.Broadcast(RuneGuid);
	return true;
}

bool UBackpackGridComponent::MoveRune(FGuid RuneGuid, FIntPoint NewPivot)
{
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < PlacedRunes.Num(); i++)
	{
		if (PlacedRunes[i].Rune.RuneGuid == RuneGuid)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
		return false;

	const FIntPoint OldPivot = PlacedRunes[FoundIndex].Pivot;
	const FRuneInstance RuneRef = PlacedRunes[FoundIndex].Rune;

	// 临时清空当前位置，避免自遮挡
	for (const FIntPoint Cell : GetRuneCells(RuneRef, OldPivot))
	{
		GridOccupancy[CellToIndex(Cell)] = -1;
	}

	// 检查新位置
	if (!CanPlaceRune(RuneRef, NewPivot))
	{
		// 回滚
		for (const FIntPoint Cell : GetRuneCells(RuneRef, OldPivot))
		{
			GridOccupancy[CellToIndex(Cell)] = FoundIndex;
		}
		return false;
	}

	// 更新到新位置
	PlacedRunes[FoundIndex].Pivot = NewPivot;
	for (const FIntPoint Cell : GetRuneCells(RuneRef, NewPivot))
	{
		GridOccupancy[CellToIndex(Cell)] = FoundIndex;
	}

	RefreshAllActivations();
	return true;
}

bool UBackpackGridComponent::CanPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot) const
{
	const TArray<FIntPoint> Cells = GetRuneCells(Rune, Pivot);
	if (Cells.IsEmpty())
		return false;

	for (const FIntPoint Cell : Cells)
	{
		if (!IsCellValid(Cell))
			return false;
		if (GridOccupancy[CellToIndex(Cell)] != -1)
			return false;
	}
	return true;
}

void UBackpackGridComponent::SetLocked(bool bLocked)
{
	bIsLocked = bLocked;
}

void UBackpackGridComponent::OnHeatPercentChanged(float HeatPercent)
{
	if (ActivationZoneConfig.HeatTierThresholds.Num() < 2)
		return;

	EHeatTier NewTier;
	if (HeatPercent >= ActivationZoneConfig.HeatTierThresholds[1])
		NewTier = EHeatTier::Tier3;
	else if (HeatPercent >= ActivationZoneConfig.HeatTierThresholds[0])
		NewTier = EHeatTier::Tier2;
	else
		NewTier = EHeatTier::Tier1;

	if (NewTier != CurrentTier)
	{
		CurrentTier = NewTier;
		OnHeatTierChanged.Broadcast(CurrentTier);
		RefreshAllActivations();
	}
}

TArray<FIntPoint> UBackpackGridComponent::GetActivationZoneCells() const
{
	return ComputeActivationZone().Array();
}

int32 UBackpackGridComponent::GetRuneIndexAtCell(FIntPoint Cell) const
{
	if (!IsCellValid(Cell))
		return -1;
	return GridOccupancy[CellToIndex(Cell)];
}

void UBackpackGridComponent::InitWithASC(UAbilitySystemComponent* ASC)
{
	CachedASC = ASC;
}

void UBackpackGridComponent::SetActivationZoneConfig(const FActivationZoneConfig& Config)
{
	ActivationZoneConfig = Config;
	RefreshAllActivations();
}

// =========================================================
// 私有算法
// =========================================================

TSet<FIntPoint> UBackpackGridComponent::ComputeActivationZone() const
{
	const int32 TierIndex = static_cast<int32>(CurrentTier);
	TSet<FIntPoint> ZoneSet;

	if (!ActivationZoneConfig.ZoneShapes.IsValidIndex(TierIndex))
		return ZoneSet;

	for (const FIntPoint Cell : ActivationZoneConfig.ZoneShapes[TierIndex].Cells)
	{
		if (IsCellValid(Cell))
			ZoneSet.Add(Cell);
	}
	return ZoneSet;
}

TArray<FIntPoint> UBackpackGridComponent::GetRuneCells(const FRuneInstance& Rune, FIntPoint Pivot) const
{
	TArray<FIntPoint> Cells;
	for (const FIntPoint Offset : Rune.Shape.Cells)
	{
		Cells.Add(Pivot + Offset);
	}
	return Cells;
}

bool UBackpackGridComponent::IsRuneInActivationZone(const FPlacedRune& Placed) const
{
	const TSet<FIntPoint> ZoneSet = ComputeActivationZone();
	if (ZoneSet.IsEmpty())
		return false;

	// 全部格子都在激活区内才算激活
	for (const FIntPoint Cell : GetRuneCells(Placed.Rune, Placed.Pivot))
	{
		if (!ZoneSet.Contains(Cell))
			return false;
	}
	return true;
}

void UBackpackGridComponent::ActivateRune(FPlacedRune& Placed)
{
	if (Placed.bIsActivated)
		return;

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(CachedASC.Get());
	if (!ASC)
		return;

	bool bActivated = false;

	// 1) 数值效果：从 DA 的 AttributeModifiers 动态构建 GE
	if (Placed.Rune.AttributeModifiers.Num() > 0)
	{
		Placed.ActiveEffectHandle = ASC->ApplyRuneModifiers(Placed.Rune.AttributeModifiers);
		bActivated |= Placed.ActiveEffectHandle.IsValid();
	}

	// 2) 行为效果：使用预制的 BehaviorEffect GE（击退、毒池等）
	if (Placed.Rune.BehaviorEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
			Placed.Rune.BehaviorEffect,
			Placed.Rune.Level,
			Context
		);
		if (Spec.IsValid())
		{
			Placed.BehaviorEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			bActivated |= Placed.BehaviorEffectHandle.IsValid();
		}
	}

	// 3) BuffFlow 可视化逻辑
	if (Placed.Rune.BuffFlowAsset)
	{
		if (UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>())
		{
			BFC->StartBuffFlow(Placed.Rune.BuffFlowAsset, Placed.Rune.RuneGuid, GetOwner());
			bActivated = true;
		}
	}

	Placed.bIsActivated = bActivated;
	if (bActivated)
	{
		OnRuneActivationChanged.Broadcast(Placed.Rune.RuneGuid, true);
	}
}

void UBackpackGridComponent::DeactivateRune(FPlacedRune& Placed)
{
	if (!Placed.bIsActivated)
		return;

	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (ASC)
	{
		// 移除数值 GE
		if (Placed.ActiveEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Placed.ActiveEffectHandle);
		}
		// 移除行为 GE
		if (Placed.BehaviorEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Placed.BehaviorEffectHandle);
		}
	}

	// 停止 BuffFlow
	if (Placed.Rune.BuffFlowAsset)
	{
		if (UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>())
		{
			BFC->StopBuffFlow(Placed.Rune.RuneGuid);
		}
	}

	Placed.ActiveEffectHandle = FActiveGameplayEffectHandle();
	Placed.BehaviorEffectHandle = FActiveGameplayEffectHandle();
	Placed.bIsActivated = false;
	OnRuneActivationChanged.Broadcast(Placed.Rune.RuneGuid, false);
}

void UBackpackGridComponent::RefreshAllActivations()
{
	for (FPlacedRune& Placed : PlacedRunes)
	{
		const bool bShouldActivate = IsRuneInActivationZone(Placed);

		if (bShouldActivate && !Placed.bIsActivated)
			ActivateRune(Placed);
		else if (!bShouldActivate && Placed.bIsActivated)
			DeactivateRune(Placed);
	}
}

int32 UBackpackGridComponent::CellToIndex(FIntPoint Cell) const
{
	return Cell.Y * GridWidth + Cell.X;
}

bool UBackpackGridComponent::IsCellValid(FIntPoint Cell) const
{
	return Cell.X >= 0 && Cell.X < GridWidth
		&& Cell.Y >= 0 && Cell.Y < GridHeight;
}