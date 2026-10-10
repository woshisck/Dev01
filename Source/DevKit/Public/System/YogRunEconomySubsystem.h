#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogRunEconomySubsystem.generated.h"

class UYogSaveSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

// ============================================================
//  UYogRunEconomySubsystem
//
//  Authoritative owner of the per-run currency (Gold).
//
//  Lives on the GameInstance so the value survives the OpenLevel
//  that every room transition performs. Disk persistence is delegated
//  to UYogSaveSubsystem via FRunCheckpointData::CurrentGold.
//
//  Gold is per-run: ResetForNewRun zeroes it on death and hub return.
// ============================================================
UCLASS()
class DEVKIT_API UYogRunEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetGold() const { return Gold; }

	/** Grants gold and records it against the lifetime earnings statistic. Ignores Amount <= 0. */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddGold(int32 Amount);

	/** Returns false and leaves Gold untouched when the balance is insufficient. */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool SpendGold(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Economy")
	bool CanAfford(int32 Cost) const { return Gold >= Cost; }

	/**
	 * Writes Gold directly without touching lifetime earnings.
	 * Used by checkpoint restore and cheats, where re-recording would inflate the statistic.
	 */
	void RestoreGold(int32 Amount);

	/** Zeroes Gold for a fresh run. Called from UYogGameInstanceBase::ClearRunState. */
	void ResetForNewRun();

	UPROPERTY(BlueprintAssignable, Category = "Economy|Events")
	FOnGoldChanged OnGoldChanged;

private:

	UYogSaveSubsystem* GetSaveSys() const;

	int32 Gold = 0;
};
