// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyCharacterBase.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "Data/EnemyWeaponDefinition.h"
#include "Component/AttributeStatComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/GASTemplate.h"
#include "Data/WeaponSkillDataAsset.h"
#include "GameModes/YogGameMode.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Component/EnemyHealthDisplayComponent.h"
#include "Component/MontageVFXBindingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Item/Weapon/WeaponInstance.h"
#include "BrainComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/InteractPromptComponent.h"
#include "Components/CapsuleComponent.h"
#include "Map/RewardPickup.h"
#include "NiagaraFunctionLibrary.h"

// Keeps multiple kill-reward pickups from stacking inside each other's 80cm collision box.
static constexpr float EnemyCharacter_KillRewardPickupSpacing = 120.0f;

// Used only when the swing has no valid id; several damage events of one swing land well inside this.
static constexpr float EnemyCharacter_CorpseHitFallbackWindow = 0.3f;

// Pickups from a burst corpse fly out this far, so they do not line up like the drop-on-death layout.
static constexpr float EnemyCharacter_CorpseBurstScatterRadius = 90.0f;

static bool EnemyWeaponHasUsableMontageConfigList(const FAbilityMontageConfigList& ConfigList)
{
	for (const FTaggedMontageConfig& Config : ConfigList.Configs)
	{
		if (Config.MontageConfig)
		{
			return true;
		}
	}

	return false;
}

static bool EnemyWeaponHasUsablePassiveActionData(const FPassiveActionData& PassiveData)
{
	return PassiveData.Montage != nullptr
		|| !PassiveData.UniqueEffects.IsEmpty()
		|| PassiveData.DissolveGameplayCueTag.IsValid();
}

static void MergeEnemyWeaponAbilityDataInto(UAbilityData& Target, const UAbilityData& Source)
{
	for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& Pair : Source.MontageMap)
	{
		if (Pair.Key.IsValid() && Pair.Value)
		{
			Target.MontageMap.Add(Pair.Key, Pair.Value);
		}
	}

	for (const TPair<FGameplayTag, FAbilityMontageConfigList>& Pair : Source.MontageConfigMap)
	{
		if (Pair.Key.IsValid() && EnemyWeaponHasUsableMontageConfigList(Pair.Value))
		{
			Target.MontageConfigMap.Add(Pair.Key, Pair.Value);
		}
	}

	for (const TPair<FGameplayTag, FPassiveActionData>& Pair : Source.PassiveMap)
	{
		if (Pair.Key.IsValid() && EnemyWeaponHasUsablePassiveActionData(Pair.Value))
		{
			Target.PassiveMap.Add(Pair.Key, Pair.Value);
		}
	}
}

static bool EnemyWeaponHasAttributeAdd(const FYogBaseAttributeData& Add)
{
	return !FMath::IsNearlyZero(Add.Attack)
		|| !FMath::IsNearlyZero(Add.AttackPower)
		|| !FMath::IsNearlyZero(Add.MaxHealth)
		|| !FMath::IsNearlyZero(Add.MaxHeat)
		|| !FMath::IsNearlyZero(Add.Shield)
		|| !FMath::IsNearlyZero(Add.AttackSpeed)
		|| !FMath::IsNearlyZero(Add.AttackRange)
		|| !FMath::IsNearlyZero(Add.Sanity)
		|| !FMath::IsNearlyZero(Add.MoveSpeed)
		|| !FMath::IsNearlyZero(Add.Dodge)
		|| !FMath::IsNearlyZero(Add.Resilience)
		|| !FMath::IsNearlyZero(Add.Resist)
		|| !FMath::IsNearlyZero(Add.DmgTaken)
		|| !FMath::IsNearlyZero(Add.Crit_Rate)
		|| !FMath::IsNearlyZero(Add.Crit_Damage)
		|| !FMath::IsNearlyZero(Add.MaxArmorHP);
}

static void AddEnemyWeaponAttribute(UAttributeStatComponent* Stats, const FGameplayAttribute& Attribute, float Delta)
{
	if (Stats && !FMath::IsNearlyZero(Delta))
	{
		Stats->AddAttribute(Attribute, Delta);
	}
}

