#include "Story/StoryEngineSubsystem.h"

#include "Data/LevelInfoPopupDA.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Story/StoryEngineSettings.h"
#include "Tutorial/TutorialManager.h"
#include "UI/YogHUD.h"

void UStoryEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfiguredRuleSets();
}

void UStoryEngineSubsystem::LoadConfiguredRuleSets()
{
	RuleSets.Reset();

	const UStoryEngineSettings* Settings = GetDefault<UStoryEngineSettings>();
	if (!Settings)
	{
		return;
	}

	for (const TSoftObjectPtr<UStoryRuleSetDA>& RuleSetPtr : Settings->RuleSets)
	{
		if (UStoryRuleSetDA* RuleSet = RuleSetPtr.LoadSynchronous())
		{
			RuleSets.Add(RuleSet);
		}
	}
}

void UStoryEngineSubsystem::SetRuleSets(const TArray<UStoryRuleSetDA*>& InRuleSets)
{
	RuleSets.Reset();
	for (UStoryRuleSetDA* RuleSet : InRuleSets)
	{
		if (RuleSet)
		{
			RuleSets.Add(RuleSet);
		}
	}
}

void UStoryEngineSubsystem::AddRuleSet(UStoryRuleSetDA* RuleSet)
{
	if (RuleSet)
	{
		RuleSets.AddUnique(RuleSet);
	}
}

void UStoryEngineSubsystem::BroadcastStoryEvent(FGameplayTag EventTag, APlayerController* PlayerController)
{
	FStoryEventContext Context = FStoryEventContext::Make(EventTag);
	Context.PlayerController = PlayerController;

	if (UWorld* World = GetWorld())
	{
		Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}

	if (UYogSaveGame* Save = GetCurrentSave())
	{
		Context.RunIndex = Save->Statistics.TotalRuns;
	}

	BroadcastStoryEventWithContext(Context);
}

void UStoryEngineSubsystem::BroadcastStoryEventWithContext(FStoryEventContext Context)
{
	if (!Context.EventTag.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StoryEngine] Ignored invalid story event"));
		return;
	}

	if (Context.MapName.IsNone())
	{
		if (UWorld* World = GetWorld())
		{
			Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
		}
	}

	if (Context.RunIndex == INDEX_NONE)
	{
		if (UYogSaveGame* Save = GetCurrentSave())
		{
			Context.RunIndex = Save->Statistics.TotalRuns;
		}
	}

	PendingEvents.Add(Context);
	if (bIsProcessingEvents)
	{
		return;
	}

	bIsProcessingEvents = true;
	int32 ProcessedCount = 0;
	while (PendingEvents.Num() > 0 && ProcessedCount < MaxQueuedEvents)
	{
		const FStoryEventContext NextContext = PendingEvents[0];
		PendingEvents.RemoveAt(0);
		ProcessStoryEvent(NextContext);
		++ProcessedCount;
	}

	if (PendingEvents.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEngine] Dropped %d queued events after hitting recursion guard"), PendingEvents.Num());
		PendingEvents.Reset();
	}

	bIsProcessingEvents = false;
}

void UStoryEngineSubsystem::BroadcastStoryEventWithPayload(FGameplayTag EventTag, FGameplayTag ContextTag,
	FGameplayTag AreaTag, FGameplayTag ItemTag, AActor* SourceActor, APlayerController* PlayerController)
{
	FStoryEventContext Context = FStoryEventContext::Make(EventTag);
	Context.ContextTag = ContextTag;
	Context.AreaTag = AreaTag;
	Context.ItemTag = ItemTag;
	Context.SourceActor = SourceActor;
	Context.PlayerController = PlayerController;
	if (SourceActor)
	{
		Context.SourceName = SourceActor->GetFName();
	}

	BroadcastStoryEventWithContext(Context);
}

bool UStoryEngineSubsystem::EvaluateStoryCondition(const FStoryCondition& Condition, const FStoryEventContext& Context) const
{
	return EvaluateCondition(Condition, Context);
}

void UStoryEngineSubsystem::ExecuteStoryAction(const FStoryAction& Action, FStoryEventContext Context)
{
	DispatchAction(Action, Context);
}

