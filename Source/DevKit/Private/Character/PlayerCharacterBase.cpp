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
#include "Materials/MaterialInstanceDynamic.h"

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

		// 2. 新符文：直接进入待放置列表（玩家在背包UI手动拖入格子）
		UE_LOG(LogTemp, Log, TEXT("AddRuneToInventory: %s → 待放置列表"),
			*Rune.RuneConfig.RuneName.ToString());
	}

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

	// 注册热度阶段 Tag 事件，驱动武器边缘发光材质
	SetupHeatPhaseTagListeners();

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
	TickPlayerPhaseGlow(DeltaSeconds);
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

void APlayerCharacterBase::SetupHeatPhaseTagListeners()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	// 监听各阶段 tag 新增 → 广播对应 Phase
	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.1")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::OnHeatPhaseTagChanged);

	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.2")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::OnHeatPhaseTagChanged);

	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.3")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::OnHeatPhaseTagChanged);

	// 监听 parent tag 归零 → 广播 Phase=0（关闭发光）
	ASC->RegisterGameplayTagEvent(
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase")),
		EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::OnHeatPhaseParentTagChanged);
}

void APlayerCharacterBase::OnHeatPhaseTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0) return; // 移除事件交由 parent tag 回调处理

	static const FGameplayTag Phase1 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.1"));
	static const FGameplayTag Phase2 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.2"));
	static const FGameplayTag Phase3 = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.3"));

	int32 BroadcastPhase = 0;
	if      (Tag == Phase1) BroadcastPhase = 1;
	else if (Tag == Phase2) BroadcastPhase = 2;
	else if (Tag == Phase3) BroadcastPhase = 3;
	OnHeatPhaseChanged.Broadcast(BroadcastPhase);
	StartPlayerPhaseGlow(BroadcastPhase);

	// 手柄震动
	if (PhaseUpForceFeedback)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientPlayForceFeedback(PhaseUpForceFeedback);
		}
	}
}

void APlayerCharacterBase::OnHeatPhaseParentTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount == 0)
	{
		OnHeatPhaseChanged.Broadcast(0);
	}
}

void APlayerCharacterBase::StartPlayerPhaseGlow(int32 Phase)
{
	if (Phase == 0 || !PhaseUpPlayerOverlayMaterial) return;

	// Phase 1=冷白/淡蓝(0.3强) Phase 2=暖橙(0.7强) Phase 3=金色(1.5强)
	static const FLinearColor PhaseColors[] =
	{
		FLinearColor(0.f,   0.f,   0.f  ),  // 0: 无
		FLinearColor(0.9f,  1.0f,  1.8f ),  // 1: 冷白/淡蓝
		FLinearColor(2.5f,  1.0f,  0.08f),  // 2: 暖橙
		FLinearColor(5.5f,  4.0f,  0.3f ),  // 3: 金色
		FLinearColor(5.5f,  4.0f,  0.3f ),  // 4: 占位
	};

	if (!PlayerOverlayDynMat)
	{
		PlayerOverlayDynMat = UMaterialInstanceDynamic::Create(PhaseUpPlayerOverlayMaterial, this);
	}

	const int32 Idx = FMath::Clamp(Phase, 0, 4);
	PlayerOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), PhaseColors[Idx]);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 0.f);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), 1.f);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("IridIntensity"), GlowIridIntensity);
	GetMesh()->SetOverlayMaterial(PlayerOverlayDynMat);
	PhaseGlowElapsed = 0.f;
}

void APlayerCharacterBase::TickPlayerPhaseGlow(float DeltaTime)
{
	if (PhaseGlowElapsed < 0.f) return;

	PhaseGlowElapsed += DeltaTime;

	const float HoldEnd      = GlowSweepDuration + GlowHoldDuration;
	const float TotalDuration = HoldEnd + GlowFadeDuration;

	float SweepProgress, GlowAlpha;

	if (PhaseGlowElapsed < GlowSweepDuration)
	{
		SweepProgress = PhaseGlowElapsed / GlowSweepDuration;
		GlowAlpha     = 1.f;
	}
	else if (PhaseGlowElapsed < HoldEnd)
	{
		SweepProgress = 1.f;
		GlowAlpha     = 1.f;
	}
	else if (PhaseGlowElapsed < TotalDuration)
	{
		SweepProgress = 1.f;
		GlowAlpha     = 1.f - (PhaseGlowElapsed - HoldEnd) / GlowFadeDuration;
	}
	else
	{
		GetMesh()->SetOverlayMaterial(nullptr);
		PhaseGlowElapsed = -1.f;
		return;
	}

	if (PlayerOverlayDynMat)
	{
		PlayerOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), SweepProgress);
		PlayerOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), GlowAlpha);
	}
}

