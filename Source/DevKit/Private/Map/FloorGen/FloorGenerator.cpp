#include "Map/FloorGen/FloorGenerator.h"

#include "Data/FloorGenConfig.h"
#include "Data/RoomDataAsset.h"
#include "HAL/PlatformTime.h"
#include "Math/RandomStream.h"

DEFINE_LOG_CATEGORY(LogYogFloorGen);

namespace
{
	struct FFloorGenNode
	{
		int32 Parent = INDEX_NONE;
		int32 Degree = 0;
		int32 RuleIndex = INDEX_NONE;
		float Depth = 0.f;
		bool bMainPath = false;
		FGameplayTag Type;

		// Rooms this node may use, best first. Placement walks the list when a room does not fit.
		TArray<URoomDataAsset*> Candidates;
	};

	struct FFloorGenOption
	{
		URoomDataAsset* Room = nullptr;
		FIntPoint Origin = FIntPoint::ZeroValue;
		FRoomDoorSocket ParentSocket;
		FRoomDoorSocket ChildSocket;
	};

	struct FFloorGenPlaced
	{
		URoomDataAsset* Room = nullptr;
		FIntPoint Origin = FIntPoint::ZeroValue;
		TArray<FIntPoint> Cells;
		TArray<FRoomDoorSocket> UsedSockets;
		FRoomDoorSocket ParentSocket;
		FRoomDoorSocket ChildSocket;
	};
}

static const ERoomDoorDir FloorGen_AllDirs[] = {
	ERoomDoorDir::PosX, ERoomDoorDir::NegX, ERoomDoorDir::PosY, ERoomDoorDir::NegY};

static void FloorGen_GetFootprint(const URoomDataAsset* Room, TArray<FIntPoint>& OutCells)
{
	OutCells = Room->FootprintCells;
	if (OutCells.Num() == 0)
	{
		OutCells.Add(FIntPoint::ZeroValue);
	}
}

static void FloorGen_GetSockets(const URoomDataAsset* Room, TArray<FRoomDoorSocket>& OutSockets)
{
	OutSockets.Reset();
	if (Room->DoorSockets.Num() > 0)
	{
		OutSockets = Room->DoorSockets;
		return;
	}

	TArray<FIntPoint> Cells;
	FloorGen_GetFootprint(Room, Cells);
	const TSet<FIntPoint> CellSet(Cells);
	for (const FIntPoint& Cell : Cells)
	{
		for (const ERoomDoorDir Dir : FloorGen_AllDirs)
		{
			if (CellSet.Contains(Cell + FFloorGrid::DirOffset(Dir)))
			{
				continue;
			}
			FRoomDoorSocket Socket;
			Socket.Cell = Cell;
			Socket.Direction = Dir;
			OutSockets.Add(Socket);
		}
	}
}

static bool FloorGen_ValidateRoom(const URoomDataAsset* Room, EFloorGenFailure& OutFailure, FString& OutDetail)
{
	TArray<FIntPoint> Cells;
	FloorGen_GetFootprint(Room, Cells);

	TSet<FIntPoint> CellSet;
	for (const FIntPoint& Cell : Cells)
	{
		bool bAlreadyInSet = false;
		CellSet.Add(Cell, &bAlreadyInSet);
		if (bAlreadyInSet)
		{
			OutFailure = EFloorGenFailure::InvalidFootprint;
			OutDetail = Room->GetName();
			return false;
		}
	}

	if (!CellSet.Contains(FIntPoint::ZeroValue))
	{
		OutFailure = EFloorGenFailure::InvalidFootprint;
		OutDetail = Room->GetName();
		return false;
	}

	TSet<FIntPoint> Visited;
	TArray<FIntPoint> Stack;
	Stack.Add(FIntPoint::ZeroValue);
	Visited.Add(FIntPoint::ZeroValue);
	while (Stack.Num() > 0)
	{
		const FIntPoint Current = Stack.Pop();
		for (const ERoomDoorDir Dir : FloorGen_AllDirs)
		{
			const FIntPoint Next = Current + FFloorGrid::DirOffset(Dir);
			if (CellSet.Contains(Next) && !Visited.Contains(Next))
			{
				Visited.Add(Next);
				Stack.Add(Next);
			}
		}
	}
	if (Visited.Num() != CellSet.Num())
	{
		OutFailure = EFloorGenFailure::InvalidFootprint;
		OutDetail = Room->GetName();
		return false;
	}

	for (const FRoomDoorSocket& Socket : Room->DoorSockets)
	{
		const bool bOnOuterEdge = CellSet.Contains(Socket.Cell)
			&& !CellSet.Contains(Socket.Cell + FFloorGrid::DirOffset(Socket.Direction));
		if (!bOnOuterEdge)
		{
			OutFailure = EFloorGenFailure::InvalidSocket;
			OutDetail = Room->GetName();
			return false;
		}
	}
	return true;
}

