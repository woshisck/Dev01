#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BackpackGridComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Engine/AssetManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

void UWeaponDefinition::SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar)
{
	// ── 0. 清理旧武器（解绑委托 + 销毁） ────────────────────────────────
	if (ReceivingChar->EquippedWeaponInstance)
	{
		ReceivingChar->OnHeatPhaseChanged.RemoveDynamic(
			ReceivingChar->EquippedWeaponInstance, &AWeaponInstance::OnHeatPhaseChanged);
		ReceivingChar->EquippedWeaponInstance->Destroy();
		ReceivingChar->EquippedWeaponInstance = nullptr;
	}

	// ── 0.5 清理旧武器类型 Tag（避免装备替换时残留导致两类 GA 都通过）────
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ReceivingChar->GetAbilitySystemComponent()))
	{
		YogASC->ClearWeaponTypeTags();
	}

	AWeaponInstance* LastSpawnedWeapon = nullptr;

	//ASAP: TODO: save the param to weapon actor(WeaponInstance)?
	for (FWeaponSpawnData& WeaponActorInst : ActorsToSpawn)
	{
		TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
		FName Socket = WeaponActorInst.AttachSocket;
		FTransform Transform = WeaponActorInst.AttachTransform;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AWeaponInstance* NewActor = ReceivingChar->GetWorld()->SpawnActor<AWeaponInstance>(WeaponActorClass, FTransform::Identity, SpawnParams);

		NewActor->AttachSocket = Socket;
		NewActor->AttachTransform = Transform;
		NewActor->WeaponLayer = WeaponActorInst.WeaponLayer;

		NewActor->EquipWeaponToCharacter(ReceivingChar);

		{
			UCharacterData* CD = ReceivingChar->GetCharacterDataComponent()->GetCharacterData();
			UE_LOG(LogTemp, Warning, TEXT("[WeaponSetup][WeaponDefinition] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | NewAbilityData=%s"),
				*ReceivingChar->GetName(),
				CD ? *CD->GetName() : TEXT("null"),
				CD ? (int32)CD->HasAnyFlags(RF_ClassDefaultObject) : -1,
				CD ? (int32)CD->HasAnyFlags(RF_Transient) : -1,
				WeaponAbilityData ? *WeaponAbilityData->GetName() : TEXT("null"));
			CD->AbilityData = WeaponAbilityData;
		}

		LastSpawnedWeapon = NewActor;
	}

	// ── 热度委托绑定 + 阶段追赶同步 ─────────────────────────────────────
	if (LastSpawnedWeapon)
	{
		LastSpawnedWeapon->HeatOverlayMaterial = HeatOverlayMaterial;
		ReceivingChar->OnHeatPhaseChanged.AddDynamic(LastSpawnedWeapon, &AWeaponInstance::OnHeatPhaseChanged);
		ReceivingChar->EquippedWeaponInstance = LastSpawnedWeapon;

		// 查当前热度阶段并立刻同步（切关恢复时 Phase 可能已非 0）
		int32 CurrentPhase = 0;
		if (UAbilitySystemComponent* ASC = ReceivingChar->GetAbilitySystemComponent())
		{
			if      (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.3")))) CurrentPhase = 3;
			else if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.2")))) CurrentPhase = 2;
			else if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Heat.Phase.1")))) CurrentPhase = 1;
		}
		ReceivingChar->OnHeatPhaseChanged.Broadcast(CurrentPhase);
	}

	// 记录当前装备的武器 DA，供切关时写入 RunState
	ReceivingChar->EquippedWeaponDef = this;

	// ── 注入背包配置（格子尺寸 + 激活区） ───────────────────────────────
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDefinition] SetupWeaponToCharacter reached end. BackpackConfig W=%d H=%d, Char=%s"),
		BackpackConfig.GridWidth, BackpackConfig.GridHeight,
		ReceivingChar ? *ReceivingChar->GetName() : TEXT("null"));

	if (UBackpackGridComponent* BG = ReceivingChar ? ReceivingChar->BackpackGridComponent.Get() : nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDefinition] Calling ApplyBackpackConfig W=%d H=%d"), BackpackConfig.GridWidth, BackpackConfig.GridHeight);
		BG->ApplyBackpackConfig(
			BackpackConfig.GridWidth,
			BackpackConfig.GridHeight,
			BackpackConfig.ActivationZoneConfig);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDefinition] BackpackGridComponent is NULL — skipping ApplyBackpackConfig"));
	}

	if (UCombatDeckComponent* CombatDeck = ReceivingChar ? ReceivingChar->CombatDeckComponent.Get() : nullptr)
	{
		CombatDeck->LoadDeckFromWeapon(this);
	}

	// ── 武器类型 Tag 守卫：挂当前 WeaponType LooseTag ─────────────────
	// 让玩家专属攻击 GA 的 ActivationRequiredTags 能匹配通过；
	// ClearWeaponTypeTags 已在函数顶部清理过旧 Tag，此处只需 Apply 新 Tag
	if (UYogAbilitySystemComponent* YogASC = Cast<UYogAbilitySystemComponent>(ReceivingChar->GetAbilitySystemComponent()))
	{
		YogASC->ApplyWeaponTypeTag(WeaponType);
	}

	//TODO: DEPRECATED : for loop grant ability
	//for (const UYogAbilitySet* YogAbilitiesSet : AbilitySetsToGrant)
	//{
	//	for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
	//	{
	//		ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
	//	}
	//}

	//if (WeaponLayer)
	//{
	//	UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
	//	AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponLayer));
	//}
}