static void MultiplyEnemyWeaponAttribute(UAttributeStatComponent* Stats, const FGameplayAttribute& Attribute, float Multiplier)
{
	if (Stats && !FMath::IsNearlyEqual(Multiplier, 1.0f))
	{
		Stats->MultiplyAttribute(Attribute, Multiplier);
	}
}

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
	HealthDisplayComponent = CreateDefaultSubobject<UEnemyHealthDisplayComponent>(TEXT("EnemyHealthDisplayComponent"));

	// 近战默认命中框：C++ 实现，无需在每个角色蓝图 Class Defaults 中单独配置
	DefaultMeleeTargetType = UYogTargetType_Enemy::StaticClass();
	

	//static ConstructorHelpers::FClassFinder<UYogGameplayAbility> Ability_Blueprint_Class(TEXT("Blueprint'/Game/Code/Weapon/GA_MobAbility'"));
	//if (Ability_Blueprint_Class.Succeeded())
	//{
	//	UClass* MyActorClass = Ability_Blueprint_Class.Class.Get();
	//	Ability_Class = Ability_Blueprint_Class.Class.Get();
	//	UE_LOG(LogTemp, Warning, TEXT("class name:%s"), *Ability_Class->GetName());
	//}
	 

	//Script / Engine.Blueprint'/Game/Code/Weapon/GA_MobAbility.GA_MobAbility'
	//static ConstructorHelpers::FClassFinder<AActor> BlueprintClassFinder(TEXT("/Game/Code/Weapon/GA_WeaponAtk.GA_WeaponAtk_C"));
	//if (BlueprintClassFinder.Succeeded())
	//{
	//	Ability_Class = BlueprintClassFinder.Class;
	//}


}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	UEnemyData* RuntimeEnemyData = CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
	UEnemyWeaponDefinition* WeaponToApply = PendingEnemyWeaponDefinition.Get();
	if (!WeaponToApply && RuntimeEnemyData)
	{
		WeaponToApply = RuntimeEnemyData->DefaultWeaponDefinition.Get();
		if (!WeaponToApply && !RuntimeEnemyData->AllowedWeaponDefinitions.IsEmpty())
		{
			TArray<UEnemyWeaponDefinition*> ValidWeapons;
			for (UEnemyWeaponDefinition* Candidate : RuntimeEnemyData->AllowedWeaponDefinitions)
			{
				if (Candidate)
				{
					ValidWeapons.Add(Candidate);
				}
			}
			if (!ValidWeapons.IsEmpty())
			{
				WeaponToApply = ValidWeapons[FMath::RandRange(0, ValidWeapons.Num() - 1)];
			}
		}
	}
	ApplyEnemyWeaponDefinition(WeaponToApply);

	// 注册死亡判断：不依赖 PossessedBy 的时序，BeginPlay 里直接绑定
	if (AttributeStatsComponent)
	{
		AttributeStatsComponent->OnHealthChange.AddDynamic(this, &AEnemyCharacterBase::OnHealthChangedForDeath);
	}

	// 从 EnemyData DA 推送参数到 ASC（填表即生效，不需改每个敌人 BP）
	if (CharacterDataComponent && AbilitySystemComponent)
	{
		if (UEnemyData* ED = Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()))
		{
			AbilitySystemComponent->RecentlyDamagedStateDuration = ED->RecentlyDamagedStateDuration;
			if (ED->MovementTuning.MaxWalkSpeedOverride > 0.0f)
			{
				GetCharacterMovement()->MaxWalkSpeed = ED->MovementTuning.MaxWalkSpeedOverride;
			}
		}
	}

	if (EquippedEnemyWeaponDefinition && AttributeStatsComponent)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = AttributeStatsComponent->GetStat_MoveSpeed();
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ReceivedDamage.AddDynamic(this, &AEnemyCharacterBase::OnReceivedDamageForAI);
	}

	// 注册到 GameMode 的敌人列表（供 CameraPawn 战斗感知使用）
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RegisterEnemy(this);
	}
}

void AEnemyCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

bool AEnemyCharacterBase::IsAlive() const
{
	if (!AttributeStatsComponent) return true;
	return AttributeStatsComponent->GetStat_Health() > 0.f;
}

