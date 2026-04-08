#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Abilities/PassiveAbility.h"
#include "GameplayEffect.h"

#include "SaveGame/YogSaveGame.h"
#include "Data/YogGameData.h"
#include "DevAssetManager.h"



// Sets default values
UYogAbilitySystemComponent::UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//
}




// =========================================================
// 状态冲突系统
// =========================================================

void UYogAbilitySystemComponent::InitConflictTable()
{
	ConflictMap.Reset();
	if (!ConflictTable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StateConflict] ConflictTable is null on %s, system disabled."), *GetNameSafe(GetOwner()));
		return;
	}

	for (const FStateConflictRule& Rule : ConflictTable->Rules)
	{
		if (!Rule.ActiveTag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[StateConflict] Rule with invalid ActiveTag found in %s, skipped."), *GetNameSafe(ConflictTable));
			continue;
		}
		ConflictMap.Add(Rule.ActiveTag, Rule);
	}

	UE_LOG(LogTemp, Log, TEXT("[StateConflict] Initialized %d rules on %s."), ConflictMap.Num(), *GetNameSafe(GetOwner()));
}

void UYogAbilitySystemComponent::SetConflictTable(UStateConflictDataAsset* NewTable)
{
	ConflictTable = NewTable;
	InitConflictTable();
}

void UYogAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);

	// 防递归：BlockAbilitiesWithTags 内部也会触发 OnTagUpdated
	if (bProcessingConflict)
		return;

	const FStateConflictRule* Rule = ConflictMap.Find(Tag);
	if (!Rule)
		return;

	TGuardValue<bool> Guard(bProcessingConflict, true);

	if (TagExists)
	{
		// Tag 加上 → Block + Cancel
		if (!Rule->BlockTags.IsEmpty())
			BlockAbilitiesWithTags(Rule->BlockTags);

		if (!Rule->CancelTags.IsEmpty())
			CancelAbilities(&Rule->CancelTags);
	}
	else
	{
		// Tag 移除 → 解除 Block
		if (!Rule->BlockTags.IsEmpty())
			UnBlockAbilitiesWithTags(Rule->BlockTags);
	}
}

// =========================================================

void UYogAbilitySystemComponent::ApplyAbilityData(UAbilityData* abilityData)
{


}

void UYogAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	//const TSubclassOf<UGameplayEffect> DynamicTagGE = UDevAssetManager::GetSubclass(UYogGameData::Get().DynamicTagGameplayEffect);
	//if (!DynamicTagGE)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UYogGameData::Get().DynamicTagGameplayEffect.GetAssetName());
	//	return;
	//}

	//const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	//FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	//if (!Spec)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
	//	return;
	//}

	//Spec->DynamicGrantedTags.AddTag(Tag);

	//ApplyGameplayEffectSpecToSelf(*Spec);
}

void UYogAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	//const TSubclassOf<UGameplayEffect> DynamicTagGE = UDevAssetManager::GetSubclass(ULyraGameData::Get().DynamicTagGameplayEffect);
	//if (!DynamicTagGE)
	//{
	//	UE_LOG(LogLyraAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *ULyraGameData::Get().DynamicTagGameplayEffect.GetAssetName());
	//	return;
	//}

	//FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	//Query.EffectDefinition = DynamicTagGE;

	//RemoveActiveEffects(Query);
}

void UYogAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
}



void UYogAbilitySystemComponent::GetOwnedGameplayTag()
{
	FGameplayTagContainer player_owned_tags;
	this->GetOwnedGameplayTags(player_owned_tags);

	int32 TagCount = player_owned_tags.Num();

	UE_LOG(LogTemp, Log, TEXT("Player has %d owned tags:"), TagCount);

	for (const FGameplayTag& Tag : player_owned_tags)
	{
		UE_LOG(LogTemp, Log, TEXT("  - %s"), *Tag.ToString());
	}
}

TMap<FGameplayTag, int32> UYogAbilitySystemComponent::GetPlayerOwnedTagsWithCounts()
{

	TMap<FGameplayTag, int32> TagCounts;

	FGameplayTagContainer OwnedTags;
	this->GetOwnedGameplayTags(OwnedTags);

	// Get stack count for each tag
	for (const FGameplayTag& Tag : OwnedTags)
	{
		int32 StackCount = this->GetGameplayTagCount(Tag);
		if (StackCount > 0)
		{
			TagCounts.Add(Tag, StackCount);
		}
	}

	return TagCounts;
}

