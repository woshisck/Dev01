#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "UI/CombatLogStatics.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Abilities/PassiveAbility.h"
#include "GameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "AIController.h"
#include "BrainComponent.h"

#include "SaveGame/YogSaveGame.h"
#include "Data/YogGameData.h"
#include "Data/StateConflictDataAsset.h"
#include "DevAssetManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	bool IsDefaultMovementBlockStateTag(const FGameplayTag& Tag)
	{
		static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
		static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback"), false);

		return (HitReactTag.IsValid() && Tag.MatchesTagExact(HitReactTag)) ||
			(DeadTag.IsValid() && Tag.MatchesTagExact(DeadTag)) ||
			(KnockbackTag.IsValid() && Tag.MatchesTagExact(KnockbackTag));
	}

	bool IsHitReactStateTag(const FGameplayTag& Tag)
	{
		static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
		return HitReactTag.IsValid() && Tag.MatchesTagExact(HitReactTag);
	}

	bool HasAnyDefaultMovementBlockStateTag(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return false;
		}

		static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact"), false);
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
		static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback"), false);

		return (HitReactTag.IsValid() && ASC->HasMatchingGameplayTag(HitReactTag)) ||
			(DeadTag.IsValid() && ASC->HasMatchingGameplayTag(DeadTag)) ||
			(KnockbackTag.IsValid() && ASC->HasMatchingGameplayTag(KnockbackTag));
	}

	bool HasPlayerAttackStateTag(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return false;
		}

		static const FGameplayTag LightAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"), false);
		static const FGameplayTag HeavyAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"), false);
		static const FGameplayTag DashAttackTag =
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.DashAtk"), false);

		return (LightAttackTag.IsValid() && ASC->HasMatchingGameplayTag(LightAttackTag)) ||
			(HeavyAttackTag.IsValid() && ASC->HasMatchingGameplayTag(HeavyAttackTag)) ||
			(DashAttackTag.IsValid() && ASC->HasMatchingGameplayTag(DashAttackTag));
	}

}

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
			UE_LOG(LogTemp, Log, TEXT("[StateConflict] Map built: %s -> %s"), *StateTag.ToString(), *Pair.Key.ToString());
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

bool UYogAbilitySystemComponent::HasActiveStatusNiagaraForTag(FGameplayTag Tag) const
{
	const TObjectPtr<UNiagaraComponent>* FoundComponent = ActiveStatusNiagaraEffects.Find(Tag);
	return FoundComponent && IsValid(FoundComponent->Get());
}

UNiagaraSystem* UYogAbilitySystemComponent::GetStatusNiagaraSystemForTag(FGameplayTag Tag) const
{
	static const FGameplayTag BurningTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);

	if (BurningTag.IsValid() && Tag == BurningTag)
	{
		return LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor.NS_Fire_Floor"));
	}

	return nullptr;
}

void UYogAbilitySystemComponent::HandleStatusNiagaraTag(const FGameplayTag& Tag, bool bTagExists)
{
	static const FGameplayTag BurningTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Burning"), false);

	if (!BurningTag.IsValid() || Tag != BurningTag)
	{
		return;
	}

	if (bTagExists)
	{
		UNiagaraSystem* BurnSystem = GetStatusNiagaraSystemForTag(Tag);
		StartStatusNiagara(
			Tag,
			BurnSystem,
			FName(TEXT("spine_03")),
			{ FName(TEXT("spine_02")), FName(TEXT("pelvis")), FName(TEXT("root")) },
			FVector(0.f, 0.f, 6.f),
			FRotator::ZeroRotator,
			FVector(0.28f));
	}
	else
	{
		StopStatusNiagara(Tag);
	}
}