static EFloorGenFailure FloorGen_ValidateConfig(const UFloorGenConfig& Config, FString& OutDetail)
{
	if (Config.GridSize.X <= 0 || Config.GridSize.Y <= 0)
	{
		OutDetail = TEXT("GridSize must be positive");
		return EFloorGenFailure::InvalidConfig;
	}

	int32 StartRules = 0;
	int32 EndRules = 0;
	for (int32 RuleIndex = 0; RuleIndex < Config.RequiredRooms.Num(); ++RuleIndex)
	{
		const FFloorRoomRule& Rule = Config.RequiredRooms[RuleIndex];
		if (!Rule.RoomType.IsValid())
		{
			OutDetail = FString::Printf(TEXT("RequiredRooms[%d] has no RoomType"), RuleIndex);
			return EFloorGenFailure::InvalidConfig;
		}

		const bool bStart = Rule.Placement == EFloorRoomPlacement::Start;
		const bool bEnd = Rule.Placement == EFloorRoomPlacement::End;
		StartRules += bStart ? 1 : 0;
		EndRules += bEnd ? 1 : 0;
		if ((bStart || bEnd) && Rule.Count != 1)
		{
			OutDetail = FString::Printf(TEXT("RequiredRooms[%d] is Start/End and must have Count = 1"), RuleIndex);
			return EFloorGenFailure::InvalidConfig;
		}
	}
	if (StartRules != 1 || EndRules != 1)
	{
		OutDetail = TEXT("RequiredRooms needs exactly one Start rule and one End rule");
		return EFloorGenFailure::InvalidConfig;
	}

	TSet<const URoomDataAsset*> Checked;
	TArray<const URoomDataAsset*> ToCheck;
	for (const URoomDataAsset* Room : Config.RoomPool)
	{
		ToCheck.Add(Room);
	}
	for (const FFloorRoomRule& Rule : Config.RequiredRooms)
	{
		ToCheck.Add(Rule.OverrideRoom);
	}
	for (const URoomDataAsset* Room : ToCheck)
	{
		if (!Room || Checked.Contains(Room))
		{
			continue;
		}
		Checked.Add(Room);

		EFloorGenFailure Failure = EFloorGenFailure::None;
		if (!FloorGen_ValidateRoom(Room, Failure, OutDetail))
		{
			return Failure;
		}
	}
	return EFloorGenFailure::None;
}

static void FloorGen_OrderByWeight(TArray<URoomDataAsset*>& InOut, FRandomStream& Rng)
{
	TArray<URoomDataAsset*> Remaining = MoveTemp(InOut);
	InOut.Reset();
	while (Remaining.Num() > 0)
	{
		float Total = 0.f;
		for (const URoomDataAsset* Room : Remaining)
		{
			Total += Room->GenWeight;
		}

		float Roll = Rng.FRand() * Total;
		int32 Picked = Remaining.Num() - 1;
		for (int32 Index = 0; Index < Remaining.Num(); ++Index)
		{
			Roll -= Remaining[Index]->GenWeight;
			if (Roll <= 0.f)
			{
				Picked = Index;
				break;
			}
		}
		InOut.Add(Remaining[Picked]);
		Remaining.RemoveAt(Picked);
	}
}

static bool FloorGen_NodeFitsRule(const FFloorGenNode& Node, const FFloorRoomRule& Rule)
{
	if (Node.RuleIndex != INDEX_NONE)
	{
		return false;
	}
	if (Rule.Placement == EFloorRoomPlacement::MainPath && !Node.bMainPath)
	{
		return false;
	}
	if (Rule.Placement == EFloorRoomPlacement::SideBranch && Node.bMainPath)
	{
		return false;
	}
	if (Node.Depth < Rule.DepthRange.X - KINDA_SMALL_NUMBER || Node.Depth > Rule.DepthRange.Y + KINDA_SMALL_NUMBER)
	{
		return false;
	}
	// A leaf of the room tree is the only node that ends up with exactly one door.
	if (Rule.bDeadEnd && Node.Degree != 1)
	{
		return false;
	}
	return true;
}