void UYogAbilitySystemComponent::PrintPlayerOwnedTagsWithCounts(TMap<FGameplayTag, int32> TagCounts)
{
	for (const auto& Pair : TagCounts)
	{
		UE_LOG(LogTemp, Log, TEXT("Tag: %s, Count: %d"),
			*Pair.Key.ToString(),
			Pair.Value);
	}
}

void UYogAbilitySystemComponent::RemoveGameplayTagWithCount(FGameplayTag Tag, int32 Count)
{
	int stack = this->GetTagCount(Tag);

	if (stack <= 1)
	{
		this->RemoveLooseGameplayTag(Tag, 1);
	}
	else
	{
		this->SetLooseGameplayTagCount(Tag, stack - Count);
	}
}


void UYogAbilitySystemComponent::AddGameplayTagWithCount(FGameplayTag Tag, int32 Count)
{
	this->AddLooseGameplayTag(Tag, Count);
}


void UYogAbilitySystemComponent::ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	ReceivedDamage.Broadcast(SourceASC, Damage);

	// 广播前检查 TargetASC (this) 及其 Avatar 是否仍然有效
	// 同帧内多次命中已死亡角色（DoT/AoE）会导致 TargetASC pending kill，
	// Blueprint 侧的 GA_Passive_knockback 访问 pending kill 对象会报错
	if (SourceASC && IsValid(this) && IsValid(GetAvatarActor()))
	{
		SourceASC->DealtDamage.Broadcast(this, Damage);
	}
}

void UYogAbilitySystemComponent::AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock)
{
	this->AddLooseGameplayTags(TagsToBlock);
	this->SetTagMapCount(Tag, 1);
}



bool UYogAbilitySystemComponent::TryActivateRandomAbilitiesByTag(const FGameplayTagContainer& GameplayTagContainer, bool bAllowRemoteActivation)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivatePtrs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivatePtrs);
	if (AbilitiesToActivatePtrs.Num() < 1)
	{
		return false;
	}

	// Convert from pointers (which can be reallocated, since they point to internal data) to copies of that data
	TArray<FGameplayAbilitySpec> AbilitiesToActivate;
	AbilitiesToActivate.Reserve(AbilitiesToActivatePtrs.Num());
	Algo::Transform(AbilitiesToActivatePtrs, AbilitiesToActivate, [](FGameplayAbilitySpec* SpecPtr) { return *SpecPtr; });

	bool bSuccess = false;

	int32 RandomIndex = FMath::RandRange(0, AbilitiesToActivate.Num() - 1);


	bSuccess |= TryActivateAbility(AbilitiesToActivate[RandomIndex].Handle, bAllowRemoteActivation);
	return bSuccess;
	//AbilitiesToActivate.Random
	//for (const FGameplayAbilitySpec& GameplayAbilitySpec : AbilitiesToActivate)
	//{
	//	ensure(IsValid(GameplayAbilitySpec.Ability));
	//	bSuccess |= TryActivateAbility(GameplayAbilitySpec.Handle, bAllowRemoteActivation);
	//}

}

void UYogAbilitySystemComponent::RemoveActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToUnblock)
{
	this->RemoveLooseGameplayTags(TagsToUnblock);
	this->SetTagMapCount(Tag, 0);
}

UYogGameplayAbility* UYogAbilitySystemComponent::GetCurrentAbilityInstance()
{
	// Iterate through all activatable abilities
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		// Each spec can have multiple instances if instanced per execution
		for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
		{
			if (AbilityInstance && AbilityInstance->IsActive())
			{
				// Cast to your custom ability type
				if (UYogGameplayAbility* YogAbility = Cast<UYogGameplayAbility>(AbilityInstance))
				{
					return YogAbility;
				}
			}
		}
	}

	return nullptr; // No active ability found


	//for (const FGameplayAbilitySpec& Spec : this->GetActivatableAbilities())
	//{
	//	if (Spec.IsActive())
	//	{
	//		UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
	//		if (AbilityInstance)
	//		{
	//			// Cast to your custom ability class
	//			return Cast<UYogGameplayAbility>(AbilityInstance);
	//		}
	//	}
	//}
	//return nullptr;

}

void UYogAbilitySystemComponent::LogAllGrantedAbilities()
{
	TArray<FGameplayAbilitySpec>& AbilitySpecs = this->GetActivatableAbilities();

	for (FGameplayAbilitySpec& Spec : AbilitySpecs)
	{
		if (UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec.Ability))
		{
			UE_LOG(LogTemp, Warning, TEXT("granted abilities is: %s"), *Ability->GetName());
		}
	}

    int32 TotalAbilities = AbilitySpecs.Num();
    UE_LOG(LogTemp, Warning, TEXT("Total number of granted abilities: %d"), TotalAbilities);

}

