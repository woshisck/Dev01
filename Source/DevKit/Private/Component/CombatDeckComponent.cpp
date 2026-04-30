#include "Component/CombatDeckComponent.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "FlowAsset.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	const FName CombatDeckOwnerSourceWeapon(TEXT("Weapon"));
	const FName CombatDeckOwnerSourceReward(TEXT("Reward"));
}

UCombatDeckComponent::UCombatDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	RefillActiveSequence();
}

void UCombatDeckComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceShuffle(DeltaTime);
}

FCombatCardResolveResult UCombatDeckComponent::ResolveAttackCard(ECardRequiredAction ActionType, bool bIsComboFinisher, bool bFromDashSave)
{
	FCombatCardResolveResult Result;

	if (DeckState == EDeckState::EmptyShuffling || !ActiveSequence.IsValidIndex(CurrentIndex))
	{
		return Result;
	}

	FCombatCardInstance Card = ActiveSequence[CurrentIndex];
	Result.bHadCard = true;
	Result.ConsumedCard = Card;
	Result.bActionMatched = DoesActionMatch(Card.Config.RequiredAction, ActionType);
	Result.bTriggeredBaseFlow = true;

	ExecuteFlow(Card.Config.BaseFlow, Card);

	if (Result.bActionMatched)
	{
		Result.bTriggeredMatchedFlow = true;
		ExecuteFlow(Card.Config.MatchedFlow, Card);

		if (Card.Config.LinkMode != ECardLinkMode::None)
		{
			Result.bTriggeredLink = true;
			if (Card.Config.LinkMode == ECardLinkMode::PassToNext)
			{
				PendingLinkContext = Card;
			}
		}

		const bool bFinisherAllowed = !Card.Config.bRequiresComboFinisher || bIsComboFinisher;
		Result.bTriggeredFinisher = Card.Config.CardType == ECombatCardType::Finisher && bFinisherAllowed;
	}

	LastResolvedCard = Card;
	CurrentIndex++;
	Result.bStartedShuffle = CurrentIndex >= ActiveSequence.Num();

	OnCardConsumed.Broadcast(Card, Result);
	if (Result.bActionMatched)
	{
		OnActionMatched.Broadcast(Result);
	}
	if (Result.bTriggeredLink)
	{
		OnLinkTriggered.Broadcast(Result);
	}
	if (Result.bTriggeredFinisher)
	{
		OnFinisherTriggered.Broadcast(Result);
	}

	if (Result.bStartedShuffle)
	{
		StartShuffle();
		OnShuffleStarted.Broadcast(Result);
	}

	return Result;
}

void UCombatDeckComponent::LoadDeckFromWeapon(const UWeaponDefinition* WeaponDefinition)
{
	if (WeaponDefinition)
	{
		const TArray<TObjectPtr<URuneDataAsset>>& SourceCards = WeaponDefinition->InitialCombatDeck.IsEmpty()
			? WeaponDefinition->InitialRunes
			: WeaponDefinition->InitialCombatDeck;

		TArray<URuneDataAsset*> SourceAssets;
		SourceAssets.Reserve(SourceCards.Num());
		for (URuneDataAsset* SourceCard : SourceCards)
		{
			SourceAssets.Add(SourceCard);
		}

		LoadDeckFromSourceAssets(SourceAssets, WeaponDefinition->ShuffleCooldownDuration, WeaponDefinition->MaxActiveSequenceSize);
		return;
	}

	LoadDeckFromSourceAssets({}, ShuffleCooldownDuration, MaxActiveSequenceSize);
}

