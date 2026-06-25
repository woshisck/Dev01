#include "Component/CombatDeckComponent.h"

#include "UI/CombatLogStatics.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_SpawnBuffFlowProjectile.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Combat/FinisherDeprecation.h"
#include "FlowAsset.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Story/FirstRunTutorialDirectorSubsystem.h"

namespace
{
	const FName CombatDeckOwnerSourceWeapon(TEXT("Weapon"));
	const FName CombatDeckOwnerSourceReward(TEXT("Reward"));
	const FName CombatDeckOwnerSourceShop(TEXT("Shop"));

	// Derives a stable, collision-free GUID for a card's pre-commit flow slot.
	// XOR on A and D ensures the result never equals the source GUID.
	FGuid CombatDeck_PreCommitGuid(const FGuid& CardGuid)
	{
		return FGuid(CardGuid.A ^ 0x50524543u, CardGuid.B, CardGuid.C, CardGuid.D ^ 0x4f4d4954u);
	}

	const TArray<TObjectPtr<URuneDataAsset>>* GetDefaultWeaponDeckSource(const UWeaponDefinition* WeaponDefinition)
	{
		if (!WeaponDefinition)
		{
			return nullptr;
		}

		return &WeaponDefinition->InitialCombatDeck;
	}

	void CopyDeckSourceAssets(const TArray<TObjectPtr<URuneDataAsset>>* SourceCards, TArray<URuneDataAsset*>& OutSourceAssets)
	{
		if (!SourceCards)
		{
			return;
		}

		OutSourceAssets.Reserve(SourceCards->Num());
		for (const TObjectPtr<URuneDataAsset>& SourceCard : *SourceCards)
		{
			OutSourceAssets.Add(SourceCard.Get());
		}
	}

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
				if (!BuffFlowProjectileNode->TriggerGameplayEventTag.Value.IsValid() && !BuffFlowProjectileNode->ExpireGameplayEventTag.Value.IsValid())
				{
					continue;
				}

				int32 MaxSpawnCount = FMath::Max(1, BuffFlowProjectileNode->ProjectileCount.Value);
				if (BuffFlowProjectileNode->bAddComboStacksToProjectileCount)
				{
					MaxSpawnCount += FMath::Max(0, BuffFlowProjectileNode->MaxBonusProjectiles);
				}

