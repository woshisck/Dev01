// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacterBase.h"
#include "UI/BackpackStyleDataAsset.h"
#include "UI/DamageEdgeFlashWidget.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/YogSpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Abilities/GA_PlayerAttackCombos.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
#include "Character/YogPlayerControllerBase.h"
#include "Camera/YogCameraPawn.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Buff/Aura/AuraBase.h"
#include "SaveGame/YogSaveGame.h"
#include "AbilitySystem/Abilities/YogAbilitySet.h"
#include "Data/GASTemplate.h"
#include "Item/ItemSpawner.h"
#include "Component/BackpackGridComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "Component/CombatDeckComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Component/SacrificeRuneComponent.h"
#include "Combat/FinisherDeprecation.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Component/SkillChargeComponent.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Component/YogCameraOcclusionFadeComponent.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/SacrificeGraceDA.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "System/YogGameInstanceBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/YogGameMode.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Story/StoryEngineSubsystem.h"
#include "Tutorial/TutorialManager.h"
#include "Visual/TimeDilationVisualSubsystem.h"
#include "YogBlueprintFunctionLibrary.h"

namespace
{
FGameplayTag GetPostAttackRecoveryTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.PostAttackRecovery"), false);
}

FGameplayTag GetRecoveryCancelBonusTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.RecoveryCancelBonus"), false);
}

void AddSacrificeCostModifier(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, float Delta)
{
	if (!Effect || FMath::IsNearlyZero(Delta))
	{
		return;
	}

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Delta));
	Effect->Modifiers.Add(Modifier);
}

bool ApplySacrificeCostStateToASC(UAbilitySystemComponent* ASC, const FSacrificeOfferingCostState& State, UObject* SourceObject)
{
	if (!ASC)
	{
		return false;
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddSacrificeCostModifier(Effect, UBaseAttributeSet::GetAttackAttribute(), State.AttackDelta);
	AddSacrificeCostModifier(Effect, UBaseAttributeSet::GetDmgTakenAttribute(), State.DmgTakenDelta);
	AddSacrificeCostModifier(Effect, UBaseAttributeSet::GetCrit_RateAttribute(), State.CritRateDelta);
	AddSacrificeCostModifier(Effect, UBaseAttributeSet::GetCrit_DamageAttribute(), State.CritDamageDelta);

	if (Effect->Modifiers.IsEmpty())
	{
		return true;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (AActor* SourceActor = Cast<AActor>(SourceObject))
	{
		Context.AddInstigator(SourceActor, SourceActor);
	}
	Context.AddSourceObject(SourceObject);
	FGameplayEffectSpec Spec(Effect, Context, 1.0f);
	return ASC->ApplyGameplayEffectSpecToSelf(Spec).IsValid();
}

void CopyDeckRuntimeStateToRunStateFields(
	const FWeaponCombatDeckRuntimeState& DeckState,
	TArray<TObjectPtr<URuneDataAsset>>& OutCards,
	TArray<ECombatCardLinkOrientation>& OutOrientations,
	float& OutShuffleCooldownDuration,
	int32& OutMaxActiveSequenceSize)
{
	OutCards.Reset(DeckState.SourceAssets.Num());
	for (const TObjectPtr<URuneDataAsset>& SourceAsset : DeckState.SourceAssets)
	{
		if (SourceAsset)
		{
			OutCards.Add(SourceAsset);
		}
	}

	OutOrientations = DeckState.AttackCardOrientations;
	OutShuffleCooldownDuration = DeckState.ShuffleCooldownDuration;
	OutMaxActiveSequenceSize = DeckState.MaxActiveSequenceSize;
}

void RestoreDeckRuntimeStateFromRunStateFields(
	FWeaponCombatDeckRuntimeState& DeckState,
	const TArray<TObjectPtr<URuneDataAsset>>& SourceAssets,
	const TArray<ECombatCardLinkOrientation>& Orientations,
	float ShuffleCooldownDuration,
	int32 MaxActiveSequenceSize)
{
	DeckState.Reset();
	DeckState.SourceAssets = SourceAssets;
	DeckState.AttackCardOrientations = Orientations;
	DeckState.ShuffleCooldownDuration = ShuffleCooldownDuration;
	DeckState.MaxActiveSequenceSize = MaxActiveSequenceSize;
	DeckState.bInitialized = true;
}

bool HasUsableMontageConfigList(const FAbilityMontageConfigList& ConfigList)
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

bool HasUsablePassiveActionData(const FPassiveActionData& PassiveData)
{
	return PassiveData.Montage != nullptr
		|| !PassiveData.UniqueEffects.IsEmpty()
		|| PassiveData.DissolveGameplayCueTag.IsValid();
}

void MergeAbilityDataInto(UAbilityData& Target, const UAbilityData& Source)
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
		if (Pair.Key.IsValid() && HasUsableMontageConfigList(Pair.Value))
		{
			Target.MontageConfigMap.Add(Pair.Key, Pair.Value);
		}
	}

	for (const TPair<FGameplayTag, FPassiveActionData>& Pair : Source.PassiveMap)
	{
		if (Pair.Key.IsValid() && HasUsablePassiveActionData(Pair.Value))
		{
			Target.PassiveMap.Add(Pair.Key, Pair.Value);
		}
	}
}
}

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<UYogSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = DefaultCameraBoomLength;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bfollowPlayer = true;
	CameraBoom->bReverseLag = false;
	CameraBoom->bStayOffset = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->FieldOfView = DefaultCameraFOV;

	CameraOcclusionFadeComponent = CreateDefaultSubobject<UYogCameraOcclusionFadeComponent>(TEXT("CameraOcclusionFadeComponent"));

	BackpackGridComponent = CreateDefaultSubobject<UBackpackGridComponent>(TEXT("BackpackGridComponent"));
	CombatDeckComponent = CreateDefaultSubobject<UCombatDeckComponent>(TEXT("CombatDeckComponent"));
	CombatItemComponent = CreateDefaultSubobject<UCombatItemComponent>(TEXT("CombatItemComponent"));
	ActiveSkillComponent = CreateDefaultSubobject<UPlayerActiveSkillComponent>(TEXT("ActiveSkillComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
	BuffFlowComponent = CreateDefaultSubobject<UBuffFlowComponent>(TEXT("BuffFlowComponent"));
	SacrificeRuneComponent = CreateDefaultSubobject<USacrificeRuneComponent>(TEXT("SacrificeRuneComponent"));
	SkillChargeComponent = CreateDefaultSubobject<USkillChargeComponent>(TEXT("SkillChargeComponent"));
	MontageVFXBindingComponent = CreateDefaultSubobject<UMontageVFXBindingComponent>(TEXT("MontageVFXBindingComponent"));

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
}

void APlayerCharacterBase::FinishDying()
{
	if (bWaitingForDeathReviveChoice)
	{
		return;
	}

	Super::FinishDying();
}

void APlayerCharacterBase::PrepareForDeathReviveChoice()
{
	bWaitingForDeathReviveChoice = true;
}

void APlayerCharacterBase::ReviveFromDeath(float ReviveHealthPercent, float ProtectionDuration)
{
	UYogAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer DeathTags;
	DeathTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.Dead"), false));
	ASC->CancelAbilities(&DeathTags);
	ASC->SetLooseGameplayTagCount(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false), 0);

	bIsDead = false;
	ResetDeathStartedBroadcast();
	bWaitingForDeathReviveChoice = false;
	CustomTimeDilation = 1.f;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bPauseAnims = false;
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	UnblockMovementControl();

	const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float ReviveHealth = AYogGameMode::CalculatePlayerReviveHealth(MaxHealth, ReviveHealthPercent);
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), ReviveHealth);

	static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
	static const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Invulnerable"), false);
	if (SuperArmorTag.IsValid())
	{
		ASC->AddLooseGameplayTag(SuperArmorTag);
	}
	if (InvulnerableTag.IsValid())
	{
		ASC->AddLooseGameplayTag(InvulnerableTag);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveProtectionTimerHandle);
		World->GetTimerManager().SetTimer(
			ReviveProtectionTimerHandle,
			this,
			&APlayerCharacterBase::EndReviveProtection,
			FMath::Max(0.f, ProtectionDuration),
			false);
	}
}

