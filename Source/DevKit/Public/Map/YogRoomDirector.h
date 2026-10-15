#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnLuaInterface.h"
#include "YogRoomDirector.generated.h"

class APortal;

/**
 * Host for a map's room flow. Carries no room logic: it binds a Lua module and exposes the one
 * call that module cannot make itself, so "clear the mobs, then open the portals" lives in the
 * map's own script rather than in AYogGameMode.
 */
UCLASS()
class DEVKIT_API AYogRoomDirector : public AActor, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	AYogRoomDirector();

	/** Module path relative to Content/Script, dot separated. e.g. "Maps.RoomDirector". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room|Lua")
	FString LuaModule = TEXT("Maps.RoomDirector");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room|Debug")
	bool bVerbose = false;

	/**
	 * Opens Portal towards the level authored on the portal itself.
	 * Exists only because APortal::Open takes a TArray<FBuffEntry> that Lua has no reason to build.
	 */
	UFUNCTION(BlueprintCallable, Category = "Room|Portals")
	bool OpenPortal(APortal* Portal);

	/** Implemented in Lua. Raised one tick after BeginPlay, once every placed actor exists. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Room")
	void OnRoomStart();

	virtual FString GetModuleName_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	void StartRoom();
};
