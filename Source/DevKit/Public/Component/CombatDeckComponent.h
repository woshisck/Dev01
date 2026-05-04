#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "Data/WeaponComboConfigDA.h"
#include "CombatDeckComponent.generated.h"

class UBuffFlowComponent;
class UWeaponDefinition;

UENUM(BlueprintType)
enum class ECombatLinkBreakReason : uint8
{
	None            UMETA(DisplayName = "None"),
	ComboStateExited UMETA(DisplayName = "Combo State Exited"),
	ShuffleStarted UMETA(DisplayName = "Shuffle Started"),
	WeaponChanged  UMETA(DisplayName = "Weapon Changed"),
	ConditionFailed UMETA(DisplayName = "Condition Failed"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatDeckActionContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	ECardRequiredAction ActionType = ECardRequiredAction::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	int32 ComboIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	FName ComboNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	FGameplayTagContainer ComboTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	TObjectPtr<UWeaponDefinition> WeaponDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	bool bComboContinued = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	bool bExitedComboState = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	bool bFromDashSave = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	ECombatCardTriggerTiming TriggerTiming = ECombatCardTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck")
	FGuid AttackInstanceGuid;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Combat Deck")
	FGuid InstanceGuid;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Combat Deck")
	TObjectPtr<URuneDataAsset> SourceData = nullptr;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Combat Deck")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Combat Deck")
	FName OwnerSource = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardConfig Config;

	UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Combat Deck")
	ECombatCardLinkOrientation LinkOrientation = ECombatCardLinkOrientation::Forward;

