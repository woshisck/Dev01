#include "Component/BackpackGridComponent.h"
#include "Engine/Engine.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagsManager.h"
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

// =========================================================
// 金币（Economy）
// =========================================================

void UBackpackGridComponent::AddGold(int32 Amount)
{
	if (Amount <= 0) return;
	Gold += Amount;
	OnGoldChanged.Broadcast(Gold);
}

bool UBackpackGridComponent::SpendGold(int32 Amount)
{
	if (Amount < 0 || Gold < Amount) return false;
	Gold -= Amount;
	OnGoldChanged.Broadcast(Gold);
	return true;
}

bool UBackpackGridComponent::CanAffordRune(const URuneDataAsset* DA) const
{
	if (!DA) return false;
	return Gold >= DA->RuneInfo.RuneConfig.GoldCost;
}

bool UBackpackGridComponent::BuyRune(URuneDataAsset* DA)
{
	if (!DA) return false;
	return SpendGold(DA->RuneInfo.RuneConfig.GoldCost);
}

bool UBackpackGridComponent::SellRune(FGuid RuneGuid)
{
	// 先找到符文，取出 GoldCost，再移除
	int32 GoldRefund = 0;
	for (const FPlacedRune& PR : PlacedRunes)
	{
		if (PR.Rune.RuneGuid == RuneGuid)
		{
			GoldRefund = PR.Rune.RuneConfig.GoldCost / 2;
			break;
		}
	}

	if (!RemoveRune(RuneGuid))
		return false;

	AddGold(GoldRefund);
	return true;
}

void UBackpackGridComponent::EnsureGridInitialized()
{
	if (GridOccupancy.IsEmpty())
	{
		GridOccupancy.Init(-1, GridWidth * GridHeight);
	}
	if (ActivationZoneConfig.ZoneShapes.IsEmpty())
	{
		ActivationZoneConfig = FActivationZoneConfig::MakeDefault();
	}
}

void UBackpackGridComponent::BeginPlay()
{
	Super::BeginPlay();

	// 若已被提前初始化（切关恢复路径在 BeginPlay 前调用了 TryPlaceRune），跳过重置
	EnsureGridInitialized();

	// 自动放置符文（延迟一帧，确保 ASC 已初始化）
	if (PermanentRunes.Num() > 0 || DebugTestRunes.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UBackpackGridComponent::DebugPlaceTestRunes);
	}

	// 隐藏被动符文：延迟一帧启动 BuffFlow，不占格子，不进 UI
	if (HiddenPassiveRunes.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UBackpackGridComponent::ActivateHiddenPassiveRunes);
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
				*Instance.RuneConfig.RuneName.ToString());
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
					*Instance.RuneConfig.RuneName.ToString(), Pivot.X, Pivot.Y);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("DebugPlaceTestRunes: [%s] FAILED at (%d,%d) - collision or out of bounds"),
					*Instance.RuneConfig.RuneName.ToString(), Pivot.X, Pivot.Y);
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
							*Instance.RuneConfig.RuneName.ToString(), X, Y);
						bPlaced = true;
					}
				}
			}
			if (!bPlaced)
			{
				UE_LOG(LogTemp, Warning, TEXT("DebugPlaceTestRunes: [%s] FAILED - no space"),
					*Instance.RuneConfig.RuneName.ToString());
			}
		}
	}
}

// =========================================================
// 隐藏被动符文
// =========================================================

void UBackpackGridComponent::ActivateHiddenPassiveRunes()
{
	UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>();
	if (!BFC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] ActivateHiddenPassiveRunes: BuffFlowComponent not found"));
		return;
	}

	for (URuneDataAsset* DA : HiddenPassiveRunes)
	{
		if (!DA || !DA->RuneInfo.Flow.FlowAsset) continue;

		FRuneInstance Instance = DA->CreateInstance();
		BFC->StartBuffFlow(Instance.Flow.FlowAsset, Instance.RuneGuid, GetOwner());

		UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] HiddenPassive activated: %s"),
			*Instance.RuneConfig.RuneName.ToString());
	}
}

