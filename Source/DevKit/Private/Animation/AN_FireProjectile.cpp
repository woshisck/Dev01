#include "Animation/AN_FireProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Character/PlayerCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponTypes.h"
#include "Data/RangedProjectileDefinition.h"
#include "Projectile/YogBulletManagerSubsystem.h"

namespace
{
	// Snapshot of the fire-time damage magnitude, mirroring the old GA_RangeAttack path.
	float AN_FireProjectile_ComputeMagnitude(const URangedProjectileDefinition* Def, UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return Def->BaseEffectMagnitude;
		}

		const float Attack      = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
		const float AttackPower = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());

		return Def->BaseEffectMagnitude
			+ Attack      * Def->CreatorAttackMagnitudeScale
			+ AttackPower * Def->CreatorAttackPowerMagnitudeScale;
	}
}

UAN_FireProjectile::UAN_FireProjectile()
{
}

void UAN_FireProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	UWeaponDefinition* WeaponDef = Player->EquippedWeaponDef;
	if (!WeaponDef || WeaponDef->WeaponType != EWeaponType::Ranged)
	{
		return;
	}

	URangedProjectileDefinition* ProjDef = WeaponDef->ProjectileDefinition;
	if (!ProjDef)
	{
		return;
	}

	UWorld* World = Player->GetWorld();
	UYogBulletManagerSubsystem* BulletMgr = World ? World->GetSubsystem<UYogBulletManagerSubsystem>() : nullptr;
	if (!BulletMgr)
	{
		return;
	}

	FTransform SpawnTransform = Player->GetActorTransform();
	if (!MuzzleSocketName.IsNone())
	{
		SpawnTransform = MeshComp->GetSocketTransform(MuzzleSocketName);
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);

	FYogBulletSpawnParams Params;
	Params.SpawnLocation          = SpawnTransform.GetLocation();
	Params.Direction              = SpawnTransform.GetRotation().GetForwardVector();
	Params.Speed                  = ProjDef->Speed;
	Params.Lifetime               = ProjDef->Lifetime;
	Params.CollisionRadius        = ProjDef->CollisionRadius;
	Params.bPiercing              = ProjDef->bPiercing;
	Params.MaxHits                = ProjDef->MaxHits;
	Params.HitEventTag            = ProjDef->HitEventTag;
	Params.ExpireEventTag         = ProjDef->ExpireEventTag;
	Params.bSendHitEventToCreator = ProjDef->bSendHitEventToCreator;
	Params.EffectMagnitude        = AN_FireProjectile_ComputeMagnitude(ProjDef, SourceASC);
	Params.SourceCharacter        = Player;
	Params.SourceASC              = SourceASC;
	Params.TravelNiagaraSystem    = ProjDef->TravelNiagaraSystem;
	Params.TravelNiagaraScale     = ProjDef->TravelNiagaraScale;
	Params.HitNiagaraSystem       = ProjDef->HitNiagaraSystem;
	Params.HitNiagaraScale        = ProjDef->HitNiagaraScale;
	Params.ExpireNiagaraSystem    = ProjDef->ExpireNiagaraSystem;
	Params.ExpireNiagaraScale     = ProjDef->ExpireNiagaraScale;

	BulletMgr->SpawnBullet(Params);
}

FString UAN_FireProjectile::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Fire Projectile | %s"),
		MuzzleSocketName.IsNone() ? TEXT("root") : *MuzzleSocketName.ToString());
}