				const float SpawnDelay = FMath::Max(0.f, BuffFlowProjectileNode->SpawnInterval) * static_cast<float>(FMath::Max(0, MaxSpawnCount - 1));
				const float Lifetime = SpawnDelay + FMath::Max(0.f, BuffFlowProjectileNode->Lifetime.Value);
				MaxProjectileLifetime = FMath::Max(MaxProjectileLifetime, Lifetime);
			}
		}

		if (!bHasWaitEventNode || MaxProjectileLifetime <= 0.f)
		{
			return 0.f;
		}

		return FMath::Clamp(MaxProjectileLifetime + 0.35f, 0.25f, 5.0f);
	}

	bool CombatDeckCardHasId(const FCombatCardConfig& Config, const TCHAR* TagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		return Tag.IsValid() && Config.CardIdTag == Tag;
	}

	bool CombatDeckCardHasEffect(const FCombatCardConfig& Config, const TCHAR* TagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		return Tag.IsValid() && Config.CardEffectTags.HasTagExact(Tag);
	}

	FString BuffLeafFromLegacyLeaf(const FString& Leaf, const bool bIdentityTag)
	{
		if (Leaf == TEXT("Buff.AttackUp") || Leaf == TEXT("AttackUp"))
		{
			return TEXT("AttackUp");
		}
		if (Leaf == TEXT("Defense.ReduceDamage") || Leaf == TEXT("ReduceDamage"))
		{
			return TEXT("ReduceDamage");
		}
		if (Leaf == TEXT("Burn"))
		{
			return TEXT("Fire");
		}
		if (Leaf == TEXT("Burning"))
		{
			return TEXT("Fire");
		}
		if (Leaf == TEXT("Poisoned"))
		{
			return TEXT("Poison");
		}
		if (Leaf == TEXT("Bleeding"))
		{
			return TEXT("Bleed");
		}
		if (Leaf == TEXT("Frozen"))
		{
			return TEXT("Freeze");
		}
		if (Leaf == TEXT("Stunned"))
		{
			return TEXT("Stun");
		}
		if (Leaf == TEXT("Rended"))
		{
			return TEXT("Rend");
		}
		if (Leaf == TEXT("Wounded"))
		{
			return TEXT("Wound");
		}
		if (Leaf == TEXT("Feared"))
		{
			return TEXT("Fear");
		}
		if (Leaf == TEXT("Cursed"))
		{
			return TEXT("Curse");
		}
		if (Leaf == TEXT("Shielded"))
		{
			return TEXT("Shield");
		}
		if (Leaf == TEXT("Heavy"))
		{
			return bIdentityTag ? TEXT("WeaponSkillFinisher") : TEXT("Detonate");
		}
		return Leaf;
	}

	bool TryExtractLegacyCombatCardLeaf(const FString& TagString, FString& OutLeaf, bool& bOutIdentityTag)
	{
		static constexpr const TCHAR* RuneIdPrefix = TEXT("Rune.ID.");
		static constexpr const TCHAR* CardIdPrefix = TEXT("Card.ID.");
		static constexpr const TCHAR* RuneEffectPrefix = TEXT("Rune.Effect.");
		static constexpr const TCHAR* CardEffectPrefix = TEXT("Card.Effect.");

		if (TagString.StartsWith(RuneIdPrefix))
		{
			OutLeaf = TagString.RightChop(FCString::Strlen(RuneIdPrefix));
			bOutIdentityTag = true;
			return true;
		}
		if (TagString.StartsWith(CardIdPrefix))
		{
			OutLeaf = TagString.RightChop(FCString::Strlen(CardIdPrefix));
			bOutIdentityTag = true;
			return true;
		}
		if (TagString.StartsWith(RuneEffectPrefix))
		{
			OutLeaf = TagString.RightChop(FCString::Strlen(RuneEffectPrefix));
			bOutIdentityTag = false;
			return true;
		}
		if (TagString.StartsWith(CardEffectPrefix))
		{
			OutLeaf = TagString.RightChop(FCString::Strlen(CardEffectPrefix));
			bOutIdentityTag = false;
			return true;
		}

		return false;
	}

	void AddRequestedTag(TArray<FGameplayTag>& OutTags, const FString& TagString)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
		if (Tag.IsValid())
		{
			OutTags.AddUnique(Tag);
		}
	}

	TArray<FGameplayTag> GetEquivalentCombatCardTags(const FGameplayTag& Tag)
	{
		TArray<FGameplayTag> EquivalentTags;
		if (!Tag.IsValid())
		{
			return EquivalentTags;
		}

		const FString TagString = Tag.ToString();
		FString Leaf;
		bool bIdentityTag = false;
		if (TryExtractLegacyCombatCardLeaf(TagString, Leaf, bIdentityTag))
		{
			AddRequestedTag(EquivalentTags, FString(TEXT("Buff.")) + BuffLeafFromLegacyLeaf(Leaf, bIdentityTag));
			AddRequestedTag(EquivalentTags, FString(TEXT("Rune.ID.")) + Leaf);
			AddRequestedTag(EquivalentTags, FString(TEXT("Card.ID.")) + Leaf);
			AddRequestedTag(EquivalentTags, FString(TEXT("Rune.Effect.")) + Leaf);
			AddRequestedTag(EquivalentTags, FString(TEXT("Card.Effect.")) + Leaf);
			return EquivalentTags;
		}

		static constexpr const TCHAR* BuffStatusPrefix = TEXT("Buff.Status.");
		if (TagString.StartsWith(BuffStatusPrefix))
		{
			Leaf = TagString.RightChop(FCString::Strlen(BuffStatusPrefix));
			AddRequestedTag(EquivalentTags, FString(TEXT("Buff.")) + BuffLeafFromLegacyLeaf(Leaf, false));
			return EquivalentTags;
		}

		static constexpr const TCHAR* BuffPrefix = TEXT("Buff.");
		if (TagString.StartsWith(BuffPrefix))
		{
			const FString BuffLeaf = TagString.RightChop(FCString::Strlen(BuffPrefix));
			TArray<FString> LegacyLeaves;
			TArray<FString> LegacyStatusLeaves;
			LegacyLeaves.Add(BuffLeaf);
			if (BuffLeaf == TEXT("Fire"))
			{
				LegacyLeaves.Add(TEXT("Burn"));
				LegacyStatusLeaves.Add(TEXT("Burning"));
			}
			else if (BuffLeaf == TEXT("Poison"))
			{
				LegacyStatusLeaves.Add(TEXT("Poisoned"));
			}
			else if (BuffLeaf == TEXT("Bleed"))
			{
				LegacyStatusLeaves.Add(TEXT("Bleeding"));
			}
			else if (BuffLeaf == TEXT("Freeze"))
			{
				LegacyStatusLeaves.Add(TEXT("Frozen"));
			}
			else if (BuffLeaf == TEXT("Stun"))
			{
				LegacyStatusLeaves.Add(TEXT("Stunned"));
			}
			else if (BuffLeaf == TEXT("Rend"))
			{
				LegacyStatusLeaves.Add(TEXT("Rended"));
			}
			else if (BuffLeaf == TEXT("Wound"))
			{
				LegacyStatusLeaves.Add(TEXT("Wounded"));
			}
			else if (BuffLeaf == TEXT("Fear"))
			{
				LegacyStatusLeaves.Add(TEXT("Feared"));
			}
			else if (BuffLeaf == TEXT("Curse"))
			{
				LegacyStatusLeaves.Add(TEXT("Cursed"));
			}
			else if (BuffLeaf == TEXT("Shield"))
			{
				LegacyStatusLeaves.Add(TEXT("Shielded"));
			}
			else if (BuffLeaf == TEXT("ShadowMark"))
			{
				LegacyStatusLeaves.Add(TEXT("ShadowMark"));
			}
			else if (BuffLeaf == TEXT("Detonate"))
			{
				LegacyLeaves.Add(TEXT("Heavy"));
			}
			else if (BuffLeaf == TEXT("WeaponSkillFinisher"))
			{
				LegacyLeaves.Add(TEXT("Heavy"));
			}
			else if (BuffLeaf == TEXT("ReduceDamage"))
			{
				LegacyLeaves.Add(TEXT("Defense.ReduceDamage"));
			}

			for (const FString& LegacyLeaf : LegacyLeaves)
			{
				AddRequestedTag(EquivalentTags, FString(TEXT("Rune.ID.")) + LegacyLeaf);
				AddRequestedTag(EquivalentTags, FString(TEXT("Card.ID.")) + LegacyLeaf);
				AddRequestedTag(EquivalentTags, FString(TEXT("Rune.Effect.")) + LegacyLeaf);
				AddRequestedTag(EquivalentTags, FString(TEXT("Card.Effect.")) + LegacyLeaf);
			}
			for (const FString& LegacyStatusLeaf : LegacyStatusLeaves)
			{
				AddRequestedTag(EquivalentTags, FString(TEXT("Buff.Status.")) + LegacyStatusLeaf);
			}
		}
		return EquivalentTags;
	}

	FGameplayTag GetFormalCombatCardTag(const FGameplayTag& Tag)
	{
		if (!Tag.IsValid())
		{
			return FGameplayTag();
		}

		const FString TagString = Tag.ToString();
		if (TagString.StartsWith(TEXT("Buff.")))
		{
			return Tag;
		}

		FString Leaf;
		bool bIdentityTag = false;
		if (TryExtractLegacyCombatCardLeaf(TagString, Leaf, bIdentityTag))
		{
			const FString FormalTagString = FString(TEXT("Buff.")) + BuffLeafFromLegacyLeaf(Leaf, bIdentityTag);
			const FGameplayTag FormalTag = FGameplayTag::RequestGameplayTag(FName(*FormalTagString), false);
			return FormalTag.IsValid() ? FormalTag : Tag;
		}

		return Tag;
	}

	bool ContainerHasTagOrEquivalent(const FGameplayTagContainer& Container, const FGameplayTag& Tag)
	{
		if (!Tag.IsValid())
		{
			return false;
		}

		if (Container.HasTag(Tag))
		{
			return true;
		}

		for (const FGameplayTag& EquivalentTag : GetEquivalentCombatCardTags(Tag))
		{
			if (Container.HasTag(EquivalentTag))
			{
				return true;
			}
		}
		return false;
	}

	bool ContainerHasAllTagsOrEquivalent(const FGameplayTagContainer& ActualTags, const FGameplayTagContainer& RequiredTags)
	{
		for (const FGameplayTag& RequiredTag : RequiredTags)
		{
			if (!ContainerHasTagOrEquivalent(ActualTags, RequiredTag))
			{
				return false;
			}
		}

		return true;
	}

	void AddCombatCardEffect(FCombatCardConfig& Config, const TCHAR* TagName)
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
		if (Tag.IsValid())
		{
			Config.CardEffectTags.AddTag(Tag);
		}
	}

	bool IsDeprecatedFinisherCardConfig(const FCombatCardConfig& Config)
	{
		if (!DevKit::Combat::IsFinisherAbilityDeprecated())
		{
			return false;
		}

		return Config.CardType == ECombatCardType::Finisher
			|| CombatDeckCardHasId(Config, TEXT("Buff.Finisher"))
			|| CombatDeckCardHasId(Config, TEXT("Rune.ID.Finisher"))
			|| CombatDeckCardHasId(Config, TEXT("Card.ID.Finisher"))
			|| CombatDeckCardHasEffect(Config, TEXT("Buff.Finisher"))
			|| CombatDeckCardHasEffect(Config, TEXT("Rune.Effect.Finisher"))
			|| CombatDeckCardHasEffect(Config, TEXT("Card.Effect.Finisher"));
	}
}