float AEnemyCharacterBase::GetDeathDisappearDelayAfterAnimation(bool /*bHasDissolveCue*/) const
{
	const UEnemyData* RuntimeEnemyData = CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
	if (!RuntimeEnemyData)
	{
		RuntimeEnemyData = GetDefault<UEnemyData>();
	}

	return FMath::Max(0.0f, RuntimeEnemyData->DeathDisappearDelayAfterAnimation);
}

void AEnemyCharacterBase::SetAIAttackRuntimeContext(
	bool bIsReposition,
	bool bRequestRepositionOnResolve,
	AActor* TargetActor,
	float DistanceToTarget)
{
	PendingAIAttackContext.bValid = true;
	PendingAIAttackContext.bIsReposition = bIsReposition;
	PendingAIAttackContext.bRequestRepositionOnResolve = bRequestRepositionOnResolve;
	PendingAIAttackContext.TargetActor = TargetActor;
	PendingAIAttackContext.DistanceToTarget = DistanceToTarget;
}

bool AEnemyCharacterBase::ConsumeAIAttackRuntimeContext(FEnemyAIAttackRuntimeContext& OutContext)
{
	if (!PendingAIAttackContext.bValid)
	{
		return false;
	}

	OutContext = PendingAIAttackContext;
	ClearAIAttackRuntimeContext();
	return true;
}

void AEnemyCharacterBase::ClearAIAttackRuntimeContext()
{
	PendingAIAttackContext = FEnemyAIAttackRuntimeContext();
}

void AEnemyCharacterBase::DestroySpawnedEnemyWeaponActors()
{
	for (TObjectPtr<AActor>& SpawnedActor : SpawnedEnemyWeaponActors)
	{
		if (SpawnedActor && IsValid(SpawnedActor))
		{
			SpawnedActor->Destroy();
		}
	}

	SpawnedEnemyWeaponActors.Reset();
}

const UWeaponDefinitionBase* AEnemyCharacterBase::GetEffectiveWeaponDefinition() const
{
	return EquippedEnemyWeaponDefinition.Get();
}

UEnemyData* AEnemyCharacterBase::GetEnemyData() const
{
	return CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
}

EEnemyCombatTier AEnemyCharacterBase::GetCombatTier() const
{
	const UEnemyData* Data = GetEnemyData();
	return Data ? Data->CombatTier : EEnemyCombatTier::Normal;
}

FText AEnemyCharacterBase::GetEnemyDisplayName() const
{
	const UEnemyData* Data = GetEnemyData();
	if (Data && !Data->DisplayName.IsEmpty())
	{
		return Data->DisplayName;
	}

	return FText::FromString(GetName());
}

AWeaponInstance* AEnemyCharacterBase::GetEquippedWeaponActor() const
{
	for (const TObjectPtr<AActor>& SpawnedActor : SpawnedEnemyWeaponActors)
	{
		if (AWeaponInstance* Weapon = Cast<AWeaponInstance>(SpawnedActor.Get()))
		{
			return Weapon;
		}
	}

	return nullptr;
}

void AEnemyCharacterBase::SetPendingEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition)
{
	PendingEnemyWeaponDefinition = WeaponDefinition;
}