void APlayerCharacterBase::EndReviveProtection()
{
	if (UYogAbilitySystemComponent* ASC = GetASC())
	{
		static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
		static const FGameplayTag InvulnerableTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Invulnerable"), false);
		if (SuperArmorTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(SuperArmorTag);
		}
		if (InvulnerableTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(InvulnerableTag);
		}
	}
}

void APlayerCharacterBase::ApplyAbilityDataFromWeapon(UWeaponDefinition* WeaponDefinition)
{
	UCharacterDataComponent* CDC = GetCharacterDataComponent();
	UCharacterData* CharacterData = CDC ? CDC->GetCharacterData() : nullptr;
	if (!CharacterData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponAbilityData] Skip: no runtime CharacterData on %s for weapon %s"),
			*GetNameSafe(this), *GetNameSafe(WeaponDefinition));
		return;
	}

	TArray<UAbilityData*> SourceData;
	UAbilityData* BaseAbilityData = CDC ? CDC->GetBaseAbilityData() : CharacterData->AbilityData.Get();
	if (BaseAbilityData)
	{
		SourceData.Add(BaseAbilityData);
	}
	int32 WeaponSourceCount = 0;
	if (WeaponDefinition)
	{
		auto AddWeaponAbilityDataSource = [&SourceData, &WeaponSourceCount](UAbilityData* AbilityData)
		{
			if (AbilityData)
			{
				SourceData.Add(AbilityData);
				++WeaponSourceCount;
			}
		};

		AddWeaponAbilityDataSource(WeaponDefinition->AttackAbilityData);
		AddWeaponAbilityDataSource(WeaponDefinition->WeaponSkillAbilityData);
		AddWeaponAbilityDataSource(WeaponDefinition->SpecialAbilityData);
		AddWeaponAbilityDataSource(WeaponDefinition->PassiveAbilityData);
	}

	if (SourceData.IsEmpty())
	{
		CharacterData->AbilityData = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[WeaponAbilityData] Cleared: no base or weapon AbilityData on %s for weapon %s"),
			*GetNameSafe(this),
			*GetNameSafe(WeaponDefinition));
		return;
	}

	if (SourceData.Num() == 1)
	{
		CharacterData->AbilityData = SourceData[0];
	}
	else
	{
		UAbilityData* RuntimeAbilityData = NewObject<UAbilityData>(this);
		RuntimeAbilityData->SetFlags(RF_Transient);
		for (const UAbilityData* Source : SourceData)
		{
			if (!Source)
			{
				continue;
			}

			MergeAbilityDataInto(*RuntimeAbilityData, *Source);
		}
		CharacterData->AbilityData = RuntimeAbilityData;
	}

	UE_LOG(LogTemp, Log, TEXT("[WeaponAbilityData] Applied Base=%s WeaponSources=%d Weapon=%s Character=%s Runtime=%s"),
		*GetNameSafe(BaseAbilityData),
		WeaponSourceCount,
		*GetNameSafe(WeaponDefinition),
		*GetNameSafe(this),
		*GetNameSafe(CharacterData->AbilityData));
}

