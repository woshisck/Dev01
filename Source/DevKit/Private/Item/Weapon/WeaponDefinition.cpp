#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Engine/AssetManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

namespace
{
	constexpr bool bDisableLegacyHeatBackpackRuneForCardTest = true;
}

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

		LastSpawnedWeapon = NewActor;
	}

	// ── 热度委托绑定 + 阶段追赶同步 ─────────────────────────────────────
	if (LastSpawnedWeapon && !bDisableLegacyHeatBackpackRuneForCardTest)
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
	else if (LastSpawnedWeapon)
	{
		ReceivingChar->EquippedWeaponInstance = LastSpawnedWeapon;
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDefinition] Legacy heat weapon overlay disabled for combat card test"));
	}

	// 记录当前装备的武器 DA，供切关时写入 RunState
	ReceivingChar->EquippedWeaponDef = this;

	if (UCombatDeckComponent* CombatDeck = ReceivingChar ? ReceivingChar->CombatDeckComponent.Get() : nullptr)
	{
		CombatDeck->LoadDeckFromWeapon(this);
	}

	if (UComboRuntimeComponent* ComboRuntime = ReceivingChar ? ReceivingChar->ComboRuntimeComponent.Get() : nullptr)
	{
		ComboRuntime->LoadComboGraph(GameplayAbilityComboGraph);
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
