#include "Component/CombatDeckComponent.h"

#include "UI/CombatLogStatics.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "FlowAsset.h"
#include "GameModes/YogGameMode.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName CombatDeckOwnerSourceWeapon(TEXT("Weapon"));
	const FName CombatDeckOwnerSourceReward(TEXT("Reward"));
	const FName CombatDeckOwnerSourceShop(TEXT("Shop"));

	float GetProjectileEventFlowStopDelay(const UFlowAsset* FlowAsset)
	{
		if (!FlowAsset)
		{
			return 0.f;
		}

		bool bHasWaitEventNode = false;
		float MaxProjectileLifetime = 0.f;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (const UBFNode_WaitGameplayEvent* WaitNode = Cast<UBFNode_WaitGameplayEvent>(Pair.Value))
			{
				bHasWaitEventNode = bHasWaitEventNode || WaitNode->EventTag.IsValid();
			}

			if (const UBFNode_SpawnSlashWaveProjectile* ProjectileNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value))
			{
				if (!ProjectileNode->HitGameplayEventTag.IsValid() && !ProjectileNode->ExpireGameplayEventTag.IsValid())
				{
					continue;
				}

				const float Speed = FMath::Max(1.f, ProjectileNode->Speed);
				int32 MaxSpawnCount = FMath::Max(1, ProjectileNode->ProjectileCount);
				if (ProjectileNode->bAddComboStacksToProjectileCount)
				{
					MaxSpawnCount += FMath::Max(0, ProjectileNode->MaxBonusProjectiles);
				}

				const float SequentialDelay = ProjectileNode->bSpawnProjectilesSequentially
					? FMath::Max(0.f, ProjectileNode->SequentialProjectileSpawnInterval) * static_cast<float>(FMath::Max(0, MaxSpawnCount - 1))
					: 0.f;
				const float Lifetime = SequentialDelay + FMath::Max(0.f, ProjectileNode->MaxDistance) / Speed;
				MaxProjectileLifetime = FMath::Max(MaxProjectileLifetime, Lifetime);
				continue;
			}

			if (const UBFNode_SpawnBuffFlowProjectile* BuffFlowProjectileNode = Cast<UBFNode_SpawnBuffFlowProjectile>(Pair.Value))
			{
				if (!BuffFlowProjectileNode->TriggerGameplayEventTag.IsValid() && !BuffFlowProjectileNode->ExpireGameplayEventTag.IsValid())
				{
					continue;
				}

				int32 MaxSpawnCount = FMath::Max(1, BuffFlowProjectileNode->ProjectileCount);
				if (BuffFlowProjectileNode->bAddComboStacksToProjectileCount)
				{
					MaxSpawnCount += FMath::Max(0, BuffFlowProjectileNode->MaxBonusProjectiles);
				}

				const float SpawnDelay = FMath::Max(0.f, BuffFlowProjectileNode->SpawnInterval) * static_cast<float>(FMath::Max(0, MaxSpawnCount - 1));
				const float Lifetime = SpawnDelay + FMath::Max(0.f, BuffFlowProjectileNode->Lifetime);
				MaxProjectileLifetime = FMath::Max(MaxProjectileLifetime, Lifetime);
			}
		}

		if (!bHasWaitEventNode || MaxProjectileLifetime <= 0.f)
		{
			return 0.f;
		}

		return FMath::Clamp(MaxProjectileLifetime + 0.35f, 0.25f, 5.0f);
	}
}

UCombatDeckComponent::UCombatDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	TemporaryInitialFinisherRune = TSoftObjectPtr<URuneDataAsset>(
		FSoftObjectPath(TEXT("/Game/YogRuneEditor/Runes/DA_Rune_Finisher.DA_Rune_Finisher")));
	TemporaryFinisherLockedReasonText = FText::FromString(TEXT("Finisher unlocks after 3 completed battles"));
}

void UCombatDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	RefillActiveSequence();
	RefreshCardPassiveFlows();
}

void UCombatDeckComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllCardPassiveFlows();
	Super::EndPlay(EndPlayReason);
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
	Result.ActionContext = Context;

	if (Context.bExitedComboState)
	{
		BreakPendingLink(ECombatLinkBreakReason::ComboStateExited, &Context);
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

	if (PendingLinkContext.IsValidCard() && !Context.bComboContinued)
	{
		BreakPendingLink(ECombatLinkBreakReason::ComboStateExited, &Context);
	}

	const float CurrentCardComboMultiplier = GetComboEffectMultiplier(Card.Config, Context);
	Result.bHadCard = true;
	Result.ConsumedCard = Card;
	Result.bActionMatched = !IsLinkCardType(Card.Config.CardType)
		? true
		: DoesActionMatch(Card.Config.RequiredAction, Context.ActionType);
	SetAppliedMultiplier(Result, 1.0f, CurrentCardComboMultiplier);
	Result.ReasonText = Card.Config.HUDReasonText;

	int32 RequiredBattles = 0;
	int32 CurrentBattles = 0;
	if (IsTemporaryFinisherLocked(Card, RequiredBattles, CurrentBattles))
	{
		BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Context);
		Result.bActionMatched = false;
		Result.bCardTemporarilyLocked = true;
		Result.TemporaryUnlockRequiredCompletedBattles = RequiredBattles;
		Result.TemporaryUnlockCurrentCompletedBattles = CurrentBattles;
		Result.ReasonText = TemporaryFinisherLockedReasonText;

		if (Context.AttackInstanceGuid.IsValid())
		{
			ResolvedAttackGuids.Add(Context.AttackInstanceGuid);
		}

		CurrentIndex++;
		Result.bStartedShuffle = CurrentIndex >= ActiveSequence.Num();

		OnCardConsumed.Broadcast(Card, Result);
		if (Result.bStartedShuffle)
		{
			StartShuffle();
			OnShuffleStarted.Broadcast(Result);
		}

		PushCombatCardConsumeLog(Result);
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
			SetAppliedMultiplier(Result,
				ReversedRecipe ? ReversedRecipe->Multiplier : PendingLinkContext.Config.LinkConfig.BackwardEffect.Multiplier,
				CurrentCardComboMultiplier);
			const FText LinkReasonText = ReversedRecipe ? ReversedRecipe->ReasonText : PendingLinkContext.Config.LinkConfig.BackwardEffect.ReasonText;
			Result.ReasonText = LinkReasonText.IsEmpty()
				? PendingLinkContext.Config.HUDReasonText
				: LinkReasonText;
			ExecuteFlow(ReversedRecipe ? ReversedRecipe->LinkFlow : PendingLinkContext.Config.LinkConfig.BackwardEffect.Flow, PendingLinkContext, Context, Result);
			PendingLinkContext = FCombatCardInstance();
			PendingLinkActionContext = FCombatDeckActionContext();
		}
		else
		{
			BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Context);
		}
	}

	const bool bComboRequirementSatisfied = !Card.Config.bRequiresComboFinisher || Context.bIsComboFinisher;
	Result.bTriggeredFinisher = Result.bActionMatched
		&& bComboRequirementSatisfied
		&& Card.Config.CardType == ECombatCardType::Finisher;

	if (Result.bActionMatched && bComboRequirementSatisfied)
	{
		const bool bCanUseRecipeLinks = IsLinkCardType(Card.Config.CardType) && !Card.Config.LinkRecipes.IsEmpty();
		const bool bCanUseLinkConfig = IsLinkCardType(Card.Config.CardType)
			&& Card.Config.LinkRecipes.IsEmpty()
			&& Card.Config.LinkConfig.Direction != ECombatCardLinkDirection::None;

		if (bCanUseRecipeLinks)
		{
			if (Card.LinkOrientation == ECombatCardLinkOrientation::Reversed)
			{
				const bool bHasReversedRecipe = Context.bComboContinued
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
					PendingLinkActionContext = Context;
				}
				else
				{
					Result.bTriggeredBaseFlow = true;
					ExecuteFlow(Card.Config.BaseFlow, Card, Context, Result);
				}
			}
			else
			{
				const FCombatCardLinkRecipe* ForwardRecipe = FindMatchingLinkRecipe(Card, ECombatCardLinkOrientation::Forward, LastResolvedCard, Context);

				if (ForwardRecipe)
				{
					Result.bTriggeredMatchedFlow = true;
					Result.bTriggeredLink = true;
					Result.bTriggeredForwardLink = true;
					Result.LinkedSourceCard = LastResolvedCard;
					Result.LinkedTargetCard = Card;
					SetAppliedMultiplier(Result, ForwardRecipe->Multiplier, CurrentCardComboMultiplier);
					Result.ReasonText = ForwardRecipe->ReasonText.IsEmpty()
						? Card.Config.HUDReasonText
						: ForwardRecipe->ReasonText;
					ExecuteFlow(ForwardRecipe->LinkFlow, Card, Context, Result);
				}
				else
				{
					Result.bTriggeredBaseFlow = true;
					ExecuteFlow(Card.Config.BaseFlow, Card, Context, Result);
				}
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
				SetAppliedMultiplier(Result, Card.Config.LinkConfig.ForwardEffect.Multiplier, CurrentCardComboMultiplier);
				Result.ReasonText = Card.Config.LinkConfig.ForwardEffect.ReasonText.IsEmpty()
					? Card.Config.HUDReasonText
					: Card.Config.LinkConfig.ForwardEffect.ReasonText;
				ExecuteFlow(Card.Config.LinkConfig.ForwardEffect.Flow, Card, Context, Result);
			}
			else
			{
				Result.bTriggeredBaseFlow = true;
				ExecuteFlow(Card.Config.BaseFlow, Card, Context, Result);
			}

			if (!bForwardMatched && Context.bComboContinued && WantsBackwardLink(Card.Config) && Card.Config.LinkConfig.BackwardEffect.Flow)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
				PendingLinkActionContext = Context;
			}
		}
		else
		{
			Result.bTriggeredBaseFlow = true;
			ExecuteFlow(Card.Config.BaseFlow, Card, Context, Result);

			Result.bTriggeredMatchedFlow = true;
			ExecuteFlow(Card.Config.MatchedFlow, Card, Context, Result);

			if (Card.Config.LinkMode == ECardLinkMode::PassToNext && Context.bComboContinued)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
				PendingLinkActionContext = Context;
			}
			else if (Card.Config.LinkMode == ECardLinkMode::ReadPrevious)
			{
				Result.bTriggeredLink = LastResolvedCard.IsValidCard() || PendingLinkContext.IsValidCard();
				Result.bTriggeredForwardLink = Result.bTriggeredLink;
				Result.LinkedSourceCard = LastResolvedCard;
				Result.LinkedTargetCard = Card;
				PendingLinkContext = FCombatCardInstance();
				PendingLinkActionContext = FCombatDeckActionContext();
			}
		}
	}
	else
	{
		Result.bTriggeredBaseFlow = true;
		ExecuteFlow(Card.Config.BaseFlow, Card, Context, Result);
	}

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

	// ── 512版本：推卡牌消耗行到战斗日志 ────────────────────────────────
	PushCombatCardConsumeLog(Result);

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
		if (UFlowAsset* ActiveFlow = BuffFlowComponent->GetActiveBuffFlowAsset(Card.InstanceGuid))
		{
			const float DeferredStopDelay = GetProjectileEventFlowStopDelay(ActiveFlow);
			if (DeferredStopDelay > 0.f && GetWorld())
			{
				const FGuid CardGuid = Card.InstanceGuid;
				FTimerHandle TimerHandle;
				GetWorld()->GetTimerManager().SetTimer(
					TimerHandle,
					FTimerDelegate::CreateWeakLambda(BuffFlowComponent, [BuffFlowComponent, CardGuid]()
					{
						BuffFlowComponent->StopBuffFlow(CardGuid);
					}),
					DeferredStopDelay,
					false);

				UE_LOG(LogTemp, Warning, TEXT("[CombatDeckFlow] Deferred StopCardFlow Guid=%s Flow=%s Delay=%.2f"),
					*Card.InstanceGuid.ToString(),
					*GetNameSafe(ActiveFlow),
					DeferredStopDelay);
				return;
			}
		}

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
	StopAllCardPassiveFlows();

	DeckList.Reset();
	ShuffleCooldownDuration = FMath::Max(0.0f, InShuffleCooldownDuration);
	MaxActiveSequenceSize = FMath::Max(0, InMaxActiveSequenceSize);

	TArray<URuneDataAsset*> EffectiveSourceAssets = SourceAssets;
	AppendTemporaryInitialFinisherCard(EffectiveSourceAssets);

	for (URuneDataAsset* RuneAsset : EffectiveSourceAssets)
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
	PendingLinkActionContext = FCombatDeckActionContext();
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();
	RefillActiveSequence();
	RefreshCardPassiveFlows();
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

