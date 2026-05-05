// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Projectile/MusketBullet.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Component/CombatDeckComponent.h"

namespace
{
	FGameplayTag MusketCueTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName));
	}
}

UGA_MusketBase::UGA_MusketBase()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // C++ 默认值：无需 Blueprint 子类即可直接使用
    BulletClass             = AMusketBullet::StaticClass();
    BulletDamageEffectClass = UGE_MusketBullet_Damage::StaticClass();

    // 武器类型守卫：只有 ASC 持有 Weapon.Type.Ranged LooseTag 时才能激活
    // （装备远程武器时由 WeaponDefinition::SetupWeaponToCharacter 挂上）
    ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Weapon.Type.Ranged")));
}

bool UGA_MusketBase::InitCharacterCache(const FGameplayAbilityActorInfo* ActorInfo)
{
    CachedCharacter = ActorInfo ? Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
    return CachedCharacter != nullptr;
}

// ── Ammo ──────────────────────────────────────────────────────────────────────

float UGA_MusketBase::GetCurrentAmmo() const
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    return ASC ? ASC->GetNumericAttribute(UPlayerAttributeSet::GetCurrentAmmoAttribute()) : 0.f;
}

float UGA_MusketBase::GetMaxAmmo() const
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    return ASC ? ASC->GetNumericAttribute(UPlayerAttributeSet::GetMaxAmmoAttribute()) : 6.f;
}

bool UGA_MusketBase::HasAmmo() const
{
    return GetCurrentAmmo() >= 1.f;
}

void UGA_MusketBase::ConsumeOneAmmo()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    ASC->SetNumericAttributeBase(UPlayerAttributeSet::GetCurrentAmmoAttribute(),
        FMath::Max(0.f, GetCurrentAmmo() - 1.f));
}

void UGA_MusketBase::AddOneAmmo()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    ASC->SetNumericAttributeBase(UPlayerAttributeSet::GetCurrentAmmoAttribute(),
        FMath::Min(GetCurrentAmmo() + 1.f, GetMaxAmmo()));
}

void UGA_MusketBase::ClearAllAmmo()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC) ASC->SetNumericAttributeBase(UPlayerAttributeSet::GetCurrentAmmoAttribute(), 0.f);
}

void UGA_MusketBase::SetAmmoToMax()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC) ASC->SetNumericAttributeBase(UPlayerAttributeSet::GetCurrentAmmoAttribute(), GetMaxAmmo());
}

// ── Movement ──────────────────────────────────────────────────────────────────

void UGA_MusketBase::LockMovement()
{
    if (CachedCharacter && !bMovementLocked)
    {
        CachedCharacter->DisableMovement();
        bMovementLocked = true;
    }
}

void UGA_MusketBase::UnlockMovement()
{
    if (CachedCharacter && bMovementLocked)
    {
        CachedCharacter->EnableMovement();
        bMovementLocked = false;
    }
}

// ── Bullet ────────────────────────────────────────────────────────────────────

FVector UGA_MusketBase::GetMuzzleLocation() const
{
    if (!CachedCharacter) return FVector::ZeroVector;

    // 1. 角色主 Mesh Socket
    if (USkeletalMeshComponent* Mesh = CachedCharacter->GetMesh())
    {
        if (Mesh->DoesSocketExist(MuzzleSocketName))
            return Mesh->GetSocketLocation(MuzzleSocketName);
    }

    // 2. 装备武器 Mesh Socket
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(CachedCharacter))
    {
        if (Player->EquippedWeaponInstance)
        {
            TArray<USkeletalMeshComponent*> Meshes;
            Player->EquippedWeaponInstance->GetComponents<USkeletalMeshComponent>(Meshes);
            for (USkeletalMeshComponent* WMesh : Meshes)
            {
                if (WMesh->DoesSocketExist(MuzzleSocketName))
                    return WMesh->GetSocketLocation(MuzzleSocketName);
            }
        }
    }

    // 3. 近似位置（角色前方 80cm，高度 60cm）
    return CachedCharacter->GetActorLocation()
         + CachedCharacter->GetActorForwardVector() * 80.f
         + FVector(0.f, 0.f, 60.f);
}