	bool IsValidCard() const
	{
		return Config.bIsCombatCard;
	}
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bHadCard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bActionMatched = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredBaseFlow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredMatchedFlow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredLink = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredForwardLink = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bPendingBackwardLink = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredBackwardLink = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bLinkBroken = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bTriggeredFinisher = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bStartedShuffle = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardInstance ConsumedCard;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	ECombatLinkBreakReason LinkBreakReason = ECombatLinkBreakReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardInstance LinkedSourceCard;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardInstance LinkedTargetCard;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	float AppliedMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FText ReasonText;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardEffectContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatDeckActionContext ActionContext;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardInstance SourceCard;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardResolveResult ResolveResult;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	int32 ComboIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FName ComboNodeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FGameplayTagContainer ComboTags;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	float EffectMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	int32 ComboBonusStacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bFromLink = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bIsComboFinisher = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckLoaded, const TArray<FCombatCardInstance>&, ActiveSequence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCardConsumed, const FCombatCardInstance&, Card, const FCombatCardResolveResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatCardResult, const FCombatCardResolveResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShuffleProgress, float, NormalizedProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardAddedToDeck, const FCombatCardInstance&, Card);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UCombatDeckComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatDeckComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	FCombatCardResolveResult ResolveAttackCard(ECardRequiredAction ActionType, bool bIsComboFinisher, bool bFromDashSave);

	FCombatCardResolveResult ResolveAttackCard(const FCombatDeckActionContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	FCombatCardResolveResult ResolveAttackCardWithContext(const FCombatDeckActionContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void StopCardFlow(const FCombatCardInstance& Card);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void NotifyComboStateExited();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void LoadDeckFromWeapon(const UWeaponDefinition* WeaponDefinition);

	void LoadDeckFromSourceAssets(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize);

	TArray<URuneDataAsset*> GetDeckSourceAssets() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	bool AddCardFromRuneReward(URuneDataAsset* RuneAsset);

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	TArray<FCombatCardInstance> GetDeckSnapshot() const { return ActiveSequence; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	TArray<FCombatCardInstance> GetFullDeckSnapshot() const { return DeckList; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	TArray<FCombatCardInstance> GetRemainingDeckSnapshot() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool MoveCardInDeck(int32 FromIndex, int32 InsertIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool SetCardLinkOrientationByIndex(int32 CardIndex, ECombatCardLinkOrientation Orientation);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool ToggleCardLinkOrientationByIndex(int32 CardIndex);

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	int32 GetCurrentIndex() const { return CurrentIndex; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	int32 GetRemainingCardCount() const;

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	EDeckState GetDeckState() const { return DeckState; }

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void SetShuffleCooldownDuration(float InDuration);

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	float GetShuffleCooldownDuration() const { return ShuffleCooldownDuration; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	int32 GetMaxActiveSequenceSize() const { return MaxActiveSequenceSize; }

	void SetDeckListForTest(const TArray<FCombatCardConfig>& InCards);
	void AdvanceShuffleForTest(float DeltaTime);
	void SavePendingLinkContextForDash();
	bool RestorePendingLinkContextFromDash();
	void ClearDashSavedLinkContext();

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnDeckLoaded OnDeckLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCardConsumed OnCardConsumed;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnActionMatched;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnCardReleased;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnLinkTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnForwardLinkTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnBackwardLinkPending;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnBackwardLinkTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnLinkBroken;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnFinisherTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnShuffleStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnShuffleProgress OnShuffleProgress;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnDeckLoaded OnShuffleCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnRewardAddedToDeck OnRewardAddedToDeck;

private:
	UPROPERTY(SaveGame)
	TArray<FCombatCardInstance> DeckList;

	UPROPERTY()
	TArray<FCombatCardInstance> ActiveSequence;

	UPROPERTY(SaveGame)
	int32 CurrentIndex = 0;

	UPROPERTY(SaveGame)
	EDeckState DeckState = EDeckState::Ready;

	UPROPERTY(SaveGame)
	float ShuffleCooldownRemaining = 0.0f;

	UPROPERTY(SaveGame)
	float ShuffleCooldownDuration = 1.0f;

	UPROPERTY(SaveGame)
	int32 MaxActiveSequenceSize = 0;

	UPROPERTY()
	FCombatCardInstance LastResolvedCard;

	UPROPERTY()
	FCombatCardInstance PendingLinkContext;

	UPROPERTY()
	FCombatDeckActionContext PendingLinkActionContext;

	UPROPERTY()
	FCombatCardInstance DashSavedLinkContext;

	UPROPERTY()
	FCombatDeckActionContext DashSavedLinkActionContext;

	TSet<FGuid> ResolvedAttackGuids;

	FCombatCardInstance MakeCardFromRune(URuneDataAsset* RuneAsset, FName OwnerSource) const;
	void RefillActiveSequence();
	void ResetRuntimeStateAfterDeckEdit();
	void StartDeckEditReload();
	void StartShuffle();
	void AdvanceShuffle(float DeltaTime);
	void ExecuteFlow(UFlowAsset* FlowAsset, const FCombatCardInstance& Card, const FCombatDeckActionContext& Context, const FCombatCardResolveResult& Result) const;
	bool DoesActionMatch(ECardRequiredAction RequiredAction, ECardRequiredAction ActionType) const;
	void BreakPendingLink(ECombatLinkBreakReason Reason, const FCombatDeckActionContext* BreakContext = nullptr);
	int32 GetComboBonusStacks(const FCombatDeckActionContext& Context) const;
	float GetComboEffectMultiplier(const FCombatCardConfig& Config, const FCombatDeckActionContext& Context) const;
	void SetAppliedMultiplier(FCombatCardResolveResult& Result, float LinkMultiplier, float ComboMultiplier) const;
	FCombatCardEffectContext BuildCombatCardEffectContext(const FCombatCardInstance& Card, const FCombatDeckActionContext& Context, const FCombatCardResolveResult& Result) const;
	bool DoesLinkConditionMatch(const FCombatCardLinkCondition& Condition, const FCombatCardInstance& NeighborCard, const FCombatDeckActionContext& Context) const;
	const FCombatCardLinkRecipe* FindMatchingLinkRecipe(const FCombatCardInstance& LinkCard, ECombatCardLinkOrientation Direction, const FCombatCardInstance& NeighborCard, const FCombatDeckActionContext& Context) const;
	bool IsLinkCardType(ECombatCardType CardType) const;
	bool IsCardTypeCompatible(ECombatCardType RequiredType, ECombatCardType ActualType) const;
	bool WantsForwardLink(const FCombatCardConfig& Config) const;
	bool WantsBackwardLink(const FCombatCardConfig& Config) const;
};