TArray<FCombatCardInstance> UCombatDeckComponent::GetFullDeckSnapshot() const
{
	return BuildTemporaryLockViewCards(DeckList);
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
		RemainingCards.Add(BuildTemporaryLockViewCard(ActiveSequence[Index]));
	}

	return RemainingCards;
}

void UCombatDeckComponent::RefreshDeckView()
{
	if (DeckState != EDeckState::Ready)
	{
		return;
	}

	OnDeckLoaded.Broadcast(GetRemainingDeckSnapshot());
}

bool UCombatDeckComponent::MoveCardInDeck(int32 FromIndex, int32 InsertIndex)
{
	if (!DeckList.IsValidIndex(FromIndex) || InsertIndex < 0 || InsertIndex > DeckList.Num())
	{
		return false;
	}

	if (FromIndex == InsertIndex || FromIndex + 1 == InsertIndex)
	{
		return false;
	}

	FCombatCardInstance MovedCard = DeckList[FromIndex];
	DeckList.RemoveAt(FromIndex);
	const int32 AdjustedInsertIndex = FromIndex < InsertIndex ? InsertIndex - 1 : InsertIndex;
	DeckList.Insert(MovedCard, FMath::Clamp(AdjustedInsertIndex, 0, DeckList.Num()));
	StartDeckEditReload();
	return true;
}

