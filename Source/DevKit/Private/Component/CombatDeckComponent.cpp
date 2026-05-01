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
	FCombatDeckActionContext Context;
	Context.ActionType = ActionType;
	Context.bIsComboFinisher = bIsComboFinisher;
	Context.bFromDashSave = bFromDashSave;
	Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
	Context.AttackInstanceGuid = FGuid::NewGuid();
	return ResolveAttackCardWithContext(Context);
}

FCombatCardResolveResult UCombatDeckComponent::ResolveAttackCard(const FCombatDeckActionContext& Context)
{
	return ResolveAttackCardWithContext(Context);
}

FCombatCardResolveResult UCombatDeckComponent::ResolveAttackCardWithContext(const FCombatDeckActionContext& Context)
{
	FCombatCardResolveResult Result;

	if (Context.bExitedComboState)
	{
		BreakPendingLink(ECombatLinkBreakReason::ComboStateExited);
	}

	if (DeckState == EDeckState::EmptyShuffling || !ActiveSequence.IsValidIndex(CurrentIndex))
	{
		return Result;
	}

	if (Context.AttackInstanceGuid.IsValid() && ResolvedAttackGuids.Contains(Context.AttackInstanceGuid))
	{
		return Result;
	}

	FCombatCardInstance Card = ActiveSequence[CurrentIndex];
	const bool bAllowOnCommitCardAtHitNotify =
		Context.TriggerTiming == ECombatCardTriggerTiming::OnHit
		&& Card.Config.TriggerTiming == ECombatCardTriggerTiming::OnCommit;
	if (Card.Config.TriggerTiming != Context.TriggerTiming && !bAllowOnCommitCardAtHitNotify)
	{
		return Result;
	}

	if (PendingLinkContext.IsValidCard())
	{
		const FCombatCardLinkRecipe* ReversedRecipe = FindMatchingLinkRecipe(PendingLinkContext, ECombatCardLinkOrientation::Reversed, Card, Context);
		const bool bLegacyBackwardMatched = !ReversedRecipe && PendingLinkContext.Config.LinkRecipes.IsEmpty()
			&& DoesLinkConditionMatch(PendingLinkContext.Config.LinkConfig.BackwardEffect.Condition, Card, Context);
		if (ReversedRecipe || bLegacyBackwardMatched)
		{
			Result.bTriggeredLink = true;
			Result.bTriggeredBackwardLink = true;
			Result.LinkedSourceCard = PendingLinkContext;
			Result.LinkedTargetCard = Card;
			Result.AppliedMultiplier = ReversedRecipe ? ReversedRecipe->Multiplier : PendingLinkContext.Config.LinkConfig.BackwardEffect.Multiplier;
			const FText LinkReasonText = ReversedRecipe ? ReversedRecipe->ReasonText : PendingLinkContext.Config.LinkConfig.BackwardEffect.ReasonText;
			Result.ReasonText = LinkReasonText.IsEmpty()
				? PendingLinkContext.Config.HUDReasonText
				: LinkReasonText;
			ExecuteFlow(ReversedRecipe ? ReversedRecipe->LinkFlow : PendingLinkContext.Config.LinkConfig.BackwardEffect.Flow, PendingLinkContext);
			PendingLinkContext = FCombatCardInstance();
		}
		else
		{
			BreakPendingLink(ECombatLinkBreakReason::ConditionFailed);
		}
	}

	Result.bHadCard = true;
	Result.ConsumedCard = Card;
	Result.bActionMatched = !IsLinkCardType(Card.Config.CardType)
		? true
		: DoesActionMatch(Card.Config.RequiredAction, Context.ActionType);
	if (!Result.bTriggeredBackwardLink)
	{
		Result.AppliedMultiplier = 1.0f;
	}
	Result.ReasonText = Card.Config.HUDReasonText;

	const bool bComboRequirementSatisfied = !Card.Config.bRequiresComboFinisher || Context.bIsComboFinisher;
	if (Result.bActionMatched && bComboRequirementSatisfied)
	{
		const bool bCanUseRecipeLinks = IsLinkCardType(Card.Config.CardType) && !Card.Config.LinkRecipes.IsEmpty();
		const bool bCanUseLinkConfig = IsLinkCardType(Card.Config.CardType)
			&& Card.Config.LinkRecipes.IsEmpty()
			&& Card.Config.LinkConfig.Direction != ECombatCardLinkDirection::None;

		if (bCanUseRecipeLinks)
		{
			const FCombatCardLinkRecipe* ForwardRecipe = Card.LinkOrientation == ECombatCardLinkOrientation::Forward
				? FindMatchingLinkRecipe(Card, ECombatCardLinkOrientation::Forward, LastResolvedCard, Context)
				: nullptr;

			if (ForwardRecipe)
			{
				Result.bTriggeredMatchedFlow = true;
				Result.bTriggeredLink = true;
				Result.bTriggeredForwardLink = true;
				Result.LinkedSourceCard = LastResolvedCard;
				Result.LinkedTargetCard = Card;
				Result.AppliedMultiplier = ForwardRecipe->Multiplier;
				Result.ReasonText = ForwardRecipe->ReasonText.IsEmpty()
					? Card.Config.HUDReasonText
					: ForwardRecipe->ReasonText;
				ExecuteFlow(ForwardRecipe->LinkFlow, Card);
			}
			else
			{
				Result.bTriggeredBaseFlow = true;
				ExecuteFlow(Card.Config.BaseFlow, Card);
			}

			const bool bHasReversedRecipe = Card.LinkOrientation == ECombatCardLinkOrientation::Reversed
				&& Card.Config.LinkRecipes.ContainsByPredicate([](const FCombatCardLinkRecipe& Recipe)
				{
					return Recipe.Direction == ECombatCardLinkOrientation::Reversed && Recipe.LinkFlow;
				});
			if (bHasReversedRecipe)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
			}
		}
		else if (bCanUseLinkConfig)
		{
			const bool bForwardMatched = WantsForwardLink(Card.Config)
				&& Card.Config.LinkConfig.ForwardEffect.Flow
				&& LastResolvedCard.IsValidCard()
				&& DoesLinkConditionMatch(Card.Config.LinkConfig.ForwardEffect.Condition, LastResolvedCard, Context);

			if (bForwardMatched)
			{
				Result.bTriggeredMatchedFlow = true;
				Result.bTriggeredLink = true;
				Result.bTriggeredForwardLink = true;
				Result.LinkedSourceCard = LastResolvedCard;
				Result.LinkedTargetCard = Card;
				Result.AppliedMultiplier = Card.Config.LinkConfig.ForwardEffect.Multiplier;
				Result.ReasonText = Card.Config.LinkConfig.ForwardEffect.ReasonText.IsEmpty()
					? Card.Config.HUDReasonText
					: Card.Config.LinkConfig.ForwardEffect.ReasonText;
				ExecuteFlow(Card.Config.LinkConfig.ForwardEffect.Flow, Card);
			}
			else
			{
				Result.bTriggeredBaseFlow = true;
				ExecuteFlow(Card.Config.BaseFlow, Card);
			}

			if (!bForwardMatched && WantsBackwardLink(Card.Config) && Card.Config.LinkConfig.BackwardEffect.Flow)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
			}
		}
		else
		{
			Result.bTriggeredBaseFlow = true;
			ExecuteFlow(Card.Config.BaseFlow, Card);

			Result.bTriggeredMatchedFlow = true;
			ExecuteFlow(Card.Config.MatchedFlow, Card);

			if (Card.Config.LinkMode == ECardLinkMode::PassToNext)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
			}
			else if (Card.Config.LinkMode == ECardLinkMode::ReadPrevious)
			{
				Result.bTriggeredLink = LastResolvedCard.IsValidCard() || PendingLinkContext.IsValidCard();
				Result.bTriggeredForwardLink = Result.bTriggeredLink;
				Result.LinkedSourceCard = LastResolvedCard;
				Result.LinkedTargetCard = Card;
				PendingLinkContext = FCombatCardInstance();
			}
		}
	}
	else
	{
		Result.bTriggeredBaseFlow = true;
		ExecuteFlow(Card.Config.BaseFlow, Card);
	}

	Result.bTriggeredFinisher = Result.bActionMatched
		&& bComboRequirementSatisfied
		&& Card.Config.CardType == ECombatCardType::Finisher;

	LastResolvedCard = Card;
	if (Context.AttackInstanceGuid.IsValid())
	{
		ResolvedAttackGuids.Add(Context.AttackInstanceGuid);
	}

	CurrentIndex++;
	Result.bStartedShuffle = CurrentIndex >= ActiveSequence.Num();

	OnCardConsumed.Broadcast(Card, Result);
	if (Result.bTriggeredBaseFlow)
	{
		OnCardReleased.Broadcast(Result);
	}
	if (Result.bActionMatched)
	{
		OnActionMatched.Broadcast(Result);
	}
	if (Result.bTriggeredLink)
	{
		OnLinkTriggered.Broadcast(Result);
	}
	if (Result.bTriggeredForwardLink)
	{
		OnForwardLinkTriggered.Broadcast(Result);
	}
	if (Result.bPendingBackwardLink)
	{
		OnBackwardLinkPending.Broadcast(Result);
	}
	if (Result.bTriggeredBackwardLink)
	{
		OnBackwardLinkTriggered.Broadcast(Result);
	}
	if (Result.bLinkBroken)
	{
		OnLinkBroken.Broadcast(Result);
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

void UCombatDeckComponent::StopCardFlow(const FCombatCardInstance& Card)
{
	if (!Card.IsValidCard())
	{
		return;
	}

	if (UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr)
	{
		BuffFlowComponent->StopBuffFlow(Card.InstanceGuid);
	}
}

void UCombatDeckComponent::NotifyComboStateExited()
{
	BreakPendingLink(ECombatLinkBreakReason::ComboStateExited);
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
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	DashSavedLinkContext = FCombatCardInstance();
	ResolvedAttackGuids.Reset();
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
		Card.LinkOrientation = Config.DefaultLinkOrientation;
		DeckList.Add(Card);
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	DashSavedLinkContext = FCombatCardInstance();
	ResolvedAttackGuids.Reset();
	RefillActiveSequence();
}

void UCombatDeckComponent::AdvanceShuffleForTest(float DeltaTime)
{
	AdvanceShuffle(DeltaTime);
}

void UCombatDeckComponent::SavePendingLinkContextForDash()
{
	DashSavedLinkContext = PendingLinkContext;
}

bool UCombatDeckComponent::RestorePendingLinkContextFromDash()
{
	if (!DashSavedLinkContext.IsValidCard())
	{
		return false;
	}

	PendingLinkContext = DashSavedLinkContext;
	DashSavedLinkContext = FCombatCardInstance();
	return true;
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
	Card.LinkOrientation = Card.Config.DefaultLinkOrientation;

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
	BreakPendingLink(ECombatLinkBreakReason::ShuffleStarted);
	DeckState = EDeckState::EmptyShuffling;
	ShuffleCooldownRemaining = ShuffleCooldownDuration;
	CurrentIndex = 0;
	ActiveSequence.Reset();
	PendingLinkContext = FCombatCardInstance();
	ResolvedAttackGuids.Reset();

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

void UCombatDeckComponent::BreakPendingLink(ECombatLinkBreakReason Reason)
{
	if (!PendingLinkContext.IsValidCard())
	{
		return;
	}

	FCombatCardResolveResult Result;
	Result.bLinkBroken = true;
	Result.LinkBreakReason = Reason;
	Result.LinkedSourceCard = PendingLinkContext;
	Result.ReasonText = PendingLinkContext.Config.HUDReasonText;
	PendingLinkContext = FCombatCardInstance();
	OnLinkBroken.Broadcast(Result);
}

bool UCombatDeckComponent::DoesActionMatch(ECardRequiredAction RequiredAction, ECardRequiredAction ActionType) const
{
	return RequiredAction == ECardRequiredAction::Any || RequiredAction == ActionType;
}

bool UCombatDeckComponent::DoesLinkConditionMatch(const FCombatCardLinkCondition& Condition, const FCombatCardInstance& NeighborCard, const FCombatDeckActionContext& Context) const
{
	if (!NeighborCard.IsValidCard())
	{
		return false;
	}

	if (IsLinkCardType(NeighborCard.Config.CardType))
	{
		return false;
	}

	if (Condition.RequiredNeighborTypes.IsEmpty()
		&& Condition.RequiredNeighborTags.IsEmpty()
		&& Condition.RequiredNeighborIdTags.IsEmpty()
		&& Condition.RequiredNeighborEffectTags.IsEmpty()
		&& IsLinkCardType(NeighborCard.Config.CardType))
	{
		return false;
	}

	if (!DoesActionMatch(Condition.RequiredAction, Context.ActionType))
	{
		return false;
	}

	if (!Condition.RequiredNeighborTypes.IsEmpty())
	{
		bool bTypeMatched = false;
		for (const ECombatCardType RequiredType : Condition.RequiredNeighborTypes)
		{
			if (IsCardTypeCompatible(RequiredType, NeighborCard.Config.CardType))
			{
				bTypeMatched = true;
				break;
			}
		}
		if (!bTypeMatched)
		{
			return false;
		}
	}

	if (!Condition.RequiredNeighborTags.IsEmpty() && !NeighborCard.Config.CardTags.HasAll(Condition.RequiredNeighborTags))
	{
		return false;
	}

	if (!Condition.RequiredNeighborIdTags.IsEmpty())
	{
		FGameplayTagContainer NeighborIdTags;
		if (NeighborCard.Config.CardIdTag.IsValid())
		{
			NeighborIdTags.AddTag(NeighborCard.Config.CardIdTag);
		}
		if (!NeighborIdTags.HasAll(Condition.RequiredNeighborIdTags))
		{
			return false;
		}
	}

	if (!Condition.RequiredNeighborEffectTags.IsEmpty() && !NeighborCard.Config.CardEffectTags.HasAll(Condition.RequiredNeighborEffectTags))
	{
		return false;
	}

	if (!Condition.RequiredComboTags.IsEmpty() && !Context.ComboTags.HasAll(Condition.RequiredComboTags))
	{
		return false;
	}

	return true;
}

const FCombatCardLinkRecipe* UCombatDeckComponent::FindMatchingLinkRecipe(const FCombatCardInstance& LinkCard, ECombatCardLinkOrientation Direction, const FCombatCardInstance& NeighborCard, const FCombatDeckActionContext& Context) const
{
	if (!LinkCard.IsValidCard() || !IsLinkCardType(LinkCard.Config.CardType) || !NeighborCard.IsValidCard())
	{
		return nullptr;
	}

	for (const FCombatCardLinkRecipe& Recipe : LinkCard.Config.LinkRecipes)
	{
		if (Recipe.Direction == Direction
			&& Recipe.LinkFlow
			&& DoesLinkConditionMatch(Recipe.Condition, NeighborCard, Context))
		{
			return &Recipe;
		}
	}

	return nullptr;
}

bool UCombatDeckComponent::IsLinkCardType(ECombatCardType CardType) const
{
	return CardType == ECombatCardType::Link;
}

bool UCombatDeckComponent::IsCardTypeCompatible(ECombatCardType RequiredType, ECombatCardType ActualType) const
{
	if (RequiredType == ActualType)
	{
		return true;
	}

	const bool bRequiredNormal = RequiredType == ECombatCardType::Normal || RequiredType == ECombatCardType::Attack;
	const bool bActualNormal = ActualType == ECombatCardType::Normal || ActualType == ECombatCardType::Attack;
	return bRequiredNormal && bActualNormal;
}

bool UCombatDeckComponent::WantsForwardLink(const FCombatCardConfig& Config) const
{
	return Config.LinkConfig.Direction == ECombatCardLinkDirection::ForwardReadPrevious
		|| Config.LinkConfig.Direction == ECombatCardLinkDirection::Both;
}

bool UCombatDeckComponent::WantsBackwardLink(const FCombatCardConfig& Config) const
{
	return Config.LinkConfig.Direction == ECombatCardLinkDirection::BackwardEmpowerNext
		|| Config.LinkConfig.Direction == ECombatCardLinkDirection::Both;
}