void UYogAbilitySystemComponent::StartStatusNiagara(const FGameplayTag& Tag, UNiagaraSystem* NiagaraSystem,
	FName AttachSocketName, const TArray<FName>& FallbackSocketNames, FVector LocationOffset,
	FRotator RotationOffset, FVector Scale)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (const TObjectPtr<UNiagaraComponent>* ExistingComponent = ActiveStatusNiagaraEffects.Find(Tag))
	{
		if (IsValid(ExistingComponent->Get()))
		{
			return;
		}
		ActiveStatusNiagaraEffects.Remove(Tag);
	}

	if (!NiagaraSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusNiagara] Skip: NiagaraSystem is null Tag=%s Owner=%s"),
			*Tag.ToString(), *GetNameSafe(GetOwner()));
		return;
	}

	AActor* TargetActor = GetAvatarActor();
	if (!TargetActor)
	{
		TargetActor = GetOwner();
	}

	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusNiagara] Skip: target actor is null Tag=%s"), *Tag.ToString());
		return;
	}

	USceneComponent* AttachComponent = TargetActor->GetRootComponent();
	FName ResolvedSocket = NAME_None;
	if (USkeletalMeshComponent* MeshComponent = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachComponent = MeshComponent;
		if (AttachSocketName != NAME_None && MeshComponent->DoesSocketExist(AttachSocketName))
		{
			ResolvedSocket = AttachSocketName;
		}
		else
		{
			for (const FName& FallbackSocketName : FallbackSocketNames)
			{
				if (FallbackSocketName != NAME_None && MeshComponent->DoesSocketExist(FallbackSocketName))
				{
					ResolvedSocket = FallbackSocketName;
					break;
				}
			}
		}
	}

	UNiagaraComponent* NiagaraComponent = nullptr;
	if (AttachComponent)
	{
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			AttachComponent,
			ResolvedSocket,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			false,
			true,
			ENCPoolMethod::None,
			false);
		if (NiagaraComponent)
		{
			NiagaraComponent->SetRelativeScale3D(Scale);
		}
	}
	else if (UWorld* World = GetWorld())
	{
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			NiagaraSystem,
			TargetActor->GetActorLocation() + LocationOffset,
			TargetActor->GetActorRotation() + RotationOffset,
			Scale,
			false,
			true,
			ENCPoolMethod::None,
			false);
	}

	if (!NiagaraComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StatusNiagara] Spawn failed Tag=%s Owner=%s System=%s"),
			*Tag.ToString(), *GetNameSafe(TargetActor), *GetNameSafe(NiagaraSystem));
		return;
	}

	NiagaraComponent->SetAutoDestroy(false);
	ActiveStatusNiagaraEffects.Add(Tag, NiagaraComponent);
	UE_LOG(LogTemp, Warning, TEXT("[StatusNiagara] Started Tag=%s Owner=%s System=%s Socket=%s Scale=%s"),
		*Tag.ToString(),
		*GetNameSafe(TargetActor),
		*GetNameSafe(NiagaraSystem),
		*ResolvedSocket.ToString(),
		*Scale.ToString());
}

void UYogAbilitySystemComponent::StopStatusNiagara(const FGameplayTag& Tag)
{
	TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;
	if (!ActiveStatusNiagaraEffects.RemoveAndCopyValue(Tag, NiagaraComponent))
	{
		return;
	}

	if (NiagaraComponent)
	{
		NiagaraComponent->Deactivate();
		NiagaraComponent->DestroyComponent();
	}

	UE_LOG(LogTemp, Warning, TEXT("[StatusNiagara] Stopped Tag=%s Owner=%s"),
		*Tag.ToString(), *GetNameSafe(GetOwner()));
}

void UYogAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);

	UE_LOG(LogTemp, Log, TEXT("[OnTagUpdated] Tag=%s Exists=%d Owner=%s"),
		*Tag.ToString(), (int32)TagExists, *GetNameSafe(GetOwner()));

	HandleStatusNiagaraTag(Tag, TagExists);

	// =========================================================
	// 阻断分类：Tag 出现/消失时按 BlockCategoryMap 执行对应阻断
	// =========================================================
	const TArray<FGameplayTag>* Categories = StateToBlockCategories.Find(Tag);
	const bool bDefaultMovementBlockState = IsDefaultMovementBlockStateTag(Tag);
	if (Categories || bDefaultMovementBlockState)
	{
		const int32 CategoryCount = Categories ? Categories->Num() : 0;
		UE_LOG(LogTemp, Warning, TEXT("[StateConflict] Tag=%s matched %d block categories on %s"),
			*Tag.ToString(), CategoryCount, *GetNameSafe(GetOwner()));
		static const FGameplayTag MovementCategory = FGameplayTag::RequestGameplayTag(TEXT("Block.Movement"));
		static const FGameplayTag AICategory       = FGameplayTag::RequestGameplayTag(TEXT("Block.AI"));

		AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwner());

		// ---- Block.Movement ----
		const bool bBlocksMovement = bDefaultMovementBlockState || (Categories && Categories->Contains(MovementCategory));
		if (Owner && bBlocksMovement)
		{
			if (TagExists)
			{
				if (IsHitReactStateTag(Tag))
				{
					// HitReact should block player/AI control while still allowing montage root motion.
					Owner->BlockMovementControl();
				}
				else
				{
					Owner->DisableMovement();
				}
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
				if (!bStillBlocked)
				{
					bStillBlocked = HasAnyDefaultMovementBlockStateTag(this);
				}
				if (!bStillBlocked && Owner->IsAlive())
				{
					if (IsHitReactStateTag(Tag))
					{
						Owner->UnblockMovementControl();
					}
					else
					{
						Owner->EnableMovement();
					}
				}
			}
		}

		// ---- Block.AI ----
		if (Categories && Categories->Contains(AICategory))
		{
			AAIController* AI = Owner ? Cast<AAIController>(Owner->GetController()) : nullptr;
			UE_LOG(LogTemp, Warning, TEXT("[Block.AI] Tag=%s Exists=%d Owner=%s AI=%s"),
				*Tag.ToString(), (int32)TagExists, *GetNameSafe(Owner), *GetNameSafe(AI));
			if (AI)
			{
				if (UBrainComponent* Brain = AI->GetBrainComponent())
				{
					if (TagExists)
					{
						UE_LOG(LogTemp, Warning, TEXT("[Block.AI] -> PauseLogic on %s"), *GetNameSafe(Owner));
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
						UE_LOG(LogTemp, Warning, TEXT("[Block.AI] Tag=%s Exists=0 | bStillBlocked=%d → %s"),
							*Tag.ToString(), (int32)bStillBlocked,
							bStillBlocked ? TEXT("SKIP ResumeLogic") : TEXT("ResumeLogic"));
						if (!bStillBlocked)
							Brain->ResumeLogic(Tag.ToString());
					}
				}
			}
		}
	}

	// =========================================================
	// SuperArmor Fresnel 闪光：tag 加/减自动 Start/Stop，统一覆盖
	// C++ Poise 自动霸体路径与 FA AddTag 路径（敌人无畏符文 E001）
	// =========================================================
	{
		static const FGameplayTag SuperArmorTag =
			FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
		if (SuperArmorTag.IsValid() && Tag == SuperArmorTag)
		{
			if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(GetAvatarActor()))
			{
				if (TagExists)
				{
					Char->StartSuperArmorFlash();
					FGameplayTagContainer HitReactTags;
					HitReactTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"), false));
					if (!HitReactTags.IsEmpty())
					{
						CancelAbilities(&HitReactTags);
					}
				}
				else
				{
					Char->StopSuperArmorFlash();
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

// ── 武器类型 Tag 守卫 ─────────────────────────────────────────────
namespace
{
	FGameplayTag GetWeaponTypeTag(EWeaponType Type)
	{
		static const FGameplayTag MeleeTag  = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"),  false);
		static const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
		switch (Type)
		{
		case EWeaponType::Melee:  return MeleeTag;
		case EWeaponType::Ranged: return RangedTag;
		default:                  return FGameplayTag();
		}
	}
}

void UYogAbilitySystemComponent::ClearWeaponTypeTags()
{
	// 用 SetLooseGameplayTagCount(Tag, 0) 清零，避免 RemoveLooseGameplayTag 只减一次计数导致残留
	static const FGameplayTag MeleeTag  = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Melee"),  false);
	static const FGameplayTag RangedTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged"), false);
	if (MeleeTag.IsValid())  SetLooseGameplayTagCount(MeleeTag,  0);
	if (RangedTag.IsValid()) SetLooseGameplayTagCount(RangedTag, 0);
}

void UYogAbilitySystemComponent::ApplyWeaponTypeTag(EWeaponType Type)
{
	// 先清零再设 1，确保任意时刻只持有当前武器类型 Tag（重复装备/恢复时无残留）
	ClearWeaponTypeTags();
	const FGameplayTag NewTag = GetWeaponTypeTag(Type);
	if (NewTag.IsValid())
	{
		SetLooseGameplayTagCount(NewTag, 1);
	}

	UE_LOG(LogTemp, Log, TEXT("[WeaponType] ApplyWeaponTypeTag: %s on %s"),
		*NewTag.ToString(), *GetNameSafe(GetAvatarActor()));
}

void UYogAbilitySystemComponent::SuppressNextDamageFeedback()
{
	bSuppressNextDamageFeedback = true;
}

bool UYogAbilitySystemComponent::ConsumeSuppressNextDamageFeedback()
{
	const bool bSuppress = bSuppressNextDamageFeedback;
	bSuppressNextDamageFeedback = false;
	return bSuppress;
}

void UYogAbilitySystemComponent::ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage, const bool bSuppressHitReact)
{
	ReceivedDamage.Broadcast(SourceASC, Damage);

	// 广播前检查 TargetASC (this) 及其 Avatar 是否仍然有效
	// 同帧内多次命中已死亡角色（DoT/AoE）会导致 TargetASC pending kill，
	// Blueprint 侧的 GA_Passive_knockback 访问 pending kill 对象会报错
	if (SourceASC && IsValid(this) && IsValid(GetAvatarActor()))
	{
		SourceASC->DealtDamage.Broadcast(this, Damage);
	}

	if (!IsValid(GetAvatarActor()))
		return;

	if (bSuppressHitReact)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitReact] Skip DoT damage Target=%s Damage=%.1f"),
			*GetNameSafe(GetAvatarActor()),
			Damage);
		return;
	}

	static const FGameplayTag DefaultHitReactEventTag =
		FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"), false);
	const FGameplayTag EffectiveHitReactEventTag = HitReactEventTag.IsValid()
		? HitReactEventTag
		: DefaultHitReactEventTag;
	if (!EffectiveHitReactEventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitReact] Skip: invalid HitReactEventTag Target=%s Configured=%s Default=%s"),
			*GetNameSafe(GetAvatarActor()),
			*HitReactEventTag.ToString(),
			*DefaultHitReactEventTag.ToString());
		return;
	}

	// =========================================================
	// 韧性（Poise）比较：决定是否触发受击动画
	// 攻击方有效韧性 = Resilience属性 + 动作韧性（由 GA 在命中前设置）
	// =========================================================
	float AttackerPoise = 0.f;
	if (SourceASC)
	{
		AttackerPoise = SourceASC->GetNumericAttribute(UBaseAttributeSet::GetResilienceAttribute())
		              + SourceASC->CurrentActionPoiseBonus;
		SourceASC->CurrentActionPoiseBonus = 0.f; // 读取后立即清零，避免跨帧残留
	}

	const bool bPlayerAttackHitReactImmune = Cast<APlayerCharacterBase>(GetAvatarActor()) && HasPlayerAttackStateTag(this);
	const float DefenderBasePoise = GetNumericAttribute(UBaseAttributeSet::GetResilienceAttribute());
	constexpr float PlayerAttackUninterruptiblePoise = 9999.f;
	const float DefenderPoise = bPlayerAttackHitReactImmune
		? FMath::Max(DefenderBasePoise, PlayerAttackUninterruptiblePoise)
		: DefenderBasePoise;
	static const FGameplayTag SuperArmorTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
	const bool bHasSuperArmorTag = SuperArmorTag.IsValid() && HasMatchingGameplayTag(SuperArmorTag);
	const bool bHadBlockingSuperArmor = bHasSuperArmorTag || bPoiseSuperArmorActive;
	bool bActivatedSuperArmor = false;
	APawn* DefenderPawn = Cast<APawn>(GetAvatarActor());
	const bool bEnemyDefender = DefenderPawn && !DefenderPawn->IsPlayerControlled();

	UE_LOG(LogTemp, Warning, TEXT("[Poise] Target=%s Attacker=%.0f Defender=%.0f BaseDefender=%.0f PlayerAttackImmune=%d SuperArmorTag=%d BlockingSuperArmor=%d Enemy=%d"),
		*GetNameSafe(GetAvatarActor()),
		AttackerPoise,
		DefenderPoise,
		DefenderBasePoise,
		bPlayerAttackHitReactImmune ? 1 : 0,
		(int32)bHasSuperArmorTag,
		(int32)bHadBlockingSuperArmor,
		(int32)bEnemyDefender);

	// =========================================================
	// 霸体计数（仅非玩家）：连续受到真实伤害 → 进入霸体
	// =========================================================
	if (bEnemyDefender && !bHadBlockingSuperArmor)
	{
		PoiseHitCount++;

		UE_LOG(LogTemp, Log, TEXT("[Poise] PoiseHitCount=%d/%d on %s"),
			PoiseHitCount, SuperArmorThreshold, *GetNameSafe(GetAvatarActor()));

		// 5s 无受击后重置计数
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(
				PoiseResetTimer, this, &UYogAbilitySystemComponent::OnPoiseResetTimerEnd, 5.f, false);

		if (SuperArmorThreshold > 0 && PoiseHitCount >= SuperArmorThreshold)
		{
			PoiseHitCount = 0;
			bActivatedSuperArmor = true;
			bPoiseSuperArmorActive = true;
			// AddLooseGameplayTag 会触发 OnTagUpdated → 自动 StartSuperArmorFlash
			if (SuperArmorTag.IsValid())
			{
				AddLooseGameplayTag(SuperArmorTag);
			}
			UE_LOG(LogTemp, Warning, TEXT("[Poise] SuperArmor ACTIVATED on %s (%.1fs)"),
				*GetNameSafe(GetAvatarActor()), SuperArmorDuration);
			TriggerSuperArmorCounterAttack();

			if (UWorld* W = GetWorld())
				W->GetTimerManager().SetTimer(
					SuperArmorTimer, this, &UYogAbilitySystemComponent::OnSuperArmorTimerEnd, SuperArmorDuration, false);
		}
	}

	// Permanent Resilience is still logged for balancing, but it should not make players
	// or enemies permanently immune to hit reaction. Player attack state is the explicit
	// uninterruptible case; enemy super armor is handled below.
	const bool bPoiseBlocksHitReact = bPlayerAttackHitReactImmune;
	const bool bBlockingSuperArmor = bHadBlockingSuperArmor || bActivatedSuperArmor ||
		(SuperArmorTag.IsValid() && HasMatchingGameplayTag(SuperArmorTag));
	if (bPoiseBlocksHitReact || bBlockingSuperArmor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitReact][Poise] SkipHitReact Target=%s AttackerPoise=%.0f DefenderPoise=%.0f Enemy=%d PoiseBlocked=%d SuperArmorTag=%d BlockingSuperArmor=%d ActivatedSuperArmorNow=%d"),
			*GetNameSafe(GetAvatarActor()),
			AttackerPoise,
			DefenderPoise,
			bEnemyDefender ? 1 : 0,
			bPoiseBlocksHitReact ? 1 : 0,
			bHasSuperArmorTag ? 1 : 0,
			bBlockingSuperArmor ? 1 : 0,
			bActivatedSuperArmor ? 1 : 0);
		return;
	}

	// =========================================================
	// 触发受击事件（Action.HitReact.Front / .Back 等子级）
	// =========================================================
	FGameplayEventData EventData;
	EventData.Instigator     = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	EventData.EventMagnitude = Damage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetAvatarActor(), EffectiveHitReactEventTag, EventData);
	UE_LOG(LogTemp, Warning, TEXT("[HitReact] Sent Target=%s Event=%s Damage=%.1f Instigator=%s"),
		*GetNameSafe(GetAvatarActor()),
		*EffectiveHitReactEventTag.ToString(),
		Damage,
		*GetNameSafe(EventData.Instigator.Get()));
}

