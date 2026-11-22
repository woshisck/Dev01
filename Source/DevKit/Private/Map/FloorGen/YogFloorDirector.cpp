#include "Map/FloorGen/YogFloorDirector.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Data/FloorGenConfig.h"
#include "Data/RoomDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Map/FloorGen/FloorGenerator.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "Map/PortalLinkedDoor.h"
#include "UObject/ConstructorHelpers.h"

// Fixed key so a regeneration replaces the previous on-screen stats instead of stacking.
static constexpr uint64 FloorGen_StatsMessageKey = 0x466C6F6F72476E31ull;

static FColor FloorGen_TypeColor(const FGameplayTag& Type)
{
	static const FGameplayTag TAG_Start = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Start")), false);
	static const FGameplayTag TAG_Boss = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Boss")), false);
	static const FGameplayTag TAG_Elite = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Elite")), false);
	static const FGameplayTag TAG_Shop = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Shop")), false);
	static const FGameplayTag TAG_Pray = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Pray")), false);
	static const FGameplayTag TAG_Treasure = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Treasure")), false);
	static const FGameplayTag TAG_Story = FGameplayTag::RequestGameplayTag(FName(TEXT("Room.Type.Story")), false);

	if (Type == TAG_Start)
	{
		return FColor::Green;
	}
	if (Type == TAG_Boss)
	{
		return FColor::Red;
	}
	if (Type == TAG_Elite)
	{
		return FColor::Orange;
	}
	if (Type == TAG_Shop)
	{
		return FColor::Cyan;
	}
	if (Type == TAG_Pray)
	{
		return FColor::Purple;
	}
	if (Type == TAG_Treasure)
	{
		return FColor(255, 215, 0);
	}
	if (Type == TAG_Story)
	{
		return FColor(255, 105, 180);
	}
	return FColor(128, 128, 128);
}

static int64 FloorGen_EdgeKey(const FIntPoint& Cell, ERoomDoorDir Dir)
{
	return (static_cast<int64>(Cell.X) + 1000) * 10000000 + (static_cast<int64>(Cell.Y) + 1000) * 10 + static_cast<int64>(Dir);
}

AYogFloorDirector::AYogFloorDirector()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		WallMesh = CubeFinder.Object;
	}

	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AYogFloorDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateFloor(Seed);
	}
}

void AYogFloorDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDebug();
	UnloadRooms();
	Super::EndPlay(EndPlayReason);
}

bool AYogFloorDirector::GenerateFloor(int32 InSeed)
{
	UnloadRooms();

	const int32 UsedSeed = InSeed != 0 ? InSeed : FMath::RandRange(1, MAX_int32 - 1);

	FFloorGenStats Stats;
	const bool bSuccess = FFloorGenerator::Generate(Config, UsedSeed, Layout, Stats);
	bHasLayout = bSuccess;

	if (bSuccess)
	{
		StatsText = FString::Printf(TEXT("Floor seed %d | rooms %d | main path %d | seed tries %d | placement attempts %d | %.2f ms"),
			UsedSeed, Layout.Rooms.Num(), Layout.MainPath.Num(), Stats.SeedAttempts, Stats.PlacementAttempts, Stats.ElapsedMs);
		UE_LOG(LogYogFloorGen, Log, TEXT("%s"), *StatsText);
	}
	else
	{
		StatsText = FString::Printf(TEXT("Floor generation FAILED | seed %d | %s"), UsedSeed, *Stats.DescribeFailure());
		UE_LOG(LogYogFloorGen, Warning, TEXT("%s (config %s)"), *StatsText, *GetNameSafe(Config));
	}

	if (bSuccess && bStreamRooms)
	{
		StreamRooms();
	}
	if (bSuccess && bBuildWalls)
	{
		BuildWallsAndDoors();
	}
	if (bSuccess && bPopulateRooms)
	{
		PopulateRooms();
	}
	if (bSuccess && bMovePlayerToStart)
	{
		MovePlayerToStart();
	}

	RedrawDebug();

	if (bSuccess)
	{
		OnFloorGenerated();
	}
	return bSuccess;
}

FVector AYogFloorDirector::GetRoomCenter(const FFloorRoomInstance& Room) const
{
	FVector Sum = FVector::ZeroVector;
	for (const FIntPoint& Cell : Room.Cells)
	{
		Sum += CellCenter(Cell);
	}
	return Sum / FMath::Max(1, Room.Cells.Num());
}