UCombatDeckComponent::UCombatDeckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	if (!DevKit::Combat::IsFinisherAbilityDeprecated())
	{
		TemporaryInitialFinisherRune = TSoftObjectPtr<URuneDataAsset>(
			FSoftObjectPath(TEXT("/Game/YogRuneEditor/Runes/DA_Rune_Finisher.DA_Rune_Finisher")));
	}
	TemporaryFinisherLockedReasonText = FText::GetEmpty();
}

void UCombatDeckComponent::BeginPlay()
{
	Super::BeginPlay();
	PruneDeprecatedFinisherCards();
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
	Context.ReleaseMode = bIsComboFinisher ? ECombatCardReleaseMode::Finisher : ECombatCardReleaseMode::Normal;
	Context.FlowRole = bIsComboFinisher ? ECombatDeckFlowRole::Finisher : ECombatDeckFlowRole::Starter;
	Context.bFromDashSave = bFromDashSave;
	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
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
	if (Result.ActionContext.bIsComboFinisher)
	{
		Result.ActionContext.ReleaseMode = ECombatCardReleaseMode::Finisher;
	}

	const bool bUseSingleActionSlot = IsSingleActionSlot(Result.ActionContext.ActionSlot);
	if (Context.bExitedComboState)
	{
		ResetCombatSequenceProgress(ECombatLinkBreakReason::ComboStateExited, &Result.ActionContext);
	}
	else if (!Context.bComboContinued && CurrentIndex > 0)
	{
		ResetCombatSequenceProgress(ECombatLinkBreakReason::ComboStateExited, &Result.ActionContext);
	}

	if (Context.AttackInstanceGuid.IsValid() && ResolvedAttackGuids.Contains(Context.AttackInstanceGuid))
	{
		return Result;
	}

	FCombatCardInstance Card;
	if (bUseSingleActionSlot)
	{
		const FCombatCardInstance* SlotCard = GetSingleActionSlotCard(Result.ActionContext.ActionSlot);
		if (!SlotCard || !SlotCard->IsValidCard())
		{
			return Result;
		}

		Card = *SlotCard;
	}
	else
	{
		if (DeckState == EDeckState::EmptyShuffling || !ActiveSequence.IsValidIndex(CurrentIndex))
		{
			return Result;
		}

		Card = ActiveSequence[CurrentIndex];
	}

	Result.bReleaseModeMatched = DoesReleaseModeMatch(Card.Config, Result.ActionContext);
	Result.bActionSlotMatched = DoesActionSlotMatch(Card.Config.RequiredActionSlot, Result.ActionContext.ActionSlot);
	if (Result.ActionContext.FlowRole == ECombatDeckFlowRole::Any
		&& Card.Config.RequiredFlowRole != ECombatDeckFlowRole::Any)
	{
		Result.ActionContext.FlowRole = Card.Config.RequiredFlowRole;
	}
	Result.bFlowRoleMatched = DoesFlowRoleMatch(Card.Config.RequiredFlowRole, Result.ActionContext.FlowRole);

	// OnCommit path: runs PreCommitFlow (e.g. trail setup) before the card resolves.
	// CurrentIndex is NOT advanced so the same card is still available for OnHit resolution.
	if (Result.ActionContext.TriggerTiming == ECombatCardTriggerTiming::OnCommit && !Result.ActionContext.bConsumeOnCommit)
	{
		if (!Result.bReleaseModeMatched || !Result.bActionSlotMatched || !Result.bFlowRoleMatched)
		{
			return Result;
		}

		if (Card.Config.PreCommitFlow)
		{
			// Use a derived GUID so BaseFlow (keyed at Card.InstanceGuid) never stops
			// or overwrites this flow when bRestartExistingFlow fires at OnHit time.
			FCombatCardInstance PreCommitCard = Card;
			PreCommitCard.InstanceGuid = CombatDeck_PreCommitGuid(Card.InstanceGuid);
			ExecuteFlow(Card.Config.PreCommitFlow, PreCommitCard, Result.ActionContext, Result);
			// Do not set Result.bHadCard: this is pre-montage setup, not a card resolve.
			Result.bHadCard = false;
		}
		return Result;
	}

	const bool bAllowOnCommitCardAtHitNotify =
		Result.ActionContext.TriggerTiming == ECombatCardTriggerTiming::OnHit
		&& Card.Config.TriggerTiming == ECombatCardTriggerTiming::OnCommit;
	if (Card.Config.TriggerTiming != Result.ActionContext.TriggerTiming && !bAllowOnCommitCardAtHitNotify)
	{
		return Result;
	}

	const float CurrentCardComboMultiplier = GetComboEffectMultiplier(Card.Config, Result.ActionContext);
	Result.bHadCard = true;
	Result.ResolvedCard = Card;
	Result.ConsumedCard = Card;
	if (!Result.bReleaseModeMatched)
	{
		if (PendingLinkContext.IsValidCard())
		{
			BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Result.ActionContext);
		}

		if (Result.ActionContext.AttackInstanceGuid.IsValid())
		{
			ResolvedAttackGuids.Add(Result.ActionContext.AttackInstanceGuid);
		}

		return Result;
	}

	const bool bLegacyActionMatched = !IsLinkCardType(Card.Config.CardType)
		? true
		: DoesActionMatch(Card.Config.RequiredAction, Result.ActionContext.ActionType);
	Result.bActionMatched = bLegacyActionMatched && Result.bActionSlotMatched && Result.bFlowRoleMatched;
	SetAppliedMultiplier(Result, 1.0f, CurrentCardComboMultiplier);
	Result.ReasonText = Card.Config.HUDReasonText;

	if (!Result.bActionSlotMatched || !Result.bFlowRoleMatched)
	{
		if (PendingLinkContext.IsValidCard())
		{
			BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Result.ActionContext);
		}

		return Result;
	}

	int32 RequiredBattles = 0;
	int32 CurrentBattles = 0;
	if (IsTemporaryFinisherLocked(Card, RequiredBattles, CurrentBattles))
	{
		BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Result.ActionContext);
		Result.bActionMatched = false;
		Result.bCardTemporarilyLocked = true;
		Result.TemporaryUnlockRequiredCompletedBattles = RequiredBattles;
		Result.TemporaryUnlockCurrentCompletedBattles = CurrentBattles;
		Result.ReasonText = TemporaryFinisherLockedReasonText;

		if (Result.ActionContext.AttackInstanceGuid.IsValid())
		{
			ResolvedAttackGuids.Add(Result.ActionContext.AttackInstanceGuid);
		}

		if (!bUseSingleActionSlot)
		{
			AdvanceCombatSequenceAfterUse();
		}
		Result.bStartedShuffle = false;

		OnCardResolved.Broadcast(Card, Result);
		OnCardConsumed.Broadcast(Card, Result);

		PushCombatCardResolveLog(Result);
		return Result;
	}

	if (PendingLinkContext.IsValidCard())
	{
		const FCombatCardLinkRecipe* ReversedRecipe = FindMatchingLinkRecipe(PendingLinkContext, ECombatCardLinkOrientation::Reversed, Card, Result.ActionContext);
		const bool bLegacyBackwardMatched = !ReversedRecipe && PendingLinkContext.Config.LinkRecipes.IsEmpty()
			&& DoesLinkConditionMatch(PendingLinkContext.Config.LinkConfig.BackwardEffect.Condition, Card, Result.ActionContext);
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
			ExecuteFlow(ReversedRecipe ? ReversedRecipe->LinkFlow : PendingLinkContext.Config.LinkConfig.BackwardEffect.Flow, PendingLinkContext, Result.ActionContext, Result);
			PendingLinkContext = FCombatCardInstance();
			PendingLinkActionContext = FCombatDeckActionContext();
		}
		else
		{
			BreakPendingLink(ECombatLinkBreakReason::ConditionFailed, &Result.ActionContext);
		}
	}

	const bool bIsFinisherRelease = Result.ActionContext.ReleaseMode == ECombatCardReleaseMode::Finisher || Result.ActionContext.bIsComboFinisher;
	const bool bComboRequirementSatisfied = !Card.Config.bRequiresComboFinisher || bIsFinisherRelease;
	const bool bCardUsesFinisherRelease = Card.Config.CardType == ECombatCardType::Finisher || Card.Config.bRequiresComboFinisher;
	Result.bTriggeredFinisher = Result.bActionMatched
		&& bComboRequirementSatisfied
		&& bCardUsesFinisherRelease
		&& !IsDeprecatedFinisherCardConfig(Card.Config);

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
				const bool bHasReversedRecipe = Card.Config.LinkRecipes.ContainsByPredicate([](const FCombatCardLinkRecipe& Recipe)
					{
						return Recipe.Direction == ECombatCardLinkOrientation::Reversed && Recipe.LinkFlow;
					});
				if (bHasReversedRecipe)
				{
					Result.bTriggeredLink = true;
					Result.bPendingBackwardLink = true;
					Result.LinkedSourceCard = Card;
					PendingLinkContext = Card;
					PendingLinkActionContext = Result.ActionContext;
				}
				else
				{
					Result.bTriggeredBaseFlow = true;
					ExecuteFlow(Card.Config.BaseFlow, Card, Result.ActionContext, Result);
				}
			}
			else
			{
				const FCombatCardLinkRecipe* ForwardRecipe = FindMatchingLinkRecipe(Card, ECombatCardLinkOrientation::Forward, LastResolvedCard, Result.ActionContext);

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
					ExecuteFlow(ForwardRecipe->LinkFlow, Card, Result.ActionContext, Result);
				}
				else
				{
					Result.bTriggeredBaseFlow = true;
					ExecuteFlow(Card.Config.BaseFlow, Card, Result.ActionContext, Result);
				}
			}
		}
		else if (bCanUseLinkConfig)
		{
			const bool bForwardMatched = WantsForwardLink(Card.Config)
				&& Card.Config.LinkConfig.ForwardEffect.Flow
				&& LastResolvedCard.IsValidCard()
				&& DoesLinkConditionMatch(Card.Config.LinkConfig.ForwardEffect.Condition, LastResolvedCard, Result.ActionContext);

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
				ExecuteFlow(Card.Config.LinkConfig.ForwardEffect.Flow, Card, Result.ActionContext, Result);
			}
			else
			{
				Result.bTriggeredBaseFlow = true;
				ExecuteFlow(Card.Config.BaseFlow, Card, Result.ActionContext, Result);
			}

			if (!bForwardMatched && WantsBackwardLink(Card.Config) && Card.Config.LinkConfig.BackwardEffect.Flow)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
				PendingLinkActionContext = Result.ActionContext;
			}
		}
		else
		{
			Result.bTriggeredBaseFlow = true;
			ExecuteFlow(Card.Config.BaseFlow, Card, Result.ActionContext, Result);

			Result.bTriggeredMatchedFlow = true;
			ExecuteFlow(Card.Config.MatchedFlow, Card, Result.ActionContext, Result);

			if (Card.Config.LinkMode == ECardLinkMode::PassToNext)
			{
				Result.bTriggeredLink = true;
				Result.bPendingBackwardLink = true;
				Result.LinkedSourceCard = Card;
				PendingLinkContext = Card;
				PendingLinkActionContext = Result.ActionContext;
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
		ExecuteFlow(Card.Config.BaseFlow, Card, Result.ActionContext, Result);
	}

	LastResolvedCard = Card;
	if (Result.ActionContext.AttackInstanceGuid.IsValid())
	{
		ResolvedAttackGuids.Add(Result.ActionContext.AttackInstanceGuid);
	}

	if (!bUseSingleActionSlot)
	{
		AdvanceCombatSequenceAfterUse();
	}
	Result.bStartedShuffle = false;

	OnCardResolved.Broadcast(Card, Result);
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
		RegisterTriggeredFinisherCard(Card);
		OnFinisherTriggered.Broadcast(Result);
	}

	// Push a pure card resolve row to the combat log.
	PushCombatCardResolveLog(Result);

	return Result;
}