void UYogAbilitySystemComponent::OnPoiseResetTimerEnd()
{
	PoiseHitCount = 0;
	UE_LOG(LogTemp, Log, TEXT("[Poise] PoiseHitCount reset on %s"), *GetNameSafe(GetAvatarActor()));
}

void UYogAbilitySystemComponent::TriggerSuperArmorCounterAttack()
{
	APawn* DefenderPawn = Cast<APawn>(GetAvatarActor());
	if (!DefenderPawn || DefenderPawn->IsPlayerControlled())
	{
		return;
	}

	FGameplayTagContainer HitReactTags;
	HitReactTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"), false));
	if (!HitReactTags.IsEmpty())
	{
		CancelAbilities(&HitReactTags);
	}

	if (AAIController* AI = Cast<AAIController>(DefenderPawn->GetController()))
	{
		AI->StopMovement();
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			if (Brain->IsPaused())
			{
				Brain->ResumeLogic(TEXT("SuperArmorCounterAttack"));
			}
		}
	}

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk1"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk2"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk3"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk4"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk1"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk2"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk3"), false));
	AttackTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.HAtk4"), false));

	if (AttackTags.IsEmpty())
	{
		return;
	}

	const bool bActivated = TryActivateRandomAbilitiesByTag(AttackTags, false);
	UE_LOG(LogTemp, Warning, TEXT("[Poise] SuperArmor counter attack on %s -> %d"),
		*GetNameSafe(GetAvatarActor()), (int32)bActivated);
}