void UCombatDeckComponent::LoadDeckFromSourceAssets(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize)
{
	DeckList.Reset();
	ShuffleCooldownDuration = FMath::Max(0.0f, InShuffleCooldownDuration);
	MaxActiveSequenceSize = FMath::Max(0, InMaxActiveSequenceSize);

	for (URuneDataAsset* RuneAsset : SourceAssets)
	{
		const FCombatCardInstance Card = MakeCardFromRune(RuneAsset, CombatDeckOwnerSourceWeapon);
		if (Card.IsValidCard())
		{
			DeckList.Add(Card);
		}
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	RefillActiveSequence();
}

TArray<URuneDataAsset*> UCombatDeckComponent::GetDeckSourceAssets() const
{
	TArray<URuneDataAsset*> SourceAssets;
	SourceAssets.Reserve(DeckList.Num());
	for (const FCombatCardInstance& Card : DeckList)
	{
		if (Card.SourceData)
		{
			SourceAssets.Add(Card.SourceData);
		}
	}
	return SourceAssets;
}

TArray<FCombatCardInstance> UCombatDeckComponent::GetRemainingDeckSnapshot() const
{
	TArray<FCombatCardInstance> RemainingCards;
	if (DeckState != EDeckState::Ready)
	{
		return RemainingCards;
	}

	for (int32 Index = CurrentIndex; Index < ActiveSequence.Num(); ++Index)
	{
		RemainingCards.Add(ActiveSequence[Index]);
	}

	return RemainingCards;
}

int32 UCombatDeckComponent::GetRemainingCardCount() const
{
	return GetRemainingDeckSnapshot().Num();
}

bool UCombatDeckComponent::AddCardFromRuneReward(URuneDataAsset* RuneAsset)
{
	const FCombatCardInstance Card = MakeCardFromRune(RuneAsset, CombatDeckOwnerSourceReward);
	if (!Card.IsValidCard())
	{
		return false;
	}

	DeckList.Add(Card);
	OnRewardAddedToDeck.Broadcast(Card);

	if (DeckState == EDeckState::Ready && ActiveSequence.IsEmpty())
	{
		RefillActiveSequence();
	}

	return true;
}

void UCombatDeckComponent::SetShuffleCooldownDuration(float InDuration)
{
	ShuffleCooldownDuration = FMath::Max(0.0f, InDuration);
}

void UCombatDeckComponent::SetDeckListForTest(const TArray<FCombatCardConfig>& InCards)
{
	DeckList.Reset();
	for (const FCombatCardConfig& Config : InCards)
	{
		FCombatCardInstance Card;
		Card.InstanceGuid = FGuid::NewGuid();
		Card.Config = Config;
		DeckList.Add(Card);
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	RefillActiveSequence();
}

void UCombatDeckComponent::AdvanceShuffleForTest(float DeltaTime)
{
	AdvanceShuffle(DeltaTime);
}

FCombatCardInstance UCombatDeckComponent::MakeCardFromRune(URuneDataAsset* RuneAsset, FName OwnerSource) const
{
	FCombatCardInstance Card;
	if (!RuneAsset || !RuneAsset->RuneInfo.CombatCard.bIsCombatCard)
	{
		return Card;
	}

	Card.InstanceGuid = FGuid::NewGuid();
	Card.SourceData = RuneAsset;
	Card.Level = RuneAsset->RuneInfo.Level;
	Card.OwnerSource = OwnerSource;
	Card.Config = RuneAsset->RuneInfo.CombatCard;

	if (Card.Config.DisplayName.IsEmpty())
	{
		Card.Config.DisplayName = FText::FromName(RuneAsset->RuneInfo.RuneConfig.RuneName);
	}

	return Card;
}

void UCombatDeckComponent::RefillActiveSequence()
{
	ActiveSequence.Reset();
	CurrentIndex = 0;

	const int32 DesiredCount = MaxActiveSequenceSize > 0
		? FMath::Min(MaxActiveSequenceSize, DeckList.Num())
		: DeckList.Num();

	for (int32 i = 0; i < DesiredCount; ++i)
	{
		ActiveSequence.Add(DeckList[i]);
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	OnDeckLoaded.Broadcast(ActiveSequence);
}

void UCombatDeckComponent::StartShuffle()
{
	DeckState = EDeckState::EmptyShuffling;
	ShuffleCooldownRemaining = ShuffleCooldownDuration;
	CurrentIndex = 0;
	ActiveSequence.Reset();
	PendingLinkContext = FCombatCardInstance();

	if (ShuffleCooldownDuration <= KINDA_SMALL_NUMBER)
	{
		RefillActiveSequence();
	}
}

void UCombatDeckComponent::AdvanceShuffle(float DeltaTime)
{
	if (DeckState != EDeckState::EmptyShuffling)
	{
		return;
	}

	ShuffleCooldownRemaining = FMath::Max(0.0f, ShuffleCooldownRemaining - FMath::Max(0.0f, DeltaTime));

	const float Progress = ShuffleCooldownDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: 1.0f - (ShuffleCooldownRemaining / ShuffleCooldownDuration);
	OnShuffleProgress.Broadcast(FMath::Clamp(Progress, 0.0f, 1.0f));

	if (ShuffleCooldownRemaining <= KINDA_SMALL_NUMBER)
	{
		RefillActiveSequence();
		OnShuffleCompleted.Broadcast(ActiveSequence);
	}
}

void UCombatDeckComponent::ExecuteFlow(UFlowAsset* FlowAsset, const FCombatCardInstance& Card) const
{
	if (!FlowAsset)
	{
		return;
	}

	if (UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr)
	{
		BuffFlowComponent->StartBuffFlow(FlowAsset, Card.InstanceGuid, GetOwner(), true);
	}
}

bool UCombatDeckComponent::DoesActionMatch(ECardRequiredAction RequiredAction, ECardRequiredAction ActionType) const
{
	return RequiredAction == ECardRequiredAction::Any || RequiredAction == ActionType;
}
