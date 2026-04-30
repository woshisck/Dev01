#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "CombatDeckComponent.generated.h"

class UBuffFlowComponent;
class UWeaponDefinition;

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
	bool bTriggeredFinisher = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	bool bStartedShuffle = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Deck")
	FCombatCardInstance ConsumedCard;
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

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void LoadDeckFromWeapon(const UWeaponDefinition* WeaponDefinition);

	void LoadDeckFromSourceAssets(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize);

	TArray<URuneDataAsset*> GetDeckSourceAssets() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	bool AddCardFromRuneReward(URuneDataAsset* RuneAsset);

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	TArray<FCombatCardInstance> GetDeckSnapshot() const { return ActiveSequence; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	TArray<FCombatCardInstance> GetRemainingDeckSnapshot() const;

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

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnDeckLoaded OnDeckLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCardConsumed OnCardConsumed;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnActionMatched;

	UPROPERTY(BlueprintAssignable, Category = "Combat Deck|Events")
	FOnCombatCardResult OnLinkTriggered;

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

	FCombatCardInstance MakeCardFromRune(URuneDataAsset* RuneAsset, FName OwnerSource) const;
	void RefillActiveSequence();
	void StartShuffle();
	void AdvanceShuffle(float DeltaTime);
	void ExecuteFlow(UFlowAsset* FlowAsset, const FCombatCardInstance& Card) const;
	bool DoesActionMatch(ECardRequiredAction RequiredAction, ECardRequiredAction ActionType) const;
};