void UYogAbilitySystemComponent::OnSuperArmorTimerEnd()
{
	bPoiseSuperArmorActive = false;
	// RemoveLooseGameplayTag 会触发 OnTagUpdated → 自动 StopSuperArmorFlash
	RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor")));
	UE_LOG(LogTemp, Warning, TEXT("[Poise] SuperArmor EXPIRED on %s"), *GetNameSafe(GetAvatarActor()));
}

// =========================================================
// 冲刺连招保存
// =========================================================

void UYogAbilitySystemComponent::ApplyDashSave(const FGameplayTagContainer& Tags)
{
	// 先清理上次残留（双冲刺连打时保护）
	ConsumeDashSave();

	DashSaveComboTags = Tags;
	for (const FGameplayTag& Tag : DashSaveComboTags)
		AddLooseGameplayTag(Tag);

	// 2s 内未消费则自动清理（防止玩家没有接攻击导致 Tag 残留）
	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(
			DashSaveExpireTimer, this, &UYogAbilitySystemComponent::DashSaveExpired, 2.f, false);

	UE_LOG(LogTemp, Log, TEXT("[DashSave] Applied %d combo tags on %s (2s window)"),
		Tags.Num(), *GetNameSafe(GetAvatarActor()));
}

bool UYogAbilitySystemComponent::ConsumeDashSave()
{
	if (DashSaveComboTags.IsEmpty())
		return false;

	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(DashSaveExpireTimer);

	for (const FGameplayTag& Tag : DashSaveComboTags)
		SetLooseGameplayTagCount(Tag, 0);

	DashSaveComboTags.Reset();
	UE_LOG(LogTemp, Log, TEXT("[DashSave] Consumed on %s"), *GetNameSafe(GetAvatarActor()));
	return true;
}