void UCombatDeckComponent::StopCardFlow(const FCombatCardInstance& Card)
{
	if (!Card.IsValidCard())
	{
		return;
	}

	ReleaseTriggeredFinisherCard(Card);

	if (UBuffFlowComponent* BuffFlowComponent = GetOwner() ? GetOwner()->FindComponentByClass<UBuffFlowComponent>() : nullptr)
	{
		// Always stop the pre-commit flow slot first (no-op if not present).
		BuffFlowComponent->StopBuffFlow(CombatDeck_PreCommitGuid(Card.InstanceGuid));

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
	ResetCombatSequenceProgress(ECombatLinkBreakReason::ComboStateExited);
}

void UCombatDeckComponent::LoadDeckFromWeapon(const UWeaponDefinition* WeaponDefinition)
{
	if (WeaponDefinition)
	{
		TArray<URuneDataAsset*> SourceAssets;
		BuildDefaultWeaponDeckSourceAssets(WeaponDefinition, SourceAssets);
		LoadDeckFromSourceAssetsInternal(SourceAssets, 0.0f, 0, false);
		return;
	}

	LoadDeckFromSourceAssetsInternal({}, ShuffleCooldownDuration, MaxActiveSequenceSize, false);
}

void UCombatDeckComponent::BuildDefaultWeaponDeckSourceAssets(const UWeaponDefinition* WeaponDefinition, TArray<URuneDataAsset*>& OutSourceAssets) const
{
	OutSourceAssets.Reset();
	if (!WeaponDefinition)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UYogSaveSubsystem* SaveSys = GameInstance->GetSubsystem<UYogSaveSubsystem>();
				SaveSys && SaveSys->IsFirstRunTutorialCompleted())
			{
				UFirstRunTutorialDirectorSubsystem::BuildDefaultPostTutorialDeck(OutSourceAssets);
			}
		}
	}

	if (OutSourceAssets.IsEmpty())
	{
		CopyDeckSourceAssets(GetDefaultWeaponDeckSource(WeaponDefinition), OutSourceAssets);
	}
}