void AEnemyCharacterBase::ApplyEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition)
{
	if (!WeaponDefinition || EquippedEnemyWeaponDefinition == WeaponDefinition)
	{
		return;
	}

	UEnemyData* RuntimeEnemyData = CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
	if (!RuntimeEnemyData)
	{
		return;
	}

	if (WeaponDefinition->AbilityData)
	{
		UAbilityData* RuntimeAbilityData = NewObject<UAbilityData>(this);
		RuntimeAbilityData->SetFlags(RF_Transient);

		if (UAbilityData* BaseAbilityData = CharacterDataComponent ? CharacterDataComponent->GetBaseAbilityData() : RuntimeEnemyData->AbilityData.Get())
		{
			MergeEnemyWeaponAbilityDataInto(*RuntimeAbilityData, *BaseAbilityData);
		}
		MergeEnemyWeaponAbilityDataInto(*RuntimeAbilityData, *WeaponDefinition->AbilityData);
		RuntimeEnemyData->AbilityData = RuntimeAbilityData;
	}

	DestroySpawnedEnemyWeaponActors();

	for (const FWeaponSpawnData& SpawnData : WeaponDefinition->ActorsToSpawn)
	{
		if (!SpawnData.ActorToSpawn || !GetWorld() || !GetMesh())
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AWeaponInstance* WeaponActor = GetWorld()->SpawnActor<AWeaponInstance>(
			SpawnData.ActorToSpawn,
			FTransform::Identity,
			SpawnParams);
		if (!WeaponActor)
		{
			continue;
		}

		WeaponActor->AttachSocket = SpawnData.AttachSocket;
		WeaponActor->AttachTransform = SpawnData.AttachTransform;
		WeaponActor->WeaponLayer = SpawnData.WeaponLayer;
		WeaponActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SpawnData.AttachSocket);
		WeaponActor->SetActorRelativeTransform(SpawnData.AttachTransform);
		SpawnedEnemyWeaponActors.Add(WeaponActor);
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		for (const TSubclassOf<UAnimInstance>& AnimLayer : WeaponDefinition->AnimLayers)
		{
			if (AnimLayer)
			{
				AnimInstance->LinkAnimClassLayers(AnimLayer);
			}
		}
		for (const FWeaponSpawnData& SpawnData : WeaponDefinition->ActorsToSpawn)
		{
			if (SpawnData.WeaponLayer)
			{
				AnimInstance->LinkAnimClassLayers(SpawnData.WeaponLayer);
			}
		}
	}

	if (AttributeStatsComponent)
	{
		const FYogBaseAttributeData& Add = WeaponDefinition->AttributeModifiers.Add;
		if (EnemyWeaponHasAttributeAdd(Add))
		{
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackAttribute(), Add.Attack);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackPowerAttribute(), Add.AttackPower);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetMaxHealthAttribute(), Add.MaxHealth);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetHealthAttribute(), Add.MaxHealth);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetMaxHeatAttribute(), Add.MaxHeat);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetShieldAttribute(), Add.Shield);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackSpeedAttribute(), Add.AttackSpeed);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackRangeAttribute(), Add.AttackRange);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetSanityAttribute(), Add.Sanity);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetMoveSpeedAttribute(), Add.MoveSpeed);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetDodgeAttribute(), Add.Dodge);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetResilienceAttribute(), Add.Resilience);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetResistAttribute(), Add.Resist);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetDmgTakenAttribute(), Add.DmgTaken);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetCrit_RateAttribute(), Add.Crit_Rate);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetCrit_DamageAttribute(), Add.Crit_Damage);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetMaxArmorHPAttribute(), Add.MaxArmorHP);
			AddEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetArmorHPAttribute(), Add.MaxArmorHP);
		}

		MultiplyEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackPowerAttribute(), WeaponDefinition->AttributeModifiers.AttackPowerMultiplier);
		MultiplyEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetAttackSpeedAttribute(), WeaponDefinition->AttributeModifiers.AttackSpeedMultiplier);
		MultiplyEnemyWeaponAttribute(AttributeStatsComponent, UBaseAttributeSet::GetMoveSpeedAttribute(), WeaponDefinition->AttributeModifiers.MoveSpeedMultiplier);
	}

	if (AbilitySystemComponent)
	{
		for (const TSubclassOf<UGameplayEffect>& PassiveEffect : WeaponDefinition->PassiveEffects)
		{
			if (!PassiveEffect)
			{
				continue;
			}

			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(WeaponDefinition);
			FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(PassiveEffect, 1, Context);
			if (Spec.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	EquippedEnemyWeaponDefinition = WeaponDefinition;
	PendingEnemyWeaponDefinition = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[EnemyWeapon] Applied Weapon=%s Enemy=%s AbilityData=%s"),
		*GetNameSafe(WeaponDefinition),
		*GetNameSafe(this),
		*GetNameSafe(RuntimeEnemyData->AbilityData.Get()));
}

void AEnemyCharacterBase::OnHealthChangedForDeath(float NewHealth)
{
	if (NewHealth <= 0.f && !bIsDead)
	{
		Die();
	}
}

void AEnemyCharacterBase::OnReceivedDamageForAI(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	if (Damage <= 0.0f || !IsAlive())
	{
		return;
	}

	AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	if (AYogAIController* YogAI = Cast<AYogAIController>(GetController()))
	{
		YogAI->EnterCombat(SourceActor, true);
	}
}

void AEnemyCharacterBase::Die()
{
	if (bIsDead) return;
	bIsDead = true;

	// Must precede Super::Die(): it starts GA_Dead, which asks IsLootableCorpse() to pick the death flow,
	// and a montage-less enemy reaches that decision synchronously.
	RollKillRewards();

	Super::Die();

	// 关闭 AI：停止 StateTree、停止移动、清除焦点、关掉自动朝向，防止尸体仍然朝玩家转向
	// Super::Die() 已关闭胶囊碰撞但不会停 AI；StateTree 仍会下发 MoveTo，CMC.Velocity 不为 0 时
	// bOrientRotationToMovement 会把死掉的角色继续旋转去对着玩家。
	if (AYogAIController* YogAI = Cast<AYogAIController>(GetController()))
	{
		if (UBrainComponent* Brain = YogAI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
		YogAI->StopMovement();
		YogAI->ClearFocus(EAIFocusPriority::Gameplay);
		YogAI->ClearFocus(EAIFocusPriority::Move);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->DisableMovement();
	}
	bUseControllerRotationYaw = false;

	// 通知 GameMode 击杀计数，触发波次/关卡完成检查
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (bCountsForLevelClear)
		{
			GM->LastEnemyKillLocation = GetActorLocation();
			GM->UpdateFinishLevel(1);
		}
		// 从相机战斗感知列表中移除
		GM->UnregisterEnemy(this);
	}

	// 绝对兜底：正常死亡销毁由 GA_Dead 处理（有蒙太奇→动画结束销毁；无蒙太奇→2秒后销毁）
	// 此处仅在 GA_Dead 完全未激活（配置遗漏）时作为最后保险，设置为足够长的时间
	// Fallback only: the normal path is GA_Dead -> montage/dissolve -> FinishDying().
	// Keep this short enough that a missing cooked death ability cannot leave dead enemies in the room,
	// but offset by the EnemyData delay so it never cuts a configured long corpse time short.
	SetLifeSpan(8.0f + GetDeathDisappearDelayAfterAnimation(false));
}

void AEnemyCharacterBase::FinishDying()
{
	if (bLootableCorpse)
	{
		// GA_Dead and BFNode_FinishLifecycle can both end up here.
		if (bCorpseActive)
		{
			return;
		}

		// Die() armed a destroy fallback for a missing GA_Dead; the corpse outlives it on purpose.
		SetLifeSpan(0.f);
		DestroySpawnedEnemyWeaponActors();
		EnterLootableCorpseState();
		BroadcastCharacterDied();
		return;
	}

	// Must run before Super, which broadcasts the death delegates and then destroys this actor.
	SpawnKillRewardPickups(true, 0.f);
	DestroySpawnedEnemyWeaponActors();
	Super::FinishDying();
}

void AEnemyCharacterBase::RollKillRewards()
{
	PendingCorpseBundledLoot.Reset();
	PendingCorpseSkillLoot.Reset();
	bLootableCorpse = false;

	const UEnemyData* RuntimeEnemyData = CharacterDataComponent ? Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()) : nullptr;
	if (!RuntimeEnemyData || !RuntimeEnemyData->bEnableKillRewards || RuntimeEnemyData->KillRewards.IsEmpty())
	{
		return;
	}

	for (const FEnemyKillRewardEntry& Entry : RuntimeEnemyData->KillRewards)
	{
		if (FMath::FRand() > Entry.DropChance)
		{
			continue;
		}

		FLootOption Loot = Entry.Loot;
		if (Loot.LootType == ELootType::WeaponSkill)
		{
			// Quantity multipliers are meaningless for a skill; WeaponSkillCharges is authored directly.
			if (Loot.WeaponSkillAsset)
			{
				PendingCorpseSkillLoot.Add(Loot);
			}
			continue;
		}

		Loot.Amount *= FMath::RandRange(Entry.MinQuantityMultiplier, Entry.MaxQuantityMultiplier);
		PendingCorpseBundledLoot.Add(Loot);
	}

	bLootableCorpse = RuntimeEnemyData->bLootableCorpse
		&& (!PendingCorpseBundledLoot.IsEmpty() || !PendingCorpseSkillLoot.IsEmpty());
}