void UYogAbilitySystemComponent::DashSaveExpired()
{
	UE_LOG(LogTemp, Log, TEXT("[DashSave] Expired (not consumed) on %s"), *GetNameSafe(GetAvatarActor()));
	ConsumeDashSave();
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

bool UYogAbilitySystemComponent::DebugHasAbilityClass(AActor* Actor, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!Actor || !AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DebugHasAbilityClass] Actor or AbilityClass is null."));
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DebugHasAbilityClass] No ASC found on %s."), *GetNameSafe(Actor));
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			UE_LOG(LogTemp, Log, TEXT("[DebugHasAbilityClass] %s HAS %s (Handle=%s)"),
				*GetNameSafe(Actor), *GetNameSafe(AbilityClass), *Spec.Handle.ToString());
			return true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[DebugHasAbilityClass] %s does NOT have %s"),
		*GetNameSafe(Actor), *GetNameSafe(AbilityClass));
	return false;
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

void UYogAbilitySystemComponent::LogDamageDealt(AActor* Target, float Damage, FName DamageType)
{
	// 屏幕滚动槽：3000-3029（30 条），每条显示 4 秒
	static int32 RollingSlot = 0;
	const int32 MsgKey = 3000 + (RollingSlot++ % 30);

	const FString SourceName = GetNameSafe(GetAvatarActor());
	const FString TargetName = GetNameSafe(Target);

	FColor MsgColor = FColor::Orange;
	if (DamageType == FName("Bleed"))              MsgColor = FColor::Red;
	else if (DamageType == FName("Attack_Crit"))   MsgColor = FColor::Yellow;
	else if (DamageType.ToString().StartsWith("Rune")) MsgColor = FColor::Purple;

	const FString Msg = FString::Printf(TEXT("[DmgLog] %.1f  [%s]  → %s"),
		Damage, *DamageType.ToString(), *TargetName);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(MsgKey, 4.f, MsgColor, Msg);

	UE_LOG(LogTemp, Log, TEXT("[DmgLog] %s → %s | %.1f | %s"),
		*SourceName, *TargetName, Damage, *DamageType.ToString());

	// 简化形式广播（无动作系数分解）
	FDamageBreakdown Simple;
	Simple.FinalDamage   = Damage;
	Simple.DamageType    = DamageType;
	Simple.bIsCrit       = (DamageType == FName("Attack_Crit"));
	Simple.ActionName    = DamageType;
	Simple.TargetName    = TargetName;
	Simple.SourceName    = GetNameSafe(GetAvatarActor());
	OnDamageBreakdown.Broadcast(Simple);
	UCombatLogStatics::PushEntry(Simple);
}

