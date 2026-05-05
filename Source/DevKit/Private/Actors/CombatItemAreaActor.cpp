#include "Actors/CombatItemAreaActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/CombatItemComponent.h"
#include "Components/DecalComponent.h"
#include "Data/EnemyData.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

ACombatItemAreaActor::ACombatItemAreaActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(SceneRoot);
	NiagaraComponent->bAutoActivate = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	DecalComponent->SetupAttachment(SceneRoot);
	DecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	DecalComponent->SetVisibility(false);

	SetActorEnableCollision(false);
}

void ACombatItemAreaActor::BeginPlay()
{
	Super::BeginPlay();
}

void ACombatItemAreaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndSmokePlayerState();
	Super::EndPlay(EndPlayReason);
}

void ACombatItemAreaActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += FMath::Max(0.0f, DeltaSeconds);
	UpdateRadius();
	UpdateVisualScale();

	switch (AreaMode)
	{
	case ECombatItemAreaMode::Thunder:
		TickThunder(DeltaSeconds);
		break;
	case ECombatItemAreaMode::Smoke:
		TickSmoke(DeltaSeconds);
		break;
	default:
		break;
	}
}

void ACombatItemAreaActor::InitializeThunderArea(APlayerCharacterBase* InOwnerPlayer, const FCombatItemConfig& InConfig)
{
	OwnerPlayer = InOwnerPlayer;
	Config = InConfig;
	AreaMode = ECombatItemAreaMode::Thunder;
	ElapsedTime = 0.0f;
	TickAccumulator = Config.Thunder.TickInterval;
	CurrentRadius = FMath::Max(0.0f, Config.Thunder.Radius);

	if (OwnerPlayer)
	{
		AttachToActor(OwnerPlayer, FAttachmentTransformRules::KeepWorldTransform);
		SetActorLocation(OwnerPlayer->GetActorLocation());
	}

	if (NiagaraComponent && Config.NiagaraSystem)
	{
		NiagaraComponent->SetAsset(Config.NiagaraSystem);
		NiagaraComponent->Activate(true);
	}

	if (DecalComponent && Config.DecalMaterial)
	{
		DecalComponent->SetDecalMaterial(Config.DecalMaterial);
		DecalComponent->SetVisibility(true);
	}

	UpdateVisualScale();
	SetLifeSpan(FMath::Max(0.01f, Config.Thunder.Duration));
}

void ACombatItemAreaActor::InitializeSmokeArea(APlayerCharacterBase* InOwnerPlayer, const FCombatItemConfig& InConfig)
{
	OwnerPlayer = InOwnerPlayer;
	Config = InConfig;
	AreaMode = ECombatItemAreaMode::Smoke;
	ElapsedTime = 0.0f;
	TickAccumulator = 0.0f;
	CurrentRadius = FMath::Max(0.0f, Config.Smoke.InitialRadius);

	if (NiagaraComponent && Config.NiagaraSystem)
	{
		NiagaraComponent->SetAsset(Config.NiagaraSystem);
		NiagaraComponent->Activate(true);
	}

	if (DecalComponent && Config.DecalMaterial)
	{
		DecalComponent->SetDecalMaterial(Config.DecalMaterial);
		DecalComponent->SetVisibility(true);
	}

	UpdateVisualScale();
	UpdateSmokePlayerState();
	SetLifeSpan(FMath::Max(0.01f, Config.Smoke.Duration));
}

void ACombatItemAreaActor::UpdateRadius()
{
	if (AreaMode == ECombatItemAreaMode::Smoke)
	{
		const float Duration = FMath::Max(0.01f, Config.Smoke.Duration);
		const float Alpha = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);
		CurrentRadius = FMath::Lerp(
			FMath::Max(0.0f, Config.Smoke.InitialRadius),
			FMath::Max(0.0f, Config.Smoke.FinalRadius),
			Alpha);
		return;
	}

	CurrentRadius = FMath::Max(0.0f, Config.Thunder.Radius);
}

void ACombatItemAreaActor::TickThunder(float DeltaSeconds)
{
	TickAccumulator += FMath::Max(0.0f, DeltaSeconds);
	const float Interval = FMath::Max(0.01f, Config.Thunder.TickInterval);
	while (TickAccumulator >= Interval)
	{
		TickAccumulator -= Interval;
		ApplyThunderTick();
	}
}

