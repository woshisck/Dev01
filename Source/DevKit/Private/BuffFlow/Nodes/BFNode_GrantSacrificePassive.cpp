#include "BuffFlow/Nodes/BFNode_GrantSacrificePassive.h"

#include "Character/PlayerCharacterBase.h"

UBFNode_GrantSacrificePassive::UBFNode_GrantSacrificePassive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Sacrifice");
#endif
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Remove")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_GrantSacrificePassive::ExecuteBuffFlowInput(const FName& PinName)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetBuffOwner());
	if (!Player || !Player->SacrificeRuneComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GrantSacrificePassive] Failed: owner is not player or component missing Owner=%s Passive=%d"),
			*GetNameSafe(GetBuffOwner()),
			static_cast<int32>(Config.PassiveType));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (PinName == TEXT("Remove"))
	{
		if (bGranted && RuntimeGrantGuid.IsValid())
		{
			Player->SacrificeRuneComponent->RemoveSacrificePassive(RuntimeGrantGuid);
			bGranted = false;
		}
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (!RuntimeGrantGuid.IsValid())
	{
		RuntimeGrantGuid = FGuid::NewGuid();
	}

	Player->SacrificeRuneComponent->AddSacrificePassive(RuntimeGrantGuid, Config);
	bGranted = true;
	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_GrantSacrificePassive::Cleanup()
{
	if (bGranted && RuntimeGrantGuid.IsValid())
	{
		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetBuffOwner()))
		{
			if (Player->SacrificeRuneComponent)
			{
				Player->SacrificeRuneComponent->RemoveSacrificePassive(RuntimeGrantGuid);
			}
		}
		bGranted = false;
	}

	Super::Cleanup();
}