void APlayerCharacterBase::ApplyCurrentEquipmentAbilityData()
{
	if (EquippedWeaponDef)
	{
		ApplyAbilityDataFromWeapon(EquippedWeaponDef);
	}
	else
	{
		ApplyAbilityDataFromWeapon(nullptr);
	}
}

void APlayerCharacterBase::CaptureEquippedWeaponDeckState()
{
	if (!EquippedWeaponDef)
	{
		EquippedWeaponDeckState.Reset();
		return;
	}

	if (!CombatDeckComponent)
	{
		InitializeWeaponDeckStateFromDefinition(EquippedWeaponDeckState, EquippedWeaponDef);
		return;
	}

	EquippedWeaponDeckState.Reset();

	const TArray<URuneDataAsset*> SourceAssets = CombatDeckComponent->GetDeckSourceAssets();
	EquippedWeaponDeckState.SourceAssets.Reserve(SourceAssets.Num());
	for (URuneDataAsset* SourceAsset : SourceAssets)
	{
		if (SourceAsset)
		{
			EquippedWeaponDeckState.SourceAssets.Add(SourceAsset);
		}
	}

	const TArray<FCombatCardInstance> AttackCards = CombatDeckComponent->GetFullDeckSnapshot();
	EquippedWeaponDeckState.AttackCardOrientations.Reserve(AttackCards.Num());
	for (const FCombatCardInstance& Card : AttackCards)
	{
		EquippedWeaponDeckState.AttackCardOrientations.Add(Card.LinkOrientation);
	}

	EquippedWeaponDeckState.ShuffleCooldownDuration = CombatDeckComponent->GetShuffleCooldownDuration();
	EquippedWeaponDeckState.MaxActiveSequenceSize = CombatDeckComponent->GetMaxActiveSequenceSize();
	EquippedWeaponDeckState.bInitialized = true;
}

void APlayerCharacterBase::InitializeEquippedWeaponDeckStateFromDefinition()
{
	InitializeWeaponDeckStateFromDefinition(EquippedWeaponDeckState, EquippedWeaponDef);
}

void APlayerCharacterBase::InitializeInactiveWeaponDeckStateFromDefinition()
{
	InitializeWeaponDeckStateFromDefinition(InactiveWeaponDeckState, InactiveWeaponDef);
}

void APlayerCharacterBase::InitializeWeaponDeckStateFromDefinition(FWeaponCombatDeckRuntimeState& DeckState, const UWeaponDefinition* WeaponDefinition) const
{
	DeckState.Reset();
	if (!WeaponDefinition)
	{
		return;
	}

	TArray<URuneDataAsset*> SourceAssets;
	if (CombatDeckComponent)
	{
		CombatDeckComponent->BuildDefaultWeaponDeckSourceAssets(WeaponDefinition, SourceAssets);
	}

	DeckState.SourceAssets.Reserve(SourceAssets.Num());
	for (URuneDataAsset* SourceAsset : SourceAssets)
	{
		if (SourceAsset)
		{
			DeckState.SourceAssets.Add(SourceAsset);
		}
	}

	DeckState.ShuffleCooldownDuration = WeaponDefinition->ShuffleCooldownDuration;
	DeckState.MaxActiveSequenceSize = WeaponDefinition->MaxActiveSequenceSize;
	DeckState.bInitialized = true;
}

void APlayerCharacterBase::LoadCombatDeckFromWeaponDeckState(FWeaponCombatDeckRuntimeState& DeckState, const UWeaponDefinition* WeaponDefinition)
{
	if (!CombatDeckComponent)
	{
		return;
	}

	if (!WeaponDefinition)
	{
		DeckState.Reset();
		TArray<URuneDataAsset*> EmptyDeck;
		CombatDeckComponent->LoadDeckFromExactSourceAssets(EmptyDeck, 0.0f, 0);
		return;
	}

	if (!DeckState.bInitialized)
	{
		InitializeWeaponDeckStateFromDefinition(DeckState, WeaponDefinition);
	}

	TArray<URuneDataAsset*> SourceAssets;
	SourceAssets.Reserve(DeckState.SourceAssets.Num());
	for (const TObjectPtr<URuneDataAsset>& SourceAsset : DeckState.SourceAssets)
	{
		if (SourceAsset)
		{
			SourceAssets.Add(SourceAsset.Get());
		}
	}

	CombatDeckComponent->LoadDeckFromExactSourceAssets(
		SourceAssets,
		DeckState.ShuffleCooldownDuration,
		DeckState.MaxActiveSequenceSize);

	if (!DeckState.AttackCardOrientations.IsEmpty())
	{
		CombatDeckComponent->ApplyDeckOrientations(DeckState.AttackCardOrientations);
	}
}