bool UCombatDeckComponent::SetCardLinkOrientationByIndex(int32 CardIndex, ECombatCardLinkOrientation Orientation)
{
	if (!DeckList.IsValidIndex(CardIndex) || !IsLinkCardType(DeckList[CardIndex].Config.CardType))
	{
		return false;
	}

	DeckList[CardIndex].LinkOrientation = Orientation;
	ResetRuntimeStateAfterDeckEdit();
	return true;
}

bool UCombatDeckComponent::ToggleCardLinkOrientationByIndex(int32 CardIndex)
{
	if (!DeckList.IsValidIndex(CardIndex) || !IsLinkCardType(DeckList[CardIndex].Config.CardType))
	{
		return false;
	}

	const ECombatCardLinkOrientation NewOrientation = DeckList[CardIndex].LinkOrientation == ECombatCardLinkOrientation::Forward
		? ECombatCardLinkOrientation::Reversed
		: ECombatCardLinkOrientation::Forward;
	return SetCardLinkOrientationByIndex(CardIndex, NewOrientation);
}

void UCombatDeckComponent::ApplyDeckOrientations(const TArray<ECombatCardLinkOrientation>& Orientations)
{
	const int32 Count = FMath::Min(DeckList.Num(), Orientations.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		DeckList[i].LinkOrientation = Orientations[i];
	}
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
	StartPassiveFlowsForCard(Card);

	if (DeckState == EDeckState::Ready && ActiveSequence.IsEmpty())
	{
		RefillActiveSequence();
	}

	return true;
}