void UYogAbilitySystemComponent::LogDamageDealtDetailed(AActor* Target, const FDamageBreakdown& Breakdown)
{
	// 屏幕滚动槽：3000-3029（30 条），每条显示 4 秒
	static int32 RollingSlot = 0;
	const int32 MsgKey = 3000 + (RollingSlot++ % 30);

	FColor MsgColor = FColor::Orange;
	if (Breakdown.DamageType == FName("Bleed"))              MsgColor = FColor::Red;
	else if (Breakdown.bIsCrit)                              MsgColor = FColor::Yellow;
	else if (Breakdown.DamageType.ToString().StartsWith("Rune")) MsgColor = FColor::Purple;

	// 格式：[轻击2]  25 × 0.88 × 1.00 ★CRIT = 44.0  → BP_Enemy_Rat
	const FString CritStr = Breakdown.bIsCrit ? TEXT(" ★CRIT") : TEXT("");
	const FString Msg = FString::Printf(
		TEXT("[%s]  %.0f × %.2f × %.2f%s = %.1f  → %s"),
		*Breakdown.ActionName.ToString(),
		Breakdown.BaseAttack,
		Breakdown.ActionMultiplier,
		Breakdown.DmgTakenMult,
		*CritStr,
		Breakdown.FinalDamage,
		*Breakdown.TargetName);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(MsgKey, 4.f, MsgColor, Msg);

	UE_LOG(LogTemp, Log, TEXT("[DmgLog] %s → %s | %.1f | %s"),
		*GetNameSafe(GetAvatarActor()), *Breakdown.TargetName, Breakdown.FinalDamage, *Breakdown.DamageType.ToString());

	OnDamageBreakdown.Broadcast(Breakdown);
	UCombatLogStatics::PushEntry(Breakdown);
}