void AYogFloorDirector::RegisterSpawnedPart(AActor* Part)
{
	if (IsValid(Part))
	{
		SpawnedParts.Add(Part);
	}
}

void AYogFloorDirector::PopulateRooms()
{
	UWorld* World = GetWorld();
	if (!World || !Config)
	{
		return;
	}

	const double FloorZ = GetActorLocation().Z + FloorTopZ;

	for (const FFloorRoomInstance& Room : Layout.Rooms)
	{
		for (const FFloorRoomPopulation& Population : Config->RoomPopulation)
		{
			if (!Population.RoomType.IsValid() || Population.RoomType != Room.RoomType)
			{
				continue;
			}

			FVector Center = GetRoomCenter(Room);
			Center.Z = FloorZ;

			for (const FFloorSpawnEntry& Entry : Population.Entries)
			{
				if (!Entry.ActorClass)
				{
					UE_LOG(LogYogFloorGen, Warning, TEXT("RoomPopulation entry for %s has no ActorClass"), *Population.RoomType.ToString());
					continue;
				}

				const FTransform SpawnTransform(FRotator(0.f, Entry.Yaw, 0.f), Center + Entry.Offset);

				// Deferred so WeaponDefinition is set before OnConstruction builds the display mesh.
				AActor* Spawned = World->SpawnActorDeferred<AActor>(
					Entry.ActorClass, SpawnTransform, nullptr, nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (!Spawned)
				{
					continue;
				}

				if (AWeaponSpawner* WeaponSpawner = Cast<AWeaponSpawner>(Spawned))
				{
					WeaponSpawner->WeaponDefinition = Entry.WeaponDefinition;
				}

				Spawned->FinishSpawning(SpawnTransform);
				SpawnedParts.Add(Spawned);
			}
		}
	}
}

void AYogFloorDirector::StreamRooms()
{
	UWorld* World = GetWorld();
	if (!World || !Config)
	{
		return;
	}

	for (const FFloorRoomInstance& Room : Layout.Rooms)
	{
		if (!Room.Room || Room.Room->LevelAsset.IsNull())
		{
			continue;
		}

		const FVector Location = GetActorLocation()
			+ FVector(Room.Origin.X * Config->CellSize.X, Room.Origin.Y * Config->CellSize.Y, 0.0);

		bool bLoaded = false;
		ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
			this, Room.Room->LevelAsset, FTransform(Location), bLoaded);
		if (!bLoaded || !Streaming)
		{
			UE_LOG(LogYogFloorGen, Warning, TEXT("Failed to stream room %s (%s)"),
				*GetNameSafe(Room.Room), *Room.Room->LevelAsset.ToString());
			continue;
		}
		StreamedRooms.Add(Streaming);
	}

	// Block until every room is loaded and visible so the floor exists before the player is placed.
	if (!StreamedRooms.IsEmpty())
	{
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
	}
}

void AYogFloorDirector::MovePlayerToStart()
{
	UWorld* World = GetWorld();
	if (!World || Layout.Rooms.IsEmpty() || Layout.Rooms[0].Cells.IsEmpty())
	{
		return;
	}

	FVector Sum = FVector::ZeroVector;
	for (const FIntPoint& Cell : Layout.Rooms[0].Cells)
	{
		Sum += CellCenter(Cell);
	}
	FVector SpawnLocation = Sum / Layout.Rooms[0].Cells.Num();
	SpawnLocation.Z = GetActorLocation().Z + FloorTopZ + PlayerSpawnHeight;

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		// PlayerStart placed in the editor is Static by default and refuses to move.
		if (USceneComponent* StartRoot = It->GetRootComponent())
		{
			StartRoot->SetMobility(EComponentMobility::Movable);
		}
		It->SetActorLocation(SpawnLocation);
	}

	// The GameMode may already have spawned the pawn before this director's BeginPlay ran.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr)
		{
			// No sweep: the spawn point must not fail because a wall or the old PlayerStart overlaps.
			Pawn->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void AYogFloorDirector::SpawnWallSegment(const FVector& Center, const FVector& Size)
{
	UWorld* World = GetWorld();
	if (!World || !WallMesh)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Wall = World->SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator, Params);
	if (!Wall)
	{
		return;
	}

	UStaticMeshComponent* MeshComp = Wall->GetStaticMeshComponent();
	MeshComp->SetMobility(EComponentMobility::Movable);
	MeshComp->SetStaticMesh(WallMesh);
	// The engine cube is 100 units on a side.
	Wall->SetActorScale3D(Size / 100.0);
	Wall->SetActorLabel(TEXT("FloorGen_Wall"));
	SpawnedParts.Add(Wall);
}

