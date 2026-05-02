// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyCharacterBase.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Data/EnemyData.h"
#include "Component/CharacterDataComponent.h"
#include "Controller/YogAIController.h"
#include "Data/GASTemplate.h"
#include "GameModes/YogGameMode.h"
#include "BuffFlow/BuffFlowComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));

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

	// 通知 GameMode 击杀计数，触发波次/关卡完成检查
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->LastEnemyKillLocation = GetActorLocation();
		GM->UpdateFinishLevel(1);
		// 从相机战斗感知列表中移除
		GM->UnregisterEnemy(this);
	}

	// 绝对兜底：正常死亡销毁由 GA_Dead 处理（有蒙太奇→动画结束销毁；无蒙太奇→2秒后销毁）
	// 此处仅在 GA_Dead 完全未激活（配置遗漏）时作为最后保险，设置为足够长的时间
	// Fallback only: the normal path is GA_Dead -> montage/dissolve -> FinishDying().
	// Keep this short enough that a missing cooked death ability cannot leave dead enemies in the room.
	SetLifeSpan(8.0f);
}

void AEnemyCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}


