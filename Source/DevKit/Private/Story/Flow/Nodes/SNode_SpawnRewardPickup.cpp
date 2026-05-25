#include "Story/Flow/Nodes/SNode_SpawnRewardPickup.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RewardPickup.h"
#include "Story/Encounter/StoryFlowProxy.h"

USNode_SpawnRewardPickup::USNode_SpawnRewardPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Gameplay");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SpawnRewardPickup::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	if (!World || RewardLootOptions.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SpawnRewardPickup] skipped: World=%s LootCount=%d"),
			World ? TEXT("OK") : TEXT("NULL"), RewardLootOptions.Num());
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	TSubclassOf<ARewardPickup> PickupClass = RewardPickupClass;
	if (!PickupClass)
	{
		if (const AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World)))
		{
			PickupClass = GM->RewardPickupClass;
		}
	}

	if (!PickupClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SpawnRewardPickup] skipped: RewardPickupClass is not set."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	const FTransform ContextTransform = ResolveContextTransform();
	const int32 Count = FMath::Max(1, RewardPickupCount);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector SpreadOffset(0.f, Index * 120.f, 0.f);
		const FVector SpawnLocation = ContextTransform.GetLocation() + RewardSpawnOffset + SpreadOffset;
		ARewardPickup* Pickup = World->SpawnActor<ARewardPickup>(PickupClass, SpawnLocation, FRotator::ZeroRotator);
		if (!Pickup) continue;

		Pickup->bAllowPickupOutsideArrangement = bAllowPickupOutsideArrangement;
		Pickup->bUseFixedLootOptions = true;
		Pickup->FixedLootOptions = RewardLootOptions;
		Pickup->AssignLoot(RewardLootOptions);
		Pickup->SetActorHiddenInGame(false);
		Pickup->SetActorEnableCollision(true);
	}

	TriggerOutput(TEXT("Out"), true);
}

FTransform USNode_SpawnRewardPickup::ResolveContextTransform() const
{
	if (const AStoryFlowProxy* Proxy = GetStoryProxy())
	{
		return Proxy->GetContextTransform();
	}
	if (const AActor* Owner = TryGetRootFlowActorOwner())
	{
		return Owner->GetActorTransform();
	}
	return FTransform::Identity;
}
