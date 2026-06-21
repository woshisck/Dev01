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
#include "GameModes/YogGameMode.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Component/EnemyHealthDisplayComponent.h"
#include "Component/MontageVFXBindingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Item/Weapon/WeaponInstance.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BrainComponent.h"

namespace
{
	bool EnemyWeaponHasUsableMontageConfigList(const FAbilityMontageConfigList& ConfigList)
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

	bool EnemyWeaponHasUsablePassiveActionData(const FPassiveActionData& PassiveData)
	{
		return PassiveData.Montage != nullptr
			|| !PassiveData.UniqueEffects.IsEmpty()
			|| PassiveData.DissolveGameplayCueTag.IsValid();
	}

	void MergeEnemyWeaponAbilityDataInto(UAbilityData& Target, const UAbilityData& Source)
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

	bool EnemyWeaponHasAttributeAdd(const FYogBaseAttributeData& Add)
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

	void AddEnemyWeaponAttribute(UAttributeStatComponent* Stats, const FGameplayAttribute& Attribute, float Delta)
	{
		if (Stats && !FMath::IsNearlyZero(Delta))
		{
			Stats->AddAttribute(Attribute, Delta);
		}
	}

	void MultiplyEnemyWeaponAttribute(UAttributeStatComponent* Stats, const FGameplayAttribute& Attribute, float Multiplier)
	{
		if (Stats && !FMath::IsNearlyEqual(Multiplier, 1.0f))
		{
			Stats->MultiplyAttribute(Attribute, Multiplier);
		}
	}
}

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
	HealthDisplayComponent = CreateDefaultSubobject<UEnemyHealthDisplayComponent>(TEXT("EnemyHealthDisplayComponent"));
	MontageVFXBindingComponent = CreateDefaultSubobject<UMontageVFXBindingComponent>(TEXT("MontageVFXBindingComponent"));

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

	// 从 EnemyData DA 推送霸体参数到 ASC（填表即生效，不需改每个敌人 BP）
	if (CharacterDataComponent && AbilitySystemComponent)
	{
		if (UEnemyData* ED = Cast<UEnemyData>(CharacterDataComponent->GetCharacterData()))
		{
			AbilitySystemComponent->SuperArmorThreshold = ED->SuperArmorThreshold;
			AbilitySystemComponent->SuperArmorDuration  = ED->SuperArmorDuration;
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
	return FMath::Max(0.0f, DeathDisappearDelayAfterAnimation);
}

void AEnemyCharacterBase::SetAIAttackRuntimeContext(const FEnemyAIAttackOption& AttackOption, AActor* TargetActor, float DistanceToTarget)
{
	PendingAIAttackContext.bValid = true;
	PendingAIAttackContext.AttackOption = AttackOption;
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

	if (WeaponDefinition->bOverrideAttackProfile)
	{
		RuntimeEnemyData->AttackProfile = WeaponDefinition->AttackProfile;
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

	UE_LOG(LogTemp, Log, TEXT("[EnemyWeapon] Applied Weapon=%s Enemy=%s AbilityData=%s AttackProfileAttacks=%d"),
		*GetNameSafe(WeaponDefinition),
		*GetNameSafe(this),
		*GetNameSafe(RuntimeEnemyData->AbilityData.Get()),
		RuntimeEnemyData->AttackProfile.Attacks.Num());
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

	Super::Die();

	// 关闭 AI：停止 BT、停止移动、清除焦点、关掉自动朝向，防止尸体仍然朝玩家转向
	// Super::Die() 已关闭胶囊碰撞但不会停 AI；BT 仍会下发 MoveTo，CMC.Velocity 不为 0 时
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
	// Keep this short enough that a missing cooked death ability cannot leave dead enemies in the room.
	SetLifeSpan(8.0f);
}

void AEnemyCharacterBase::FinishDying()
{
	DestroySpawnedEnemyWeaponActors();
	Super::FinishDying();
}

void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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