// =========================================================
// 公开接口
// =========================================================

bool UBackpackGridComponent::TryPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot)
{
	if (bIsLocked)
		return false;

	// 懒初始化：BeginPlay 前（如切关恢复）调用时，主动初始化网格
	EnsureGridInitialized();

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
	// 网格未初始化时（BeginPlay 尚未执行）拒绝查询，由 TryPlaceRune 的 EnsureGridInitialized 保证初始化
	if (GridOccupancy.IsEmpty())
		return false;

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

	// Consumer 限制：不能放入任何热度阶段的激活区格子
	if (Rune.RuneConfig.ChainRole == ERuneChainRole::Consumer)
	{
		const TSet<FIntPoint> AllZoneCells = ComputeAllPossibleActivationCells();
		for (const FIntPoint Cell : Cells)
		{
			if (AllZoneCells.Contains(Cell))
			{
				UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] Consumer 符文 [%s] 不能放入激活区格子 (%d,%d)"),
					*Rune.RuneConfig.RuneName.ToString(), Cell.X, Cell.Y);
				return false;
			}
		}
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

	// 通知热度条 UI
	BroadcastHeatUI(HeatValue);
}

void UBackpackGridComponent::BroadcastHeatUI(float KnownHeatValue)
{
	float NormalizedHeat = 0.f;
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		const float MaxHeat = CachedASC->GetGameplayAttributeValue(
			UBaseAttributeSet::GetMaxHeatAttribute(), bFound);
		if (MaxHeat > KINDA_SMALL_NUMBER)
			NormalizedHeat = FMath::Clamp(KnownHeatValue / MaxHeat, 0.f, 1.f);
	}
	OnHeatBarUpdate.Broadcast(NormalizedHeat, CurrentPhase);
}

void UBackpackGridComponent::IncrementPhase()
{
	static constexpr int32 MaxPhase = 3;
	if (CurrentPhase >= MaxPhase)
		return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	// 移除旧阶段 Tag
	if (ASC && CurrentPhase > 0)
	{
		FGameplayTag OldTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Buff.Status.Heat.Phase.%d"), CurrentPhase)), false);
		if (OldTag.IsValid())
			ASC->RemoveLooseGameplayTag(OldTag);
	}

	CurrentPhase++;

	// 授予新阶段 Tag
	if (ASC)
	{
		FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Buff.Status.Heat.Phase.%d"), CurrentPhase)), false);
		if (NewTag.IsValid())
			ASC->AddLooseGameplayTag(NewTag);
	}

	RefreshAllActivations();

	// 输出激活区格子，方便调试
	{
		TSet<FIntPoint> Zone = ComputeActivationZone();
		FString ZoneStr;
		for (const FIntPoint& C : Zone)
			ZoneStr += FString::Printf(TEXT("(%d,%d) "), C.X, C.Y);
		UE_LOG(LogTemp, Warning, TEXT("[Heat] Phase %d → %d 完成 | 激活区 %d 格: %s"),
			CurrentPhase - 1, CurrentPhase, Zone.Num(), *ZoneStr);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
				FString::Printf(TEXT("[热度升阶] Phase %d → %d  激活区 %d 格"),
					CurrentPhase - 1, CurrentPhase, Zone.Num()));
	}

	// 通知热度条 UI（阶段变化时热度已被重置为 0）
	BroadcastHeatUI(0.f);
}