TArray<FAbilitySaveData> UYogAbilitySystemComponent::GetAllGrantedAbilities()
{
	TArray<FAbilitySaveData> array_result;


	TArray<FGameplayAbilitySpec>& AbilitySpecs = this->GetActivatableAbilities();

	for (FGameplayAbilitySpec& Spec : AbilitySpecs)
	{
		if (UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec.Ability))
		{
			FAbilitySaveData YogAbilitySaveData;

			YogAbilitySaveData.Level = Spec.Level;
			YogAbilitySaveData.AbilityClass = Ability->StaticClass();
			
			array_result.Add(YogAbilitySaveData);

			//UE_LOG(LogTemp, Warning, TEXT("granted abilities is: %s"), *Ability->GetName());
		}
	}

	return array_result;
}

void UYogAbilitySystemComponent::GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UYogGameplayAbility*>& ActiveAbilities)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivate;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivate, false);

	// Iterate the list of all ability specs
	for (FGameplayAbilitySpec* Spec : AbilitiesToActivate)
	{
		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			ActiveAbilities.Add(Cast<UYogGameplayAbility>(ActiveAbility));
		}
	}

}

void UYogAbilitySystemComponent::OnAbilityActivated(UYogGameplayAbility* ActivatedAbility)
{
	//CurrentActiveAbility = ActivatedAbility;
}

void UYogAbilitySystemComponent::OnAbilityEnded(const FAbilityEndedData& EndedData)
{
	//if (CurrentActiveAbility == EndedData.AbilityThatEnded)
	//{
	//	CurrentActiveAbility = nullptr;
	//}
}

void UYogAbilitySystemComponent::AddEffectToContainer(FGameplayTag gameplayTag, TSubclassOf<UGameplayEffect> GameplayEffect, int effect_level)
{
	FYogGameplayEffectContainer* FoundContainer = EffectContainerMap.Find(gameplayTag);
	if (FoundContainer)
	{

		for (FYogEffectPorperty& yogEffectProperty : FoundContainer->EffectClasses)
		{
			if (GameplayEffect.Get() == yogEffectProperty.GameplayEffect.Get())
			{
				yogEffectProperty.EffectLevel = yogEffectProperty.EffectLevel + effect_level;
				return;
			}

		}
		FYogEffectPorperty effectProperty;
		effectProperty.GameplayEffect = GameplayEffect;
		effectProperty.EffectLevel = effect_level;
		FoundContainer->EffectClasses.Add(effectProperty);
	}
}

void UYogAbilitySystemComponent::RemoveEffectFromContainer(FGameplayTag gameplayTag, TSubclassOf<UGameplayEffect> GameplayEffect, int effect_level)
{
	FYogGameplayEffectContainer* FoundContainer = EffectContainerMap.Find(gameplayTag);
	if (FoundContainer)
	{

		for (int32 i = FoundContainer->EffectClasses.Num() - 1; i >= 0; --i)
		{
			FYogEffectPorperty& yogEffectProperty = FoundContainer->EffectClasses[i];

			if (GameplayEffect.Get() == yogEffectProperty.GameplayEffect.Get())
			{
				if (yogEffectProperty.EffectLevel > effect_level)
				{
					// Reduce the level
					yogEffectProperty.EffectLevel -= effect_level;
				}
				else
				{
					// Remove the entire element
					FoundContainer->EffectClasses.RemoveAt(i);
				}

				// If you want to stop after processing the first matching effect, add:
				// return;
				// If you want to process ALL matching effects, continue the loop
			}
		}
	}
}

void UYogAbilitySystemComponent::SetAbilityRetriggerable(FGameplayAbilitySpecHandle Handle, bool bCanRetrigger)
{
	FGameplayAbilitySpec* Spec = this->FindAbilitySpecFromHandle(Handle);
	if (Spec && Spec->Ability)
	{
		// For instanced abilities
		if (UYogGameplayAbility* AbilityInstance = Cast<UYogGameplayAbility>(Spec->GetPrimaryInstance()))
		{
			AbilityInstance->UpdateRetrigger(bCanRetrigger);

		}
		// For non-instanced but might be relevant for some cases
		else
		{
			UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec->Ability);
			Ability->UpdateRetrigger(bCanRetrigger);

		}

		// Mark the spec as dirty if needed
		this->MarkAbilitySpecDirty(*Spec);
	}
}



void UYogAbilitySystemComponent::RemoveRuneModifiers(FActiveGameplayEffectHandle Handle)
{
	if (Handle.IsValid())
	{
		RemoveActiveGameplayEffect(Handle);
	}
}