void AYogFloorDirector::BuildWallsAndDoors()
{
	UWorld* World = GetWorld();
	if (!World || !Config)
	{
		return;
	}

	TSet<int64> DoorEdges;
	for (const FFloorDoorLink& Door : Layout.Doors)
	{
		DoorEdges.Add(FloorGen_EdgeKey(Door.CellA, Door.DirA));
		DoorEdges.Add(FloorGen_EdgeKey(Door.CellB, Door.DirB));
	}

	const FVector2D CellSize = Config->CellSize;
	const double WallCenterZ = GetActorLocation().Z + FloorTopZ + WallHeight * 0.5;
	static const ERoomDoorDir AllDirs[] = { ERoomDoorDir::PosX, ERoomDoorDir::NegX, ERoomDoorDir::PosY, ERoomDoorDir::NegY };

	for (const FFloorRoomInstance& Room : Layout.Rooms)
	{
		const TSet<FIntPoint> RoomCells(Room.Cells);
		for (const FIntPoint& Cell : Room.Cells)
		{
			for (const ERoomDoorDir Dir : AllDirs)
			{
				if (RoomCells.Contains(Cell + FFloorGrid::DirOffset(Dir)))
				{
					continue;
				}

				const FIntPoint Offset = FFloorGrid::DirOffset(Dir);
				const bool bAlongY = Offset.X != 0;
				const double RunLength = bAlongY ? CellSize.Y : CellSize.X;
				const double InwardInset = WallThickness * 0.5;

				// Edge midpoint pulled inside the footprint so walls never cross into the neighbour cell.
				FVector EdgeCenter = CellCenter(Cell);
				EdgeCenter.X += Offset.X * (CellSize.X * 0.5 - InwardInset);
				EdgeCenter.Y += Offset.Y * (CellSize.Y * 0.5 - InwardInset);
				EdgeCenter.Z = WallCenterZ;

				const FVector RunAxis = bAlongY ? FVector(0.0, 1.0, 0.0) : FVector(1.0, 0.0, 0.0);
				auto MakeSize = [&](double Length)
				{
					return bAlongY ? FVector(WallThickness, Length, WallHeight) : FVector(Length, WallThickness, WallHeight);
				};

				if (!DoorEdges.Contains(FloorGen_EdgeKey(Cell, Dir)))
				{
					SpawnWallSegment(EdgeCenter, MakeSize(RunLength));
					continue;
				}

				const double SegmentLength = (RunLength - DoorWidth) * 0.5;
				if (SegmentLength > KINDA_SMALL_NUMBER)
				{
					const double SegmentOffset = DoorWidth * 0.5 + SegmentLength * 0.5;
					SpawnWallSegment(EdgeCenter - RunAxis * SegmentOffset, MakeSize(SegmentLength));
					SpawnWallSegment(EdgeCenter + RunAxis * SegmentOffset, MakeSize(SegmentLength));
				}
			}
		}
	}

	if (!DoorClass)
	{
		return;
	}

	for (const FFloorDoorLink& Link : Layout.Doors)
	{
		// The door sits on the shared boundary between the two cells.
		const FVector Location = DoorPosition(Link.CellA, Link.DirA) + FVector(0.0, 0.0, FloorTopZ);
		const bool bAlongY = FFloorGrid::DirOffset(Link.DirA).X != 0;
		const FRotator Rotation(0.0, bAlongY ? 90.0 : 0.0, 0.0);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* DoorActor = World->SpawnActor<APortalLinkedDoor>(DoorClass, Location, Rotation, Params))
		{
			SpawnedParts.Add(DoorActor);
		}
	}
}

void AYogFloorDirector::UnloadRooms()
{
	for (AActor* Part : SpawnedParts)
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
	}
	SpawnedParts.Reset();

	for (ULevelStreamingDynamic* Streaming : StreamedRooms)
	{
		if (IsValid(Streaming))
		{
			Streaming->SetShouldBeVisible(false);
			Streaming->SetIsRequestingUnloadAndRemoval(true);
		}
	}
	StreamedRooms.Reset();
}

