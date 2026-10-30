// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemyTrait_SwarmFrenzy.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Controller/YogAIController.h"
#include "Data/EnemyData.h"
#include "Engine/World.h"
#include "GameModes/YogGameMode.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static bool EnemyTraitSwarmFrenzy_Matches(
	const UEnemyData* OwnerData,
	const AEnemyCharacterBase* Other,
	EEnemyTraitCountFilter Filter)
{
	if (Filter == EEnemyTraitCountFilter::AnyEnemy)
	{
		return true;
	}

	const UEnemyData* OtherData = Other ? Other->GetEnemyData() : nullptr;
	if (!OwnerData || !OtherData)
	{
		return false;
	}

	if (Filter == EEnemyTraitCountFilter::FactionTag)
	{
		return OwnerData->FactionTag.IsValid() && OwnerData->FactionTag == OtherData->FactionTag;
	}

	if (!OwnerData->EnemyId.IsNone())
	{
		return OwnerData->EnemyId == OtherData->EnemyId;
	}
	return OwnerData == OtherData;
}

UEnemyTrait_SwarmFrenzy::UEnemyTrait_SwarmFrenzy()
{
	Tiers.AddDefaulted();
}

void UEnemyTrait_SwarmFrenzy::ActivateTrait(AEnemyCharacterBase* Owner) const
{
	if (!IsValid(Owner) || !Owner->GetWorld())
	{
		return;
	}

	AYogGameMode* GM = Cast<AYogGameMode>(Owner->GetWorld()->GetAuthGameMode());
	if (!GM)
	{
		return;
	}

	FEnemyTraitRuntime& Runtime = Owner->FindOrAddTraitRuntime(this);
	if (Runtime.EnemyRegisteredHandle.IsValid())
	{
		return;
	}

	const TWeakObjectPtr<const UEnemyTrait_SwarmFrenzy> WeakThis(this);
	const TWeakObjectPtr<AEnemyCharacterBase> WeakOwner(Owner);
	auto OnRegistryChanged = [WeakThis, WeakOwner](AEnemyCharacterBase* /*Changed*/)
	{
		if (WeakThis.IsValid() && WeakOwner.IsValid())
		{
			WeakThis->ScheduleRecount(WeakOwner.Get());
		}
	};

	Runtime.EnemyRegisteredHandle = GM->OnEnemyRegisteredNative.AddWeakLambda(Owner, OnRegistryChanged);
	Runtime.EnemyUnregisteredHandle = GM->OnEnemyUnregisteredNative.AddWeakLambda(Owner, OnRegistryChanged);

	ScheduleRecount(Owner);

	Super::ActivateTrait(Owner);
}

void UEnemyTrait_SwarmFrenzy::DeactivateTrait(AEnemyCharacterBase* Owner) const
{
	if (!IsValid(Owner))
	{
		return;
	}

	FEnemyTraitRuntime* Runtime = Owner->FindTraitRuntime(this);
	if (!Runtime)
	{
		return;
	}

	if (UWorld* World = Owner->GetWorld())
	{
		if (AYogGameMode* GM = Cast<AYogGameMode>(World->GetAuthGameMode()))
		{
			GM->OnEnemyRegisteredNative.Remove(Runtime->EnemyRegisteredHandle);
			GM->OnEnemyUnregisteredNative.Remove(Runtime->EnemyUnregisteredHandle);
		}
	}

	SetTier(Owner, *Runtime, INDEX_NONE);
	Owner->RemoveTraitRuntime(this);

	Super::DeactivateTrait(Owner);
}

void UEnemyTrait_SwarmFrenzy::ScheduleRecount(AEnemyCharacterBase* Owner) const
{
	FEnemyTraitRuntime* Runtime = Owner ? Owner->FindTraitRuntime(this) : nullptr;
	if (!Runtime || Runtime->bRecountPending)
	{
		return;
	}
	Runtime->bRecountPending = true;

	// Deferred so the spawning enemy has finished BeginPlay (health initialized) and every AI controller
	// has started its StateTree, otherwise a Frenzy event sent this frame would be dropped.
	const TWeakObjectPtr<const UEnemyTrait_SwarmFrenzy> WeakThis(this);
	const TWeakObjectPtr<AEnemyCharacterBase> WeakOwner(Owner);
	Owner->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(Owner,
		[WeakThis, WeakOwner]()
		{
			if (!WeakThis.IsValid() || !WeakOwner.IsValid())
			{
				return;
			}

			if (FEnemyTraitRuntime* Pending = WeakOwner->FindTraitRuntime(WeakThis.Get()))
			{
				Pending->bRecountPending = false;
				WeakThis->Recount(WeakOwner.Get());
			}
		}));
}