void APlayerCharacterBase::CaptureCombatLoadoutForRunState(FRunState& OutState)
{
	CaptureEquippedWeaponDeckState();
	if (InactiveWeaponDef && !InactiveWeaponDeckState.bInitialized)
	{
		InitializeInactiveWeaponDeckStateFromDefinition();
	}

	OutState.EquippedWeaponDef = EquippedWeaponDef;
	OutState.InactiveWeaponDef = InactiveWeaponDef;
	CopyDeckRuntimeStateToRunStateFields(
		EquippedWeaponDeckState,
		OutState.CombatDeckCards,
		OutState.CombatDeckCardOrientations,
		OutState.CombatDeckShuffleCooldownDuration,
		OutState.CombatDeckMaxActiveSequenceSize);
	CopyDeckRuntimeStateToRunStateFields(
		InactiveWeaponDeckState,
		OutState.InactiveCombatDeckCards,
		OutState.InactiveCombatDeckCardOrientations,
		OutState.InactiveCombatDeckShuffleCooldownDuration,
		OutState.InactiveCombatDeckMaxActiveSequenceSize);
}

void APlayerCharacterBase::RestoreInactiveWeaponFromDefinition(UWeaponDefinition* WeaponDefinition)
{
	if (InactiveWeaponInstance)
	{
		OnHeatPhaseChanged.RemoveDynamic(InactiveWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		InactiveWeaponInstance->Destroy();
		InactiveWeaponInstance = nullptr;
	}

	InactiveWeaponDef = WeaponDefinition;
	InactiveWeaponFromSpawner = nullptr;
	InactiveWeaponDeckState.Reset();
	if (!WeaponDefinition || !GetWorld())
	{
		return;
	}

	AWeaponInstance* LastSpawnedWeapon = nullptr;
	for (const FWeaponSpawnData& WeaponSpawnData : WeaponDefinition->ActorsToSpawn)
	{
		FWeaponSpawnData SpawnData = WeaponSpawnData;
		SpawnData.bShouldSaveToGame = true;
		AWeaponInstance* NewInactiveWeapon = UYogBlueprintFunctionLibrary::SpawnWeaponOnCharacter(this, GetTransform(), SpawnData, false);
		if (NewInactiveWeapon)
		{
			NewInactiveWeapon->HeatOverlayMaterial = WeaponDefinition->HeatOverlayMaterial;
			NewInactiveWeapon->SetActorHiddenInGame(true);
			NewInactiveWeapon->SetActorEnableCollision(false);
			LastSpawnedWeapon = NewInactiveWeapon;
		}
	}

	InactiveWeaponInstance = LastSpawnedWeapon;
	RelinkWeaponAnimLayer();
}

void APlayerCharacterBase::ResetToDefaultUnarmedCombatState()
{
	if (EquippedWeaponInstance)
	{
		OnHeatPhaseChanged.RemoveDynamic(EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		EquippedWeaponInstance->Destroy();
		EquippedWeaponInstance = nullptr;
	}

	if (InactiveWeaponInstance)
	{
		OnHeatPhaseChanged.RemoveDynamic(InactiveWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		InactiveWeaponInstance->Destroy();
		InactiveWeaponInstance = nullptr;
	}

	EquippedWeaponDef = nullptr;
	EquippedFromSpawner = nullptr;
	InactiveWeaponDef = nullptr;
	InactiveWeaponFromSpawner = nullptr;
	EquippedWeaponDeckState.Reset();
	InactiveWeaponDeckState.Reset();

	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		YogASC->ClearWeaponTypeTags();
	}

	if (DefaultUnarmedWeaponDef)
	{
		DefaultUnarmedWeaponDef->SetupWeaponToCharacter(GetMesh(), this);
		// SetupWeaponToCharacter sets EquippedWeaponDef = this, but unarmed state must keep
		// EquippedWeaponDef null so TryPickupWeapon equips the next real weapon as primary.
		EquippedWeaponDef = nullptr;
	}
	else
	{
		ApplyAbilityDataFromWeapon(nullptr);
	}
}

bool APlayerCharacterBase::CanSwitchWeapon() const
{
	if (!InactiveWeaponDef)
	{
		return false;
	}

	const UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent());
	return !YogASC || !YogASC->IsPlayerActionMontageLocked() || IsInPostAttackRecoveryWindow();
}

bool APlayerCharacterBase::IsInPostAttackRecoveryWindow() const
{
	const FGameplayTag RecoveryTag = GetPostAttackRecoveryTag();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return RecoveryTag.IsValid() && ASC && ASC->GetTagCount(RecoveryTag) > 0;
}

void APlayerCharacterBase::SwitchWeapon()
{
	if (!CanSwitchWeapon())
	{
		return;
	}

	const bool bRecoveryCancelSwitch = IsInPostAttackRecoveryWindow();

	CaptureEquippedWeaponDeckState();
	if (!InactiveWeaponDeckState.bInitialized)
	{
		InitializeInactiveWeaponDeckStateFromDefinition();
	}

	if (bRecoveryCancelSwitch)
	{
		ApplyRecoveryCancelWeaponSwitchBonus();
	}

	if (EquippedWeaponInstance)
	{
		OnHeatPhaseChanged.RemoveDynamic(EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		EquippedWeaponInstance->SetActorHiddenInGame(true);
		EquippedWeaponInstance->SetActorEnableCollision(false);
	}

	if (InactiveWeaponInstance)
	{
		InactiveWeaponInstance->SetActorHiddenInGame(false);
		InactiveWeaponInstance->SetActorEnableCollision(true);
	}

	Swap(EquippedWeaponDef, InactiveWeaponDef);
	Swap(EquippedWeaponInstance, InactiveWeaponInstance);
	Swap(EquippedFromSpawner, InactiveWeaponFromSpawner);
	Swap(EquippedWeaponDeckState, InactiveWeaponDeckState);

	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
	{
		YogASC->ClearWeaponTypeTags();
		if (EquippedWeaponDef)
		{
			YogASC->ApplyWeaponTypeTag(EquippedWeaponDef->WeaponType);
		}
	}

	ApplyAbilityDataFromWeapon(EquippedWeaponDef);

	LoadCombatDeckFromWeaponDeckState(EquippedWeaponDeckState, EquippedWeaponDef);

	if (EquippedWeaponInstance)
	{
		OnHeatPhaseChanged.AddDynamic(EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		OnHeatPhaseChanged.Broadcast(CurrentHeatPhase);
	}

	RelinkWeaponAnimLayer();
	OnWeaponSwitched.Broadcast();
}

void APlayerCharacterBase::ApplyRecoveryCancelWeaponSwitchBonus()
{
	if (ActiveSkillComponent)
	{
		ActiveSkillComponent->ClearCooldowns();
	}

	const FGameplayTag BonusTag = GetRecoveryCancelBonusTag();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!BonusTag.IsValid() || !ASC)
	{
		return;
	}

	ASC->SetLooseGameplayTagCount(BonusTag, 1);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryCancelBonusTimerHandle);
		if (RecoveryCancelBonusDuration <= 0.0f)
		{
			ClearRecoveryCancelBonus();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				RecoveryCancelBonusTimerHandle,
				this,
				&APlayerCharacterBase::ClearRecoveryCancelBonus,
				RecoveryCancelBonusDuration,
				false);
		}
	}
}