// Step 1: abstract room tree. No positions yet.
static EFloorGenFailure FloorGen_BuildGraph(const UFloorGenConfig& Config, FRandomStream& Rng,
	TArray<FFloorGenNode>& OutNodes, FString& OutDetail)
{
	const TArray<FFloorRoomRule>& Rules = Config.RequiredRooms;

	int32 StartRule = INDEX_NONE;
	int32 EndRule = INDEX_NONE;
	int32 RequiredMain = 0;
	int32 RequiredSide = 0;
	for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
	{
		const FFloorRoomRule& Rule = Rules[RuleIndex];
		switch (Rule.Placement)
		{
		case EFloorRoomPlacement::Start:
			StartRule = RuleIndex;
			break;
		case EFloorRoomPlacement::End:
			EndRule = RuleIndex;
			break;
		case EFloorRoomPlacement::MainPath:
			RequiredMain += Rule.Count;
			break;
		case EFloorRoomPlacement::SideBranch:
			RequiredSide += Rule.Count;
			break;
		default:
			break;
		}
	}

	const int32 RoomCount = Rng.RandRange(Config.RoomCountMin, FMath::Max(Config.RoomCountMin, Config.RoomCountMax));
	int32 MainLen = Rng.RandRange(Config.MainPathMin, FMath::Max(Config.MainPathMin, Config.MainPathMax));
	// Required side rooms need space outside the main path, so a long main path yields to them.
	MainLen = FMath::Min(MainLen, RoomCount - RequiredSide);
	if (MainLen < 2 + RequiredMain)
	{
		OutDetail = FString::Printf(TEXT("rooms %d, main path rules %d, side branch rules %d"),
			RoomCount, RequiredMain, RequiredSide);
		return EFloorGenFailure::NotEnoughRooms;
	}

	OutNodes.Reset();
	for (int32 Index = 0; Index < MainLen; ++Index)
	{
		FFloorGenNode Node;
		Node.Parent = Index - 1;
		Node.Depth = static_cast<float>(Index) / static_cast<float>(MainLen - 1);
		Node.bMainPath = true;
		OutNodes.Add(Node);
	}

	// Anchors with the fewest branches go first so a 1x1 room is not asked to host several branches.
	const bool bStartIsDeadEnd = StartRule != INDEX_NONE && Rules[StartRule].bDeadEnd;
	const int32 FirstAnchor = bStartIsDeadEnd ? 1 : 0;
	TArray<int32> BranchCount;
	BranchCount.Init(0, MainLen);
	int32 SideRemaining = RoomCount - MainLen;
	while (SideRemaining > 0)
	{
		int32 MinBranches = MAX_int32;
		for (int32 Index = FirstAnchor; Index < MainLen - 1; ++Index)
		{
			MinBranches = FMath::Min(MinBranches, BranchCount[Index]);
		}

		TArray<int32> Anchors;
		for (int32 Index = FirstAnchor; Index < MainLen - 1; ++Index)
		{
			if (BranchCount[Index] == MinBranches)
			{
				Anchors.Add(Index);
			}
		}
		if (Anchors.Num() == 0)
		{
			OutDetail = TEXT("no main path room can host a side branch");
			return EFloorGenFailure::NotEnoughRooms;
		}

		const int32 Anchor = Anchors[Rng.RandRange(0, Anchors.Num() - 1)];
		++BranchCount[Anchor];

		const int32 Length = Rng.RandRange(1, FMath::Min(FMath::Max(1, Config.BranchMaxLength), SideRemaining));
		int32 Previous = Anchor;
		for (int32 Step = 0; Step < Length; ++Step)
		{
			FFloorGenNode Node;
			Node.Parent = Previous;
			Node.Depth = OutNodes[Anchor].Depth;
			Previous = OutNodes.Add(Node);
		}
		SideRemaining -= Length;
	}

	for (int32 Index = 0; Index < OutNodes.Num(); ++Index)
	{
		const int32 Parent = OutNodes[Index].Parent;
		if (Parent != INDEX_NONE)
		{
			++OutNodes[Parent].Degree;
			++OutNodes[Index].Degree;
		}
	}

	OutNodes[0].RuleIndex = StartRule;
	OutNodes[MainLen - 1].RuleIndex = EndRule;

	TArray<int32> Pending;
	for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
	{
		const FFloorRoomRule& Rule = Rules[RuleIndex];
		if (Rule.Placement == EFloorRoomPlacement::Start || Rule.Placement == EFloorRoomPlacement::End)
		{
			continue;
		}
		for (int32 Copy = 0; Copy < Rule.Count; ++Copy)
		{
			Pending.Add(RuleIndex);
		}
	}

	// Most constrained rule first, so loose rules do not take the only node a strict rule could use.
	while (Pending.Num() > 0)
	{
		int32 BestPending = INDEX_NONE;
		TArray<int32> BestNodes;
		for (int32 PendingIndex = 0; PendingIndex < Pending.Num(); ++PendingIndex)
		{
			const FFloorRoomRule& Rule = Rules[Pending[PendingIndex]];
			TArray<int32> Fitting;
			for (int32 NodeIndex = 0; NodeIndex < OutNodes.Num(); ++NodeIndex)
			{
				if (FloorGen_NodeFitsRule(OutNodes[NodeIndex], Rule))
				{
					Fitting.Add(NodeIndex);
				}
			}
			if (BestPending == INDEX_NONE || Fitting.Num() < BestNodes.Num())
			{
				BestPending = PendingIndex;
				BestNodes = MoveTemp(Fitting);
			}
		}

		if (BestNodes.Num() == 0)
		{
			OutDetail = Rules[Pending[BestPending]].RoomType.ToString();
			return EFloorGenFailure::NoNodeForRule;
		}

		const int32 NodeIndex = BestNodes[Rng.RandRange(0, BestNodes.Num() - 1)];
		OutNodes[NodeIndex].RuleIndex = Pending[BestPending];
		Pending.RemoveAt(BestPending);
	}

	static const FGameplayTag TAG_Normal = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Normal")), false);
	for (FFloorGenNode& Node : OutNodes)
	{
		const FFloorRoomRule* Rule = Node.RuleIndex != INDEX_NONE ? &Rules[Node.RuleIndex] : nullptr;
		Node.Type = Rule ? Rule->RoomType : TAG_Normal;

		if (Rule && Rule->OverrideRoom)
		{
			Node.Candidates.Add(Rule->OverrideRoom);
			continue;
		}

		for (URoomDataAsset* Room : Config.RoomPool)
		{
			if (Room && Room->GenWeight > 0.f && Room->RoomTags.HasTagExact(Node.Type))
			{
				Node.Candidates.Add(Room);
			}
		}
		if (Node.Candidates.Num() == 0)
		{
			OutDetail = Node.Type.ToString();
			return EFloorGenFailure::NoRoomForRule;
		}
		FloorGen_OrderByWeight(Node.Candidates, Rng);
	}
	return EFloorGenFailure::None;
}