AMusketBullet* UGA_MusketBase::SpawnBullet(float YawOffsetDeg, float Damage)
{
    if (!CachedCharacter || !BulletClass) return nullptr;

    UWorld* World = GetWorld();
    if (!World) return nullptr;

    const float     BulletYaw  = CachedCharacter->GetActorRotation().Yaw + YawOffsetDeg;
    const FRotator  BulletRot  = FRotator(0.f, BulletYaw, 0.f);
    const FVector   SpawnLoc   = GetMuzzleLocation();

    FActorSpawnParameters Params;
    Params.Instigator                            = Cast<APawn>(CachedCharacter);
    Params.SpawnCollisionHandlingOverride        = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AMusketBullet* Bullet = World->SpawnActor<AMusketBullet>(BulletClass, SpawnLoc, BulletRot, Params);
    if (Bullet)
    {
        Bullet->InitBullet(Cast<ACharacter>(CachedCharacter), Damage, BulletDamageEffectClass);
    }
    return Bullet;
}

FGuid UGA_MusketBase::ResolveCombatDeckOnFire(
    ECardRequiredAction ActionType,
    bool bIsComboFinisher,
    bool bFromDashSave,
    float Damage,
    float BaseYawOffsetDeg)
{
    const FGuid AttackGuid = FGuid::NewGuid();
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(CachedCharacter);
    if (!Player || !Player->CombatDeckComponent)
    {
        return AttackGuid;
    }

    FCombatDeckActionContext Context;
    Context.ActionType = ActionType;
    Context.bIsComboFinisher = bIsComboFinisher;
    Context.bFromDashSave = bFromDashSave;
    Context.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
    Context.AttackInstanceGuid = AttackGuid;
    Context.AttackDamage = Damage;
    Context.RangedBaseYawOffsetDeg = BaseYawOffsetDeg;
    Context.RangedProjectileClass = BulletClass;
    Context.RangedDamageEffectClass = BulletDamageEffectClass;
    Player->CombatDeckComponent->ResolveAttackCardWithContext(Context);
    return AttackGuid;
}

void UGA_MusketBase::ApplyCombatDeckContextToBullet(
    AMusketBullet* Bullet,
    ECardRequiredAction ActionType,
    bool bIsComboFinisher,
    bool bFromDashSave,
    const FGuid& AttackGuid,
    float Damage) const
{
    if (!Bullet)
    {
        return;
    }

    Bullet->SetCombatDeckContextWithGuid(
        ActionType,
        bIsComboFinisher,
        bFromDashSave,
        AttackGuid,
        Damage);
}

// ── Cues ──────────────────────────────────────────────────────────────────────

void UGA_MusketBase::ExecuteFireCue()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    FGameplayCueParameters P;
    P.Instigator = CachedCharacter;
    ASC->ExecuteGameplayCue(MusketCueTag(TEXT("GameplayCue.Musket.Fire")), P);
}

void UGA_MusketBase::ExecuteReloadCue()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    FGameplayCueParameters P;
    P.Instigator = CachedCharacter;
    ASC->ExecuteGameplayCue(MusketCueTag(TEXT("GameplayCue.Musket.Reload")), P);
}

void UGA_MusketBase::ExecuteChargeFullCue()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) return;
    FGameplayCueParameters P;
    P.Instigator = CachedCharacter;
    ASC->ExecuteGameplayCue(MusketCueTag(TEXT("GameplayCue.Musket.ChargeFull")), P);
}

// ── Stats ─────────────────────────────────────────────────────────────────────

float UGA_MusketBase::GetBaseAttack() const
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    return ASC ? ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute()) : 100.f;
}
