#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "DevourOfferDataAsset.generated.h"

class UTexture2D;

/**
 * What devouring a corpse grants. All buff logic lives in the Lua module; C++ only decides when each
 * phase starts and ends, and carries the devour across rooms and save games.
 *
 * The module is required from Content/Script and must return a table. Every function is optional and
 * receives the player, this offer (read tuning from Offer.NumberParams / Offer.EffectParams) and the
 * devour's Id, which stays the same for the whole run so one player can hold several devours of one offer:
 *
 *   OnTempBegin(Player, Offer, Id, RoomsEntered)   the devour, and again in every room the temp phase
 *                                                  still covers (the player is respawned per room)
 *   OnRoomEntered(Player, Offer, Id, RoomsEntered) a room entered without respawning the player
 *                                                  (cheat Yog_DevourNextRoom, future streamed floors)
 *   OnTempEnd(Player, Offer, Id)                   the temp phase ends on a live player
 *   OnPermanentBegin(Player, Offer, Id)            maturity, and again in every later room
 *   OnRemoved(Player, Offer, Id)                   run reset (back to hub)
 *
 * The module table is shared by every player and devour, so per-devour state must be keyed by Player
 * and Id, never stored on the module itself. Every call is pcall-guarded.
 */
UCLASS(BlueprintType)
class DEVKIT_API UDevourOfferDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Module path relative to Content/Script, dot separated. e.g. "Devour.Default". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|Lua")
	FString LuaModule;

	/** Designer tuning handed to the script through the Offer argument. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|Lua")
	TMap<FName, float> NumberParams;

	/** GameplayEffects the script can look up by name, so effect assets are chosen in the editor, not by path in Lua. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|Lua")
	TMap<FName, TSubclassOf<UGameplayEffect>> EffectParams;

	// Rooms entered after the devour room during which the temp phase lasts. Entering the next one matures it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour", meta = (ClampMin = "1"))
	int32 DurationRooms = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|UI", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Devour|UI")
	TObjectPtr<UTexture2D> Icon;
};

/** One devour carried across a level change in FRunState. */
USTRUCT()
struct DEVKIT_API FDevourRunEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UDevourOfferDataAsset> Offer = nullptr;

	UPROPERTY()
	int32 Id = 0;

	UPROPERTY()
	int32 RoomsEntered = 0;

	UPROPERTY()
	bool bMatured = false;
};

/** Save-game form of FDevourRunEntry. */
USTRUCT()
struct DEVKIT_API FDevourCheckpointEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UDevourOfferDataAsset> Offer;

	UPROPERTY()
	int32 Id = 0;

	UPROPERTY()
	int32 RoomsEntered = 0;

	UPROPERTY()
	bool bMatured = false;
};