void AEnemyCharacterBase::SpawnKillRewardPickups(bool bPlayFocusCue, float ScatterRadius, TArray<ARewardPickup*>* OutPickups)
{
	if (PendingCorpseBundledLoot.IsEmpty() && PendingCorpseSkillLoot.IsEmpty())
	{
		return;
	}

	AYogGameMode* GameMode = GetWorld() ? Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!GameMode)
	{
		return;
	}

	const FVector DeathLocation = GetActorLocation();
	int32 PickupIndex = 0;
	bool bSpawnedAny = false;

	const auto SpawnOne = [&](const TArray<FLootOption>& Loot)
	{
		FVector SpawnLocation = DeathLocation;
		if (ScatterRadius > 0.f)
		{
			const FVector2D Offset = FMath::RandPointInCircle(ScatterRadius);
			SpawnLocation += FVector(Offset.X, Offset.Y, 0.f);
		}
		else
		{
			SpawnLocation += FVector(PickupIndex * EnemyCharacter_KillRewardPickupSpacing, 0.f, 0.f);
		}

		++PickupIndex;
		ARewardPickup* Pickup = GameMode->SpawnEnemyKillRewardPickup(this, Loot, SpawnLocation, bPlayFocusCue);
		if (!Pickup)
		{
			return;
		}

		bSpawnedAny = true;
		if (OutPickups)
		{
			OutPickups->Add(Pickup);
		}
	};

	if (!PendingCorpseBundledLoot.IsEmpty())
	{
		SpawnOne(PendingCorpseBundledLoot);
	}

	// Each skill needs its own pickup: ShouldGrantLootImmediatelyForOptions rejects the whole
	// option array if any entry is a Rune, which would route the skill into the Rune-only card UI.
	for (const FLootOption& SkillLoot : PendingCorpseSkillLoot)
	{
		SpawnOne({ SkillLoot });
	}

	// Kept when nothing spawned (no RewardPickupClass) so a later interact can retry.
	if (bSpawnedAny)
	{
		PendingCorpseBundledLoot.Reset();
		PendingCorpseSkillLoot.Reset();
	}
}