void ACombatItemAreaActor::TickSmoke(float DeltaSeconds)
{
	TickAccumulator += FMath::Max(0.0f, DeltaSeconds);
	const float Interval = FMath::Max(0.01f, Config.Smoke.TickInterval);
	if (TickAccumulator >= Interval)
	{
		TickAccumulator = 0.0f;
		UpdateSmokePlayerState();
	}
}

void ACombatItemAreaActor::ApplyThunderTick()
{
	AActor* SourceActor = OwnerPlayer.Get();
	if (!SourceActor || Config.Thunder.DamagePerTick <= 0.0f)
	{
		return;
	}

	for (AActor* TargetActor : CollectEnemyTargets(CurrentRadius))
	{
		UCombatItemComponent::ApplyItemPureDamage(SourceActor, TargetActor, Config.Thunder.DamagePerTick, TEXT("Item_ThunderStone"), true);
		if (Config.Thunder.bInterruptMinions && ShouldInterruptAsMinion(TargetActor))
		{
			SendThunderInterrupt(TargetActor, Config.Thunder.DamagePerTick);
		}
	}
}

void ACombatItemAreaActor::UpdateSmokePlayerState()
{
	if (!OwnerPlayer)
	{
		return;
	}

	const bool bInside = FVector::Dist2D(OwnerPlayer->GetActorLocation(), GetActorLocation()) <= CurrentRadius;
	if (bInside == bPlayerInsideSmoke)
	{
		return;
	}

	bPlayerInsideSmoke = bInside;
	if (UCombatItemComponent* ItemComponent = OwnerPlayer->FindComponentByClass<UCombatItemComponent>())
	{
		ItemComponent->SetPlayerInsideSmoke(bPlayerInsideSmoke, Config.Smoke.DodgeBonus);
	}
}

void ACombatItemAreaActor::EndSmokePlayerState()
{
	if (AreaMode != ECombatItemAreaMode::Smoke || !bPlayerInsideSmoke || !OwnerPlayer)
	{
		return;
	}

	bPlayerInsideSmoke = false;
	if (UCombatItemComponent* ItemComponent = OwnerPlayer->FindComponentByClass<UCombatItemComponent>())
	{
		ItemComponent->SetPlayerInsideSmoke(false, Config.Smoke.DodgeBonus);
	}
}

void ACombatItemAreaActor::UpdateVisualScale()
{
	const float Diameter = FMath::Max(CurrentRadius * 2.0f, 1.0f);
	if (DecalComponent)
	{
		DecalComponent->DecalSize = FVector(32.0f, Diameter, Diameter);
	}
	if (NiagaraComponent)
	{
		NiagaraComponent->SetVariableFloat(TEXT("User.Radius"), CurrentRadius);
		NiagaraComponent->SetWorldScale3D(FVector(FMath::Max(CurrentRadius / 100.0f, 0.01f)));
	}
}

TArray<AActor*> ACombatItemAreaActor::CollectEnemyTargets(float Radius) const
{
	TArray<AActor*> Targets;
	UWorld* World = GetWorld();
	if (!World || Radius <= 0.0f)
	{
		return Targets;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatItemArea), false, this);
	if (OwnerPlayer)
	{
		QueryParams.AddIgnoredActor(OwnerPlayer);
	}

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (Actor && Actor != OwnerPlayer && Actor->IsA<AEnemyCharacterBase>())
		{
			Targets.AddUnique(Actor);
		}
	}

	return Targets;
}

bool ACombatItemAreaActor::ShouldInterruptAsMinion(AActor* TargetActor) const
{
	const AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(TargetActor);
	if (!Enemy || !Enemy->IsAlive())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"), false);
	if (ASC
		&& ((SuperArmorTag.IsValid() && ASC->HasMatchingGameplayTag(SuperArmorTag))
			|| (DeadTag.IsValid() && ASC->HasMatchingGameplayTag(DeadTag))))
	{
		return false;
	}

	const UCharacterDataComponent* DataComponent = Enemy->GetCharacterDataComponent();
	const UEnemyData* EnemyData = DataComponent ? Cast<UEnemyData>(DataComponent->GetCharacterData()) : nullptr;
	return EnemyData && EnemyData->DifficultyScore <= 4;
}

void ACombatItemAreaActor::SendThunderInterrupt(AActor* TargetActor, float Damage) const
{
	static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"), false);
	if (!HitReactTag.IsValid() || !TargetActor)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = HitReactTag;
	Payload.Instigator = OwnerPlayer;
	Payload.Target = TargetActor;
	Payload.EventMagnitude = Damage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, HitReactTag, Payload);
}