bool UCombatDeckComponent::AddCardFromRuneShop(URuneDataAsset* RuneAsset)
{
	const FCombatCardInstance Card = MakeCardFromRune(RuneAsset, CombatDeckOwnerSourceShop);
	if (!Card.IsValidCard())
	{
		return false;
	}

	DeckList.Add(Card);
	OnRewardAddedToDeck.Broadcast(Card);
	StartPassiveFlowsForCard(Card);

	if (DeckState == EDeckState::Ready && ActiveSequence.IsEmpty())
	{
		RefillActiveSequence();
	}

	return true;
}

bool UCombatDeckComponent::RemoveCardAtIndex(int32 CardIndex)
{
	if (!DeckList.IsValidIndex(CardIndex))
	{
		return false;
	}

	StopPassiveFlowsForCard(DeckList[CardIndex].InstanceGuid);
	DeckList.RemoveAt(CardIndex);
	StartDeckEditReload();
	return true;
}

void UCombatDeckComponent::SetShuffleCooldownDuration(float InDuration)
{
	ShuffleCooldownDuration = FMath::Max(0.0f, InDuration);
}

void UCombatDeckComponent::SetDeckListForTest(const TArray<FCombatCardConfig>& InCards)
{
	StopAllCardPassiveFlows();

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
	PendingLinkActionContext = FCombatDeckActionContext();
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
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
	DashSavedLinkActionContext = PendingLinkActionContext;
}

bool UCombatDeckComponent::RestorePendingLinkContextFromDash()
{
	if (!DashSavedLinkContext.IsValidCard())
	{
		return false;
	}

	PendingLinkContext = DashSavedLinkContext;
	PendingLinkActionContext = DashSavedLinkActionContext;
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
	return true;
}

void UCombatDeckComponent::ClearDashSavedLinkContext()
{
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
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
		Card.Config.DisplayName = FText::FromName(RuneAsset->GetRuneName());
	}

	return Card;
}

void UCombatDeckComponent::AppendTemporaryInitialFinisherCard(TArray<URuneDataAsset*>& SourceAssets) const
{
	if (!bGrantTemporaryInitialFinisherCard)
	{
		return;
	}

	URuneDataAsset* FinisherRune = TemporaryInitialFinisherRune.LoadSynchronous();
	if (!FinisherRune || !FinisherRune->RuneInfo.CombatCard.bIsCombatCard
		|| FinisherRune->RuneInfo.CombatCard.CardType != ECombatCardType::Finisher)
	{
		return;
	}

	SourceAssets.RemoveAll([FinisherRune](const URuneDataAsset* RuneAsset)
	{
		return RuneAsset == FinisherRune;
	});

	const int32 InsertIndex = MaxActiveSequenceSize > 0
		? FMath::Clamp(MaxActiveSequenceSize - 1, 0, SourceAssets.Num())
		: SourceAssets.Num();
	SourceAssets.Insert(FinisherRune, InsertIndex);
}

bool UCombatDeckComponent::IsTemporaryFinisherLocked(
	const FCombatCardInstance& Card,
	int32& OutRequiredBattles,
	int32& OutCurrentBattles) const
{
	OutRequiredBattles = FMath::Max(0, TemporaryFinisherUnlockCompletedBattles);
	OutCurrentBattles = 0;

	if (!Card.IsValidCard() || Card.Config.CardType != ECombatCardType::Finisher || OutRequiredBattles <= 0)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const AYogGameMode* GameMode = World ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
	OutCurrentBattles = GameMode ? FMath::Max(0, GameMode->GetCompletedCombatBattleCount()) : 0;
	return OutCurrentBattles < OutRequiredBattles;
}

FCombatCardInstance UCombatDeckComponent::BuildTemporaryLockViewCard(const FCombatCardInstance& Card) const
{
	FCombatCardInstance ViewCard = Card;
	int32 RequiredBattles = 0;
	int32 CurrentBattles = 0;
	ViewCard.bTemporarilyLocked = IsTemporaryFinisherLocked(Card, RequiredBattles, CurrentBattles);
	ViewCard.TemporaryUnlockRequiredCompletedBattles = RequiredBattles;
	ViewCard.TemporaryUnlockCurrentCompletedBattles = CurrentBattles;
	return ViewCard;
}

