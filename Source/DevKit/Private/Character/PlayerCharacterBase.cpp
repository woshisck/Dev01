// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Character/YogPlayerControllerBase.h"
#include "Camera/YogCameraPawn.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Buff/Aura/AuraBase.h"
#include "SaveGame/YogSaveGame.h"
#include "UObject/ConstructorHelpers.h"
#include "Data/GASTemplate.h"
#include "Item/ItemSpawner.h"
#include "Component/BackpackGridComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Component/SkillChargeComponent.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "System/YogGameInstanceBase.h"
#include "Item/Weapon/WeaponDefinition.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{
	BackpackGridComponent = CreateDefaultSubobject<UBackpackGridComponent>(TEXT("BackpackGridComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
	SkillChargeComponent = CreateDefaultSubobject<USkillChargeComponent>(TEXT("SkillChargeComponent"));

	// 近战默认命中框：C++ 实现，无需在每个角色蓝图 Class Defaults 中单独配置
	DefaultMeleeTargetType = UYogTargetType_Player::StaticClass();
}

void APlayerCharacterBase::SetOwnCamera(AYogCameraPawn* cameraActor)
{

	CameraPawnActor = cameraActor;
}

AYogCameraPawn* APlayerCharacterBase::GetOwnCamera()
{

	return CameraPawnActor;
}


void APlayerCharacterBase::Die()
{
	Super::Die();

	// 死亡时清空跑局状态，下一局从默认值开始
	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ClearRunState();
	}
}

void APlayerCharacterBase::RestoreRunStateFromGI()
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (!GI || !GI->PendingRunState.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE skipped — no valid state (bIsValid=%d)"),
			GI ? (int32)GI->PendingRunState.bIsValid : -1);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE — HP=%.1f Gold=%d Phase=%d Runes=%d"),
		GI->PendingRunState.CurrentHP, GI->PendingRunState.CurrentGold,
		GI->PendingRunState.CurrentPhase, GI->PendingRunState.PlacedRunes.Num());

	const FRunState& State = GI->PendingRunState;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	// 恢复 HP
	if (ASC && State.CurrentHP > 0.f)
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), State.CurrentHP);
	}

	// 恢复金币（现在由 BackpackGridComponent 持有）
	if (BackpackGridComponent)
	{
		BackpackGridComponent->Gold = FMath::Max(0, State.CurrentGold);
		BackpackGridComponent->OnGoldChanged.Broadcast(BackpackGridComponent->Gold);
	}

	// 恢复符文（仅非永久符文；永久符文由 BackpackGridComponent::BeginPlay 自动放置）
	if (BackpackGridComponent)
	{
		for (const FPlacedRune& PR : State.PlacedRunes)
		{
			if (!PR.bIsPermanent)
			{
				BackpackGridComponent->TryPlaceRune(PR.Rune, PR.Pivot);
			}
		}

		// 恢复热度阶段，并将热度重置到该阶段起点（避免热度值从 0 / 默认值开始）
		BackpackGridComponent->RestorePhase(State.CurrentPhase);
		BackpackGridComponent->ResetHeatToPhaseFloor();
	}

	// 恢复整理阶段已选但尚未放置的符文
	PendingRunes = State.PendingRunes;

	// 恢复武器装备
	if (State.EquippedWeaponDef)
	{
		State.EquippedWeaponDef->SetupWeaponToCharacter(GetMesh(), this);
		UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE Weapon — %s"), *State.EquippedWeaponDef->GetName());
	}
}

void APlayerCharacterBase::ItemInteract(const AItemSpawner* item)
{

	TArray<AActor*> actor_overlap_list;
	TSubclassOf<AItemSpawner> class_filter;
	this->GetCapsuleComponent()->GetOverlappingActors(actor_overlap_list, AItemSpawner::StaticClass());
	for (AActor* Actor : actor_overlap_list)
	{
		if (Actor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Overlapping Actor: %s"), *Actor->GetName());
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("item"));
}

void APlayerCharacterBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (AItemSpawner* Spawner = Cast<AItemSpawner>(OtherActor))
	{
		OverlappingSpawner = Spawner;
	}
}

void APlayerCharacterBase::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (OtherActor == OverlappingSpawner)
	{
		OverlappingSpawner = nullptr;
	}
}

UBackpackGridComponent* APlayerCharacterBase::GetBackpackGridComponent()
{
	return BackpackGridComponent;
}

