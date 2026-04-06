#include "Component/BackpackGridComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
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

	// Transcendence：激活区同 Tier3（4×4）
	FRuneShape TranscendenceShape = Tier3Shape;

	Config.ZoneShapes = { Tier1Shape, Tier2Shape, Tier3Shape, TranscendenceShape };
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

	// 自动放置符文（延迟一帧，确保 ASC 已初始化）
	if (PermanentRunes.Num() > 0 || DebugTestRunes.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UBackpackGridComponent::DebugPlaceTestRunes);
	}
}

void UBackpackGridComponent::DebugPlaceTestRunes()
{
	// 永久符文：自动寻位放置，并标记 bIsPermanent（始终激活，跳过区域检查）
	for (URuneDataAsset* DA : PermanentRunes)
	{
		if (!DA) continue;
		FRuneInstance Instance = DA->CreateInstance();
		bool bPlaced = false;
		for (int32 Y = 0; Y < GridHeight && !bPlaced; Y++)
		{
			for (int32 X = 0; X < GridWidth && !bPlaced; X++)
			{
				if (TryPlaceRune(Instance, FIntPoint(X, Y)))
				{
					// 标记为永久（TryPlaceRune 内部 RefreshAllActivations 时 bIsPermanent 还是 false，
					// 需要标记后再触发一次激活）
					for (int32 i = PlacedRunes.Num() - 1; i >= 0; i--)
					{
						if (PlacedRunes[i].Rune.RuneGuid == Instance.RuneGuid)
						{
							PlacedRunes[i].bIsPermanent = true;
							ActivateRune(PlacedRunes[i]); // 直接激活，跳过区域检查
							break;
						}
					}
					bPlaced = true;
				}
			}
		}
		if (!bPlaced)
		{
			UE_LOG(LogTemp, Warning, TEXT("PermanentRune [%s] FAILED - no space"),
				*Instance.RuneName.ToString());
		}
	}

	// Debug 符文：可选指定位置
	for (int32 i = 0; i < DebugTestRunes.Num(); i++)
	{
		if (!DebugTestRunes[i])
			continue;

		FRuneInstance Instance = DebugTestRunes[i]->CreateInstance();

		// 优先使用手动指定的位置，否则自动寻位
		if (DebugTestPositions.IsValidIndex(i))
		{
			FIntPoint Pivot = DebugTestPositions[i];
			if (TryPlaceRune(Instance, Pivot))
			{
				UE_LOG(LogTemp, Log, TEXT("DebugPlaceTestRunes: [%s] placed at (%d,%d)"),
					*Instance.RuneName.ToString(), Pivot.X, Pivot.Y);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DebugPlaceTestRunes: [%s] FAILED at (%d,%d) - collision or out of bounds"),
					*Instance.RuneName.ToString(), Pivot.X, Pivot.Y);
			}
		}
		else
		{
			// 自动寻位：逐格尝试
			bool bPlaced = false;
			for (int32 Y = 0; Y < GridHeight && !bPlaced; Y++)
			{
				for (int32 X = 0; X < GridWidth && !bPlaced; X++)
				{
					if (TryPlaceRune(Instance, FIntPoint(X, Y)))
					{
						UE_LOG(LogTemp, Log, TEXT("DebugPlaceTestRunes: [%s] auto-placed at (%d,%d)"),
							*Instance.RuneName.ToString(), X, Y);
						bPlaced = true;
					}
				}
			}
			if (!bPlaced)
			{
				UE_LOG(LogTemp, Warning, TEXT("DebugPlaceTestRunes: [%s] FAILED - no space"),
					*Instance.RuneName.ToString());
			}
		}
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

void UBackpackGridComponent::OnHeatValueChanged(float HeatValue)
{
	// 边沿触发 + Phase>0 保护：
	// 只在热度从 >0 跌落到 <=0、且当前已有阶段时广播，避免：
	//   1. 游戏开始 Phase=0 时启动无意义的计时器
	//   2. Timer 被反复启动（"Timer already active"）
	if (HeatValue <= 0.f && PreviousHeatValue > 0.f && CurrentPhase > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] Heat→0 edge (Phase=%d) → OnHeatReachedZero"), CurrentPhase);
		OnHeatReachedZero.Broadcast();
	}
	else if (HeatValue > 0.f && PreviousHeatValue <= 0.f)
	{
		OnHeatAboveZero.Broadcast();
	}
	PreviousHeatValue = HeatValue;
}

