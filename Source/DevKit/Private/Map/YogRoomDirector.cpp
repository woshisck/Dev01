#include "Map/YogRoomDirector.h"

#include "Engine/World.h"
#include "Map/Portal.h"
#include "TimerManager.h"

AYogRoomDirector::AYogRoomDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AYogRoomDirector::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay order between actors placed in a level is undefined, so a spawner this room drives
	// may not have picked its Lua rule yet. One tick later every placed actor has begun play.
	GetWorldTimerManager().SetTimerForNextTick(this, &AYogRoomDirector::StartRoom);
}

void AYogRoomDirector::StartRoom()
{
	OnRoomStart();
}

FString AYogRoomDirector::GetModuleName_Implementation() const
{
	// Every character of this string becomes part of Lua's require path, so surrounding whitespace
	// or a ".lua" suffix silently resolves to a file that does not exist.
	FString Module = LuaModule.TrimStartAndEnd();
	Module.RemoveFromEnd(TEXT(".lua"));
	return Module;
}

bool AYogRoomDirector::OpenPortal(APortal* Portal)
{
	if (!IsValid(Portal))
	{
		return false;
	}

	// APortal::TryEnter refuses a portal that has no level, so opening one without a destination
	// would read as open and then silently reject the player.
	if (Portal->DestinationLevel.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RoomDirector] %s has no DestinationLevel; leaving it closed."),
			*GetNameSafe(Portal));
		return false;
	}

	Portal->Open(Portal->DestinationLevel, nullptr, TArray<FBuffEntry>());
	return true;
}