void AEnemyCharacterBase::EnterLootableCorpseState()
{
	bCorpseActive = true;

	const UEnemyData* RuntimeEnemyData = GetEnemyData();
	UCapsuleComponent* BodyCapsule = GetCapsuleComponent();
	if (!RuntimeEnemyData || !BodyCapsule)
	{
		return;
	}

	const float BodyRadius = BodyCapsule->GetScaledCapsuleRadius();
	const float BodyHalfHeight = BodyCapsule->GetScaledCapsuleHalfHeight();

	// QueryOnly + WorldDynamic: a Pawn object type here would make every Pawn query (projectiles,
	// radius GE nodes) hit the corpse, so melee finds it through its own WorldDynamic overlap instead.
	const auto MakeVolume = [&](const TCHAR* Name, float Radius) -> UCapsuleComponent*
	{
		UCapsuleComponent* Volume = NewObject<UCapsuleComponent>(this, Name);
		Volume->InitCapsuleSize(Radius, FMath::Max(Radius, BodyHalfHeight));
		Volume->SetupAttachment(BodyCapsule);
		Volume->SetCollisionObjectType(ECC_WorldDynamic);
		Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
		Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		return Volume;
	};

	CorpseHitVolume = MakeVolume(TEXT("CorpseHitVolume"), BodyRadius);
	CorpseHitVolume->SetGenerateOverlapEvents(false);
	CorpseHitVolume->RegisterComponent();

	CorpseInteractVolume = MakeVolume(TEXT("CorpseInteractVolume"), FMath::Max(RuntimeEnemyData->CorpseInteractRadius, BodyRadius));
	CorpseInteractVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CorpseInteractVolume->SetGenerateOverlapEvents(true);
	CorpseInteractVolume->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacterBase::OnCorpseInteractBegin);
	CorpseInteractVolume->OnComponentEndOverlap.AddDynamic(this, &AEnemyCharacterBase::OnCorpseInteractEnd);
	CorpseInteractVolume->RegisterComponent();

	CorpsePromptComp = NewObject<UInteractPromptComponent>(this, TEXT("CorpseInteractPrompt"));
	CorpsePromptComp->PromptHeight = BodyHalfHeight + 60.f;
	CorpsePromptComp->RegisterComponent();

	// A player already standing on the corpse never gets a begin-overlap event otherwise.
	CorpseInteractVolume->UpdateOverlaps();
}

