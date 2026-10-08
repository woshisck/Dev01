#include "Story/StoryEventManager.h"

#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryEventRegistryDA.h"

void UStoryEventManager::SetRegistry(UStoryEventRegistryDA* InRegistry)
{
	Registry = InRegistry;
}

void UStoryEventManager::ProcessCampaignStage(int32 FloorIndex, FGameplayTag StageTag, FGameplayTagContainer EventTags,
	URoomDataAsset* RoomData, APlayerController* PlayerController)
{
	TArray<FGameplayTag> Tags;
	EventTags.GetGameplayTagArray(Tags);

	for (const FGameplayTag& EventTag : Tags)
	{
		FStoryEventRuntimeContext Context = BuildContext(FloorIndex, StageTag, EventTag, RoomData);
		if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
			: nullptr)
		{
			FStoryEventContext StoryContext = FStoryEventContext::Make(EventTag);
			StoryContext.AreaTag = StageTag;
			StoryContext.ContextTag = StageTag;
			StoryContext.FloorIndex = FloorIndex;
			StoryContext.PlayerController = PlayerController;
			StoryEngine->BroadcastStoryEventWithContext(StoryContext);
		}

		const FStoryEventEntry* Entry = Registry ? Registry->FindEntry(EventTag) : nullptr;
		if (!Entry)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[StoryEvent] Unconfigured event tag: %s"), *EventTag.ToString());
			OnStoryEventSkipped.Broadcast(Context);
			continue;
		}

		Context.ActionType = Entry->ActionType;
		Context.ResolvedLevelFlow = Entry->LevelFlow;

		if (Entry->bFireOncePerRun && FiredRunEventTags.HasTagExact(EventTag))
		{
			Context.Result = EStoryEventDispatchResult::SkippedAlreadyFired;
			OnStoryEventSkipped.Broadcast(Context);
			continue;
		}

		bool bHandled = false;
		switch (Entry->ActionType)
		{
		case EStoryEventActionType::Deprecated_TutorialPopup:
			bHandled = false;
			break;
		case EStoryEventActionType::LevelFlow:
			bHandled = DispatchLevelFlow(*Entry, Context);
			break;
		case EStoryEventActionType::BroadcastOnly:
		case EStoryEventActionType::None:
			bHandled = true;
			break;
		default:
			break;
		}

		Context.Result = bHandled ? EStoryEventDispatchResult::Triggered : EStoryEventDispatchResult::Failed;
		if (bHandled && Entry->bFireOncePerRun)
		{
			FiredRunEventTags.AddTag(EventTag);
		}

		OnStoryEventDispatched.Broadcast(Context);
	}
}

void UStoryEventManager::ResetRunEvents()
{
	FiredRunEventTags.Reset();
	if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
		: nullptr)
	{
		StoryEngine->ResetRunState();
	}
}

bool UStoryEventManager::HasFiredEvent(FGameplayTag EventTag) const
{
	return EventTag.IsValid() && FiredRunEventTags.HasTagExact(EventTag);
}

FStoryEventRuntimeContext UStoryEventManager::BuildContext(int32 FloorIndex, FGameplayTag StageTag,
	FGameplayTag EventTag, URoomDataAsset* RoomData) const
{
	FStoryEventRuntimeContext Context;
	Context.FloorIndex = FloorIndex;
	Context.StageTag = StageTag;
	Context.EventTag = EventTag;
	Context.RoomData = RoomData;
	return Context;
}

bool UStoryEventManager::DispatchLevelFlow(const FStoryEventEntry& Entry, FStoryEventRuntimeContext& Context) const
{
	if (!Entry.LevelFlow)
	{
		return false;
	}

	AYogGameMode* GameMode = GetWorld() ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr;
	if (!GameMode)
	{
		return false;
	}

	Context.ResolvedLevelFlow = Entry.LevelFlow;
	return GameMode->RunStoryLevelFlow(Entry.LevelFlow, Entry.bStopExistingStoryFlow);
}
