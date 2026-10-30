#include "Story/Flow/Nodes/SNode_SpawnRewardPickup.h"

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


	const FTransform ContextTransform = ResolveContextTransform();
	const int32 Count = FMath::Max(1, RewardPickupCount);
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector SpreadOffset(0.f, Index * 120.f, 0.f);
		const FVector SpawnLocation = ContextTransform.GetLocation() + RewardSpawnOffset + SpreadOffset;
		ARewardPickup* Pickup = ARewardPickup::SpawnWithLoot(
			World, RewardPickupClass, SpawnLocation, RewardLootOptions, bPlaySpawnFocusCue && Index == 0);
		if (!Pickup)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SNode_SpawnRewardPickup] failed to spawn pickup %d/%d. Class=%s Location=%s"),
				Index + 1,
				Count,
				*GetNameSafe(RewardPickupClass.Get()),
				*SpawnLocation.ToCompactString());
			continue;
		}

		Pickup->bUseFixedLootOptions = true;
		Pickup->FixedLootOptions = RewardLootOptions;
		++SpawnedCount;
	}

	UE_LOG(LogTemp, Log, TEXT("[SNode_SpawnRewardPickup] spawned %d/%d pickup(s). Class=%s LootCount=%d Context=%s"),
		SpawnedCount,
		Count,
		*GetNameSafe(RewardPickupClass.Get()),
		RewardLootOptions.Num(),
		*ContextTransform.GetLocation().ToCompactString());

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