void AEnemyCharacterBase::OnCorpseInteractBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player || !IsCorpseInteractable())
	{
		return;
	}

	Player->RegisterInteractable(this);
	CorpseNearbyPlayer = Player;
	ShowInteractPrompt(true);
}

void AEnemyCharacterBase::OnCorpseInteractEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player)
	{
		return;
	}

	Player->UnregisterInteractable(this);
	if (CorpseNearbyPlayer.Get() == Player)
	{
		CorpseNearbyPlayer = nullptr;
		ShowInteractPrompt(false);
	}
}

void AEnemyCharacterBase::TryInteract(APlayerCharacterBase* Player)
{
	if (!Player || !IsCorpseInteractable())
	{
		return;
	}

	// Hand the loot to real pickups and let them grant it, so gold/material/skill grants, FX events
	// and the rune selection popup stay in one place (ARewardPickup::TryPickup).
	TArray<ARewardPickup*> Pickups;
	SpawnKillRewardPickups(false, 0.f, &Pickups);
	if (Pickups.IsEmpty())
	{
		return;
	}

	bCorpseConsumed = true;
	Player->UnregisterInteractable(this);
	ShowInteractPrompt(false);
	CorpseNearbyPlayer = nullptr;

	for (ARewardPickup* Pickup : Pickups)
	{
		if (IsValid(Pickup))
		{
			Pickup->TryPickup(Player);
		}
	}

	Destroy();
}

bool AEnemyCharacterBase::RegisterCorpseHit(const FGuid& SwingId)
{
	const UWorld* World = GetWorld();
	if (!World || !IsCorpseInteractable())
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (SwingId.IsValid())
	{
		if (SwingId == LastCorpseHitSwingId)
		{
			return false;
		}
		LastCorpseHitSwingId = SwingId;
	}
	else if (LastCorpseHitTime >= 0.f && Now - LastCorpseHitTime < EnemyCharacter_CorpseHitFallbackWindow)
	{
		return false;
	}
	LastCorpseHitTime = Now;

	const UEnemyData* RuntimeEnemyData = GetEnemyData();
	++CorpseHitCount;

	if (RuntimeEnemyData && RuntimeEnemyData->CorpseHitFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, RuntimeEnemyData->CorpseHitFX, GetActorLocation());
	}

	if (CorpseHitCount >= (RuntimeEnemyData ? FMath::Max(1, RuntimeEnemyData->CorpseHitsToExplode) : 1))
	{
		ExplodeCorpse();
	}
	return true;
}

void AEnemyCharacterBase::ExplodeCorpse()
{
	if (bCorpseConsumed)
	{
		return;
	}

	const UEnemyData* RuntimeEnemyData = GetEnemyData();
	if (RuntimeEnemyData && RuntimeEnemyData->CorpseExplosionFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RuntimeEnemyData->CorpseExplosionFX, GetActorLocation());
	}

	TArray<ARewardPickup*> Pickups;
	SpawnKillRewardPickups(false, EnemyCharacter_CorpseBurstScatterRadius, &Pickups);
	if (Pickups.IsEmpty())
	{
		return;
	}

	bCorpseConsumed = true;
	if (APlayerCharacterBase* Player = CorpseNearbyPlayer.Get())
	{
		Player->UnregisterInteractable(this);
	}
	CorpseNearbyPlayer = nullptr;
	Destroy();
}

void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerCharacterBase* Player = CorpseNearbyPlayer.Get())
	{
		Player->UnregisterInteractable(this);
	}
	DestroySpawnedEnemyWeaponActors();
	Super::EndPlay(EndPlayReason);
}

void AEnemyCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent && EnemyAttributeSet && !AbilitySystemComponent->GetAttributeSet(UEnemyAttributeSet::StaticClass()))
	{
		AbilitySystemComponent->AddAttributeSetSubobject(EnemyAttributeSet.Get());
	}
}