void UBackpackGridComponent::DecrementPhase()
{
	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] DecrementPhase called (CurrentPhase=%d)"), CurrentPhase);
	if (CurrentPhase <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] DecrementPhase SKIPPED: already at Phase 0"));
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());

	// 移除当前阶段 Tag
	if (ASC)
	{
		FGameplayTag OldTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Buff.Status.Heat.Phase.%d"), CurrentPhase)), false);
		if (OldTag.IsValid())
			ASC->RemoveLooseGameplayTag(OldTag);
	}

	CurrentPhase--;

	// 授予新阶段 Tag（Phase 0 无 Tag）
	if (ASC && CurrentPhase > 0)
	{
		FGameplayTag NewTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Buff.Status.Heat.Phase.%d"), CurrentPhase)), false);
		if (NewTag.IsValid())
			ASC->AddLooseGameplayTag(NewTag);
	}

	RefreshAllActivations();

	// 输出激活区格子，方便调试
	{
		TSet<FIntPoint> Zone = ComputeActivationZone();
		FString ZoneStr;
		for (const FIntPoint& C : Zone)
			ZoneStr += FString::Printf(TEXT("(%d,%d) "), C.X, C.Y);
		UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] Phase DOWN → %d | ActivationZone [%d cells]: %s"),
			CurrentPhase, Zone.Num(), *ZoneStr);
	}

	BroadcastHeatUI(0.f);
}

void UBackpackGridComponent::ResetHeatToPhaseFloor()
{
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
		return;

	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), 0.f);
	CurrentPhase = 0;
	RefreshAllActivations();
	BroadcastHeatUI(0.f);
}

void UBackpackGridComponent::RestorePhase(int32 Phase)
{
	CurrentPhase = FMath::Clamp(Phase, 0, 3);
	RefreshAllActivations();
	BroadcastHeatUI(0.f);
}

TArray<FIntPoint> UBackpackGridComponent::GetActivationZoneCells() const
{
	return ComputeActivationZone().Array();
}

TArray<FIntPoint> UBackpackGridComponent::GetActivationZoneCellsForPhase(int32 Phase) const
{
	if (!ActivationZoneConfig.ZoneShapes.IsValidIndex(Phase))
		return {};
	return ActivationZoneConfig.ZoneShapes[Phase].Cells;
}

int32 UBackpackGridComponent::GetRuneIndexAtCell(FIntPoint Cell) const
{
	if (GridOccupancy.IsEmpty())
		return -1;
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

void UBackpackGridComponent::ApplyBackpackConfig(int32 InGridWidth, int32 InGridHeight, const FActivationZoneConfig& InConfig)
{
	GridWidth  = FMath::Max(1, InGridWidth);
	GridHeight = FMath::Max(1, InGridHeight);
	ActivationZoneConfig = InConfig;

	// 重建占用网格（大小随新尺寸变化）
	GridOccupancy.Init(-1, GridWidth * GridHeight);

	// 重新索引仍在新网格内的已放置符文
	for (int32 i = 0; i < PlacedRunes.Num(); i++)
	{
		for (const FIntPoint Cell : GetRuneCells(PlacedRunes[i].Rune, PlacedRunes[i].Pivot))
		{
			if (IsCellValid(Cell))
				GridOccupancy[CellToIndex(Cell)] = i;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[BackpackGridComponent] ApplyBackpackConfig: Grid=%dx%d, ZoneShapes=%d"),
		GridWidth, GridHeight, ActivationZoneConfig.ZoneShapes.Num());
	RefreshAllActivations();
}

// =========================================================
// 私有算法
// =========================================================

TSet<FIntPoint> UBackpackGridComponent::ComputeActivationZone() const
{
	const int32 TierIndex = CurrentPhase;
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
	FRuneShape Shape = Rune.Shape;
	const int32 RotCount = Rune.Rotation % 4;
	for (int32 i = 0; i < RotCount; i++)
		Shape = Shape.Rotate90();

	TArray<FIntPoint> Cells;
	for (const FIntPoint Offset : Shape.Cells)
		Cells.Add(Pivot + Offset);
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

	UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] ActivateRune: %s"), *Placed.Rune.RuneConfig.RuneName.ToString());

	// Shape 空检查：没有 Cells 时符文仍可激活，但无法参与背包格子系统
	if (Placed.Rune.Shape.Cells.IsEmpty())
	{
		const FString DAName = GetNameSafe(Placed.Rune.SourceDA);
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] WARN: Rune '%s' (DA: %s) has no shape cells configured! Rune activates but occupies no grid cells."),
			*Placed.Rune.RuneConfig.RuneName.ToString(), *DAName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow,
				FString::Printf(TEXT("[Rune] Shape is NULL: %s"), *DAName));
		}
	}

	if (Placed.Rune.Flow.FlowAsset)
	{
		// FA 路径：启动 BuffFlow，由 FA 节点负责施加 GE/GA
		UBuffFlowComponent* BFC = GetOwner()->FindComponentByClass<UBuffFlowComponent>();
		if (!BFC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] ActivateRune FAILED: BuffFlowComponent not found on %s"), *GetOwner()->GetName());
			return;
		}
		UE_LOG(LogTemp, Log, TEXT("[BackpackGrid] StartBuffFlow -> Rune: %s"), *Placed.Rune.RuneConfig.RuneName.ToString());
		BFC->StartBuffFlow(Placed.Rune.Flow.FlowAsset, Placed.Rune.RuneGuid, GetOwner());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackpackGrid] ActivateRune SKIP: no FA on Rune %s"), *Placed.Rune.RuneConfig.RuneName.ToString());
		return;
	}

	Placed.bIsActivated = true;
	OnRuneActivationChanged.Broadcast(Placed.Rune.RuneGuid, true);
}

