#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Data/LevelInfoPopupDA.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterTrigger.h"
#include "LevelFlow/LevelEventTrigger.h"
#include "Story/StoryEngineSubsystem.h"
#include "UI/InfoPopupWidget.h"
#include "UI/YogHUD.h"

namespace
{
FName SanitizeTagSegment(FName RawName)
{
	FString Value = RawName.ToString();
	Value.TrimStartAndEndInline();
	Value.ReplaceInline(TEXT(" "), TEXT("_"));
	Value.ReplaceInline(TEXT("-"), TEXT("_"));

	FString Sanitized;
	for (const TCHAR Character : Value)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('.'))
		{
			Sanitized.AppendChar(Character);
		}
	}

	while (Sanitized.Contains(TEXT("..")))
	{
		Sanitized.ReplaceInline(TEXT(".."), TEXT("."));
	}
	Sanitized.RemoveFromStart(TEXT("."));
	Sanitized.RemoveFromEnd(TEXT("."));

	return Sanitized.IsEmpty() ? NAME_None : FName(*Sanitized);
}

APlayerController* ResolveEncounterPlayer(AActor* SourceActor)
{
	if (APawn* Pawn = Cast<APawn>(SourceActor))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}

	return SourceActor && SourceActor->GetWorld()
		? UGameplayStatics::GetPlayerController(SourceActor->GetWorld(), 0)
		: nullptr;
}

FText ResolveInputAwareBody(const FStoryEncounterAction& Action, const FStoryEventContext& Context)
{
	if (!Action.bUseInputTextVariants)
	{
		return Action.Body;
	}

	const APlayerController* PlayerController = Context.PlayerController;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const UCommonInputSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(LocalPlayer)
		: nullptr;
	if (!InputSubsystem)
	{
		return Action.Body;
	}

	if (InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad && !Action.GamepadBody.IsEmpty())
	{
		return Action.GamepadBody;
	}

	if (InputSubsystem->GetCurrentInputType() == ECommonInputType::MouseAndKeyboard && !Action.KeyboardMouseBody.IsEmpty())
	{
		return Action.KeyboardMouseBody;
	}

	return Action.Body;
}