void AYogFloorDirector::SetDebugDrawEnabled(bool bEnabled)
{
	bDrawDebug = bEnabled;
	RedrawDebug();
}

FVector AYogFloorDirector::CellCenter(const FIntPoint& Cell) const
{
	const FVector2D Size = Config ? Config->CellSize : FVector2D(2400.0, 2400.0);
	return GetActorLocation() + FVector((Cell.X + 0.5) * Size.X, (Cell.Y + 0.5) * Size.Y, 0.0);
}

FVector AYogFloorDirector::DoorPosition(const FIntPoint& Cell, ERoomDoorDir Dir) const
{
	const FVector2D Size = Config ? Config->CellSize : FVector2D(2400.0, 2400.0);
	const FIntPoint Offset = FFloorGrid::DirOffset(Dir);
	return CellCenter(Cell) + FVector(Offset.X * Size.X * 0.5, Offset.Y * Size.Y * 0.5, 0.0);
}

void AYogFloorDirector::ClearDebug()
{
	if (UWorld* World = GetWorld())
	{
		FlushPersistentDebugLines(World);
		FlushDebugStrings(World);
	}
	if (GEngine)
	{
		GEngine->RemoveOnScreenDebugMessage(FloorGen_StatsMessageKey);
	}
}

void AYogFloorDirector::RedrawDebug()
{
	ClearDebug();

	UWorld* World = GetWorld();
	if (!World || !bDrawDebug || !Config)
	{
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(FloorGen_StatsMessageKey, 1.0e6f,
			bHasLayout ? FColor::White : FColor::Red, StatsText);
	}

	const FVector2D CellSize = Config->CellSize;
	const FVector GridOrigin = GetActorLocation();
	const FVector GridExtent(Config->GridSize.X * CellSize.X, Config->GridSize.Y * CellSize.Y, 0.0);
	DrawDebugBox(World, GridOrigin + GridExtent * 0.5, GridExtent * 0.5, FColor(90, 90, 90), true, -1.f, 0, 20.f);

	if (!bHasLayout)
	{
		return;
	}

	// Cells are inset so neighbouring rooms stay visually separate.
	const FVector CellHalfExtent(CellSize.X * 0.5 - 60.0, CellSize.Y * 0.5 - 60.0, 20.0);
	for (const FFloorRoomInstance& Room : Layout.Rooms)
	{
		const FColor Color = FloorGen_TypeColor(Room.RoomType);
		FVector Sum = FVector::ZeroVector;
		for (const FIntPoint& Cell : Room.Cells)
		{
			DrawDebugSolidBox(World, CellCenter(Cell), CellHalfExtent, Color.WithAlpha(110), true, -1.f, 0);
			DrawDebugBox(World, CellCenter(Cell), CellHalfExtent, Color, true, -1.f, 0, 15.f);
			Sum += CellCenter(Cell);
		}

		const FVector Center = Sum / FMath::Max(1, Room.Cells.Num());
		const FString Label = FString::Printf(TEXT("%s\n%.2f"), *GetNameSafe(Room.Room), Room.Depth);
		// Duration 0 would last one frame; the labels are cleared explicitly by ClearDebug.
		DrawDebugString(World, Center + FVector(0.0, 0.0, 120.0), Label, nullptr, FColor::White, 1.0e6f, true, 1.2f);
	}

	for (const FFloorDoorLink& Door : Layout.Doors)
	{
		DrawDebugLine(World, CellCenter(Door.CellA), CellCenter(Door.CellB), FColor::White, true, -1.f, 0, 25.f);
		DrawDebugBox(World, DoorPosition(Door.CellA, Door.DirA), FVector(60.0, 60.0, 60.0), FColor::White, true, -1.f, 0, 20.f);
	}

	auto RoomCenter = [this](const FFloorRoomInstance& Room)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FIntPoint& Cell : Room.Cells)
		{
			Sum += CellCenter(Cell);
		}
		return Sum / FMath::Max(1, Room.Cells.Num());
	};
	for (int32 Index = 1; Index < Layout.MainPath.Num(); ++Index)
	{
		const FVector From = RoomCenter(Layout.Rooms[Layout.MainPath[Index - 1]]) + FVector(0.0, 0.0, 160.0);
		const FVector To = RoomCenter(Layout.Rooms[Layout.MainPath[Index]]) + FVector(0.0, 0.0, 160.0);
		DrawDebugLine(World, From, To, FColor::Yellow, true, -1.f, 0, 70.f);
	}
}