void APlayerCharacterBase::ClearRecoveryCancelBonus()
{
	const FGameplayTag BonusTag = GetRecoveryCancelBonusTag();
	if (!BonusTag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(BonusTag, 0);
	}
}

void APlayerCharacterBase::ClearRunCarriedStateForHub()
{
	ResetToDefaultUnarmedCombatState();

	PendingRunes.Reset();
	ActiveSacrificeGrace = nullptr;
	ActiveSacrificeOfferingCosts.Reset();

	if (BuffFlowComponent)
	{
		BuffFlowComponent->StopAllBuffFlows();
	}

	if (CombatDeckComponent)
	{
		TArray<URuneDataAsset*> EmptyDeck;
		CombatDeckComponent->LoadDeckFromExactSourceAssets(EmptyDeck, 0.0f, 0);
		CombatDeckComponent->RefreshDeckView();
	}

	if (BackpackGridComponent)
	{
		BackpackGridComponent->Gold = 0;
		BackpackGridComponent->OnGoldChanged.Broadcast(0);
		BackpackGridComponent->RestorePlacedRunes({}, true);
		BackpackGridComponent->RestoreRuntimeHiddenPassiveRunes({});
		BackpackGridComponent->RestorePhase(0);
		BackpackGridComponent->SetLocked(false);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), 0.0f);
	}

	if (ActiveSkillComponent)
	{
		TArray<UActiveSkillDataAsset*> EmptyLoadout;
		ActiveSkillComponent->SetSkillLoadout(EmptyLoadout);
	}

	UE_LOG(LogTemp, Log, TEXT("[RunState] Cleared carried player state for hub. Player=%s"), *GetNameSafe(this));
}

void APlayerCharacterBase::RestoreRunStateFromGI()
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (bRunStateRestoredFromGI)
	{
		return;
	}
	if (!GI || !GI->PendingRunState.bIsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE skipped — no valid state (bIsValid=%d)"),
			GI ? (int32)GI->PendingRunState.bIsValid : -1);
		return;
	}
	bRunStateRestoredFromGI = true;

	UE_LOG(LogTemp, Warning, TEXT("[RunState] RESTORE — HP=%.1f Gold=%d Phase=%d Runes=%d"),
		GI->PendingRunState.CurrentHP, GI->PendingRunState.CurrentGold,
		GI->PendingRunState.CurrentPhase, GI->PendingRunState.PlacedRunes.Num());

	const FRunState State = GI->PendingRunState;
	RestoreRunState(State);
}

