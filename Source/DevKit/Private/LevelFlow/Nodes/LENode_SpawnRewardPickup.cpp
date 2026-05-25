#include "LevelFlow/Nodes/LENode_SpawnRewardPickup.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RewardPickup.h"
#include "Story/Encounter/StoryFlowProxy.h"

ULENode_SpawnRewardPickup::ULENode_SpawnRewardPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent|Reward");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_SpawnRewardPickup::ExecuteInput(const FName& PinName)
{
	SpawnRewardPickupAtContext(ResolveWorld(), ResolveContextTransform());
	TriggerOutput(TEXT("Out"), true);
}

UWorld* ULENode_SpawnRewardPickup::ResolveWorld() const
{
	if (UWorld* World = GetWorld())
	{
		return World;
	}

	if (const AActor* OuterActor = GetTypedOuter<AActor>())
	{
		return OuterActor->GetWorld();
	}

	return nullptr;
}

FTransform ULENode_SpawnRewardPickup::ResolveContextTransform() const
{
	if (const AStoryFlowProxy* Proxy = Cast<AStoryFlowProxy>(TryGetRootFlowActorOwner()))
	{
		return Proxy->GetContextTransform();
	}

	if (const AStoryFlowProxy* Proxy = GetTypedOuter<AStoryFlowProxy>())
	{
		return Proxy->GetContextTransform();
	}

	if (const AActor* OwnerActor = TryGetRootFlowActorOwner())
	{
		return OwnerActor->GetActorTransform();
	}

	return FTransform::Identity;
}

bool ULENode_SpawnRewardPickup::SpawnRewardPickupAtContext(UWorld* World, const FTransform& ContextTransform) const
{
	if (!World || RewardLootOptions.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LENode_SpawnRewardPickup] skipped: World=%s LootCount=%d"),
			World ? TEXT("OK") : TEXT("NULL"),
			RewardLootOptions.Num());
		return false;
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
		UE_LOG(LogTemp, Warning, TEXT("[LENode_SpawnRewardPickup] skipped: RewardPickupClass is not set."));
		return false;
	}

	bool bSpawnedAny = false;
	const int32 Count = FMath::Max(1, RewardPickupCount);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector SpreadOffset(0.f, Index * 120.f, 0.f);
		const FVector SpawnLocation = ContextTransform.GetLocation() + RewardSpawnOffset + SpreadOffset;
		ARewardPickup* Pickup = World->SpawnActor<ARewardPickup>(PickupClass, SpawnLocation, FRotator::ZeroRotator);
		if (!Pickup)
		{
			continue;
		}

		Pickup->bAllowPickupOutsideArrangement = bAllowPickupOutsideArrangement;
		Pickup->bUseFixedLootOptions = true;
		Pickup->FixedLootOptions = RewardLootOptions;
		Pickup->AssignLoot(RewardLootOptions);
		Pickup->SetActorHiddenInGame(false);
		Pickup->SetActorEnableCollision(true);
		bSpawnedAny = true;
	}

	return bSpawnedAny;
}
