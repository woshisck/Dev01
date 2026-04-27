#include "BuffFlow/Actors/BFAreaDamageZone.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Character/EnemyCharacterBase.h"

namespace
{
	FGameplayTag DataDamageTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("Data.Damage")), false);
	}
}

ABFAreaDamageZone::ABFAreaDamageZone()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(200.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);
}

void ABFAreaDamageZone::InitZone(ACharacter* InSource, float InRadius, float InDuration,
                                  float InDamageAmount, float InDamageInterval,
                                  TSubclassOf<UGameplayEffect> InDamageEffect)
{
	SourceCharacter   = InSource;
	DamageMagnitude   = InDamageAmount;
	DamageInterval    = InDamageInterval;
	Duration          = InDuration;
	DamageEffectClass = InDamageEffect;

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(InRadius);
	}
}

void ABFAreaDamageZone::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	World->GetTimerManager().SetTimer(
		LifetimeTimerHandle, this, &ABFAreaDamageZone::Expire, Duration, false);

	if (DamageInterval > 0.f)
	{
		World->GetTimerManager().SetTimer(
			DamageTickTimerHandle, this, &ABFAreaDamageZone::DamageTick,
			DamageInterval, true, 0.f);
	}
	else
	{
		DamageTick();
	}

	BP_OnActivate();
}

void ABFAreaDamageZone::DamageTick()
{
	TArray<AActor*> OverlappingActors;
	CollisionSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor || Actor == this || Actor == SourceCharacter)
		{
			continue;
		}

		if (Cast<AEnemyCharacterBase>(Actor))
		{
			ApplyDamageTo(Actor);
		}
	}

	BP_OnDamageTick();
}

void ABFAreaDamageZone::ApplyDamageTo(AActor* Target)
{
	if (!Target || !DamageEffectClass || !SourceCharacter)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);

	if (!TargetASC || !SourceASC)
	{
		return;
	}

	FGameplayEffectContextHandle CtxHandle = SourceASC->MakeEffectContext();
	CtxHandle.AddInstigator(SourceCharacter, SourceCharacter);
	CtxHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, CtxHandle);

	const FGameplayTag DamageTag = DataDamageTag();
	if (SpecHandle.IsValid() && DamageTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(DamageTag, -DamageMagnitude);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void ABFAreaDamageZone::Expire()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DamageTickTimerHandle);
	}

	BP_OnExpired();
	OnExpiredDelegate.Broadcast();
	Destroy();
}