void APlayerCharacterBase::RestoreRunState(const FRunState& State)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	// 恢复 HP
	if (ASC && State.CurrentHP > 0.f)
	{
		const float MaxHealth = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		const float RestoredHP = MaxHealth > 0.f ? FMath::Clamp(State.CurrentHP, 1.f, MaxHealth) : State.CurrentHP;
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), RestoredHP);
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
	else
	{
		ResetToDefaultUnarmedCombatState();
	}

	if (CombatDeckComponent && !State.CombatDeckCards.IsEmpty())
	{
		TArray<URuneDataAsset*> RestoredDeckAssets;
		RestoredDeckAssets.Reserve(State.CombatDeckCards.Num());
		for (URuneDataAsset* SourceAsset : State.CombatDeckCards)
		{
			RestoredDeckAssets.Add(SourceAsset);
		}
		CombatDeckComponent->LoadDeckFromExactSourceAssets(
			RestoredDeckAssets,
			State.CombatDeckShuffleCooldownDuration,
			State.CombatDeckMaxActiveSequenceSize);

		if (!State.CombatDeckCardOrientations.IsEmpty())
		{
			CombatDeckComponent->ApplyDeckOrientations(State.CombatDeckCardOrientations);
		}

		CaptureEquippedWeaponDeckState();
	}
	else
	{
		InitializeEquippedWeaponDeckStateFromDefinition();
	}

	RestoreInactiveWeaponFromDefinition(State.InactiveWeaponDef);
	if (State.InactiveWeaponDef)
	{
		if (!State.InactiveCombatDeckCards.IsEmpty())
		{
			RestoreDeckRuntimeStateFromRunStateFields(
				InactiveWeaponDeckState,
				State.InactiveCombatDeckCards,
				State.InactiveCombatDeckCardOrientations,
				State.InactiveCombatDeckShuffleCooldownDuration,
				State.InactiveCombatDeckMaxActiveSequenceSize);
		}
		else
		{
			InitializeInactiveWeaponDeckStateFromDefinition();
		}
	}

	if (BackpackGridComponent)
	{
		BackpackGridComponent->RestorePlacedRunes(State.PlacedRunes);
		BackpackGridComponent->RestorePhase(State.CurrentPhase);

		if (UAbilitySystemComponent* HeatASC = GetAbilitySystemComponent();
			HeatASC
			&& HeatASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHeatAttribute())
			&& HeatASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHeatAttribute()))
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

	RestoreSacrificeOfferingCosts(State.SacrificeOfferingCosts);

	if (ActiveSkillComponent && !State.SelectedSkillLoadout.IsEmpty())
	{
		TArray<UActiveSkillDataAsset*> RestoredSkills;
		RestoredSkills.Reserve(State.SelectedSkillLoadout.Num());
		for (UActiveSkillDataAsset* Skill : State.SelectedSkillLoadout)
		{
			RestoredSkills.Add(Skill);
		}
		ActiveSkillComponent->SetSkillLoadout(RestoredSkills);
	}
}

void APlayerCharacterBase::GrantCraftedStarterRunesAsync()
{
	UYogMetaProgressionSubsystem* MetaSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr;
	if (!MetaSys) return;

	const TArray<FPrimaryAssetId>& CraftedRunes = MetaSys->GetCraftedStarterRunes();
	if (CraftedRunes.IsEmpty()) return;

	TArray<FSoftObjectPath> SoftPaths;
	SoftPaths.Reserve(CraftedRunes.Num());
	for (const FPrimaryAssetId& Id : CraftedRunes)
	{
		FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(Id);
		if (Path.IsValid())
		{
			SoftPaths.Add(Path);
		}
	}
	if (SoftPaths.IsEmpty()) return;

	TWeakObjectPtr<APlayerCharacterBase> WeakThis(this);
	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		SoftPaths,
		FStreamableDelegate::CreateLambda([WeakThis, SoftPaths]()
		{
			APlayerCharacterBase* Player = WeakThis.Get();
			if (!Player || !Player->BackpackGridComponent) return;

			for (const FSoftObjectPath& Path : SoftPaths)
			{
				if (URuneDataAsset* RuneDA = Cast<URuneDataAsset>(Path.ResolveObject()))
				{
					Player->BackpackGridComponent->AddHiddenPassiveRune(RuneDA->CreateInstance());
				}
			}
		}),
		FStreamableManager::DefaultAsyncLoadPriority
	);
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