TArray<FCombatCardInstance> UCombatDeckComponent::BuildTemporaryLockViewCards(const TArray<FCombatCardInstance>& Cards) const
{
	TArray<FCombatCardInstance> ViewCards;
	ViewCards.Reserve(Cards.Num());
	for (const FCombatCardInstance& Card : Cards)
	{
		ViewCards.Add(BuildTemporaryLockViewCard(Card));
	}
	return ViewCards;
}

void UCombatDeckComponent::PushCombatCardConsumeLog(const FCombatCardResolveResult& Result) const
{
	if (!Result.ConsumedCard.IsValidCard())
	{
		return;
	}

	FDamageBreakdown Consume;
	Consume.bIsCardEventOnly = true;
	Consume.bHadCard = true;
	Consume.bConsumedCard = true;
	Consume.bActionMatched = Result.bActionMatched;
	Consume.bTriggeredMatchedFlow = Result.bTriggeredMatchedFlow;
	Consume.bTriggeredLink = Result.bTriggeredLink || Result.bTriggeredForwardLink || Result.bTriggeredBackwardLink;
	Consume.bTriggeredFinisher = Result.bTriggeredFinisher;
	Consume.bStartedShuffle = Result.bStartedShuffle;
	Consume.CardDisplayName = Result.ConsumedCard.SourceData
		? Result.ConsumedCard.SourceData->GetRuneName()
		: Result.ConsumedCard.Config.CardIdTag.GetTagName();
	Consume.CardConsumeTiming = Result.ConsumedCard.Config.TriggerTiming == ECombatCardTriggerTiming::OnCommit
		? FName("OnCommit") : FName("OnHit");
	Consume.SourceName = GetNameSafe(GetOwner());
	Consume.GameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Consume.DamageType = Result.bCardTemporarilyLocked
		? FName("Card_Locked")
		: (Result.bStartedShuffle ? FName("Card_Shuffle") : FName("Card_Consume"));
	UCombatLogStatics::PushEntry(Consume);
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
	OnDeckLoaded.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
}

void UCombatDeckComponent::RefreshCardPassiveFlows()
{
	StopAllCardPassiveFlows();

	for (const FCombatCardInstance& Card : DeckList)
	{
		StartPassiveFlowsForCard(Card);
	}
}

void UCombatDeckComponent::StartPassiveFlowsForCard(const FCombatCardInstance& Card)
{
	if (!Card.IsValidCard() || !Card.SourceData)
	{
		return;
	}

	const TArray<TObjectPtr<UFlowAsset>>& PassiveFlows = Card.SourceData->RuneInfo.Flow.PassiveFlows;
	if (PassiveFlows.IsEmpty())
	{
		return;
	}

	UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr;
	if (!BuffFlowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckPassiveFlow] Start skipped: Owner=%s Card=%s has no BuffFlowComponent"),
			*GetNameSafe(GetOwner()),
			*Card.Config.DisplayName.ToString());
		return;
	}

	TArray<FGuid>& FlowGuids = ActiveCardPassiveFlowGuids.FindOrAdd(Card.InstanceGuid);
	for (UFlowAsset* PassiveFlow : PassiveFlows)
	{
		if (!PassiveFlow)
		{
			continue;
		}

		const FGuid FlowGuid = FGuid::NewGuid();
		FlowGuids.Add(FlowGuid);
		BuffFlowComponent->StartBuffFlowWithRune(PassiveFlow, FlowGuid, Card.SourceData, GetOwner(), true);

		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckPassiveFlow] Start Owner=%s Card=%s Flow=%s CardGuid=%s FlowGuid=%s"),
			*GetNameSafe(GetOwner()),
			*Card.Config.DisplayName.ToString(),
			*GetNameSafe(PassiveFlow),
			*Card.InstanceGuid.ToString(),
			*FlowGuid.ToString());
	}

	if (FlowGuids.IsEmpty())
	{
		ActiveCardPassiveFlowGuids.Remove(Card.InstanceGuid);
	}
}