bool DoesActorMatchStoryTarget(const AActor* Actor, FName TargetActorName, FName TargetActorTag)
{
	if (!Actor)
	{
		return false;
	}

	if (!TargetActorName.IsNone())
	{
		if (Actor->GetFName() == TargetActorName || Actor->GetName().Equals(TargetActorName.ToString(), ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	if (!TargetActorTag.IsNone() && Actor->ActorHasTag(TargetActorTag))
	{
		return true;
	}

	return false;
}

void SetActorStoryEnabled(AActor* Actor, bool bEnabled)
{
	if (!Actor)
	{
		return;
	}

	Actor->SetActorHiddenInGame(!bEnabled);
	Actor->SetActorEnableCollision(bEnabled);
	Actor->SetActorTickEnabled(bEnabled);
}
}

bool UStoryEncounterRuntimeSubsystem::TriggerEncounterNode(UStoryEncounterMap* EncounterMap, FName NodeId, AActor* SourceActor)
{
	if (!EncounterMap || NodeId.IsNone())
	{
		return false;
	}

	const FStoryEncounterNode* Node = EncounterMap->FindNode(NodeId);
	if (!Node)
	{
		return false;
	}

	FStoryEventContext Context;
	Context.SourceActor = SourceActor;
	Context.PlayerController = ResolveEncounterPlayer(SourceActor);
	Context.SourceName = NodeId;
	if (UWorld* World = GetWorld())
	{
		Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}

	if (!CanTriggerNode(EncounterMap->EncounterId, *Node, Context))
	{
		return false;
	}

	for (const FStoryEncounterAction& Action : Node->Actions)
	{
		ExecuteEncounterAction(EncounterMap->EncounterId, Action, Context);
	}

	return true;
}

bool UStoryEncounterRuntimeSubsystem::TriggerEncounterGraphNode(UStoryEncounterGraph* EncounterGraph, FName NodeId, AActor* SourceActor)
{
	if (!EncounterGraph || NodeId.IsNone())
	{
		return false;
	}

	UStoryEncounterGraphNode* GraphNode = EncounterGraph->FindNodeByStoryNodeId(NodeId);
	if (!GraphNode)
	{
		return false;
	}

	if (UStoryEncounterPointDA* Point = GraphNode->GetPoint())
	{
		return TriggerEncounterPoint(Point, SourceActor);
	}

	const FStoryEncounterNode Node = GraphNode->ToEncounterNode();
	const FName EncounterId = !GraphNode->GetEncounterId().IsNone()
		? GraphNode->GetEncounterId()
		: EncounterGraph->EncounterId;
	if (EncounterId.IsNone() || Node.NodeId.IsNone())
	{
		return false;
	}

	FStoryEventContext Context;
	Context.SourceActor = SourceActor;
	Context.PlayerController = ResolveEncounterPlayer(SourceActor);
	Context.SourceName = Node.NodeId;
	if (UWorld* World = GetWorld())
	{
		Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}

	if (!CanTriggerNode(EncounterId, Node, Context))
	{
		return false;
	}

	for (const FStoryEncounterAction& Action : Node.Actions)
	{
		ExecuteEncounterAction(EncounterId, Action, Context);
	}

	return true;
}

bool UStoryEncounterRuntimeSubsystem::TriggerEncounterPoint(UStoryEncounterPointDA* EncounterPoint, AActor* SourceActor)
{
	if (!EncounterPoint)
	{
		return false;
	}

	const FStoryEncounterNode Node = EncounterPoint->ToEncounterNode();
	if (Node.NodeId.IsNone())
	{
		return false;
	}

	FStoryEventContext Context;
	Context.SourceActor = SourceActor;
	Context.PlayerController = ResolveEncounterPlayer(SourceActor);
	Context.SourceName = Node.NodeId;
	if (UWorld* World = GetWorld())
	{
		Context.MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}

	if (!CanTriggerNode(EncounterPoint->EncounterId, Node, Context))
	{
		return false;
	}

	for (const FStoryEncounterAction& Action : Node.Actions)
	{
		ExecuteEncounterAction(EncounterPoint->EncounterId, Action, Context);
	}

	return true;
}

FString UStoryEncounterRuntimeSubsystem::MakeProgressTagName(FName EncounterId, FName ProgressKey)
{
	const FName CleanEncounterId = SanitizeTagSegment(EncounterId);
	const FName CleanProgressKey = SanitizeTagSegment(ProgressKey);
	if (CleanEncounterId.IsNone() || CleanProgressKey.IsNone())
	{
		return FString();
	}

	return FString::Printf(TEXT("Story.Encounter.Progress.%s.%s"),
		*CleanEncounterId.ToString(),
		*CleanProgressKey.ToString());
}

FGameplayTag UStoryEncounterRuntimeSubsystem::MakeProgressTag(FName EncounterId, FName ProgressKey)
{
	const FString TagName = MakeProgressTagName(EncounterId, ProgressKey);
	return TagName.IsEmpty() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(FName(*TagName), false);
}

bool UStoryEncounterRuntimeSubsystem::ConvertEncounterActionForTest(FName EncounterId,
	const FStoryEncounterAction& EncounterAction, FStoryAction& OutStoryAction)
{
	OutStoryAction = FStoryAction();

	switch (EncounterAction.Kind)
	{
	case EStoryEncounterActionKind::Dialogue:
		OutStoryAction.Type = EStoryActionType::ShowInfoHint;
		OutStoryAction.HintTitle = EncounterAction.Title;
		OutStoryAction.HintText = EncounterAction.Body;
		OutStoryAction.HintDuration = 3.f;
		return true;

	case EStoryEncounterActionKind::WeakHint:
		OutStoryAction.Type = EStoryActionType::ShowInfoHint;
		OutStoryAction.HintTitle = FText::GetEmpty();
		OutStoryAction.HintText = EncounterAction.Body;
		OutStoryAction.HintDuration = 3.f;
		return true;

	case EStoryEncounterActionKind::TutorialAreaHint:
		OutStoryAction.Type = EStoryActionType::ShowInfoHint;
		OutStoryAction.HintTitle = FText::GetEmpty();
		OutStoryAction.HintText = EncounterAction.Body;
		OutStoryAction.HintDuration = 0.f;
		return true;

	case EStoryEncounterActionKind::TutorialPopup:
		OutStoryAction.Type = EStoryActionType::ShowTutorialPopup;
		OutStoryAction.TutorialEventId = EncounterAction.TutorialEventId;
		OutStoryAction.TutorialPages = EncounterAction.TutorialPages;
		OutStoryAction.bPauseGame = EncounterAction.bPauseGame;
		return !OutStoryAction.TutorialEventId.IsNone() || OutStoryAction.TutorialPages.Num() > 0;

	case EStoryEncounterActionKind::RecordProgress:
		OutStoryAction.Type = EStoryActionType::SetFlag;
		OutStoryAction.FlagScope = EStoryFlagScope::Save;
		OutStoryAction.FlagTag = MakeProgressTag(EncounterId, EncounterAction.ProgressKey);
		return OutStoryAction.FlagTag.IsValid();

	case EStoryEncounterActionKind::UnlockFeature:
		OutStoryAction.Type = EStoryActionType::UnlockFeature;
		OutStoryAction.FeatureTag = EncounterAction.FeatureTag;
		return OutStoryAction.FeatureTag.IsValid();

	case EStoryEncounterActionKind::SetQuestObjective:
		OutStoryAction.Type = EStoryActionType::SetQuestTask;
		OutStoryAction.QuestTaskId = EncounterAction.QuestTaskTag;
		OutStoryAction.QuestTaskText = EncounterAction.Body;
		return OutStoryAction.QuestTaskId.IsValid();

	case EStoryEncounterActionKind::PlayLevelFlow:
		OutStoryAction.Type = EStoryActionType::PlayLevelFlow;
		OutStoryAction.LevelFlow = EncounterAction.LevelFlow;
		return OutStoryAction.LevelFlow != nullptr;

	case EStoryEncounterActionKind::SetActorEnabled:
	case EStoryEncounterActionKind::TeleportToNode:
	default:
		return false;
	}
}

bool UStoryEncounterRuntimeSubsystem::CanTriggerNode(FName EncounterId,
	const FStoryEncounterNode& Node, const FStoryEventContext& Context) const
{
	if (EncounterId.IsNone())
	{
		return false;
	}

	const UStoryEngineSubsystem* StoryEngine = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
		: nullptr;
	if (!StoryEngine)
	{
		return Node.Condition.Kind == EStoryEncounterConditionKind::None;
	}

	switch (Node.Condition.Kind)
	{
	case EStoryEncounterConditionKind::None:
		return true;
	case EStoryEncounterConditionKind::ProgressMissing:
		return !StoryEngine->HasStoryFlag(MakeProgressTag(EncounterId, Node.Condition.ProgressKey), EStoryFlagScope::Save);
	case EStoryEncounterConditionKind::ProgressCompleted:
		return StoryEngine->HasStoryFlag(MakeProgressTag(EncounterId, Node.Condition.ProgressKey), EStoryFlagScope::Save);
	case EStoryEncounterConditionKind::RunCountAtLeast:
		return Context.RunIndex >= Node.Condition.RunCount;
	case EStoryEncounterConditionKind::FeatureUnlocked:
	{
		FStoryCondition Condition;
		Condition.Type = EStoryConditionType::FeatureUnlocked;
		Condition.FeatureTag = Node.Condition.FeatureTag;
		return StoryEngine->EvaluateStoryCondition(Condition, Context);
	}
	default:
		return false;
	}
}

bool UStoryEncounterRuntimeSubsystem::ExecuteActorEnabledAction(
	const FStoryEncounterAction& Action,
	const FStoryEventContext& Context) const
{
	if (Action.TargetActorName.IsNone() && Action.TargetActorTag.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEncounter] SetActorEnabled skipped: no TargetActorName or TargetActorTag."));
		return false;
	}

	UWorld* World = nullptr;
	if (Context.SourceActor)
	{
		World = Context.SourceActor->GetWorld();
	}
	if (!World)
	{
		World = GetWorld();
	}
	if (!World)
	{
		return false;
	}

	int32 MatchedCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!DoesActorMatchStoryTarget(Actor, Action.TargetActorName, Action.TargetActorTag))
		{
			continue;
		}

		SetActorStoryEnabled(Actor, Action.bActorEnabled);
		++MatchedCount;
	}

	if (MatchedCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEncounter] SetActorEnabled found no actors. Name=%s Tag=%s Enabled=%d"),
			*Action.TargetActorName.ToString(),
			*Action.TargetActorTag.ToString(),
			Action.bActorEnabled ? 1 : 0);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[StoryEncounter] SetActorEnabled Name=%s Tag=%s Enabled=%d Count=%d"),
		*Action.TargetActorName.ToString(),
		*Action.TargetActorTag.ToString(),
		Action.bActorEnabled ? 1 : 0,
		MatchedCount);
	return true;
}