void UBackpackGridComponent::IncrementPhase()
{
	static constexpr int32 MaxPhase = 3;
	if (CurrentPhase >= MaxPhase)
		return;

	CurrentPhase++;
	EHeatTier NewTier = static_cast<EHeatTier>(FMath::Clamp(CurrentPhase, 0, 3));
	if (NewTier != CurrentTier)
	{
		CurrentTier = NewTier;
		OnHeatTierChanged.Broadcast(CurrentTier);
		RefreshAllActivations();
	}

	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] Phase UP → %d"), CurrentPhase);
}

void UBackpackGridComponent::DecrementPhase()
{
	if (CurrentPhase <= 0)
		return;

	CurrentPhase--;
	EHeatTier NewTier = static_cast<EHeatTier>(FMath::Clamp(CurrentPhase, 0, 3));
	if (NewTier != CurrentTier)
	{
		CurrentTier = NewTier;
		OnHeatTierChanged.Broadcast(CurrentTier);
		RefreshAllActivations();
	}

	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] Phase DOWN → %d"), CurrentPhase);
}

void UBackpackGridComponent::ResetHeatToPhaseFloor()
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
		return;

	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), 0.f);
	CurrentPhase = 0;
	CurrentTier  = EHeatTier::Tier1;
	OnHeatTierChanged.Broadcast(CurrentTier);
	RefreshAllActivations();
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

	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] ActivateRune: %s"), *Placed.Rune.RuneName.ToString());

	// GE 由 FA 内的 BFNode_ApplyRuneGE 节点负责施加（可在 Start/事件触发时执行）
	// GA 由 FA 内的 BFNode_GrantGA 节点负责授予
	// BackpackGrid 只负责启动 FA
	if (!Placed.Rune.Flow.FlowAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] ActivateRune FAILED: FlowAsset is null on %s"), *Placed.Rune.RuneName.ToString());
		return;
	}

	UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>();
	if (!BFC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] ActivateRune FAILED: BuffFlowComponent not found on %s"), *GetOwner()->GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] StartBuffFlow -> Rune: %s"), *Placed.Rune.RuneName.ToString());
	BFC->StartBuffFlow(Placed.Rune.Flow.FlowAsset, Placed.Rune.RuneGuid, GetOwner());
	Placed.bIsActivated = true;
	OnRuneActivationChanged.Broadcast(Placed.Rune.RuneGuid, true);
}

void UBackpackGridComponent::DeactivateRune(FPlacedRune& Placed)
{
	if (!Placed.bIsActivated)
		return;

	// FA 停止时，BFNode_ApplyRuneGE.Cleanup() 自动移除 GE
	//              BFNode_GrantGA.Cleanup()     自动撤销 GA
	// BackpackGrid 只负责停止 FA
	if (Placed.Rune.Flow.FlowAsset)
	{
		if (UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>())
		{
			BFC->StopBuffFlow(Placed.Rune.RuneGuid);
		}
	}

	Placed.bIsActivated = false;
	OnRuneActivationChanged.Broadcast(Placed.Rune.RuneGuid, false);
}

void UBackpackGridComponent::RefreshAllActivations()
{
	for (FPlacedRune& Placed : PlacedRunes)
	{
		// 永久符文跳过激活区检查，始终激活
		const bool bShouldActivate = Placed.bIsPermanent || IsRuneInActivationZone(Placed);

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