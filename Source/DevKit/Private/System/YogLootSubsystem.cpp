#include "System/YogLootSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Data/RewardLootTableDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "Data/WeaponSkillDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "Story/StoryEngineSubsystem.h"
#include "System/YogRunEconomySubsystem.h"
#include "System/YogSettings.h"

FGameplayTag UYogLootSubsystem::ResolveMaterialCurrencyTag(FGameplayTag ConfiguredTag)
{
	if (ConfiguredTag.IsValid())
	{
		return ConfiguredTag;
	}

	return FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.Common.A"), false);
}

int32 UYogLootSubsystem::ResolveOptionCount(const URewardLootTableDataAsset& Table) const
{
	int32 OptionCount = FMath::Clamp(Table.DefaultOptionCount, 1, 3);
	if (!Table.SingleChoiceTag.IsValid())
	{
		return OptionCount;
	}

	const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	const UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(Table.SingleChoiceTag))
	{
		OptionCount = FMath::Clamp(Table.SingleChoiceOptionCount, 1, 3);
	}
	return OptionCount;
}

TArray<FLootOption> UYogLootSubsystem::GenerateRuneOptions(const URewardLootTableDataAsset* Table, bool bAvoidRepeats)
{
	TArray<FLootOption> Options;

	if (!Table)
	{
		Table = UYogSettings::Get()->DefaultRewardLootTable.LoadSynchronous();
	}
	if (!Table || Table->RunePool.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Loot] No rune pool available. Assign DefaultRewardLootTable in Project Settings > Yog."));
		return Options;
	}

	TArray<URuneDataAsset*> Candidates;
	for (const TObjectPtr<URuneDataAsset>& Rune : Table->RunePool)
	{
		if (Rune && !Candidates.Contains(Rune.Get())
			&& !(bAvoidRepeats && OfferedRunes.Contains(Rune.Get())))
		{
			Candidates.Add(Rune.Get());
		}
	}

	for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
	{
		Candidates.Swap(Index, FMath::RandRange(0, Index));
	}

	const int32 Count = FMath::Min(ResolveOptionCount(*Table), Candidates.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FLootOption Option;
		Option.LootType = ELootType::Rune;
		Option.RuneAsset = Candidates[Index];
		Options.Add(Option);
		if (bAvoidRepeats)
		{
			OfferedRunes.Add(Candidates[Index]);
		}
	}
	return Options;
}

bool UYogLootSubsystem::GrantLoot(APlayerCharacterBase* Player, const FLootOption& Option)
{
	if (!Player)
	{
		return false;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	bool bGranted = false;

	switch (Option.LootType)
	{
	case ELootType::Rune:
		if (Option.RuneAsset)
		{
			Player->AddRuneToInventory(Option.RuneAsset->CreateInstance());
			bGranted = true;

			if (UStoryEngineSubsystem* StoryEngine = GameInstance ? GameInstance->GetSubsystem<UStoryEngineSubsystem>() : nullptr)
			{
				static const FGameplayTag RuneItemTag = FGameplayTag::RequestGameplayTag(TEXT("Story.Item.Rune"), false);
				StoryEngine->BroadcastStoryEventWithPayload(
					FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Item.Obtained"), false),
					RuneItemTag,
					FGameplayTag(),
					RuneItemTag,
					Player,
					Cast<APlayerController>(Player->GetController()));
			}
		}
		break;

	case ELootType::Gold:
		if (UYogRunEconomySubsystem* Economy = GameInstance ? GameInstance->GetSubsystem<UYogRunEconomySubsystem>() : nullptr)
		{
			Economy->AddGold(FMath::Max(1, Option.Amount));
			bGranted = true;
		}
		break;

	case ELootType::Material:
		{
			const FGameplayTag CurrencyTag = ResolveMaterialCurrencyTag(Option.MetaCurrencyTag);
			UYogMetaProgressionSubsystem* Meta = GameInstance ? GameInstance->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr;
			if (CurrencyTag.IsValid() && Meta)
			{
				Meta->AddCurrency(CurrencyTag, FMath::Max(1, Option.Amount));
				bGranted = true;
			}
		}
		break;

	case ELootType::WeaponSkill:
		bGranted = Option.WeaponSkillAsset
			&& Player->EquipWeaponSkillFromPickup(Option.WeaponSkillAsset, Option.WeaponSkillCharges);
		break;

	default:
		break;
	}

	if (bGranted)
	{
		OnLootSelected.Broadcast();
	}
	return bGranted;
}
