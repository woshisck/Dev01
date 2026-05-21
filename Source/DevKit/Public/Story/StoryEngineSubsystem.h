#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/StoryRuleSetDA.h"
#include "StoryEngineSubsystem.generated.h"

class ULevelInfoPopupDA;
class UYogSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryEngineEventReceived, FStoryEventContext, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStoryRuleExecuted, FName, RuleId, FStoryEventContext, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStoryQuestTaskChanged, FStoryQuestTaskData, TaskData);

UCLASS()
class DEVKIT_API UStoryEngineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetRuleSets(const TArray<UStoryRuleSetDA*>& InRuleSets);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void AddRuleSet(UStoryRuleSetDA* RuleSet);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void BroadcastStoryEvent(FGameplayTag EventTag, APlayerController* PlayerController = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void BroadcastStoryEventWithContext(FStoryEventContext Context);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void BroadcastStoryEventWithPayload(FGameplayTag EventTag, FGameplayTag ContextTag, FGameplayTag AreaTag,
		FGameplayTag ItemTag, AActor* SourceActor = nullptr, APlayerController* PlayerController = nullptr);

	UFUNCTION(BlueprintPure, Category = "Story")
	bool EvaluateStoryCondition(const FStoryCondition& Condition, const FStoryEventContext& Context) const;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void ExecuteStoryAction(const FStoryAction& Action, FStoryEventContext Context);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void ExecuteStoryActions(const TArray<FStoryAction>& Actions, FStoryEventContext Context);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void ResetRunState();

	UFUNCTION(BlueprintCallable, Category = "Story")
	void ResetSessionState();

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetStoryFlag(FGameplayTag FlagTag, EStoryFlagScope Scope, bool bValue);

	UFUNCTION(BlueprintPure, Category = "Story")
	bool HasStoryFlag(FGameplayTag FlagTag, EStoryFlagScope Scope) const;

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetQuestTask(FGameplayTag TaskId, FText DisplayText, FGameplayTag SourceTag, FGameplayTag RelatedFlagTag);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void CompleteQuestTask(FGameplayTag TaskId);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void SetQuestTaskState(FGameplayTag TaskId, EStoryQuestTaskState NewState);

	UFUNCTION(BlueprintPure, Category = "Story")
	bool GetQuestTask(FGameplayTag TaskId, FStoryQuestTaskData& OutTask) const;

	UFUNCTION(BlueprintPure, Category = "Story")
	TArray<FStoryQuestTaskData> GetAllQuestTasks() const;

	UFUNCTION(BlueprintPure, Category = "Story")
	TArray<FStoryQuestTaskData> GetQuestTasksByState(EStoryQuestTaskState State) const;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryEngineEventReceived OnStoryEventReceived;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryRuleExecuted OnStoryRuleExecuted;

	UPROPERTY(BlueprintAssignable, Category = "Story")
	FOnStoryQuestTaskChanged OnQuestTaskChanged;

private:
	UPROPERTY()
	TArray<TObjectPtr<UStoryRuleSetDA>> RuleSets;

	UPROPERTY()
	TSet<FGameplayTag> RunFlags;

	UPROPERTY()
	TSet<FGameplayTag> SessionFlags;

	UPROPERTY()
	TSet<FName> FiredRunRuleIds;

	UPROPERTY()
	TSet<FName> FiredSessionRuleIds;

	UPROPERTY()
	TSet<FName> FiredMapRuleIds;

	UPROPERTY()
	TMap<FGameplayTag, FStoryQuestTaskData> TransientQuestTasks;

	UPROPERTY()
	TArray<TObjectPtr<ULevelInfoPopupDA>> TransientInfoPopups;

	TArray<FStoryEventContext> PendingEvents;
	bool bIsProcessingEvents = false;

	void LoadConfiguredRuleSets();
	void ProcessStoryEvent(const FStoryEventContext& Context);
	void CollectRulesForEvent(FGameplayTag EventTag, TArray<const FStoryRule*>& OutRules) const;
	bool EvaluateRule(const FStoryRule& Rule, const FStoryEventContext& Context) const;
	bool EvaluateCondition(const FStoryCondition& Condition, const FStoryEventContext& Context) const;
	void DispatchActions(const FStoryRule& Rule, const FStoryEventContext& Context);
	void DispatchAction(const FStoryAction& Action, const FStoryEventContext& Context);

	bool ShouldSkipForFirePolicy(const FStoryRule& Rule, const FStoryEventContext& Context) const;
	void MarkRuleFired(const FStoryRule& Rule, const FStoryEventContext& Context);
	FName MakeRuleFireKey(const FStoryRule& Rule, const FStoryEventContext& Context) const;

	UYogSaveGame* GetCurrentSave() const;
	APlayerController* ResolvePlayerController(const FStoryEventContext& Context) const;
	void CommitSave() const;

	static constexpr int32 MaxQueuedEvents = 64;
};