void APlayerCharacterBase::AddRuneToInventory(const FRuneInstance& Rune)
{
	if (BackpackGridComponent)
	{
		// 1. 升级检查：背包已有同名符文 → 升级而非新占格子
		FPlacedRune* Existing = BackpackGridComponent->FindRuneByName(Rune.RuneConfig.RuneName);
		if (Existing)
		{
			if (Existing->Rune.UpgradeLevel < 2)
			{
				Existing->Rune.UpgradeLevel++;
				UE_LOG(LogTemp, Log, TEXT("AddRuneToInventory: %s 升级到 Lv.%d"),
					*Rune.RuneConfig.RuneName.ToString(), Existing->Rune.UpgradeLevel + 1);
				// 重启 BuffFlow 使新 UpgradeLevel 生效，并广播 UI 刷新事件
				BackpackGridComponent->NotifyRuneUpgraded(Existing->Rune.RuneGuid);
			}
			// 满级（UpgradeLevel == 2）：GenerateLootBatch 已过滤，正常不会到达这里
			return;
		}

		// 2. 新符文：自动寻位放置（左上角开始，逐行扫描）
		for (int32 Row = 0; Row < BackpackGridComponent->GridHeight; Row++)
		{
			for (int32 Col = 0; Col < BackpackGridComponent->GridWidth; Col++)
			{
				if (BackpackGridComponent->TryPlaceRune(Rune, FIntPoint(Col, Row)))
				{
					UE_LOG(LogTemp, Log, TEXT("AddRuneToInventory: %s 自动放置到 (%d,%d)"),
						*Rune.RuneConfig.RuneName.ToString(), Col, Row);
					return;
				}
			}
		}
	}

	// 3. 背包已满，进入待放置列表（玩家可在背包UI手动放置）
	UE_LOG(LogTemp, Warning, TEXT("AddRuneToInventory: 背包已满，%s 进入待放置列表"),
		*Rune.RuneConfig.RuneName.ToString());
	PendingRunes.Add(Rune);
}



void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 将 BackpackGridComponent 与 ASC 关联，使符文激活时可施加 GE
	if (BackpackGridComponent && GetAbilitySystemComponent())
	{
		BackpackGridComponent->InitWithASC(GetAbilitySystemComponent());
	}

	// 初始化技能充能系统
	if (SkillChargeComponent && GetAbilitySystemComponent() && PlayerAttributeSet)
	{
		SkillChargeComponent->InitWithASC(GetAbilitySystemComponent());

		// 注册冲刺充能（MaxDashCharge / DashCooldownDuration 均为 GAS 属性，符文可修改）
		// 注册键与 GA.AbilityTags 保持一致：PlayerState.AbilityCast.Dash
		SkillChargeComponent->RegisterSkill(
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash")),
			UPlayerAttributeSet::GetMaxDashChargeAttribute(),
			UPlayerAttributeSet::GetDashCooldownDurationAttribute()
		);
		// 新技能在此处继续 RegisterSkill 即可，无需额外 C++ 改动
	}

	// GAS Template 授能（在 Super::BeginPlay 中完成）可能覆盖切关前 Link 的武器动画层；
	// 在此重新 Link，确保武器层优先级高于默认层
	RelinkWeaponAnimLayer();

	//GetASC()->InitAbilityActorInfo(this, this);
	//if (GasTemplate != nullptr)
	//{
	//	for (TSubclassOf<UYogGameplayAbility> ablity_class : GasTemplate->PassiveMap)
	//	{
	//		//TODO: confirm about the inputID
	//		GetASC()->K2_GiveAbility(ablity_class, 0, 0);
	//	}
	//	for (TSubclassOf<UYogGameplayAbility> ablity_class : GasTemplate->AbilityMap)
	//	{
	//		GetASC()->K2_GiveAbility(ablity_class, 0, 0);
	//	}
	//	for (const TSubclassOf<UYogGameplayEffect> effect_class : GasTemplate->PassiveEffect)
	//	{
	//		//TODO: fix bug when the ower is null
	//		//UYogAbilitySystemComponent* asc = this->GetASC();
	//		//FGameplayEffectContextHandle Context = asc->MakeEffectContext();
	//		//FGameplayEffectSpecHandle SpecHandle = asc->MakeOutgoingSpec(effect_class, 0, Context);
	//		//
	//		//if (SpecHandle.IsValid())
	//		//{
	//		//	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	//		//	asc->ApplyGameplayEffectSpecToSelf(*Spec);
	//		//}
	//	}
	//}
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

void APlayerCharacterBase::RelinkWeaponAnimLayer()
{
	if (!EquippedWeaponDef) return;
	USkeletalMeshComponent* SkelMesh = GetMesh();
	if (!SkelMesh || !SkelMesh->GetAnimInstance()) return;

	for (const FWeaponSpawnData& SpawnData : EquippedWeaponDef->ActorsToSpawn)
	{
		if (SpawnData.WeaponLayer)
		{
			SkelMesh->GetAnimInstance()->LinkAnimClassLayers(SpawnData.WeaponLayer);
		}
	}
}

void APlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();


}