void UCombatDeckComponent::StopPassiveFlowsForCard(const FGuid& CardGuid)
{
	TArray<FGuid> FlowGuids;
	if (!ActiveCardPassiveFlowGuids.RemoveAndCopyValue(CardGuid, FlowGuids))
	{
		return;
	}

	UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr;
	if (!BuffFlowComponent)
	{
		return;
	}

	for (const FGuid& FlowGuid : FlowGuids)
	{
		BuffFlowComponent->StopBuffFlow(FlowGuid);
	}
}

void UCombatDeckComponent::StopAllCardPassiveFlows()
{
	UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr;
	if (BuffFlowComponent)
	{
		for (const TPair<FGuid, TArray<FGuid>>& Pair : ActiveCardPassiveFlowGuids)
		{
			for (const FGuid& FlowGuid : Pair.Value)
			{
				BuffFlowComponent->StopBuffFlow(FlowGuid);
			}
		}
	}

	ActiveCardPassiveFlowGuids.Reset();
}

void UCombatDeckComponent::ResetRuntimeStateAfterDeckEdit()
{
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();
	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	RefillActiveSequence();
}

void UCombatDeckComponent::StartDeckEditReload()
{
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	DashSavedLinkContext = FCombatCardInstance();
	DashSavedLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();

	DeckState = EDeckState::EmptyShuffling;
	ShuffleCooldownRemaining = FMath::Max(0.0f, ShuffleCooldownDuration * 0.5f);
	CurrentIndex = 0;
	ActiveSequence.Reset();

	FCombatCardResolveResult Result;
	Result.bStartedShuffle = true;
	Result.ReasonText = NSLOCTEXT("CombatDeck", "DeckEditReload", "Deck edited: reload started");
	OnShuffleStarted.Broadcast(Result);
	OnShuffleProgress.Broadcast(0.0f);

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckEdit] ReloadAfterMove Cooldown=%.3f FullCooldown=%.3f DeckCount=%d"),
		ShuffleCooldownRemaining,
		ShuffleCooldownDuration,
		DeckList.Num());

	if (ShuffleCooldownRemaining <= KINDA_SMALL_NUMBER)
	{
		RefillActiveSequence();
		OnShuffleCompleted.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
	}
}

void UCombatDeckComponent::StartShuffle()
{
	BreakPendingLink(ECombatLinkBreakReason::ShuffleStarted);
	DeckState = EDeckState::EmptyShuffling;
	ShuffleCooldownRemaining = ShuffleCooldownDuration;
	CurrentIndex = 0;
	ActiveSequence.Reset();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();

	if (ShuffleCooldownDuration <= KINDA_SMALL_NUMBER)
	{
		RefillActiveSequence();
		OnShuffleCompleted.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
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
		OnShuffleCompleted.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
	}
}

void UCombatDeckComponent::ExecuteFlow(
	UFlowAsset* FlowAsset,
	const FCombatCardInstance& Card,
	const FCombatDeckActionContext& Context,
	FCombatCardResolveResult& Result) const
{
	if (!FlowAsset)
	{
		return;
	}

	Result.ExecutedFlows.Add(FlowAsset);
	if (UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr)
	{
		BuffFlowComponent->StartCombatCardFlow(FlowAsset, Card, Context, Result, GetOwner(), true);
	}
}

int32 UCombatDeckComponent::GetComboBonusStacks(const FCombatDeckActionContext& Context) const
{
	return FMath::Max(0, Context.ComboIndex - 1);
}