void UStoryEngineSubsystem::ExecuteStoryActions(const TArray<FStoryAction>& Actions, FStoryEventContext Context)
{
	for (const FStoryAction& Action : Actions)
	{
		DispatchAction(Action, Context);
	}
}

void UStoryEngineSubsystem::ResetRunState()
{
	RunFlags.Reset();
	FiredRunRuleIds.Reset();
	FiredMapRuleIds.Reset();
}

void UStoryEngineSubsystem::ResetSessionState()
{
	SessionFlags.Reset();
	FiredSessionRuleIds.Reset();
	FiredMapRuleIds.Reset();
	PendingEvents.Reset();
}

void UStoryEngineSubsystem::SetStoryFlag(FGameplayTag FlagTag, EStoryFlagScope Scope, bool bValue)
{
	if (!FlagTag.IsValid())
	{
		return;
	}

	switch (Scope)
	{
	case EStoryFlagScope::Save:
		if (UYogSaveGame* Save = GetCurrentSave())
		{
			if (bValue)
			{
				Save->StoryFlags.Add(FlagTag, true);
			}
			else
			{
				Save->StoryFlags.Remove(FlagTag);
			}
			CommitSave();
		}
		break;
	case EStoryFlagScope::Run:
		if (bValue)
		{
			RunFlags.Add(FlagTag);
		}
		else
		{
			RunFlags.Remove(FlagTag);
		}
		break;
	case EStoryFlagScope::Session:
		if (bValue)
		{
			SessionFlags.Add(FlagTag);
		}
		else
		{
			SessionFlags.Remove(FlagTag);
		}
		break;
	default:
		break;
	}
}

bool UStoryEngineSubsystem::HasStoryFlag(FGameplayTag FlagTag, EStoryFlagScope Scope) const
{
	if (!FlagTag.IsValid())
	{
		return false;
	}

	switch (Scope)
	{
	case EStoryFlagScope::Save:
		if (const UYogSaveGame* Save = GetCurrentSave())
		{
			const bool* bFound = Save->StoryFlags.Find(FlagTag);
			return bFound && *bFound;
		}
		return false;
	case EStoryFlagScope::Run:
		return RunFlags.Contains(FlagTag);
	case EStoryFlagScope::Session:
		return SessionFlags.Contains(FlagTag);
	default:
		return false;
	}
}

void UStoryEngineSubsystem::SetQuestTask(FGameplayTag TaskId, FText DisplayText, FGameplayTag SourceTag, FGameplayTag RelatedFlagTag)
{
	if (!TaskId.IsValid())
	{
		return;
	}

	FStoryQuestTaskData TaskData;
	TaskData.TaskId = TaskId;
	TaskData.State = EStoryQuestTaskState::Active;
	TaskData.DisplayText = DisplayText;
	TaskData.SourceTag = SourceTag;
	TaskData.RelatedFlagTag = RelatedFlagTag;

	TransientQuestTasks.Add(TaskId, TaskData);

	if (UYogSaveGame* Save = GetCurrentSave())
	{
		Save->StoryQuestTasks.Add(TaskId, TaskData);
		CommitSave();
	}

	OnQuestTaskChanged.Broadcast(TaskData);
}

void UStoryEngineSubsystem::CompleteQuestTask(FGameplayTag TaskId)
{
	SetQuestTaskState(TaskId, EStoryQuestTaskState::Completed);
}

void UStoryEngineSubsystem::SetQuestTaskState(FGameplayTag TaskId, EStoryQuestTaskState NewState)
{
	if (!TaskId.IsValid())
	{
		return;
	}

	FStoryQuestTaskData TaskData;
	TaskData.TaskId = TaskId;
	TaskData.State = NewState;

	if (FStoryQuestTaskData* ExistingTransientTask = TransientQuestTasks.Find(TaskId))
	{
		ExistingTransientTask->State = NewState;
		TaskData = *ExistingTransientTask;
	}
	else
	{
		TransientQuestTasks.Add(TaskId, TaskData);
	}

	if (UYogSaveGame* Save = GetCurrentSave())
	{
		if (FStoryQuestTaskData* ExistingTask = Save->StoryQuestTasks.Find(TaskId))
		{
			ExistingTask->State = NewState;
			TaskData = *ExistingTask;
			TransientQuestTasks.Add(TaskId, TaskData);
		}
		else
		{
			Save->StoryQuestTasks.Add(TaskId, TaskData);
		}
		CommitSave();
	}

	OnQuestTaskChanged.Broadcast(TaskData);
}