namespace
{
	// Step 2: depth-first placement with backtracking. Nodes are indexed parent-before-child.
	struct FFloorGenPlacer
	{
		const UFloorGenConfig* Config = nullptr;
		const TArray<FFloorGenNode>* Nodes = nullptr;
		FRandomStream* Rng = nullptr;
		TArray<int32> Occupancy;
		TArray<FFloorGenPlaced> Placed;
		TMap<URoomDataAsset*, int32> Usage;
		int32 Attempts = 0;
		bool bBudgetHit = false;

		bool InGrid(const FIntPoint& Cell) const
		{
			return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Config->GridSize.X && Cell.Y < Config->GridSize.Y;
		}

		int32 CellIndex(const FIntPoint& Cell) const
		{
			return Cell.Y * Config->GridSize.X + Cell.X;
		}

		bool CanPlaceCells(const TArray<FIntPoint>& LocalCells, const FIntPoint& Origin) const
		{
			for (const FIntPoint& Local : LocalCells)
			{
				const FIntPoint Cell = Local + Origin;
				if (!InGrid(Cell) || Occupancy[CellIndex(Cell)] != INDEX_NONE)
				{
					return false;
				}
			}
			return true;
		}

		bool IsRoomAvailable(URoomDataAsset* Room) const
		{
			if (Room->MaxPerFloor <= 0)
			{
				return true;
			}
			const int32* Used = Usage.Find(Room);
			return !Used || *Used < Room->MaxPerFloor;
		}

