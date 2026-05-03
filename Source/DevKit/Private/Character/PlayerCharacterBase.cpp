// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacterBase.h"
#include "UI/BackpackStyleDataAsset.h"
#include "UI/DamageEdgeFlashWidget.h"
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
#include "Component/CombatDeckComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Component/SkillChargeComponent.h"
#include "Data/SacrificeGraceDA.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "System/YogGameInstanceBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/YogGameMode.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{
	BackpackGridComponent = CreateDefaultSubobject<UBackpackGridComponent>(TEXT("BackpackGridComponent"));
	CombatDeckComponent = CreateDefaultSubobject<UCombatDeckComponent>(TEXT("CombatDeckComponent"));
	ComboRuntimeComponent = CreateDefaultSubobject<UComboRuntimeComponent>(TEXT("ComboRuntimeComponent"));
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

	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->HandlePlayerDeath(this);
	}

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

	const FRunState State = GI->PendingRunState;
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

	// 恢复整理阶段已选但尚未放置的符文
	PendingRunes = State.PendingRunes;

	// 先恢复武器，让背包尺寸/激活区配置稳定后，再统一恢复符文效果和监听。
	if (State.EquippedWeaponDef)
	{
		State.EquippedWeaponDef->SetupWeaponToCharacter(GetMesh(), this);
		UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE Weapon - %s"), *State.EquippedWeaponDef->GetName());
	}

	if (CombatDeckComponent && !State.CombatDeckCards.IsEmpty())
	{
		TArray<URuneDataAsset*> RestoredDeckAssets;
		RestoredDeckAssets.Reserve(State.CombatDeckCards.Num());
		for (URuneDataAsset* SourceAsset : State.CombatDeckCards)
		{
			RestoredDeckAssets.Add(SourceAsset);
		}
		CombatDeckComponent->LoadDeckFromSourceAssets(
			RestoredDeckAssets,
			State.CombatDeckShuffleCooldownDuration,
			State.CombatDeckMaxActiveSequenceSize);
	}

	if (BackpackGridComponent)
	{
		BackpackGridComponent->RestorePlacedRunes(State.PlacedRunes);
		BackpackGridComponent->RestorePhase(State.CurrentPhase);

		if (UAbilitySystemComponent* HeatASC = GetAbilitySystemComponent())
		{
			const float MaxHeat = HeatASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
			float HeatToSet = State.CurrentHeat;

			if (HeatToSet > MaxHeat && MaxHeat > KINDA_SMALL_NUMBER)
			{
				BackpackGridComponent->IncrementPhase();
				HeatToSet = FMath::Max(0.f, HeatToSet - MaxHeat);
			}

			HeatToSet = FMath::Clamp(HeatToSet, 0.f, MaxHeat);
			HeatASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), HeatToSet);
			BackpackGridComponent->OnHeatValueChanged(HeatToSet);
		}
	}

	// 恢复献祭恩赐
	ActiveSacrificeGrace = State.ActiveSacrificeGrace;
	if (ActiveSacrificeGrace)
	{
		AcquireSacrificeGrace(ActiveSacrificeGrace);
	}

	// 恢复运行时隐藏被动符文（无形状、跳过格子系统）
	if (BackpackGridComponent && !State.HiddenPassiveRuneInstances.IsEmpty())
	{
		BackpackGridComponent->RestoreRuntimeHiddenPassiveRunes(State.HiddenPassiveRuneInstances);
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
}

void APlayerCharacterBase::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
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
				BackpackGridComponent->NotifyRuneUpgraded(Existing->Rune.RuneGuid);
			}
			return;
		}

		// 2. 无形状符文：跳过格子系统，直接作为隐藏被动激活
		if (Rune.Shape.Cells.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("AddRuneToInventory: %s 无形状 → 隐藏被动激活"),
				*Rune.RuneConfig.RuneName.ToString());
			BackpackGridComponent->AddHiddenPassiveRune(Rune);
			return;
		}

		// 3. 普通符文：进入待放置列表（玩家在背包 UI 手动拖入格子）
		UE_LOG(LogTemp, Log, TEXT("AddRuneToInventory: %s → 待放置列表"),
			*Rune.RuneConfig.RuneName.ToString());
	}

	PendingRunes.Add(Rune);
}



