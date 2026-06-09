#include "Component/PlayerActiveSkillComponent.h"

#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"

UPlayerActiveSkillComponent::UPlayerActiveSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerActiveSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UPlayerActiveSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	bool bChanged = false;
	for (FActiveSkillRuntimeSlot& Slot : Slots)
	{
		if (Slot.CooldownRemaining > 0.0f)
		{
			Slot.CooldownRemaining = FMath::Max(0.0f, Slot.CooldownRemaining - FMath::Max(0.0f, DeltaTime));
			bChanged = true;
		}
	}

	if (bChanged)
	{
		BroadcastSlotsChanged();
	}
}

void UPlayerActiveSkillComponent::InitializeDefaultSlots()
{
	if (const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (const UYogMetaProgressionSubsystem* Meta = GI->GetSubsystem<UYogMetaProgressionSubsystem>())
		{
			static const FGameplayTag ActiveSkillFeature = FGameplayTag::RequestGameplayTag(TEXT("Feature.Combat.ActiveSkill"), false);
			static const FGameplayTag Slot2Feature = FGameplayTag::RequestGameplayTag(TEXT("Feature.Combat.ActiveSkill.Slot2"), false);
			if (ActiveSkillFeature.IsValid() && !Meta->IsFeatureUnlocked(ActiveSkillFeature))
			{
				UnlockedSlotCount = 0;
			}
			else if (Slot2Feature.IsValid() && Meta->IsFeatureUnlocked(Slot2Feature))
			{
				UnlockedSlotCount = 2;
			}
			else
			{
				UnlockedSlotCount = 1;
			}
		}
	}

	TArray<UActiveSkillDataAsset*> Skills;
	Skills.Reserve(DefaultSkillAssets.Num());
	for (UActiveSkillDataAsset* Asset : DefaultSkillAssets)
	{
		Skills.Add(Asset);
	}
	RebuildSlotsFromAssets(Skills);
}

void UPlayerActiveSkillComponent::SetSkillLoadout(const TArray<UActiveSkillDataAsset*>& InSkills)
{
	RebuildSlotsFromAssets(InSkills);
}

void UPlayerActiveSkillComponent::RebuildSlotsFromAssets(const TArray<UActiveSkillDataAsset*>& InSkills)
{
	Slots.Reset();

	const int32 SlotLimit = FMath::Max(1, MaxSlotCount);
	for (int32 Index = 0; Index < SlotLimit; ++Index)
	{
		FActiveSkillRuntimeSlot& Slot = Slots.AddDefaulted_GetRef();
		if (InSkills.IsValidIndex(Index) && InSkills[Index])
		{
			Slot.SkillAsset = InSkills[Index];
			Slot.Config = InSkills[Index]->Config;
		}
	}

	ActiveSlotIndex = Slots.IsValidIndex(ActiveSlotIndex) ? ActiveSlotIndex : 0;
	GrantSkillAbilities();
	BroadcastSlotsChanged();
}

void UPlayerActiveSkillComponent::GrantSkillAbilities()
{
	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	if (!ASC)
	{
		return;
	}

	for (const FActiveSkillRuntimeSlot& Slot : Slots)
	{
		if (!Slot.Config.AbilityClass)
		{
			continue;
		}

		bool bAlreadyGranted = false;
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->GetClass() == Slot.Config.AbilityClass)
			{
				bAlreadyGranted = true;
				break;
			}
		}

		if (!bAlreadyGranted)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(Slot.Config.AbilityClass, 1, INDEX_NONE, this));
		}
	}
}