void UBackpackGridComponent::DeactivateRune(FPlacedRune& Placed)
{
	if (!Placed.bIsActivated)
		return;

	if (Placed.Rune.Flow.FlowAsset)
	{
		// FA 停止 → BFNode 的 Cleanup() 自动移除 GE/GA
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
	// 直接激活区
	const TSet<FIntPoint> DirectZone = ComputeActivationZone();
	// 链路传导扩展（BFS，Producer 向外传播）
	const TSet<FIntPoint> ChainZone  = ComputeChainActivatedCells(DirectZone);
	// 有效激活区 = 直接激活区 ∪ 链路激活区
	TSet<FIntPoint> EffectiveZone = DirectZone;
	EffectiveZone.Append(ChainZone);

	for (FPlacedRune& Placed : PlacedRunes)
	{
		// 永久符文跳过激活区检查，始终激活
		const bool bShouldActivate = Placed.bIsPermanent || IsRuneInZone(Placed, EffectiveZone);

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

FPlacedRune* UBackpackGridComponent::FindRuneByName(FName RuneName)
{
	for (FPlacedRune& Placed : PlacedRunes)
	{
		if (Placed.Rune.RuneConfig.RuneName == RuneName)
			return &Placed;
	}
	return nullptr;
}

void UBackpackGridComponent::NotifyRuneUpgraded(FGuid RuneGuid)
{
	for (FPlacedRune& Placed : PlacedRunes)
	{
		if (Placed.Rune.RuneGuid == RuneGuid)
		{
			// 重启 BuffFlow 使新的 UpgradeLevel 立即生效
			if (Placed.bIsActivated)
			{
				DeactivateRune(Placed);
				ActivateRune(Placed);
			}
			OnRuneActivationChanged.Broadcast(RuneGuid, Placed.bIsActivated);
			return;
		}
	}
}

TArray<FName> UBackpackGridComponent::GetMaxLevelRuneNames() const
{
	TArray<FName> Result;
	for (const FPlacedRune& Placed : PlacedRunes)
	{
		if (Placed.Rune.UpgradeLevel >= 2)
			Result.Add(Placed.Rune.RuneConfig.RuneName);
	}
	return Result;
}

bool UBackpackGridComponent::IsCellValid(FIntPoint Cell) const
{
	return Cell.X >= 0 && Cell.X < GridWidth
		&& Cell.Y >= 0 && Cell.Y < GridHeight;
}

// =========================================================
// 链路系统
// =========================================================

TSet<FIntPoint> UBackpackGridComponent::ComputeAllPossibleActivationCells() const
{
	TSet<FIntPoint> AllCells;
	for (const FRuneShape& Shape : ActivationZoneConfig.ZoneShapes)
	{
		for (const FIntPoint Cell : Shape.Cells)
		{
			if (IsCellValid(Cell))
				AllCells.Add(Cell);
		}
	}
	return AllCells;
}

FIntPoint UBackpackGridComponent::ChainDirectionToOffset(EChainDirection Dir)
{
	switch (Dir)
	{
	case EChainDirection::N:  return FIntPoint( 0, -1);
	case EChainDirection::S:  return FIntPoint( 0,  1);
	case EChainDirection::E:  return FIntPoint( 1,  0);
	case EChainDirection::W:  return FIntPoint(-1,  0);
	case EChainDirection::NE: return FIntPoint( 1, -1);
	case EChainDirection::NW: return FIntPoint(-1, -1);
	case EChainDirection::SE: return FIntPoint( 1,  1);
	case EChainDirection::SW: return FIntPoint(-1,  1);
	default:                  return FIntPoint( 0,  0);
	}
}

TSet<FIntPoint> UBackpackGridComponent::ComputeChainActivatedCells(const TSet<FIntPoint>& DirectZone) const
{
	TSet<FIntPoint> ChainZone;

	// BFS 队列：存放已确认"处于有效区内"的 Producer 的 PlacedRunes 下标
	TQueue<int32> Queue;
	TSet<int32>   Visited;

	// 将 DirectZone 内所有 Producer 符文作为种子
	for (int32 i = 0; i < PlacedRunes.Num(); i++)
	{
		const FPlacedRune& Placed = PlacedRunes[i];
		if (Placed.Rune.RuneConfig.ChainRole != ERuneChainRole::Producer)
			continue;
		if (Placed.Rune.RuneConfig.ChainDirections.IsEmpty())
			continue;

		// Producer 的所有格子必须都在 DirectZone 内才能传导
		bool bAllInZone = true;
		for (const FIntPoint Cell : GetRuneCells(Placed.Rune, Placed.Pivot))
		{
			if (!DirectZone.Contains(Cell)) { bAllInZone = false; break; }
		}
		if (bAllInZone)
		{
			Queue.Enqueue(i);
			Visited.Add(i);
		}
	}

	// BFS 传播
	while (!Queue.IsEmpty())
	{
		int32 Idx;
		Queue.Dequeue(Idx);
		const FPlacedRune& Producer = PlacedRunes[Idx];

		// 从 Producer 的每个格子，向每个配置方向传播
		for (const FIntPoint SrcCell : GetRuneCells(Producer.Rune, Producer.Pivot))
		{
			for (const EChainDirection Dir : Producer.Rune.RuneConfig.ChainDirections)
			{
				const FIntPoint Adj = SrcCell + ChainDirectionToOffset(Dir);
				if (!IsCellValid(Adj)) continue;
				if (DirectZone.Contains(Adj)) continue;   // 已在直接激活区，跳过
				if (ChainZone.Contains(Adj)) continue;    // 已传播过，跳过

				ChainZone.Add(Adj);

				// 若邻格内有 Producer，将其加入 BFS（实现多跳）
				const int32 AdjRuneIdx = GridOccupancy[CellToIndex(Adj)];
				if (AdjRuneIdx != INDEX_NONE && !Visited.Contains(AdjRuneIdx))
				{
					const FPlacedRune& AdjPlaced = PlacedRunes[AdjRuneIdx];
					if (AdjPlaced.Rune.RuneConfig.ChainRole == ERuneChainRole::Producer
						&& !AdjPlaced.Rune.RuneConfig.ChainDirections.IsEmpty())
					{
						// 邻格 Producer 被链路激活，检查其所有格子是否都在有效区内
						// （有效区 = DirectZone ∪ ChainZone，取近似：只要 Adj 在 ChainZone 即可进入队列）
						Visited.Add(AdjRuneIdx);
						Queue.Enqueue(AdjRuneIdx);
					}
				}
			}
		}
	}

	return ChainZone;
}

bool UBackpackGridComponent::IsRuneInZone(const FPlacedRune& Placed, const TSet<FIntPoint>& Zone) const
{
	if (Zone.IsEmpty()) return false;
	for (const FIntPoint Cell : GetRuneCells(Placed.Rune, Placed.Pivot))
	{
		if (!Zone.Contains(Cell)) return false;
	}
	return true;
}