void UEnemyTrait_SwarmFrenzy::Recount(AEnemyCharacterBase* Owner) const
{
	if (!IsValid(Owner) || !Owner->GetWorld() || !Owner->IsAlive())
	{
		return;
	}

	FEnemyTraitRuntime* Runtime = Owner->FindTraitRuntime(this);
	AYogGameMode* GM = Cast<AYogGameMode>(Owner->GetWorld()->GetAuthGameMode());
	if (!Runtime || !GM)
	{
		return;
	}

	const UEnemyData* OwnerData = Owner->GetEnemyData();
	int32 Count = 0;
	for (const AEnemyCharacterBase* Other : GM->GetAllAliveEnemies())
	{
		if (Other == Owner && !bCountSelf)
		{
			continue;
		}
		if (EnemyTraitSwarmFrenzy_Matches(OwnerData, Other, CountFilter))
		{
			++Count;
		}
	}

	int32 BestTier = INDEX_NONE;
	int32 BestMinCount = 0;
	for (int32 TierIndex = 0; TierIndex < Tiers.Num(); ++TierIndex)
	{
		const int32 MinCount = Tiers[TierIndex].MinCount;
		if (MinCount <= Count && MinCount > BestMinCount)
		{
			BestTier = TierIndex;
			BestMinCount = MinCount;
		}
	}

	SetTier(Owner, *Runtime, BestTier);
}

void UEnemyTrait_SwarmFrenzy::SetTier(AEnemyCharacterBase* Owner, FEnemyTraitRuntime& Runtime, int32 NewTier) const
{
	if (NewTier == Runtime.ActiveTier)
	{
		return;
	}

	static const FGameplayTag TAG_Frenzy = FGameplayTag::RequestGameplayTag(TEXT("Enemy.State.Frenzy"));
	static const FGameplayTag TAG_FrenzyEvent = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Event.Frenzy"));

	UYogAbilitySystemComponent* ASC = Owner->GetASC();
	if (ASC)
	{
		if (Runtime.TierEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Runtime.TierEffectHandle);
		}
		if (Runtime.ActiveCueTag.IsValid())
		{
			ASC->RemoveGameplayCue(Runtime.ActiveCueTag);
		}
	}
	Runtime.TierEffectHandle = FActiveGameplayEffectHandle();
	Runtime.ActiveCueTag = FGameplayTag();

	const bool bWasActive = Runtime.ActiveTier != INDEX_NONE;
	const bool bIsActive = Tiers.IsValidIndex(NewTier);
	Runtime.ActiveTier = bIsActive ? NewTier : INDEX_NONE;

	if (ASC && bWasActive != bIsActive)
	{
		if (bIsActive)
		{
			ASC->AddLooseGameplayTag(TAG_Frenzy);
		}
		else
		{
			ASC->RemoveLooseGameplayTag(TAG_Frenzy);
		}
	}

	if (!bIsActive)
	{
		return;
	}

	const FEnemyFrenzyTier& Tier = Tiers[NewTier];
	if (ASC && Tier.Effect)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(Owner);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Tier.Effect, 1.0f, EffectContext);
		if (Spec.IsValid())
		{
			Runtime.TierEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	if (ASC && Tier.CueTag.IsValid())
	{
		ASC->AddGameplayCue(Tier.CueTag);
		Runtime.ActiveCueTag = Tier.CueTag;
	}

	if (!Tier.bForceAttackOnEnter)
	{
		return;
	}

	AYogAIController* YogAI = Cast<AYogAIController>(Owner->GetController());
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Owner, 0);
	if (!YogAI || !PlayerPawn)
	{
		return;
	}

	YogAI->EnterCombat(PlayerPawn, false);
	YogAI->SendStateTreeEvent(TAG_FrenzyEvent);
}