bool UStoryEngineSubsystem::GetQuestTask(FGameplayTag TaskId, FStoryQuestTaskData& OutTask) const
{
	if (!TaskId.IsValid())
	{
		return false;
	}

	if (const UYogSaveGame* Save = GetCurrentSave())
	{
		if (const FStoryQuestTaskData* FoundTask = Save->StoryQuestTasks.Find(TaskId))
		{
			OutTask = *FoundTask;
			return true;
		}
	}

	if (const FStoryQuestTaskData* FoundTask = TransientQuestTasks.Find(TaskId))
	{
		OutTask = *FoundTask;
		return true;
	}

	return false;
}

TArray<FStoryQuestTaskData> UStoryEngineSubsystem::GetAllQuestTasks() const
{
	TMap<FGameplayTag, FStoryQuestTaskData> Tasks = TransientQuestTasks;
	if (const UYogSaveGame* Save = GetCurrentSave())
	{
		for (const TPair<FGameplayTag, FStoryQuestTaskData>& Pair : Save->StoryQuestTasks)
		{
			Tasks.Add(Pair.Key, Pair.Value);
		}
	}

	TArray<FStoryQuestTaskData> Result;
	Tasks.GenerateValueArray(Result);
	Result.Sort([](const FStoryQuestTaskData& Left, const FStoryQuestTaskData& Right)
	{
		return Left.TaskId.ToString() < Right.TaskId.ToString();
	});
	return Result;
}

TArray<FStoryQuestTaskData> UStoryEngineSubsystem::GetQuestTasksByState(EStoryQuestTaskState State) const
{
	TArray<FStoryQuestTaskData> Result;
	for (const FStoryQuestTaskData& Task : GetAllQuestTasks())
	{
		if (Task.State == State)
		{
			Result.Add(Task);
		}
	}
	return Result;
}

void UStoryEngineSubsystem::ProcessStoryEvent(const FStoryEventContext& Context)
{
	OnStoryEventReceived.Broadcast(Context);

	TArray<const FStoryRule*> MatchingRules;
	CollectRulesForEvent(Context.EventTag, MatchingRules);

	for (const FStoryRule* Rule : MatchingRules)
	{
		if (!Rule || ShouldSkipForFirePolicy(*Rule, Context) || !EvaluateRule(*Rule, Context))
		{
			continue;
		}

		DispatchActions(*Rule, Context);
		MarkRuleFired(*Rule, Context);
		OnStoryRuleExecuted.Broadcast(Rule->RuleId, Context);

		if (Rule->bStopAfterMatch)
		{
			break;
		}
	}
}

void UStoryEngineSubsystem::CollectRulesForEvent(FGameplayTag EventTag, TArray<const FStoryRule*>& OutRules) const
{
	OutRules.Reset();

	for (const UStoryRuleSetDA* RuleSet : RuleSets)
	{
		if (!RuleSet)
		{
			continue;
		}

		TArray<const FStoryRule*> RuleSetRules;
		RuleSet->GetRulesForEventSorted(EventTag, RuleSetRules);
		OutRules.Append(RuleSetRules);
	}

	OutRules.Sort([](const FStoryRule& Left, const FStoryRule& Right)
	{
		if (Left.Priority != Right.Priority)
		{
			return Left.Priority > Right.Priority;
		}
		return Left.RuleId.LexicalLess(Right.RuleId);
	});
}

bool UStoryEngineSubsystem::EvaluateRule(const FStoryRule& Rule, const FStoryEventContext& Context) const
{
	if (Rule.Conditions.IsEmpty())
	{
		return true;
	}

	if (Rule.ConditionPolicy == EStoryConditionMatchPolicy::Any)
	{
		for (const FStoryCondition& Condition : Rule.Conditions)
		{
			if (EvaluateCondition(Condition, Context))
			{
				return true;
			}
		}
		return false;
	}

	for (const FStoryCondition& Condition : Rule.Conditions)
	{
		if (!EvaluateCondition(Condition, Context))
		{
			return false;
		}
	}
	return true;
}