bool APlayerCharacterBase::ApplySacrificeOfferingCost(const FAltarSacrificeEntry& CostEntry, int32 DeckCardIndex)
{
	switch (CostEntry.CostType)
	{
	case ESacrificeOfferingCostType::SacrificeDeckCard:
		if (!CombatDeckComponent || DeckCardIndex == INDEX_NONE)
		{
			return false;
		}
		return CombatDeckComponent->RemoveCardAtIndex(DeckCardIndex);

	case ESacrificeOfferingCostType::AttackUpDamageTakenUp:
		{
			UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
			if (!ASC)
			{
				return false;
			}

			const float AttackIncreaseRatio = CostEntry.PrimaryMagnitude > 0.f ? CostEntry.PrimaryMagnitude : 0.15f;
			const float DamageTakenDelta = CostEntry.SecondaryMagnitude > 0.f ? CostEntry.SecondaryMagnitude : 0.20f;

			FSacrificeOfferingCostState State;
			State.CostType = CostEntry.CostType;
			State.AttackDelta = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute()) * AttackIncreaseRatio;
			State.DmgTakenDelta = DamageTakenDelta;

			if (!ApplySacrificeCostStateToASC(ASC, State, this))
			{
				return false;
			}
			ActiveSacrificeOfferingCosts.Add(State);
			return true;
		}

	case ESacrificeOfferingCostType::CritRateDownCritDamageUp:
		{
			UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
			if (!ASC)
			{
				return false;
			}

			const float CritRateLossRatio = CostEntry.PrimaryMagnitude > 0.f ? CostEntry.PrimaryMagnitude : 0.50f;
			const float CritDamageDelta = CostEntry.SecondaryMagnitude > 0.f ? CostEntry.SecondaryMagnitude : 0.50f;

			FSacrificeOfferingCostState State;
			State.CostType = CostEntry.CostType;
			State.CritRateDelta = -ASC->GetNumericAttribute(UBaseAttributeSet::GetCrit_RateAttribute()) * CritRateLossRatio;
			State.CritDamageDelta = CritDamageDelta;

			if (!ApplySacrificeCostStateToASC(ASC, State, this))
			{
				return false;
			}
			ActiveSacrificeOfferingCosts.Add(State);
			return true;
		}
	}

	return false;
}

void APlayerCharacterBase::RestoreSacrificeOfferingCosts(const TArray<FSacrificeOfferingCostState>& Costs)
{
	ActiveSacrificeOfferingCosts.Reset();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	for (const FSacrificeOfferingCostState& State : Costs)
	{
		if (ApplySacrificeCostStateToASC(ASC, State, this))
		{
			ActiveSacrificeOfferingCosts.Add(State);
		}
	}
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		CameraBoom->SetupAttachment(GetCapsuleComponent());
		CameraBoom->SetUsingAbsoluteRotation(true);
		CameraBoom->TargetArmLength = DefaultCameraBoomLength;
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw = false;
		CameraBoom->bInheritRoll = false;
	}

	if (FollowCamera)
	{
		FollowCamera->FieldOfView = DefaultCameraFOV;
	}

	// 将 BackpackGridComponent 与 ASC 关联，使符文激活时可施加 GE
	if (BackpackGridComponent && GetAbilitySystemComponent())
	{
		BackpackGridComponent->InitWithASC(GetAbilitySystemComponent());
	}

	// 卡牌入组事件：C++ 统一识别卡牌类型并触发一次性教程提示
	if (CombatDeckComponent)
	{
		CombatDeckComponent->OnDeckCardsEntered.AddDynamic(
			this, &APlayerCharacterBase::OnDeckCardsEnteredForTutorial);
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

	// Grant the combat-driving abilities (attack, weapon skill, dash, special). These
	// are player-only and weapon-agnostic; the current weapon supplies the montage data.
	// Granted once here so they are always available, regardless of which weapon is equipped.
	if (DefaultCombatAbilitySet)
	{
		for (const FYogAbilitySet_GameplayAbility& Entry : DefaultCombatAbilitySet->GrantedGameplayAbilities)
		{
			if (Entry.Ability)
			{
				GrantGameplayAbility(Entry.Ability, Entry.AbilityLevel);
			}
		}
	}

	// GAS Template 授能（在 Super::BeginPlay 中完成）可能覆盖切关前 Link 的武器动画层；
	// 在此重新 Link，确保武器层优先级高于默认层
	if (UYogAbilitySystemComponent* ASC = GetASC())
	{
		auto GrantIfMissing = [this, ASC](TSubclassOf<UYogGameplayAbility> AbilityClass)
		{
			if (!AbilityClass)
			{
				return;
			}

			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
				{
					return;
				}
			}

			GrantGameplayAbility(AbilityClass, 1);
		};

		GrantIfMissing(UGA_PlayerAttack_Combo1::StaticClass());
		GrantIfMissing(UGA_PlayerAttack_Combo2::StaticClass());
		GrantIfMissing(UGA_PlayerAttack_Combo3::StaticClass());
		GrantIfMissing(UGA_PlayerAttack_Combo4::StaticClass());
		GrantIfMissing(UGA_WeaponSkill_Combo1::StaticClass());
		GrantIfMissing(UGA_WeaponSkill_Combo2::StaticClass());
		GrantIfMissing(UGA_WeaponSkill_Combo3::StaticClass());
		GrantIfMissing(UGA_WeaponSkill_Combo4::StaticClass());
	}

	ApplyCurrentEquipmentAbilityData();
	RelinkWeaponAnimLayer();

	// Initialize deck/special/weapon-type from the unarmed default without calling
	// SetupWeaponToCharacter, which would set EquippedWeaponDef and cause TryPickupWeapon
	// to route the first real weapon to the inactive slot instead of equipping it.
	if (!EquippedWeaponDef && DefaultUnarmedWeaponDef)
	{
		ApplyAbilityDataFromWeapon(DefaultUnarmedWeaponDef);

		if (UCombatDeckComponent* CombatDeck = CombatDeckComponent.Get())
		{
			CombatDeck->LoadDeckFromWeapon(DefaultUnarmedWeaponDef);
		}
		if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			YogASC->ApplyWeaponTypeTag(DefaultUnarmedWeaponDef->WeaponType);
		}
	}

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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryCancelBonusTimerHandle);
	}
	ClearRecoveryCancelBonus();

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

	// [Disabled] DamageEdgeFlashWidget — kept for later use, do not delete.
	// PlayDamageScreenFlash();
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

	// StartDamageTimeDilation();
}