float UCombatDeckComponent::GetComboEffectMultiplier(const FCombatCardConfig& Config, const FCombatDeckActionContext& Context) const
{
	if (!Config.bUseComboEffectScaling)
	{
		return 1.0f;
	}

	const int32 ComboBonusStacks = GetComboBonusStacks(Context);
	const float AdditiveScalar = FMath::Min(
		FMath::Max(0.0f, Config.MaxComboScalar),
		static_cast<float>(ComboBonusStacks) * FMath::Max(0.0f, Config.ComboScalarPerIndex));
	return 1.0f + AdditiveScalar;
}

void UCombatDeckComponent::SetAppliedMultiplier(FCombatCardResolveResult& Result, float LinkMultiplier, float ComboMultiplier) const
{
	Result.AppliedMultiplier = FMath::Max(0.0f, LinkMultiplier) * FMath::Max(0.0f, ComboMultiplier);
}

FCombatCardEffectContext UCombatDeckComponent::BuildCombatCardEffectContext(
	const FCombatCardInstance& Card,
	const FCombatDeckActionContext& Context,
	const FCombatCardResolveResult& Result) const
{
	FCombatCardEffectContext EffectContext;
	EffectContext.ActionContext = Context;
	EffectContext.SourceCard = Card;
	EffectContext.ResolveResult = Result;
	EffectContext.ComboIndex = Context.ComboIndex;
	EffectContext.ComboNodeId = Context.ComboNodeId;
	EffectContext.ComboTags = Context.ComboTags;
	EffectContext.AbilityTag = Context.AbilityTag;
	EffectContext.EffectMultiplier = Result.AppliedMultiplier;
	EffectContext.ComboBonusStacks = GetComboBonusStacks(Context);
	EffectContext.bFromLink = Result.bTriggeredLink || Result.bTriggeredForwardLink || Result.bTriggeredBackwardLink;
	EffectContext.bIsComboFinisher = Context.bIsComboFinisher;
	EffectContext.bSourceCardFinisher = Card.Config.CardType == ECombatCardType::Finisher;
	EffectContext.bTriggeredFinisher = Result.bTriggeredFinisher;
	EffectContext.bTriggeredForwardLink = Result.bTriggeredForwardLink;
	EffectContext.bTriggeredBackwardLink = Result.bTriggeredBackwardLink;
	return EffectContext;
}

void UCombatDeckComponent::BreakPendingLink(ECombatLinkBreakReason Reason, const FCombatDeckActionContext* BreakContext)
{
	if (!PendingLinkContext.IsValidCard())
	{
		return;
	}

	const FCombatCardInstance BrokenCard = PendingLinkContext;
	const FCombatDeckActionContext ResolvedBreakContext = BreakContext ? *BreakContext : PendingLinkActionContext;

	FCombatCardResolveResult Result;
	Result.ActionContext = ResolvedBreakContext;
	Result.bLinkBroken = true;
	Result.LinkBreakReason = Reason;
	Result.ConsumedCard = BrokenCard;
	Result.LinkedSourceCard = BrokenCard;
	Result.ReasonText = BrokenCard.Config.HUDReasonText;
	SetAppliedMultiplier(Result, 1.0f, GetComboEffectMultiplier(BrokenCard.Config, ResolvedBreakContext));

	switch (BrokenCard.Config.LinkConfig.BreakPolicy)
	{
	case ECombatLinkBreakPolicy::ReleaseBaseFlow:
		if (BrokenCard.Config.BaseFlow)
		{
			Result.bTriggeredBaseFlow = true;
			ExecuteFlow(BrokenCard.Config.BaseFlow, BrokenCard, ResolvedBreakContext, Result);
		}
		break;
	case ECombatLinkBreakPolicy::ReleaseBreakFlow:
		if (BrokenCard.Config.LinkConfig.BreakFlow)
		{
			Result.bTriggeredMatchedFlow = true;
			ExecuteFlow(BrokenCard.Config.LinkConfig.BreakFlow, BrokenCard, ResolvedBreakContext, Result);
		}
		break;
	case ECombatLinkBreakPolicy::Fizzle:
	default:
		break;
	}

	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	if (Result.bTriggeredBaseFlow)
	{
		OnCardReleased.Broadcast(Result);
	}
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