bool UStoryEngineSubsystem::EvaluateCondition(const FStoryCondition& Condition, const FStoryEventContext& Context) const
{
	bool bResult = true;
	switch (Condition.Type)
	{
	case EStoryConditionType::None:
		bResult = true;
		break;
	case EStoryConditionType::HasFlag:
		bResult = HasStoryFlag(Condition.FlagTag, Condition.FlagScope);
		break;
	case EStoryConditionType::FeatureUnlocked:
		if (const UYogMetaProgressionSubsystem* Meta = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>()
			: nullptr)
		{
			bResult = Meta->IsFeatureUnlocked(Condition.FeatureTag);
		}
		else
		{
			bResult = false;
		}
		break;
	case EStoryConditionType::TutorialStateEquals:
		if (const UTutorialManager* TutorialManager = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTutorialManager>()
			: nullptr)
		{
			bResult = TutorialManager->GetState() == Condition.TutorialState;
		}
		else
		{
			bResult = false;
		}
		break;
	case EStoryConditionType::RunCountAtLeast:
		if (const UYogSaveGame* Save = GetCurrentSave())
		{
			bResult = Save->Statistics.TotalRuns >= Condition.RunCount;
		}
		else
		{
			bResult = false;
		}
		break;
	case EStoryConditionType::EventTagEquals:
		bResult = Condition.MatchTag.IsValid() && Context.EventTag.MatchesTagExact(Condition.MatchTag);
		break;
	case EStoryConditionType::ContextTagMatches:
		bResult = Condition.MatchTag.IsValid() &&
			(Context.ContextTag.MatchesTag(Condition.MatchTag) ||
				Context.AreaTag.MatchesTag(Condition.MatchTag) ||
				Context.ItemTag.MatchesTag(Condition.MatchTag));
		break;
	default:
		bResult = false;
		break;
	}

	return Condition.bInvert ? !bResult : bResult;
}

void UStoryEngineSubsystem::DispatchActions(const FStoryRule& Rule, const FStoryEventContext& Context)
{
	for (const FStoryAction& Action : Rule.Actions)
	{
		DispatchAction(Action, Context);
	}
}

void UStoryEngineSubsystem::DispatchAction(const FStoryAction& Action, const FStoryEventContext& Context)
{
	switch (Action.Type)
	{
	case EStoryActionType::SetFlag:
		SetStoryFlag(Action.FlagTag, Action.FlagScope, true);
		break;
	case EStoryActionType::ClearFlag:
		SetStoryFlag(Action.FlagTag, Action.FlagScope, false);
		break;
	case EStoryActionType::PlayLevelFlow:
		if (Action.LevelFlow)
		{
			AYogGameMode* GameMode = GetWorld() ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr;
			if (GameMode)
			{
				GameMode->RunStoryLevelFlow(Action.LevelFlow, Action.bStopExistingStoryFlow);
			}
		}
		break;
	case EStoryActionType::ShowTutorialPopup:
		if (UTutorialManager* TutorialManager = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTutorialManager>()
			: nullptr)
		{
			if (Action.TutorialPages.Num() > 0)
			{
				TutorialManager->ShowInlinePages(Action.TutorialPages, ResolvePlayerController(Context), Action.bPauseGame);
			}
			else
			{
				TutorialManager->ShowByEventID(Action.TutorialEventId, ResolvePlayerController(Context), Action.bPauseGame);
			}
		}
		break;
	case EStoryActionType::ShowInfoHint:
		if (APlayerController* PlayerController = ResolvePlayerController(Context))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PlayerController->GetHUD()))
			{
				ULevelInfoPopupDA* Popup = NewObject<ULevelInfoPopupDA>(this);
				Popup->Title = Action.HintTitle;
				Popup->Body = Action.HintText;
				Popup->HUDSummaryText = Action.HintText;
				Popup->DisplayDuration = Action.HintDuration;
				TransientInfoPopups.Add(Popup);
				if (TransientInfoPopups.Num() > 8)
				{
					TransientInfoPopups.RemoveAt(0);
				}
				HUD->ShowInfoPopup(Popup);
			}
		}
		break;
	case EStoryActionType::SetQuestTask:
		SetQuestTask(Action.QuestTaskId, Action.QuestTaskText, Action.QuestSourceTag, Action.RelatedFlagTag);
		break;
	case EStoryActionType::CompleteQuestTask:
		CompleteQuestTask(Action.QuestTaskId);
		break;
	case EStoryActionType::UnlockFeature:
		if (UYogMetaProgressionSubsystem* Meta = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>()
			: nullptr)
		{
			Meta->UnlockFeature(Action.FeatureTag);
		}
		break;
	case EStoryActionType::AddMetaCurrency:
		if (UYogMetaProgressionSubsystem* Meta = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>()
			: nullptr)
		{
			Meta->AddCurrency(Action.CurrencyTag, Action.Amount);
		}
		break;
	case EStoryActionType::TriggerStoryEvent:
		if (Action.EventTagToTrigger.IsValid())
		{
			FStoryEventContext NextContext = Context;
			NextContext.EventTag = Action.EventTagToTrigger;
			BroadcastStoryEventWithContext(NextContext);
		}
		break;
	case EStoryActionType::None:
	default:
		break;
	}
}