		void BuildOptions(int32 NodeIndex, TArray<FFloorGenOption>& OutOptions) const
		{
			const FFloorGenNode& Node = (*Nodes)[NodeIndex];

			TArray<FRoomDoorSocket> ParentSockets;
			if (Node.Parent != INDEX_NONE)
			{
				const FFloorGenPlaced& Parent = Placed[Node.Parent];
				TArray<FRoomDoorSocket> LocalSockets;
				FloorGen_GetSockets(Parent.Room, LocalSockets);
				for (const FRoomDoorSocket& Local : LocalSockets)
				{
					FRoomDoorSocket Absolute;
					Absolute.Cell = Local.Cell + Parent.Origin;
					Absolute.Direction = Local.Direction;
					if (!Parent.UsedSockets.Contains(Absolute))
					{
						ParentSockets.Add(Absolute);
					}
				}
			}

			for (URoomDataAsset* Room : Node.Candidates)
			{
				if (!IsRoomAvailable(Room))
				{
					continue;
				}

				TArray<FIntPoint> Footprint;
				FloorGen_GetFootprint(Room, Footprint);
				const int32 FirstOfRoom = OutOptions.Num();

				if (Node.Parent == INDEX_NONE)
				{
					for (int32 Y = 0; Y < Config->GridSize.Y; ++Y)
					{
						for (int32 X = 0; X < Config->GridSize.X; ++X)
						{
							const FIntPoint Origin(X, Y);
							if (CanPlaceCells(Footprint, Origin))
							{
								FFloorGenOption Option;
								Option.Room = Room;
								Option.Origin = Origin;
								OutOptions.Add(Option);
							}
						}
					}
				}
				else
				{
					TArray<FRoomDoorSocket> ChildSockets;
					FloorGen_GetSockets(Room, ChildSockets);
					for (const FRoomDoorSocket& ParentSocket : ParentSockets)
					{
						const FIntPoint DoorCell = ParentSocket.Cell + FFloorGrid::DirOffset(ParentSocket.Direction);
						if (!InGrid(DoorCell) || Occupancy[CellIndex(DoorCell)] != INDEX_NONE)
						{
							continue;
						}

						const ERoomDoorDir NeededDir = FFloorGrid::Opposite(ParentSocket.Direction);
						for (const FRoomDoorSocket& ChildSocket : ChildSockets)
						{
							if (ChildSocket.Direction != NeededDir)
							{
								continue;
							}
							const FIntPoint Origin = DoorCell - ChildSocket.Cell;
							if (!CanPlaceCells(Footprint, Origin))
							{
								continue;
							}

							FFloorGenOption Option;
							Option.Room = Room;
							Option.Origin = Origin;
							Option.ParentSocket = ParentSocket;
							Option.ChildSocket.Cell = DoorCell;
							Option.ChildSocket.Direction = NeededDir;
							OutOptions.Add(Option);
						}
					}
				}

				// Shuffle within one room so the weighted room order is kept.
				for (int32 Index = OutOptions.Num() - 1; Index > FirstOfRoom; --Index)
				{
					OutOptions.Swap(Index, Rng->RandRange(FirstOfRoom, Index));
				}
			}
		}

		void Apply(int32 NodeIndex, const FFloorGenOption& Option)
		{
			FFloorGenPlaced& Slot = Placed[NodeIndex];
			Slot.Room = Option.Room;
			Slot.Origin = Option.Origin;
			Slot.Cells.Reset();
			Slot.UsedSockets.Reset();

			TArray<FIntPoint> Footprint;
			FloorGen_GetFootprint(Option.Room, Footprint);
			for (const FIntPoint& Local : Footprint)
			{
				const FIntPoint Cell = Local + Option.Origin;
				Slot.Cells.Add(Cell);
				Occupancy[CellIndex(Cell)] = NodeIndex;
			}
			++Usage.FindOrAdd(Option.Room);

			const int32 Parent = (*Nodes)[NodeIndex].Parent;
			if (Parent != INDEX_NONE)
			{
				Slot.ParentSocket = Option.ParentSocket;
				Slot.ChildSocket = Option.ChildSocket;
				Slot.UsedSockets.Add(Option.ChildSocket);
				Placed[Parent].UsedSockets.Add(Option.ParentSocket);
			}
		}

