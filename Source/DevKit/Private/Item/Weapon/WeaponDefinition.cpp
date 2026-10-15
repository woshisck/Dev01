#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/BackpackGridComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Engine/AssetManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/WeaponSkillDataAsset.h"

void UWeaponDefinition::SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar)
{
	// End the outgoing skill before replacing its weapon definition or runtime
	// data so skill-specific cancellation reads the correct SourceObject.
	ReceivingChar->ClearEquippedWeaponSkillAbilityGrant();

	// ── 0. 清理旧武器（解绑委托 + 销毁） ────────────────────────────────
	if (ReceivingChar->EquippedWeaponInstance)
	{
		ReceivingChar->EquippedWeaponInstance->Destroy();
		ReceivingChar->EquippedWeaponInstance = nullptr;
	}

	// ── 0.5 清理旧武器类Tag（避免装备替换时残留导致两类 GA 都通过）────
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
	if (LastSpawnedWeapon)
	{
		ReceivingChar->EquippedWeaponInstance = LastSpawnedWeapon;
	}

	// 记录当前装备的武DA，供切关时写RunState
	ReceivingChar->EquippedWeaponDef = this;
	ReceivingChar->InitializeEquippedWeaponSkillFromDefinition();
	ReceivingChar->ApplyAbilityDataFromWeapon(this);
	ReceivingChar->RefreshEquippedWeaponSkillAbilityGrant();

	ReceivingChar->CaptureEquippedWeaponDeckState();

	// ── 武器类型 Tag 守卫：挂当前 WeaponType LooseTag ─────────────────
	// 让玩家专属攻GA ActivationRequiredTags 能匹配通过
	// ClearWeaponTypeTags 已在函数顶部清理过旧 Tag，此处只需 Apply Tag
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

bool UWeaponDefinition::CanEquipWeaponSkill(const UWeaponSkillDataAsset* WeaponSkill) const
{
	// Kill-reward pickups grant skills the weapon never listed, so the authored whitelist
	// no longer gates equipping. AvailableWeaponSkills still seeds ResolveDefaultWeaponSkill.
	return WeaponSkill
		// && AvailableWeaponSkills.Contains(WeaponSkill)
		&& WeaponSkill->AbilityClass
		&& WeaponSkill->AbilityData;
}

UWeaponSkillDataAsset* UWeaponDefinition::ResolveDefaultWeaponSkill() const
{
	if (CanEquipWeaponSkill(DefaultWeaponSkill))
	{
		return DefaultWeaponSkill;
	}

	for (UWeaponSkillDataAsset* WeaponSkill : AvailableWeaponSkills)
	{
		if (CanEquipWeaponSkill(WeaponSkill))
		{
			return WeaponSkill;
		}
	}

	return nullptr;
}
