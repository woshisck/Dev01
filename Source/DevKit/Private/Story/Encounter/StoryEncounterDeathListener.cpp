#include "Story/Encounter/StoryEncounterDeathListener.h"

#include "Character/YogCharacterBase.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

AStoryEncounterDeathListener::AStoryEncounterDeathListener()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);
}

void AStoryEncounterDeathListener::BeginPlay()
{
	Super::BeginPlay();
	BindMatchingTargets();
}

void AStoryEncounterDeathListener::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAllTargets();
	Super::EndPlay(EndPlayReason);
}

int32 AStoryEncounterDeathListener::BindMatchingTargets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	if (TargetActorName.IsNone() && TargetActorTag.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEncounterDeathListener] %s has no target actor name or tag."),
			*GetNameSafe(this));
		return 0;
	}

	int32 BoundCount = 0;
	for (TActorIterator<AYogCharacterBase> It(World); It; ++It)
	{
		AYogCharacterBase* Character = *It;
		if (DoesTargetMatch(Character) && BindTarget(Character))
		{
			++BoundCount;
		}
	}

	if (BoundCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEncounterDeathListener] %s found no target. Name=%s Tag=%s"),
			*GetNameSafe(this),
			*TargetActorName.ToString(),
			*TargetActorTag.ToString());
	}

	return BoundCount;
}

bool AStoryEncounterDeathListener::DoesTargetMatch(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	if (!TargetActorName.IsNone())
	{
		const FString TargetName = TargetActorName.ToString();
		if (Actor->GetFName() == TargetActorName || Actor->GetName().Equals(TargetName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return !TargetActorTag.IsNone() && Actor->ActorHasTag(TargetActorTag);
}

bool AStoryEncounterDeathListener::BindTarget(AYogCharacterBase* Character)
{
	if (!Character)
	{
		return false;
	}

	for (const FBoundDeathTarget& ExistingTarget : BoundTargets)
	{
		if (ExistingTarget.Character.Get() == Character)
		{
			return false;
		}
	}

	FBoundDeathTarget BoundTarget;
	BoundTarget.Character = Character;
	BoundTarget.DelegateHandle = Character->OnCharacterDiedNative.AddUObject(
		this,
		&AStoryEncounterDeathListener::HandleTargetDied);
	BoundTargets.Add(BoundTarget);
	return true;
}

bool AStoryEncounterDeathListener::IsBoundToTarget(const AYogCharacterBase* Character) const
{
	if (!Character)
	{
		return false;
	}

	for (const FBoundDeathTarget& Target : BoundTargets)
	{
		if (Target.Character.Get() == Character && Target.DelegateHandle.IsValid())
		{
			return true;
		}
	}
	return false;
}

void AStoryEncounterDeathListener::UnbindAllTargets()
{
	for (const FBoundDeathTarget& Target : BoundTargets)
	{
		if (AYogCharacterBase* Character = Target.Character.Get())
		{
			Character->OnCharacterDiedNative.Remove(Target.DelegateHandle);
		}
	}
	BoundTargets.Reset();
}

void AStoryEncounterDeathListener::HandleTargetDied(AYogCharacterBase* Character)
{
	if (!Character || (bTriggerOnce && bTriggered))
	{
		return;
	}

	if (TriggerEncounter(Character))
	{
		bTriggered = true;
		if (bTriggerOnce)
		{
			UnbindAllTargets();
		}
	}
}

bool AStoryEncounterDeathListener::TriggerEncounter(AYogCharacterBase* Character)
{
	UGameInstance* GameInstance = Character && Character->GetWorld()
		? Character->GetWorld()->GetGameInstance()
		: nullptr;
	if (!GameInstance)
	{
		GameInstance = GetGameInstance();
	}
	UStoryEncounterRuntimeSubsystem* Runtime = GameInstance
		? GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>()
		: nullptr;
	if (!Runtime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEncounterDeathListener] %s has no StoryEncounterRuntimeSubsystem."),
			*GetNameSafe(this));
		return false;
	}

	if (EncounterPoint)
	{
		return Runtime->TriggerEncounterPoint(EncounterPoint, Character);
	}

	if (EncounterGraph && !NodeId.IsNone())
	{
		return Runtime->TriggerEncounterGraphNode(EncounterGraph, NodeId, Character);
	}

	if (EncounterMap && !NodeId.IsNone())
	{
		return Runtime->TriggerEncounterNode(EncounterMap, NodeId, Character);
	}

	UE_LOG(LogTemp, Warning, TEXT("[StoryEncounterDeathListener] %s has no encounter point or graph/map node."),
		*GetNameSafe(this));
	return false;
}