bool UStoryEngineSubsystem::ShouldSkipForFirePolicy(const FStoryRule& Rule, const FStoryEventContext& Context) const
{
	if (Rule.FirePolicy == EStoryRuleFirePolicy::Always)
	{
		return false;
	}

	const FName FireKey = MakeRuleFireKey(Rule, Context);
	if (FireKey.IsNone())
	{
		return false;
	}

	switch (Rule.FirePolicy)
	{
	case EStoryRuleFirePolicy::OncePerSave:
		if (const UYogSaveGame* Save = GetCurrentSave())
		{
			return Save->StoryFiredRuleIds.Contains(FireKey);
		}
		return false;
	case EStoryRuleFirePolicy::OncePerRun:
		return FiredRunRuleIds.Contains(FireKey);
	case EStoryRuleFirePolicy::OncePerMap:
		return FiredMapRuleIds.Contains(FireKey);
	case EStoryRuleFirePolicy::Always:
	default:
		return false;
	}
}

void UStoryEngineSubsystem::MarkRuleFired(const FStoryRule& Rule, const FStoryEventContext& Context)
{
	if (Rule.FirePolicy == EStoryRuleFirePolicy::Always)
	{
		return;
	}

	const FName FireKey = MakeRuleFireKey(Rule, Context);
	if (FireKey.IsNone())
	{
		return;
	}

	switch (Rule.FirePolicy)
	{
	case EStoryRuleFirePolicy::OncePerSave:
		if (UYogSaveGame* Save = GetCurrentSave())
		{
			Save->StoryFiredRuleIds.Add(FireKey);
			CommitSave();
		}
		break;
	case EStoryRuleFirePolicy::OncePerRun:
		FiredRunRuleIds.Add(FireKey);
		break;
	case EStoryRuleFirePolicy::OncePerMap:
		FiredMapRuleIds.Add(FireKey);
		break;
	case EStoryRuleFirePolicy::Always:
	default:
		break;
	}
}

FName UStoryEngineSubsystem::MakeRuleFireKey(const FStoryRule& Rule, const FStoryEventContext& Context) const
{
	if (Rule.RuleId.IsNone())
	{
		return NAME_None;
	}

	if (Rule.FirePolicy == EStoryRuleFirePolicy::OncePerMap && !Context.MapName.IsNone())
	{
		return FName(*FString::Printf(TEXT("%s@%s"), *Rule.RuleId.ToString(), *Context.MapName.ToString()));
	}

	return Rule.RuleId;
}

UYogSaveGame* UStoryEngineSubsystem::GetCurrentSave() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			return SaveSubsystem->GetCurrentSave();
		}
	}
	return nullptr;
}

APlayerController* UStoryEngineSubsystem::ResolvePlayerController(const FStoryEventContext& Context) const
{
	if (Context.PlayerController)
	{
		return Context.PlayerController;
	}

	return GetWorld() ? UGameplayStatics::GetPlayerController(GetWorld(), 0) : nullptr;
}

void UStoryEngineSubsystem::CommitSave() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			SaveSubsystem->DoAsyncSave();
		}
	}
}