		void Undo(int32 NodeIndex)
		{
			FFloorGenPlaced& Slot = Placed[NodeIndex];
			for (const FIntPoint& Cell : Slot.Cells)
			{
				Occupancy[CellIndex(Cell)] = INDEX_NONE;
			}
			--Usage.FindOrAdd(Slot.Room);

			const int32 Parent = (*Nodes)[NodeIndex].Parent;
			if (Parent != INDEX_NONE)
			{
				Placed[Parent].UsedSockets.RemoveSingle(Slot.ParentSocket);
			}
			Slot = FFloorGenPlaced();
		}

		bool Place(int32 NodeIndex)
		{
			if (NodeIndex >= Nodes->Num())
			{
				return true;
			}

			TArray<FFloorGenOption> Options;
			BuildOptions(NodeIndex, Options);
			for (const FFloorGenOption& Option : Options)
			{
				if (Attempts >= Config->MaxPlacementAttempts)
				{
					bBudgetHit = true;
					return false;
				}
				++Attempts;

				Apply(NodeIndex, Option);
				if (Place(NodeIndex + 1))
				{
					return true;
				}
				Undo(NodeIndex);

				if (bBudgetHit)
				{
					return false;
				}
			}
			return false;
		}
	};
}

static void FloorGen_BuildLayout(const UFloorGenConfig& Config, int32 Seed, const TArray<FFloorGenNode>& Nodes,
	const TArray<FFloorGenPlaced>& Placed, FFloorLayout& OutLayout)
{
	OutLayout = FFloorLayout();
	OutLayout.Seed = Seed;
	OutLayout.GridSize = Config.GridSize;

	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FFloorGenNode& Node = Nodes[Index];
		const FFloorGenPlaced& Slot = Placed[Index];

		FFloorRoomInstance Instance;
		Instance.Room = Slot.Room;
		Instance.RoomType = Node.Type;
		Instance.Origin = Slot.Origin;
		Instance.Cells = Slot.Cells;
		Instance.Depth = Node.Depth;
		Instance.bMainPath = Node.bMainPath;
		Instance.ParentIndex = Node.Parent;
		OutLayout.Rooms.Add(Instance);

		if (Node.bMainPath)
		{
			OutLayout.MainPath.Add(Index);
		}

		if (Node.Parent != INDEX_NONE)
		{
			FFloorDoorLink Link;
			Link.RoomA = Node.Parent;
			Link.RoomB = Index;
			Link.CellA = Slot.ParentSocket.Cell;
			Link.DirA = Slot.ParentSocket.Direction;
			Link.CellB = Slot.ChildSocket.Cell;
			Link.DirB = Slot.ChildSocket.Direction;
			OutLayout.Doors.Add(Link);
		}
	}
}

