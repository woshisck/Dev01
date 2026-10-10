#include "System/YogRunEconomySubsystem.h"
#include "SaveGame/YogSaveSubsystem.h"

UYogSaveSubsystem* UYogRunEconomySubsystem::GetSaveSys() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UYogSaveSubsystem>() : nullptr;
}

void UYogRunEconomySubsystem::AddGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Gold += Amount;

	if (UYogSaveSubsystem* SaveSys = GetSaveSys())
	{
		SaveSys->RecordGoldEarned(Amount);
	}

	OnGoldChanged.Broadcast(Gold);
}

bool UYogRunEconomySubsystem::SpendGold(int32 Amount)
{
	if (Amount < 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	OnGoldChanged.Broadcast(Gold);
	return true;
}

void UYogRunEconomySubsystem::RestoreGold(int32 Amount)
{
	Gold = FMath::Max(0, Amount);
	OnGoldChanged.Broadcast(Gold);
}

void UYogRunEconomySubsystem::ResetForNewRun()
{
	Gold = 0;
	OnGoldChanged.Broadcast(Gold);
}