void APlayerCharacterBase::PlayDamageScreenFlash()
{
	// [Disabled] DamageEdgeFlashWidget — entire body commented out, kept for later use.
	// To re-enable: uncomment this body AND the call site in TakeDamage (search for
	// "[Disabled] DamageEdgeFlashWidget" above). Consider migrating creation through
	// UYogUIManagerSubsystem rather than raw CreateWidget + AddToViewport.
	/*
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
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		PC->GetViewportSize(ViewportX, ViewportY);
		if (ViewportX > 0 && ViewportY > 0)
		{
			DamageEdgeFlashWidget->SetDesiredSizeInViewport(FVector2D(ViewportX, ViewportY));
			DamageEdgeFlashWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
		}

		DamageEdgeFlashWidget->PlayEdgeFlash(
			DamageScreenFlashColor,
			DamageScreenFlashAlpha,
			DamageScreenFlashDuration,
			DamageScreenEdgeWidthRatio);
	}
	*/
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
	if (!bDamageTimeDilationVisualActive)
	{
		UTimeDilationVisualSubsystem::BeginTimeDilationVisual(this);
		bDamageTimeDilationVisualActive = true;
	}

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

	if (bDamageTimeDilationVisualActive)
	{
		UTimeDilationVisualSubsystem::EndTimeDilationVisual(this);
		bDamageTimeDilationVisualActive = false;
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

void APlayerCharacterBase::OnDeckCardsEnteredForTutorial(const TArray<FCombatCardInstance>& Cards)
{
	UTutorialManager* TM = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTutorialManager>() : nullptr;

	APlayerController* PC = GetController<APlayerController>();
	if (!PC) return;

	static const FGameplayTag HeavyHintTag   = FGameplayTag::RequestGameplayTag(TEXT("Tutorial.Hint.HeavyCard"));
	static const FGameplayTag LinkHintTag     = FGameplayTag::RequestGameplayTag(TEXT("Tutorial.Hint.LinkCard"));
	static const FGameplayTag FinisherHintTag = FGameplayTag::RequestGameplayTag(TEXT("Tutorial.Hint.Finisher"));
	static const FGameplayTag HeavyIdTag      = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Heavy"), false);
	static const FGameplayTag HeavyEffectTag  = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Heavy"), false);
	static const FGameplayTag MoonlightIdTag  = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Moonlight"), false);
	static const FGameplayTag MoonlightEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Moonlight"), false);
	static const FName WeaponOwnerSource(TEXT("Weapon"));
	bool bBroadcastedRewardCardEntered = false;

	for (const FCombatCardInstance& Card : Cards)
	{
		if (!Card.IsValidCard()) continue;
		if (Card.OwnerSource == WeaponOwnerSource)
		{
			continue;
		}

		if (!bBroadcastedRewardCardEntered)
		{
			if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
				: nullptr)
			{
				StoryEngine->BroadcastStoryEvent(
					FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.FirstRewardCardEntered"), false),
					PC);
			}
			bBroadcastedRewardCardEntered = true;
		}

		if (!TM)
		{
			continue;
		}

		const bool bIsHeavyCard =
			Card.Config.RequiredAction == ECardRequiredAction::Heavy
			|| (HeavyIdTag.IsValid() && Card.Config.CardIdTag == HeavyIdTag)
			|| (HeavyEffectTag.IsValid() && Card.Config.CardEffectTags.HasTagExact(HeavyEffectTag));
		const bool bIsMoonlightLinkCard =
			(MoonlightIdTag.IsValid() && Card.Config.CardIdTag == MoonlightIdTag)
			|| (MoonlightEffectTag.IsValid() && Card.Config.CardEffectTags.HasTagExact(MoonlightEffectTag));

		if (bIsHeavyCard)
		{
			TM->TryShowHintOnce(HeavyHintTag, TEXT("tutorial_heavy_card"), PC);
		}
		else if (bIsMoonlightLinkCard)
		{
			TM->NotifyLinkCardEnteredDeck(PC);
		}
		// 连携卡（月光等）
		else if (Card.Config.CardType == ECombatCardType::Link)
		{
			TM->TryShowHintOnce(LinkHintTag, TEXT("tutorial_card_link"), PC);
		}
		// 终结技卡
		else if (!DevKit::Combat::IsFinisherAbilityDeprecated()
			&& Card.Config.CardType == ECombatCardType::Finisher)
		{
			TM->TryShowHintOnce(FinisherHintTag, TEXT("tutorial_finisher"), PC);
		}
	}
}