// Step 3: re-checks the finished layout from the data alone, independent of how it was built.
static EFloorGenFailure FloorGen_Validate(const UFloorGenConfig& Config, const TArray<FFloorGenNode>& Nodes,
	const FFloorLayout& Layout, FString& OutDetail)
{
	auto Fail = [&OutDetail](const FString& Message)
	{
		OutDetail = Message;
		return EFloorGenFailure::ValidationFailed;
	};

	if (Layout.Rooms.Num() != Nodes.Num() || Layout.MainPath.Num() < 2 || Layout.MainPath[0] != 0)
	{
		return Fail(TEXT("room list or main path malformed"));
	}

	TSet<FIntPoint> UsedCells;
	for (int32 Index = 0; Index < Layout.Rooms.Num(); ++Index)
	{
		for (const FIntPoint& Cell : Layout.Rooms[Index].Cells)
		{
			const bool bInGrid = Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Config.GridSize.X && Cell.Y < Config.GridSize.Y;
			bool bAlreadyUsed = false;
			UsedCells.Add(Cell, &bAlreadyUsed);
			if (!bInGrid || bAlreadyUsed)
			{
				return Fail(FString::Printf(TEXT("room %d overlaps or leaves the grid at (%d,%d)"), Index, Cell.X, Cell.Y));
			}
		}
	}

	for (const FFloorDoorLink& Door : Layout.Doors)
	{
		if (!Layout.Rooms.IsValidIndex(Door.RoomA) || !Layout.Rooms.IsValidIndex(Door.RoomB))
		{
			return Fail(TEXT("door references a missing room"));
		}

		const bool bCellsMatch = Layout.Rooms[Door.RoomA].Cells.Contains(Door.CellA)
			&& Layout.Rooms[Door.RoomB].Cells.Contains(Door.CellB);
		const bool bAligned = Door.CellA + FFloorGrid::DirOffset(Door.DirA) == Door.CellB
			&& Door.DirB == FFloorGrid::Opposite(Door.DirA);
		if (!bCellsMatch || !bAligned)
		{
			return Fail(FString::Printf(TEXT("door %d-%d is not aligned"), Door.RoomA, Door.RoomB));
		}
	}

	TSet<int32> Reached;
	TArray<int32> Stack;
	Stack.Add(0);
	Reached.Add(0);
	while (Stack.Num() > 0)
	{
		const int32 Current = Stack.Pop();
		for (const FFloorDoorLink& Door : Layout.Doors)
		{
			const int32 Other = Door.RoomA == Current ? Door.RoomB : (Door.RoomB == Current ? Door.RoomA : INDEX_NONE);
			if (Other != INDEX_NONE && !Reached.Contains(Other))
			{
				Reached.Add(Other);
				Stack.Add(Other);
			}
		}
	}
	if (Reached.Num() != Layout.Rooms.Num())
	{
		return Fail(FString::Printf(TEXT("%d of %d rooms reachable from Start"), Reached.Num(), Layout.Rooms.Num()));
	}

	const TArray<FFloorRoomRule>& Rules = Config.RequiredRooms;
	TArray<int32> RuleCounts;
	RuleCounts.Init(0, Rules.Num());
	TMap<URoomDataAsset*, int32> Usage;
	for (int32 Index = 0; Index < Layout.Rooms.Num(); ++Index)
	{
		const FFloorRoomInstance& Room = Layout.Rooms[Index];
		const int32 Used = ++Usage.FindOrAdd(Room.Room);
		if (Room.Room->MaxPerFloor > 0 && Used > Room.Room->MaxPerFloor)
		{
			return Fail(FString::Printf(TEXT("%s exceeds MaxPerFloor"), *Room.Room->GetName()));
		}

		const int32 RuleIndex = Nodes[Index].RuleIndex;
		if (RuleIndex == INDEX_NONE)
		{
			continue;
		}

		const FFloorRoomRule& Rule = Rules[RuleIndex];
		++RuleCounts[RuleIndex];

		bool bPlacementOk = true;
		switch (Rule.Placement)
		{
		case EFloorRoomPlacement::Start:
			bPlacementOk = Index == 0;
			break;
		case EFloorRoomPlacement::End:
			bPlacementOk = Index == Layout.MainPath.Last();
			break;
		case EFloorRoomPlacement::MainPath:
			bPlacementOk = Room.bMainPath;
			break;
		case EFloorRoomPlacement::SideBranch:
			bPlacementOk = !Room.bMainPath;
			break;
		default:
			break;
		}

		const bool bIsEndpoint = Rule.Placement == EFloorRoomPlacement::Start || Rule.Placement == EFloorRoomPlacement::End;
		const bool bDepthOk = bIsEndpoint
			|| (Room.Depth >= Rule.DepthRange.X - KINDA_SMALL_NUMBER && Room.Depth <= Rule.DepthRange.Y + KINDA_SMALL_NUMBER);
		const bool bDeadEndOk = !Rule.bDeadEnd || Layout.CountDoors(Index) == 1;
		if (!bPlacementOk || !bDepthOk || !bDeadEndOk || Room.RoomType != Rule.RoomType)
		{
			return Fail(FString::Printf(TEXT("room %d breaks rule %d (%s)"), Index, RuleIndex, *Rule.RoomType.ToString()));
		}
	}

	for (int32 RuleIndex = 0; RuleIndex < Rules.Num(); ++RuleIndex)
	{
		if (RuleCounts[RuleIndex] != Rules[RuleIndex].Count)
		{
			return Fail(FString::Printf(TEXT("rule %d (%s) placed %d of %d"), RuleIndex,
				*Rules[RuleIndex].RoomType.ToString(), RuleCounts[RuleIndex], Rules[RuleIndex].Count));
		}
	}
	return EFloorGenFailure::None;
}