void APlayerCharacterBase::AcquireSacrificeGrace(USacrificeGraceDA* DA)
{
	if (!DA || !BuffFlowComponent) return;

	ActiveSacrificeGrace = DA;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	// 立即将热度设为满值（Override GE）
	if (ASC)
	{
		const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
		if (MaxHeat > 0.f)
		{
			ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), MaxHeat);
		}

		// 施加奖励 GE（Infinite 类型，随 Run 持续）
		if (DA->BonusEffect)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DA->BonusEffect, 1.f, Ctx);
			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	// 启动衰退 FA（FA 内含 BFNode_SacrificeDecay，In 引脚自动触发）
	if (DA->FlowAsset)
	{
		BuffFlowComponent->StartBuffFlow(DA->FlowAsset, FGuid::NewGuid(), this);
	}
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
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		YogASC->ReceivedDamage.AddUniqueDynamic(this, &APlayerCharacterBase::HandleDamageReceivedFeedback);
	}

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

	// 切关后重新激活献祭恩赐（热度重充满 + 衰退速率归零 + FA 重启）
	if (ActiveSacrificeGrace)
	{
		AcquireSacrificeGrace(ActiveSacrificeGrace);
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

void APlayerCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		YogASC->ReceivedDamage.RemoveDynamic(this, &APlayerCharacterBase::HandleDamageReceivedFeedback);
	}

	RestoreDamageTimeDilation();
	Super::EndPlay(EndPlayReason);
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DamageGlowElapsed >= 0.f)
	{
		TickDamagePlayerGlow(DeltaSeconds);
	}
	else
	{
		TickPlayerPhaseGlow(DeltaSeconds);
	}
}

void APlayerCharacterBase::HandleDamageReceivedFeedback(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	(void)SourceASC;

	if (!bEnableDamageReceivedFeedback || Damage <= 0.f)
	{
		return;
	}

	PlayDamageScreenFlash();
	StartDamagePlayerGlow();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			if (DamageReceivedForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageReceivedForceFeedback);
			}
			else if (DamageDynamicForceFeedbackIntensity > 0.f && DamageDynamicForceFeedbackDuration > 0.f)
			{
				PC->PlayDynamicForceFeedback(
					DamageDynamicForceFeedbackIntensity,
					DamageDynamicForceFeedbackDuration,
					true,
					true,
					true,
					true,
					EDynamicForceFeedbackAction::Start);
			}
		}
	}

	StartDamageTimeDilation();
}

void APlayerCharacterBase::PlayDamageScreenFlash()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (!DamageEdgeFlashWidget)
	{
		DamageEdgeFlashWidget = CreateWidget<UDamageEdgeFlashWidget>(PC, UDamageEdgeFlashWidget::StaticClass());
		if (DamageEdgeFlashWidget)
		{
			DamageEdgeFlashWidget->AddToViewport(100);
			DamageEdgeFlashWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			DamageEdgeFlashWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
			DamageEdgeFlashWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
		}
	}

	if (DamageEdgeFlashWidget)
	{
		DamageEdgeFlashWidget->PlayEdgeFlash(
			DamageScreenFlashColor,
			DamageScreenFlashAlpha,
			DamageScreenFlashDuration,
			DamageScreenEdgeThickness);
	}
}

void APlayerCharacterBase::StartDamagePlayerGlow()
{
	UMaterialInterface* OverlayMaterial = DamagePlayerOverlayMaterial.Get();
	if (!OverlayMaterial)
	{
		OverlayMaterial = PhaseUpPlayerOverlayMaterial.Get();
	}
	if (!OverlayMaterial || !GetMesh())
	{
		return;
	}

	DamageOverlayDynMat = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
	if (!DamageOverlayDynMat)
	{
		return;
	}

	DamageOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), DamagePlayerGlowColor);
	DamageOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 1.f);
	DamageOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), 1.f);
	DamageOverlayDynMat->SetScalarParameterValue(TEXT("IridIntensity"), DamagePlayerGlowIridIntensity);
	GetMesh()->SetOverlayMaterial(DamageOverlayDynMat);
	DamageGlowElapsed = 0.f;
}

