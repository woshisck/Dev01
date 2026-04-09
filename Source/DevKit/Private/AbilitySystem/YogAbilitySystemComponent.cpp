#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Abilities/PassiveAbility.h"
#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "AIController.h"
#include "BrainComponent.h"

#include "SaveGame/YogSaveGame.h"
#include "Data/YogGameData.h"
#include "Data/StateConflictDataAsset.h"
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
	BlockCategoryMap.Reset();
	StateToBlockCategories.Reset();

	// 若蓝图未手动赋值，自动从 DevAssetManager 全局配置加载
	if (!ConflictTable)
	{
		ConflictTable = UDevAssetManager::Get().GetStateConflictData();
	}

	if (!ConflictTable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StateConflict] ConflictTable is null on %s, system disabled."), *GetNameSafe(GetOwner()));
		return;
	}

	// 构建冲突规则查找表
	for (const FStateConflictRule& Rule : ConflictTable->Rules)
	{
		if (!Rule.ActiveTag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[StateConflict] Rule with invalid ActiveTag found in %s, skipped."), *GetNameSafe(ConflictTable));
			continue;
		}
		ConflictMap.Add(Rule.ActiveTag, Rule);
	}

	// 构建阻断分类表 & 反向索引（StateTag → 所属分类列表）
	for (const auto& Pair : ConflictTable->BlockCategoryMap)
	{
		BlockCategoryMap.Add(Pair.Key, Pair.Value);
		for (const FGameplayTag& StateTag : Pair.Value)
		{
			StateToBlockCategories.FindOrAdd(StateTag).Add(Pair.Key);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[StateConflict] Initialized %d rules, %d block categories on %s."),
		ConflictMap.Num(), BlockCategoryMap.Num(), *GetNameSafe(GetOwner()));
}

void UYogAbilitySystemComponent::SetConflictTable(UStateConflictDataAsset* NewTable)
{
	ConflictTable = NewTable;
	InitConflictTable();
}

void UYogAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);

	// =========================================================
	// 阻断分类：Tag 出现/消失时按 BlockCategoryMap 执行对应阻断
	// =========================================================
	if (const TArray<FGameplayTag>* Categories = StateToBlockCategories.Find(Tag))
	{
		static const FGameplayTag MovementCategory = FGameplayTag::RequestGameplayTag(TEXT("Block.Movement"));
		static const FGameplayTag AICategory       = FGameplayTag::RequestGameplayTag(TEXT("Block.AI"));

		AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwner());

		// ---- Block.Movement ----
		if (Owner && Categories->Contains(MovementCategory))
		{
			if (TagExists)
			{
				Owner->DisableMovement();
				if (AAIController* AI = Cast<AAIController>(Owner->GetController()))
					AI->StopMovement();
			}
			else
			{
				// 检查该分类下是否还有其他阻断 Tag 仍然激活
				bool bStillBlocked = false;
				if (const FGameplayTagContainer* BlockTags = BlockCategoryMap.Find(MovementCategory))
				{
					for (const FGameplayTag& BlockTag : *BlockTags)
					{
						if (HasMatchingGameplayTag(BlockTag)) { bStillBlocked = true; break; }
					}
				}
				if (!bStillBlocked && Owner->IsAlive())
					Owner->EnableMovement();
			}
		}

		// ---- Block.AI ----
		if (Categories->Contains(AICategory))
		{
			if (AAIController* AI = Owner ? Cast<AAIController>(Owner->GetController()) : nullptr)
			{
				if (UBrainComponent* Brain = AI->GetBrainComponent())
				{
					if (TagExists)
					{
						Brain->PauseLogic(Tag.ToString());
					}
					else
					{
						// 检查该分类下是否还有其他 AI 阻断 Tag 仍然激活
						bool bStillBlocked = false;
						if (const FGameplayTagContainer* BlockTags = BlockCategoryMap.Find(AICategory))
						{
							for (const FGameplayTag& BlockTag : *BlockTags)
							{
								if (HasMatchingGameplayTag(BlockTag)) { bStillBlocked = true; break; }
							}
						}
						if (!bStillBlocked)
							Brain->ResumeLogic(Tag.ToString());
					}
				}
			}
		}
	}

	// =========================================================
	// 状态冲突：防递归，BlockAbilitiesWithTags 内部也会触发 OnTagUpdated
	// =========================================================
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

	// 自动触发受击 GA（GA_GetHit 通过 Trigger: GameplayEvent 监听此 Tag）
	// HitReactEventTag 在角色蓝图 CDO 上配置，留空则跳过
	if (HitReactEventTag.IsValid() && IsValid(GetAvatarActor()))
	{
		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
		EventData.EventMagnitude = Damage;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			GetAvatarActor(), HitReactEventTag, EventData);
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

	if (GameplayTagContainer.Num() > 1)
	{
		// 多 Tag 模式：OR 语义，把每个 Tag 视为独立候选，分别查找匹配 GA，汇总去重后随机激活一个
		// 用法：填 {Enemy.Melee.LAtk1, Enemy.Melee.LAtk2, Enemy.Melee.LAtk3}
		//      → 从这三种攻击里随机选一种
		for (const FGameplayTag& Tag : GameplayTagContainer)
		{
			TArray<FGameplayAbilitySpec*> PerTagPtrs;
			GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(Tag), PerTagPtrs);
			for (FGameplayAbilitySpec* Spec : PerTagPtrs)
			{
				AbilitiesToActivatePtrs.AddUnique(Spec);
			}
		}
	}
	else
	{
		// 单 Tag 模式（原有行为）：支持父 Tag 匹配所有子级 GA
		// 用法：填 {Enemy.Melee} → 从所有 Enemy.Melee.* GA 里随机选一个
		GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivatePtrs);
	}

	if (AbilitiesToActivatePtrs.Num() < 1)
	{
		return false;
	}

	// Convert from pointers (which can be reallocated) to copies
	TArray<FGameplayAbilitySpec> AbilitiesToActivate;
	AbilitiesToActivate.Reserve(AbilitiesToActivatePtrs.Num());
	Algo::Transform(AbilitiesToActivatePtrs, AbilitiesToActivate, [](FGameplayAbilitySpec* SpecPtr) { return *SpecPtr; });

	const int32 RandomIndex = FMath::RandRange(0, AbilitiesToActivate.Num() - 1);
	return TryActivateAbility(AbilitiesToActivate[RandomIndex].Handle, bAllowRemoteActivation);
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
