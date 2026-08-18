#include "DevKitEditor/Combat/CombatMontageSyncCommandlet.h"

UCombatMontageSyncCommandlet::UCombatMontageSyncCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UCombatMontageSyncCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Error, TEXT("[CombatMontageSync] This legacy ComboGraph migration commandlet is disabled. Current combat data is authored through AbilityData and montage rows, not YogComboGraph."));
	return 1;
}