bool UPlayerActiveSkillComponent::UseActiveSkill()
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No skill slot")));
		return false;
	}

	if (ActiveSlotIndex >= FMath::Clamp(UnlockedSlotCount, 0, FMath::Max(1, MaxSlotCount)))
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Skill slot locked")));
		return false;
	}

	FActiveSkillRuntimeSlot& Slot = Slots[ActiveSlotIndex];
	if (!Slot.SkillAsset || !Slot.Config.AbilityClass)
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("No skill equipped")));
		return false;
	}

	if (Slot.CooldownRemaining > 0.0f)
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Cooling down")));
		return false;
	}

	UYogAbilitySystemComponent* ASC = GetOwnerYogASC();
	if (!ASC)
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Missing ability system")));
		return false;
	}

	GrantSkillAbilities();
	if (!ASC->TryActivateAbilityByClass(Slot.Config.AbilityClass, true))
	{
		OnSkillUseFailed.Broadcast(ActiveSlotIndex, FText::FromString(TEXT("Cannot activate skill")));
		return false;
	}

	if (Slot.Config.bResolveCombatDeckOnUse)
	{
		if (APlayerCharacterBase* PlayerOwner = GetPlayerOwner())
		{
			if (PlayerOwner->CombatDeckComponent)
			{
				FCombatDeckActionContext Context;
				Context.ActionType = ECardRequiredAction::Any;
				Context.ActionSlot = Slot.Config.CombatDeckActionSlot;
				Context.FlowRole = Slot.Config.CombatDeckFlowRole;
				Context.WeaponDef = PlayerOwner->EquippedWeaponDef;
				Context.bIsComboFinisher = Context.FlowRole == ECombatDeckFlowRole::Finisher;
				Context.ReleaseMode = Context.bIsComboFinisher ? ECombatCardReleaseMode::Finisher : ECombatCardReleaseMode::Normal;
				Context.TriggerTiming = Slot.Config.CombatDeckTriggerTiming;
				Context.bConsumeOnCommit = Context.TriggerTiming == ECombatCardTriggerTiming::OnCommit;
				Context.AttackInstanceGuid = FGuid::NewGuid();

				if (const UYogGameplayAbility* AbilityCDO = Slot.Config.AbilityClass.GetDefaultObject())
				{
					for (const FGameplayTag& Tag : AbilityCDO->AbilityTags)
					{
						Context.AbilityTag = Tag;
						break;
					}
				}

				PlayerOwner->CombatDeckComponent->ResolveAttackCardWithContext(Context);
			}
		}
	}

	Slot.CooldownRemaining = FMath::Max(0.0f, Slot.Config.Cooldown);
	const TArray<FActiveSkillSlotView> Views = GetSlotViews();
	const FActiveSkillSlotView UsedSlot = Views.IsValidIndex(ActiveSlotIndex)
		? Views[ActiveSlotIndex]
		: FActiveSkillSlotView();
	OnSkillUsed.Broadcast(ActiveSlotIndex, UsedSlot);
	BroadcastSlotsChanged();
	return true;
}

void UPlayerActiveSkillComponent::SelectNextSkill()
{
	const int32 AvailableSlots = FMath::Clamp(UnlockedSlotCount, 0, Slots.Num());
	if (AvailableSlots <= 0)
	{
		return;
	}

	SetActiveSlotIndex((ActiveSlotIndex + 1) % AvailableSlots);
}

void UPlayerActiveSkillComponent::SetActiveSlotIndex(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || NewIndex >= FMath::Clamp(UnlockedSlotCount, 0, Slots.Num()) || ActiveSlotIndex == NewIndex)
	{
		return;
	}

	ActiveSlotIndex = NewIndex;
	BroadcastSlotsChanged();
}

void UPlayerActiveSkillComponent::SetUnlockedSlotCount(int32 NewUnlockedSlotCount)
{
	const int32 ClampedCount = FMath::Clamp(NewUnlockedSlotCount, 0, FMath::Max(1, MaxSlotCount));
	if (UnlockedSlotCount == ClampedCount)
	{
		return;
	}

	UnlockedSlotCount = ClampedCount;
	if (ActiveSlotIndex >= UnlockedSlotCount)
	{
		ActiveSlotIndex = 0;
	}
	BroadcastSlotsChanged();
}

TArray<FActiveSkillSlotView> UPlayerActiveSkillComponent::GetSlotViews() const
{
	TArray<FActiveSkillSlotView> Views;
	Views.Reserve(Slots.Num());

	const int32 AvailableSlots = FMath::Clamp(UnlockedSlotCount, 0, Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FActiveSkillRuntimeSlot& Slot = Slots[Index];
		FActiveSkillSlotView& View = Views.AddDefaulted_GetRef();
		View.SkillId = Slot.Config.SkillId;
		View.DisplayName = Slot.Config.DisplayName;
		View.Icon = Slot.Config.Icon;
		View.CooldownRemaining = Slot.CooldownRemaining;
		View.CooldownDuration = Slot.Config.Cooldown;
		View.bSelected = Index == ActiveSlotIndex;
		View.bLocked = Index >= AvailableSlots;
	}

	return Views;
}

TArray<UActiveSkillDataAsset*> UPlayerActiveSkillComponent::GetSkillLoadout() const
{
	TArray<UActiveSkillDataAsset*> Loadout;
	Loadout.Reserve(Slots.Num());
	for (const FActiveSkillRuntimeSlot& Slot : Slots)
	{
		Loadout.Add(Slot.SkillAsset.Get());
	}
	return Loadout;
}

void UPlayerActiveSkillComponent::BroadcastSlotsChanged() const
{
	OnSkillSlotsChanged.Broadcast(GetSlotViews());
}

APlayerCharacterBase* UPlayerActiveSkillComponent::GetPlayerOwner() const
{
	return Cast<APlayerCharacterBase>(GetOwner());
}

UYogAbilitySystemComponent* UPlayerActiveSkillComponent::GetOwnerYogASC() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? Cast<UYogAbilitySystemComponent>(OwnerActor->FindComponentByClass<UAbilitySystemComponent>()) : nullptr;
}