void UCombatDeckComponent::LoadDeckFromSourceAssets(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize)
{
	LoadDeckFromSourceAssetsInternal(SourceAssets, InShuffleCooldownDuration, InMaxActiveSequenceSize, false);
}

void UCombatDeckComponent::LoadDeckFromExactSourceAssets(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize)
{
	LoadDeckFromSourceAssetsInternal(SourceAssets, InShuffleCooldownDuration, InMaxActiveSequenceSize, false);
}

void UCombatDeckComponent::LoadDeckFromSourceAssetsInternal(const TArray<URuneDataAsset*>& SourceAssets, float InShuffleCooldownDuration, int32 InMaxActiveSequenceSize, bool bAppendTemporaryFinisherCard)
{
	StopAllCardPassiveFlows();

	DeckList.Reset();
	ClearSingleActionSlotCards();
	// Current combat cards are not consumed and do not use shuffle downtime or active-subset limits.
	// The parameters remain on the API so older saves/Blueprint calls still load safely.
	ShuffleCooldownDuration = 0.0f;
	MaxActiveSequenceSize = 0;

	TArray<URuneDataAsset*> EffectiveSourceAssets = SourceAssets;
	if (bAppendTemporaryFinisherCard)
	{
		AppendTemporaryInitialFinisherCard(EffectiveSourceAssets);
	}

	for (URuneDataAsset* RuneAsset : EffectiveSourceAssets)
	{
		const FCombatCardInstance Card = MakeCardFromRune(RuneAsset, CombatDeckOwnerSourceWeapon);
		if (Card.IsValidCard())
		{
			RouteCardToActionSlot(Card);
		}
	}
	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();
	FinisherCardsWaitingForEffectEnd.Reset();
	PruneDeprecatedFinisherCards();
	RefillActiveSequence();
	RefreshCardPassiveFlows();
}