static EFloorGenFailure FloorGen_TryGenerate(const UFloorGenConfig& Config, int32 RequestedSeed, int32 AttemptSeed,
	FFloorLayout& OutLayout, FFloorGenStats& Stats, FString& OutDetail)
{
	FRandomStream Rng(AttemptSeed);
	TArray<FFloorGenNode> Nodes;
	const EFloorGenFailure GraphFailure = FloorGen_BuildGraph(Config, Rng, Nodes, OutDetail);
	if (GraphFailure != EFloorGenFailure::None)
	{
		return GraphFailure;
	}

	FFloorGenPlacer Placer;
	Placer.Config = &Config;
	Placer.Nodes = &Nodes;
	Placer.Rng = &Rng;
	Placer.Occupancy.Init(INDEX_NONE, Config.GridSize.X * Config.GridSize.Y);
	Placer.Placed.SetNum(Nodes.Num());

	const bool bPlaced = Placer.Place(0);
	Stats.PlacementAttempts = Placer.Attempts;
	if (!bPlaced)
	{
		OutDetail = FString::Printf(TEXT("%d rooms, %d attempts"), Nodes.Num(), Placer.Attempts);
		return Placer.bBudgetHit ? EFloorGenFailure::PlacementBudgetExceeded : EFloorGenFailure::NoPlacementFound;
	}

	FloorGen_BuildLayout(Config, RequestedSeed, Nodes, Placer.Placed, OutLayout);
	return FloorGen_Validate(Config, Nodes, OutLayout, OutDetail);
}

FString FFloorGenStats::DescribeFailure() const
{
	const TCHAR* Name = TEXT("None");
	switch (Failure)
	{
	case EFloorGenFailure::InvalidConfig:
		Name = TEXT("InvalidConfig");
		break;
	case EFloorGenFailure::NotEnoughRooms:
		Name = TEXT("NotEnoughRooms");
		break;
	case EFloorGenFailure::NoNodeForRule:
		Name = TEXT("NoNodeForRule");
		break;
	case EFloorGenFailure::NoRoomForRule:
		Name = TEXT("NoRoomForRule");
		break;
	case EFloorGenFailure::InvalidFootprint:
		Name = TEXT("InvalidFootprint");
		break;
	case EFloorGenFailure::InvalidSocket:
		Name = TEXT("InvalidSocket");
		break;
	case EFloorGenFailure::PlacementBudgetExceeded:
		Name = TEXT("PlacementBudgetExceeded");
		break;
	case EFloorGenFailure::NoPlacementFound:
		Name = TEXT("NoPlacementFound");
		break;
	case EFloorGenFailure::ValidationFailed:
		Name = TEXT("ValidationFailed");
		break;
	default:
		break;
	}
	return FailureDetail.IsEmpty() ? FString(Name) : FString::Printf(TEXT("%s %s"), Name, *FailureDetail);
}

bool FFloorGenerator::Generate(const UFloorGenConfig* Config, int32 Seed, FFloorLayout& OutLayout, FFloorGenStats& OutStats)
{
	const double StartTime = FPlatformTime::Seconds();
	OutStats = FFloorGenStats();
	OutLayout = FFloorLayout();
	OutLayout.Seed = Seed;

	auto Finish = [&OutStats, StartTime](EFloorGenFailure Failure, const FString& Detail)
	{
		OutStats.Failure = Failure;
		OutStats.FailureDetail = Detail;
		OutStats.ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
		return Failure == EFloorGenFailure::None;
	};

	if (!Config)
	{
		return Finish(EFloorGenFailure::InvalidConfig, TEXT("no config"));
	}

	// Config and room-asset errors do not depend on the seed, so they are reported without retrying.
	FString Detail;
	const EFloorGenFailure ConfigFailure = FloorGen_ValidateConfig(*Config, Detail);
	if (ConfigFailure != EFloorGenFailure::None)
	{
		return Finish(ConfigFailure, Detail);
	}

	const int32 TotalSeeds = 1 + FMath::Max(0, Config->MaxSeedRetries);
	EFloorGenFailure Failure = EFloorGenFailure::None;
	for (int32 Attempt = 0; Attempt < TotalSeeds; ++Attempt)
	{
		const int32 AttemptSeed = Attempt == 0
			? Seed
			: static_cast<int32>(HashCombineFast(GetTypeHash(Seed), GetTypeHash(Attempt)));

		OutStats.SeedAttempts = Attempt + 1;
		Detail.Reset();
		Failure = FloorGen_TryGenerate(*Config, Seed, AttemptSeed, OutLayout, OutStats, Detail);
		if (Failure == EFloorGenFailure::None)
		{
			return Finish(EFloorGenFailure::None, FString());
		}
	}

	OutLayout = FFloorLayout();
	OutLayout.Seed = Seed;
	return Finish(Failure, Detail);
}