bool UStoryEncounterRuntimeSubsystem::ExecuteTutorialAreaHintAction(
	const FStoryEncounterAction& Action,
	const FStoryEventContext& Context)
{
	APlayerController* PlayerController = Context.PlayerController;
	if (!PlayerController)
	{
		PlayerController = ResolveEncounterPlayer(Context.SourceActor);
	}

	AYogHUD* HUD = PlayerController ? Cast<AYogHUD>(PlayerController->GetHUD()) : nullptr;
	if (!HUD)
	{
		return false;
	}

	ULevelInfoPopupDA* Popup = NewObject<ULevelInfoPopupDA>(this);
	Popup->Title = FText::GetEmpty();
	Popup->Body = ResolveInputAwareBody(Action, Context);
	if (Popup->Body.IsEmpty())
	{
		Popup->Body = Action.Body;
	}
	Popup->HUDSummaryText = Popup->Body;
	Popup->DisplayDuration = 0.f;
	Popup->FadeDuration = 0.15f;

	HUD->ShowInfoPopup(Popup);

	UInfoPopupWidget* Widget = HUD->GetInfoPopupWidget();
	if (!Widget)
	{
		return true;
	}

	TWeakObjectPtr<UInfoPopupWidget> WeakWidget(Widget);
	if (ALevelEventTrigger* Trigger = Cast<ALevelEventTrigger>(Context.SourceActor))
	{
		Trigger->OnPlayerExited.AddWeakLambda(this, [WeakWidget]()
		{
			if (WeakWidget.IsValid())
			{
				WeakWidget->RequestClose();
			}
		});
	}
	else if (AStoryEncounterTrigger* StoryTrigger = Cast<AStoryEncounterTrigger>(Context.SourceActor))
	{
		StoryTrigger->OnPlayerExited.AddWeakLambda(this, [WeakWidget]()
		{
			if (WeakWidget.IsValid())
			{
				WeakWidget->RequestClose();
			}
		});
	}

	return true;
}