TArray<URuneDataAsset*> UCombatDeckComponent::GetDeckSourceAssets() const
{
	TArray<URuneDataAsset*> SourceAssets;
	SourceAssets.Reserve(DeckList.Num() + 3);
	for (const FCombatCardInstance& Card : DeckList)
	{
		if (Card.SourceData && !IsDeprecatedFinisherCardConfig(Card.Config))
		{
			SourceAssets.Add(Card.SourceData);
		}
	}

	TArray<FCombatCardInstance> SingleSlotCards;
	AppendSingleActionSlotCards(SingleSlotCards);
	for (const FCombatCardInstance& Card : SingleSlotCards)
	{
		if (Card.SourceData && !IsDeprecatedFinisherCardConfig(Card.Config))
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
	if (DeckState != EDeckState::Ready)
	{
		return {};
	}

	return BuildTemporaryLockViewCards(ActiveSequence);
}

FCombatCardInstance UCombatDeckComponent::GetActionSlotCardSnapshot(ECombatDeckActionSlot Slot) const
{
	if (const FCombatCardInstance* SlotCard = GetSingleActionSlotCard(Slot))
	{
		return BuildTemporaryLockViewCard(*SlotCard);
	}

	if (Slot == ECombatDeckActionSlot::Attack && DeckState == EDeckState::Ready && ActiveSequence.IsValidIndex(CurrentIndex))
	{
		return BuildTemporaryLockViewCard(ActiveSequence[CurrentIndex]);
	}

	return FCombatCardInstance();
}

void UCombatDeckComponent::RefreshDeckView()
{
	if (DeckState != EDeckState::Ready)
	{
		return;
	}

	OnDeckLoaded.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
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

	if (StoreCardInSingleActionSlot(Card, true))
	{
		OnRewardAddedToDeck.Broadcast(Card);
		TArray<FCombatCardInstance> EnteredCards;
		EnteredCards.Add(Card);
		OnDeckCardsEntered.Broadcast(EnteredCards);
		StartPassiveFlowsForCard(Card);
		RefreshDeckView();
		return true;
	}

	const int32 InsertIndex = MaxActiveSequenceSize > 0
		? FMath::Clamp(MaxActiveSequenceSize - 1, 0, DeckList.Num())
		: DeckList.Num();
	DeckList.Insert(Card, InsertIndex);
	OnRewardAddedToDeck.Broadcast(Card);
	TArray<FCombatCardInstance> EnteredCards;
	EnteredCards.Add(Card);
	OnDeckCardsEntered.Broadcast(EnteredCards);
	StartPassiveFlowsForCard(Card);

	StartDeckEditReload();

	return true;
}

bool UCombatDeckComponent::AddCardFromRuneShop(URuneDataAsset* RuneAsset)
{
	const FCombatCardInstance Card = MakeCardFromRune(RuneAsset, CombatDeckOwnerSourceShop);
	if (!Card.IsValidCard())
	{
		return false;
	}

	if (StoreCardInSingleActionSlot(Card, true))
	{
		OnRewardAddedToDeck.Broadcast(Card);
		TArray<FCombatCardInstance> EnteredCards;
		EnteredCards.Add(Card);
		OnDeckCardsEntered.Broadcast(EnteredCards);
		StartPassiveFlowsForCard(Card);
		RefreshDeckView();
		return true;
	}

	DeckList.Add(Card);
	OnRewardAddedToDeck.Broadcast(Card);
	TArray<FCombatCardInstance> EnteredCards;
	EnteredCards.Add(Card);
	OnDeckCardsEntered.Broadcast(EnteredCards);
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
	(void)InDuration;
	ShuffleCooldownDuration = 0.0f;
}

void UCombatDeckComponent::SetDeckListForTest(const TArray<FCombatCardConfig>& InCards)
{
	StopAllCardPassiveFlows();

	DeckList.Reset();
	ClearSingleActionSlotCards();
	for (const FCombatCardConfig& Config : InCards)
	{
		if (IsDeprecatedFinisherCardConfig(Config))
		{
			continue;
		}

		FCombatCardInstance Card;
		Card.InstanceGuid = FGuid::NewGuid();
		Card.Config = Config;
		Card.LinkOrientation = Config.DefaultLinkOrientation;
		RouteCardToActionSlot(Card);
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();
	FinisherCardsWaitingForEffectEnd.Reset();
	RefillActiveSequence();
}

void UCombatDeckComponent::AdvanceShuffleForTest(float DeltaTime)
{
	AdvanceShuffle(DeltaTime);
}

bool UCombatDeckComponent::IsCardSuppressedFromActiveSequenceForTest(const FGuid& CardGuid) const
{
	return FinisherCardsWaitingForEffectEnd.Contains(CardGuid);
}

FCombatCardInstance UCombatDeckComponent::MakeCardFromRune(URuneDataAsset* RuneAsset, FName OwnerSource) const
{
	FCombatCardInstance Card;
	if (!RuneAsset || !RuneAsset->RuneInfo.CombatCard.bIsCombatCard)
	{
		return Card;
	}

	if (IsDeprecatedFinisherCardConfig(RuneAsset->RuneInfo.CombatCard))
	{
		return Card;
	}

	Card.InstanceGuid = FGuid::NewGuid();
	Card.SourceData = RuneAsset;
	Card.Level = RuneAsset->RuneInfo.Level;
	Card.OwnerSource = OwnerSource;
	Card.Config = RuneAsset->RuneInfo.CombatCard;
	Card.LinkOrientation = Card.Config.DefaultLinkOrientation;

	const bool bIsWeaponSkillFinisherCard =
		CombatDeckCardHasId(Card.Config, TEXT("Buff.WeaponSkillFinisher"))
		|| CombatDeckCardHasEffect(Card.Config, TEXT("Buff.Detonate"))
		|| CombatDeckCardHasId(Card.Config, TEXT("Rune.ID.WeaponSkillFinisher"))
		|| CombatDeckCardHasEffect(Card.Config, TEXT("Rune.Effect.Detonate"))
		|| CombatDeckCardHasId(Card.Config, TEXT("Rune.ID.Heavy"))
		|| CombatDeckCardHasId(Card.Config, TEXT("Card.ID.Heavy"))
		|| CombatDeckCardHasEffect(Card.Config, TEXT("Rune.Effect.Heavy"))
		|| CombatDeckCardHasEffect(Card.Config, TEXT("Card.Effect.Heavy"));
	if (bIsWeaponSkillFinisherCard)
	{
		AddCombatCardEffect(Card.Config, TEXT("Buff.Attack"));
	}

	if (Card.Config.DisplayName.IsEmpty())
	{
		Card.Config.DisplayName = FText::FromName(RuneAsset->GetRuneName());
	}

	return Card;
}

bool UCombatDeckComponent::RouteCardToActionSlot(const FCombatCardInstance& Card)
{
	if (!Card.IsValidCard())
	{
		return false;
	}

	if (StoreCardInSingleActionSlot(Card, false))
	{
		return true;
	}

	DeckList.Add(Card);
	return true;
}

bool UCombatDeckComponent::StoreCardInSingleActionSlot(const FCombatCardInstance& Card, bool bStopExistingPassiveFlows)
{
	FCombatCardInstance* SlotCard = GetMutableSingleActionSlotCard(Card.Config.RequiredActionSlot);
	if (!SlotCard)
	{
		return false;
	}

	if (bStopExistingPassiveFlows && SlotCard->IsValidCard())
	{
		StopPassiveFlowsForCard(SlotCard->InstanceGuid);
	}

	*SlotCard = Card;
	return true;
}

void UCombatDeckComponent::ClearSingleActionSlotCards()
{
	SkillSlotCard = FCombatCardInstance();
	WeaponSkillSlotCard = FCombatCardInstance();
	DashSlotCard = FCombatCardInstance();
}

bool UCombatDeckComponent::IsSingleActionSlot(ECombatDeckActionSlot Slot) const
{
	return Slot == ECombatDeckActionSlot::Skill
		|| Slot == ECombatDeckActionSlot::WeaponSkill
		|| Slot == ECombatDeckActionSlot::Dash;
}

FCombatCardInstance* UCombatDeckComponent::GetMutableSingleActionSlotCard(ECombatDeckActionSlot Slot)
{
	switch (Slot)
	{
	case ECombatDeckActionSlot::Skill:
		return &SkillSlotCard;
	case ECombatDeckActionSlot::WeaponSkill:
		return &WeaponSkillSlotCard;
	case ECombatDeckActionSlot::Dash:
		return &DashSlotCard;
	default:
		return nullptr;
	}
}

const FCombatCardInstance* UCombatDeckComponent::GetSingleActionSlotCard(ECombatDeckActionSlot Slot) const
{
	switch (Slot)
	{
	case ECombatDeckActionSlot::Skill:
		return SkillSlotCard.IsValidCard() ? &SkillSlotCard : nullptr;
	case ECombatDeckActionSlot::WeaponSkill:
		return WeaponSkillSlotCard.IsValidCard() ? &WeaponSkillSlotCard : nullptr;
	case ECombatDeckActionSlot::Dash:
		return DashSlotCard.IsValidCard() ? &DashSlotCard : nullptr;
	default:
		return nullptr;
	}
}

void UCombatDeckComponent::AppendSingleActionSlotCards(TArray<FCombatCardInstance>& Cards) const
{
	if (SkillSlotCard.IsValidCard())
	{
		Cards.Add(SkillSlotCard);
	}
	if (WeaponSkillSlotCard.IsValidCard())
	{
		Cards.Add(WeaponSkillSlotCard);
	}
	if (DashSlotCard.IsValidCard())
	{
		Cards.Add(DashSlotCard);
	}
}

void UCombatDeckComponent::AppendTemporaryInitialFinisherCard(TArray<URuneDataAsset*>& SourceAssets) const
{
	if (!bGrantTemporaryInitialFinisherCard)
	{
		return;
	}

	if (DevKit::Combat::IsFinisherAbilityDeprecated())
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
	OutCurrentBattles = 0;
	OutRequiredBattles = 0;
	return false;
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

void UCombatDeckComponent::PushCombatCardResolveLog(const FCombatCardResolveResult& Result) const
{
	if (!Result.ResolvedCard.IsValidCard())
	{
		return;
	}

	FDamageBreakdown Resolve;
	Resolve.bIsCardEventOnly = true;
	Resolve.bHadCard = true;
	Resolve.bResolvedCard = true;
	Resolve.bConsumedCard = false;
	Resolve.bActionMatched = Result.bActionMatched;
	Resolve.bTriggeredMatchedFlow = Result.bTriggeredMatchedFlow;
	Resolve.bTriggeredLink = Result.bTriggeredLink || Result.bTriggeredForwardLink || Result.bTriggeredBackwardLink;
	Resolve.bTriggeredFinisher = Result.bTriggeredFinisher;
	Resolve.bStartedShuffle = Result.bStartedShuffle;
	Resolve.CardDisplayName = Result.ResolvedCard.SourceData
		? Result.ResolvedCard.SourceData->GetRuneName()
		: Result.ResolvedCard.Config.CardIdTag.GetTagName();
	Resolve.CardResolveTiming = Result.ResolvedCard.Config.TriggerTiming == ECombatCardTriggerTiming::OnCommit
		? FName("OnCommit") : FName("OnHit");
	Resolve.CardConsumeTiming = Resolve.CardResolveTiming;
	Resolve.SourceName = GetNameSafe(GetOwner());
	Resolve.GameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Resolve.DamageType = Result.bCardTemporarilyLocked
		? FName("Card_Locked")
		: (Result.bStartedShuffle ? FName("Card_Shuffle") : FName("Card_Resolve"));
	UCombatLogStatics::PushEntry(Resolve);
}

void UCombatDeckComponent::RefillActiveSequence()
{
	ActiveSequence.Reset();
	CurrentIndex = 0;

	const int32 DesiredCount = MaxActiveSequenceSize > 0
		? FMath::Min(MaxActiveSequenceSize, DeckList.Num())
		: DeckList.Num();

	for (const FCombatCardInstance& Card : DeckList)
	{
		if (ActiveSequence.Num() >= DesiredCount)
		{
			break;
		}

		if (ShouldSkipActiveSequenceRefillCard(Card))
		{
			continue;
		}

		ActiveSequence.Add(Card);
	}

	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	OnDeckLoaded.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
}

void UCombatDeckComponent::RegisterTriggeredFinisherCard(const FCombatCardInstance& Card)
{
	if (DevKit::Combat::IsFinisherAbilityDeprecated())
	{
		return;
	}

	if (Card.Config.CardType == ECombatCardType::Finisher && Card.InstanceGuid.IsValid())
	{
		FinisherCardsWaitingForEffectEnd.Add(Card.InstanceGuid);
	}
}

void UCombatDeckComponent::ReleaseTriggeredFinisherCard(const FCombatCardInstance& Card)
{
	if (Card.InstanceGuid.IsValid())
	{
		FinisherCardsWaitingForEffectEnd.Remove(Card.InstanceGuid);
	}
}

bool UCombatDeckComponent::ShouldSkipActiveSequenceRefillCard(const FCombatCardInstance& Card) const
{
	if (IsDeprecatedFinisherCardConfig(Card.Config))
	{
		return true;
	}

	return Card.Config.CardType == ECombatCardType::Finisher
		&& Card.InstanceGuid.IsValid()
		&& FinisherCardsWaitingForEffectEnd.Contains(Card.InstanceGuid);
}

void UCombatDeckComponent::PruneDeprecatedFinisherCards()
{
	if (!DevKit::Combat::IsFinisherAbilityDeprecated())
	{
		return;
	}

	DeckList.RemoveAll([this](const FCombatCardInstance& Card)
	{
		if (IsDeprecatedFinisherCardConfig(Card.Config))
		{
			StopPassiveFlowsForCard(Card.InstanceGuid);
			return true;
		}

		return false;
	});

	ActiveSequence.RemoveAll([](const FCombatCardInstance& Card)
	{
		return IsDeprecatedFinisherCardConfig(Card.Config);
	});

	auto PruneSingleSlotCard = [this](FCombatCardInstance& Card)
	{
		if (IsDeprecatedFinisherCardConfig(Card.Config))
		{
			StopPassiveFlowsForCard(Card.InstanceGuid);
			Card = FCombatCardInstance();
		}
	};
	PruneSingleSlotCard(SkillSlotCard);
	PruneSingleSlotCard(WeaponSkillSlotCard);
	PruneSingleSlotCard(DashSlotCard);

	FinisherCardsWaitingForEffectEnd.Reset();
}

void UCombatDeckComponent::RefreshCardPassiveFlows()
{
	StopAllCardPassiveFlows();

	for (const FCombatCardInstance& Card : DeckList)
	{
		StartPassiveFlowsForCard(Card);
	}

	TArray<FCombatCardInstance> SingleSlotCards;
	AppendSingleActionSlotCards(SingleSlotCards);
	for (const FCombatCardInstance& Card : SingleSlotCards)
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
	ResolvedAttackGuids.Reset();
	DeckState = EDeckState::Ready;
	ShuffleCooldownRemaining = 0.0f;
	RefillActiveSequence();
}

void UCombatDeckComponent::ResetCombatSequenceProgress(ECombatLinkBreakReason Reason, const FCombatDeckActionContext* BreakContext)
{
	BreakPendingLink(Reason, BreakContext);
	LastResolvedCard = FCombatCardInstance();
	PendingLinkContext = FCombatCardInstance();
	PendingLinkActionContext = FCombatDeckActionContext();
	ResolvedAttackGuids.Reset();
	CurrentIndex = 0;

	if (DeckState == EDeckState::Ready)
	{
		OnDeckLoaded.Broadcast(BuildTemporaryLockViewCards(ActiveSequence));
	}
}

void UCombatDeckComponent::AdvanceCombatSequenceAfterUse()
{
	if (ActiveSequence.IsEmpty())
	{
		CurrentIndex = 0;
		return;
	}

	CurrentIndex = FMath::Clamp(CurrentIndex + 1, 0, ActiveSequence.Num());
}

void UCombatDeckComponent::StartDeckEditReload()
{
	UE_LOG(LogTemp, Display, TEXT("[CombatDeckEdit] RefreshSequenceWithoutShuffle FullCooldown=%.3f DeckCount=%d"),
		ShuffleCooldownDuration,
		DeckList.Num());
	ResetRuntimeStateAfterDeckEdit();
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
	(void)Context;
	// Deprecated combat-card combo scaling is disabled for the current
	// Attack/Skill/WeaponSkill/Dash model. Card order, FlowRole, and Link
	// recipes now express build sequencing.
	return 0;
}

float UCombatDeckComponent::GetComboEffectMultiplier(const FCombatCardConfig& Config, const FCombatDeckActionContext& Context) const
{
	(void)Config;
	(void)Context;
	return 1.0f;
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
	EffectContext.ActionSlot = Context.ActionSlot;
	EffectContext.FlowRole = Context.FlowRole;
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
	Result.ResolvedCard = BrokenCard;
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
	return RequiredAction == ECardRequiredAction::Any
		|| ActionType == ECardRequiredAction::Any
		|| RequiredAction == ActionType;
}

bool UCombatDeckComponent::DoesActionSlotMatch(ECombatDeckActionSlot RequiredSlot, ECombatDeckActionSlot ActionSlot) const
{
	return RequiredSlot == ECombatDeckActionSlot::Any || RequiredSlot == ActionSlot;
}

bool UCombatDeckComponent::DoesFlowRoleMatch(ECombatDeckFlowRole RequiredRole, ECombatDeckFlowRole FlowRole) const
{
	return RequiredRole == ECombatDeckFlowRole::Any || RequiredRole == FlowRole;
}

bool UCombatDeckComponent::DoesReleaseModeMatch(const FCombatCardConfig& Config, const FCombatDeckActionContext& Context) const
{
	const bool bCardRequiresFinisherRelease = Config.CardType == ECombatCardType::Finisher || Config.bRequiresComboFinisher;
	const bool bContextIsFinisherRelease = Context.ReleaseMode == ECombatCardReleaseMode::Finisher || Context.bIsComboFinisher;
	return bCardRequiresFinisherRelease == bContextIsFinisherRelease;
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

	if (!DoesActionSlotMatch(Condition.RequiredActionSlot, Context.ActionSlot))
	{
		return false;
	}

	if (!DoesFlowRoleMatch(Condition.RequiredFlowRole, Context.FlowRole))
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
			NeighborIdTags.AddTag(GetFormalCombatCardTag(NeighborCard.Config.CardIdTag));
		}
		if (!ContainerHasAllTagsOrEquivalent(NeighborIdTags, Condition.RequiredNeighborIdTags))
		{
			return false;
		}
	}

	if (!Condition.RequiredNeighborEffectTags.IsEmpty()
		&& !ContainerHasAllTagsOrEquivalent(NeighborCard.Config.CardEffectTags, Condition.RequiredNeighborEffectTags))
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
