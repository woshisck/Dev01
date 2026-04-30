// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile/MusketBullet.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

namespace
{
    FGameplayTag MusketActDamageTag()
    {
        return FGameplayTag::RequestGameplayTag(FName(TEXT("Attribute.ActDamage")));
    }
}

AMusketBullet::AMusketBullet()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(8.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SetRootComponent(CollisionSphere);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed           = Speed;
    ProjectileMovement->MaxSpeed               = Speed;
    ProjectileMovement->ProjectileGravityScale = 0.f;
    ProjectileMovement->bShouldBounce          = false;
    ProjectileMovement->bRotationFollowsVelocity = true;
}

void AMusketBullet::InitBullet(ACharacter* InSource, float InDamage,
                               TSubclassOf<UGameplayEffect> InDamageEffect)
{
    SourceCharacter   = InSource;
    DamageMagnitude   = InDamage;
    DamageEffectClass = InDamageEffect;

    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = Speed;
        ProjectileMovement->MaxSpeed     = Speed;
        ProjectileMovement->Velocity     = GetActorForwardVector() * Speed;
    }
}

void AMusketBullet::SetCombatDeckContext(ECardRequiredAction InActionType, bool bInComboFinisher, bool bInFromDashSave)
{
    CombatDeckActionType = InActionType;
    bCombatDeckComboFinisher = bInComboFinisher;
    bCombatDeckFromDashSave = bInFromDashSave;
}

void AMusketBullet::BeginPlay()
{
    Super::BeginPlay();

    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMusketBullet::OnOverlapBegin);

    GetWorld()->GetTimerManager().SetTimer(
        LifetimeTimerHandle, this, &AMusketBullet::Expire, Lifetime, false);
}

void AMusketBullet::OnOverlapBegin(
    UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/,
    bool /*bFromSweep*/, const FHitResult& SweepHitResult)
{
    if (bHasHit || !OtherActor || OtherActor == this || OtherActor == SourceCharacter)
        return;

    bHasHit = true;
    GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);

    const FVector HitLoc = SweepHitResult.ImpactPoint.IsNearlyZero()
        ? OtherActor->GetActorLocation()
        : FVector(SweepHitResult.ImpactPoint);

    ApplyDamageTo(OtherActor, HitLoc);
    Destroy();
}

void AMusketBullet::ApplyDamageTo(AActor* Target, const FVector& HitLocation)
{
    if (!Target || !DamageEffectClass || !SourceCharacter) return;

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
    UAbilitySystemComponent* SourceASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);

    if (!TargetASC || !SourceASC) return;

    FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
    CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
    CtxHandle.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle =
        SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, CtxHandle);

    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->SetSetByCallerMagnitude(MusketActDamageTag(), DamageMagnitude);
        SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    }

    ResolveCombatDeckOnHit();
    BP_OnHitEnemy(Target, HitLocation);
}

void AMusketBullet::ResolveCombatDeckOnHit()
{
    if (bCombatDeckResolved)
    {
        return;
    }

    APlayerCharacterBase* PlayerSource = Cast<APlayerCharacterBase>(SourceCharacter);
    if (!PlayerSource || !PlayerSource->CombatDeckComponent)
    {
        return;
    }

    bCombatDeckResolved = true;
    PlayerSource->CombatDeckComponent->ResolveAttackCard(
        CombatDeckActionType,
        bCombatDeckComboFinisher,
        bCombatDeckFromDashSave);
}

void AMusketBullet::Expire()
{
    BP_OnMiss();
    Destroy();
}