void UStoryEncounterRuntimeSubsystem::ExecuteEncounterAction(FName EncounterId,
	const FStoryEncounterAction& Action, const FStoryEventContext& Context)
{
	if (Action.Kind == EStoryEncounterActionKind::SetActorEnabled)
	{
		ExecuteActorEnabledAction(Action, Context);
		return;
	}
	if (Action.Kind == EStoryEncounterActionKind::TutorialAreaHint)
	{
		ExecuteTutorialAreaHintAction(Action, Context);
		return;
	}

	UStoryEngineSubsystem* StoryEngine = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
		: nullptr;
	if (!StoryEngine)
	{
		return;
	}

	FStoryAction StoryAction;
	if (ConvertEncounterActionForTest(EncounterId, Action, StoryAction))
	{
		if (StoryAction.Type == EStoryActionType::ShowInfoHint || StoryAction.Type == EStoryActionType::SetQuestTask)
		{
			const FText InputAwareBody = ResolveInputAwareBody(Action, Context);
			if (!InputAwareBody.IsEmpty())
			{
				if (StoryAction.Type == EStoryActionType::ShowInfoHint)
				{
					StoryAction.HintText = InputAwareBody;
				}
				else
				{
					StoryAction.QuestTaskText = InputAwareBody;
				}
			}
		}

		if (StoryAction.Type == EStoryActionType::PlayLevelFlow)
		{
			if (AStoryEncounterTrigger* Trigger = Cast<AStoryEncounterTrigger>(Context.SourceActor))
			{
				if (Trigger->RunLevelFlow(StoryAction.LevelFlow, StoryAction.bStopExistingStoryFlow))
				{
					return;
				}
			}
			else if (ALevelEventTrigger* LevelTrigger = Cast<ALevelEventTrigger>(Context.SourceActor))
			{
				if (LevelTrigger->RunLevelFlow(StoryAction.LevelFlow, StoryAction.bStopExistingStoryFlow))
				{
					return;
				}
			}
		}

		StoryEngine->ExecuteStoryAction(StoryAction, Context);
	}
}