void APlayerCharacterBase::TickDamagePlayerGlow(float DeltaTime)
{
	if (DamageGlowElapsed < 0.f)
	{
		return;
	}

	DamageGlowElapsed += DeltaTime;
	const float Duration = FMath::Max(0.01f, DamagePlayerGlowDuration);
	const float GlowAlpha = 1.f - FMath::Clamp(DamageGlowElapsed / Duration, 0.f, 1.f);

	if (DamageOverlayDynMat)
	{
		DamageOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 1.f);
		DamageOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), GlowAlpha);
	}

	if (DamageGlowElapsed >= Duration)
	{
		DamageGlowElapsed = -1.f;
		if (GetMesh())
		{
			if (PhaseGlowElapsed >= 0.f && PlayerOverlayDynMat)
			{
				GetMesh()->SetOverlayMaterial(PlayerOverlayDynMat);
			}
			else
			{
				GetMesh()->SetOverlayMaterial(nullptr);
			}
		}
	}
}

void APlayerCharacterBase::StartDamageTimeDilation()
{
	if (!bEnableDamageTimeDilation || DamageTimeDilationDuration <= 0.f || DamageTimeDilationScale >= 1.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (DamageTimeDilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DamageTimeDilationTickerHandle);
		DamageTimeDilationTickerHandle.Reset();
	}
	else
	{
		PreviousDamageGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
	}

	UGameplayStatics::SetGlobalTimeDilation(World, DamageTimeDilationScale);

	TWeakObjectPtr<APlayerCharacterBase> WeakThis = this;
	DamageTimeDilationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float) -> bool
		{
			if (WeakThis.IsValid())
			{
				WeakThis->RestoreDamageTimeDilation();
			}
			return false;
		}),
		DamageTimeDilationDuration);
}

void APlayerCharacterBase::RestoreDamageTimeDilation()
{
	const bool bHadActiveDilation = DamageTimeDilationTickerHandle.IsValid();
	if (bHadActiveDilation)
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DamageTimeDilationTickerHandle);
		DamageTimeDilationTickerHandle.Reset();
	}

	if (bHadActiveDilation)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGlobalTimeDilation(World, PreviousDamageGlobalTimeDilation);
		}
	}
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
	CurrentHeatPhase = BroadcastPhase;
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
		CurrentHeatPhase = 0;
		OnHeatPhaseChanged.Broadcast(0);
	}
}

void APlayerCharacterBase::StartPlayerPhaseGlow(int32 Phase)
{
	if (Phase == 0 || !PhaseUpPlayerOverlayMaterial) return;

	FLinearColor GlowColors[5] = {
		FLinearColor::Black,
		FLinearColor(0.9f,  1.0f,  1.8f ),
		FLinearColor(2.5f,  1.0f,  0.08f),
		FLinearColor(5.5f,  4.0f,  0.3f ),
		FLinearColor(5.5f,  4.0f,  0.3f ),
	};
	if (HeatStyleDA)
	{
		GlowColors[1] = HeatStyleDA->Phase1GlowColor;
		GlowColors[2] = HeatStyleDA->Phase2GlowColor;
		GlowColors[3] = HeatStyleDA->Phase3GlowColor;
		GlowColors[4] = HeatStyleDA->Phase3GlowColor;
	}

	if (!PlayerOverlayDynMat)
	{
		PlayerOverlayDynMat = UMaterialInstanceDynamic::Create(PhaseUpPlayerOverlayMaterial, this);
	}

	const int32 Idx = FMath::Clamp(Phase, 0, 4);
	PlayerOverlayDynMat->SetVectorParameterValue(TEXT("EmissiveColor"), GlowColors[Idx]);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("SweepProgress"), 0.f);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("GlowAlpha"), 1.f);
	PlayerOverlayDynMat->SetScalarParameterValue(TEXT("IridIntensity"), GlowIridIntensity);
	if (DamageGlowElapsed < 0.f)
	{
		GetMesh()->SetOverlayMaterial(PlayerOverlayDynMat);
	}
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

