#include "Story/StoryEventManager.h"

#include "Story/StoryEventRegistryDA.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "Tutorial/TutorialManager.h"

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
		const FStoryEventEntry* Entry = Registry ? Registry->FindEntry(EventTag) : nullptr;
		if (!Entry)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[StoryEvent] Unconfigured event tag: %s"), *EventTag.ToString());
			OnStoryEventSkipped.Broadcast(Context);
			continue;
		}

		Context.ActionType = Entry->ActionType;
		Context.ResolvedTutorialEventID = Entry->TutorialEventID.IsNone()
			? FName(*EventTag.ToString())
			: Entry->TutorialEventID;

		if (Entry->bFireOncePerRun && FiredRunEventTags.HasTagExact(EventTag))
		{
			Context.Result = EStoryEventDispatchResult::SkippedAlreadyFired;
			OnStoryEventSkipped.Broadcast(Context);
			continue;
		}

		if (ShouldSkipForTutorialState(*Entry))
		{
			Context.Result = EStoryEventDispatchResult::SkippedTutorialCompleted;
			OnStoryEventSkipped.Broadcast(Context);
			continue;
		}

		bool bHandled = false;
		switch (Entry->ActionType)
		{
		case EStoryEventActionType::TutorialPopup:
			bHandled = DispatchTutorialPopup(*Entry, Context, PlayerController);
			break;
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

bool UStoryEventManager::ShouldSkipForTutorialState(const FStoryEventEntry& Entry) const
{
	if (!Entry.bOnlyWhenTutorialIncomplete)
	{
		return false;
	}

	const UTutorialManager* TutorialManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTutorialManager>()
		: nullptr;
	return TutorialManager && TutorialManager->GetState() == ETutorialState::Completed;
}

bool UStoryEventManager::DispatchTutorialPopup(const FStoryEventEntry& Entry, FStoryEventRuntimeContext& Context,
	APlayerController* PlayerController)
{
	UTutorialManager* TutorialManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTutorialManager>()
		: nullptr;
	if (!TutorialManager || !PlayerController)
	{
		return false;
	}

	const FName TutorialEventID = Entry.TutorialEventID.IsNone()
		? FName(*Entry.EventTag.ToString())
		: Entry.TutorialEventID;
	Context.ResolvedTutorialEventID = TutorialEventID;
	return TutorialManager->ShowByEventID(TutorialEventID, PlayerController, Entry.bPauseGame);
}
