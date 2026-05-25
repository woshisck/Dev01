#include "Story/Flow/Nodes/SNode_GiveCard.h"

#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/RuneDataAsset.h"

USNode_GiveCard::USNode_GiveCard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Gameplay");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins =
	{
		FFlowPin(TEXT("Out")),
		FFlowPin(TEXT("DeckFailed")),
		FFlowPin(TEXT("InventoryFailed")),
	};
}

void USNode_GiveCard::ExecuteInput(const FName& PinName)
{
	if (!CardToGive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_GiveCard] CardToGive is null."));
		TriggerOutput(TEXT("DeckFailed"), true);
		return;
	}

	APlayerController* PC = GetPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;

	// 1. 写入战斗卡组
	UCombatDeckComponent* Deck = Pawn ? Pawn->FindComponentByClass<UCombatDeckComponent>() : nullptr;
	if (!Deck)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_GiveCard] CombatDeckComponent not found on player pawn."));
		TriggerOutput(TEXT("DeckFailed"), true);
		return;
	}
	if (!Deck->AddCardFromRuneReward(CardToGive))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_GiveCard] AddCardFromRuneReward failed for %s."), *GetNameSafe(CardToGive));
		TriggerOutput(TEXT("DeckFailed"), true);
		return;
	}

	// 2. 写入背包
	APlayerCharacterBase* PlayerChar = Cast<APlayerCharacterBase>(Pawn);
	if (!PlayerChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_GiveCard] APlayerCharacterBase not found."));
		TriggerOutput(TEXT("InventoryFailed"), true);
		return;
	}
	PlayerChar->AddRuneToInventory(CardToGive->CreateInstance());

	TriggerOutput(TEXT("Out"), true);
}
