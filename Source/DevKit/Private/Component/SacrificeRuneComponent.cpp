#include "Component/SacrificeRuneComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Actors/SacrificeShadowEchoActor.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Engine/World.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Nodes/FlowNode.h"

namespace
{
	FGameplayTag TagOrFallback(const FGameplayTag& ConfiguredTag, const TCHAR* FallbackTagName)
	{
		if (ConfiguredTag.IsValid())
		{
			return ConfiguredTag;
		}
		return FGameplayTag::RequestGameplayTag(FName(FallbackTagName), false);
	}

	float DistancePointToSegment2D(const FVector& Point, const FVector& SegmentStart, const FVector& SegmentEnd)
	{
		const FVector A(SegmentStart.X, SegmentStart.Y, 0.0f);
		const FVector B(SegmentEnd.X, SegmentEnd.Y, 0.0f);
		const FVector P(Point.X, Point.Y, 0.0f);
		const FVector AB = B - A;
		const float LengthSq = AB.SizeSquared();
		if (LengthSq <= KINDA_SMALL_NUMBER)
		{
			return FVector::Dist2D(Point, SegmentStart);
		}
		const float T = FMath::Clamp(FVector::DotProduct(P - A, AB) / LengthSq, 0.0f, 1.0f);
		return FVector::Dist2D(P, A + AB * T);
	}
}

USacrificeRuneComponent::USacrificeRuneComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USacrificeRuneComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureBindings();
}

void USacrificeRuneComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerCharacterBase* Player = GetPlayerOwner())
	{
		if (Player->CombatDeckComponent)
		{
			Player->CombatDeckComponent->OnCardConsumed.RemoveDynamic(this, &USacrificeRuneComponent::HandleCardConsumed);
		}
		if (UYogAbilitySystemComponent* ASC = Player->GetASC())
		{
			ASC->DealtDamage.RemoveDynamic(this, &USacrificeRuneComponent::HandlePlayerDamageDealt);
			for (const TPair<FGuid, FActiveGameplayEffectHandle>& Pair : DashChargeEffectHandles)
			{
				if (Pair.Value.IsValid())
				{
					ASC->RemoveActiveGameplayEffect(Pair.Value);
				}
			}
		}
	}

	DashChargeEffectHandles.Reset();
	RemoveAllShadowMarks();
	KnockbackTracks.Reset();
	if (ActiveShadow)
	{
		ActiveShadow->Destroy();
		ActiveShadow = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void USacrificeRuneComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	TArray<TObjectKey<AActor>> ExpiredMarks;
	for (const TPair<TObjectKey<AActor>, FShadowMarkState>& Pair : ShadowMarks)
	{
		if (!Pair.Value.Target.IsValid() || Pair.Value.ExpireTime <= Now)
		{
			ExpiredMarks.Add(Pair.Key);
		}
	}
	for (const TObjectKey<AActor>& Key : ExpiredMarks)
	{
		if (const FShadowMarkState* State = ShadowMarks.Find(Key))
		{
			if (AActor* Target = State->Target.Get())
			{
				if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
				{
					ASC->RemoveLooseGameplayTag(State->MarkTag);
				}
			}
		}
		ShadowMarks.Remove(Key);
	}

	TickKnockbackCollisionTracks(DeltaTime);
}

void USacrificeRuneComponent::AddSacrificePassive(FGuid SourceGuid, const FSacrificeRunePassiveConfig& Config)
{
	if (!SourceGuid.IsValid())
	{
		SourceGuid = FGuid::NewGuid();
	}

	ActivePassives.Add(SourceGuid, Config);
	RebuildEffectiveConfigs();
	EnsureBindings();

	if (Config.PassiveType == ESacrificeRunePassiveType::ShadowMark)
	{
		ApplyDashChargeBonus(SourceGuid, Config);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SacrificeRune] Added Passive=%d Guid=%s Owner=%s"),
		static_cast<int32>(Config.PassiveType),
		*SourceGuid.ToString(),
		*GetNameSafe(GetOwner()));
}

void USacrificeRuneComponent::RemoveSacrificePassive(FGuid SourceGuid)
{
	const FSacrificeRunePassiveConfig* ExistingConfig = ActivePassives.Find(SourceGuid);
	if (ExistingConfig && ExistingConfig->PassiveType == ESacrificeRunePassiveType::ShadowMark)
	{
		RemoveDashChargeBonus(SourceGuid);
	}

	ActivePassives.Remove(SourceGuid);
	RebuildEffectiveConfigs();
}

void USacrificeRuneComponent::NotifyDashExecuted(const FVector& StartLocation, const FVector& EndLocation, const FVector& Direction)
{
	if (const FSacrificeRunePassiveConfig* Config = GetConfig(ESacrificeRunePassiveType::MoonlightShadow))
	{
		SpawnMoonlightShadow(StartLocation, Direction, *Config);
	}
	if (const FSacrificeRunePassiveConfig* Config = GetConfig(ESacrificeRunePassiveType::ShadowMark))
	{
		ApplyDashShadowMarks(StartLocation, EndLocation, *Config);
	}
}

bool USacrificeRuneComponent::HasPassive(ESacrificeRunePassiveType Type) const
{
	return EffectiveConfigs.Contains(Type);
}

APlayerCharacterBase* USacrificeRuneComponent::GetPlayerOwner() const
{
	return Cast<APlayerCharacterBase>(GetOwner());
}

UYogAbilitySystemComponent* USacrificeRuneComponent::GetPlayerASC() const
{
	const APlayerCharacterBase* Player = GetPlayerOwner();
	return Player ? Player->GetASC() : nullptr;
}

const FSacrificeRunePassiveConfig* USacrificeRuneComponent::GetConfig(ESacrificeRunePassiveType Type) const
{
	return EffectiveConfigs.Find(Type);
}

void USacrificeRuneComponent::RebuildEffectiveConfigs()
{
	EffectiveConfigs.Reset();
	for (const TPair<FGuid, FSacrificeRunePassiveConfig>& Pair : ActivePassives)
	{
		FSacrificeRunePassiveConfig& Effective = EffectiveConfigs.FindOrAdd(Pair.Value.PassiveType);
		Effective.PassiveType = Pair.Value.PassiveType;
		Effective.ShadowAttackCharges = FMath::Max(Effective.ShadowAttackCharges, Pair.Value.ShadowAttackCharges);
		Effective.ShadowLifetime = FMath::Max(Effective.ShadowLifetime, Pair.Value.ShadowLifetime);
		Effective.ShadowDamageMultiplier = FMath::Max(Effective.ShadowDamageMultiplier, Pair.Value.ShadowDamageMultiplier);
		Effective.ShadowAttackRange = FMath::Max(Effective.ShadowAttackRange, Pair.Value.ShadowAttackRange);
		Effective.ShadowAttackConeDegrees = FMath::Max(Effective.ShadowAttackConeDegrees, Pair.Value.ShadowAttackConeDegrees);
		Effective.DashMarkRadius = FMath::Max(Effective.DashMarkRadius, Pair.Value.DashMarkRadius);
		Effective.MarkDuration = FMath::Max(Effective.MarkDuration, Pair.Value.MarkDuration);
		Effective.MarkExplosionDamage = FMath::Max(Effective.MarkExplosionDamage, Pair.Value.MarkExplosionDamage);
		Effective.MarkExplosionRadius = FMath::Max(Effective.MarkExplosionRadius, Pair.Value.MarkExplosionRadius);
		Effective.ShadowMarkTag = Pair.Value.ShadowMarkTag.IsValid() ? Pair.Value.ShadowMarkTag : Effective.ShadowMarkTag;
		Effective.KnockbackDistance = FMath::Max(Effective.KnockbackDistance, Pair.Value.KnockbackDistance);
		Effective.KnockbackCollisionDamage = FMath::Max(Effective.KnockbackCollisionDamage, Pair.Value.KnockbackCollisionDamage);
		Effective.KnockbackCollisionRadius = FMath::Max(Effective.KnockbackCollisionRadius, Pair.Value.KnockbackCollisionRadius);
		Effective.KnockbackCollisionDuration = FMath::Max(Effective.KnockbackCollisionDuration, Pair.Value.KnockbackCollisionDuration);
		Effective.KnockbackCollisionTickInterval = FMath::Max(0.01f, Pair.Value.KnockbackCollisionTickInterval);
	}
}

void USacrificeRuneComponent::EnsureBindings()
{
	if (bBindingsInitialized)
	{
		return;
	}

	APlayerCharacterBase* Player = GetPlayerOwner();
	if (!Player)
	{
		return;
	}

	if (UYogAbilitySystemComponent* ASC = Player->GetASC())
	{
		ASC->DealtDamage.AddUniqueDynamic(this, &USacrificeRuneComponent::HandlePlayerDamageDealt);
	}
	if (Player->CombatDeckComponent)
	{
		Player->CombatDeckComponent->OnCardConsumed.AddUniqueDynamic(this, &USacrificeRuneComponent::HandleCardConsumed);
	}
	bBindingsInitialized = true;
}

void USacrificeRuneComponent::RemoveAllShadowMarks()
{
	for (const TPair<TObjectKey<AActor>, FShadowMarkState>& Pair : ShadowMarks)
	{
		if (AActor* Target = Pair.Value.Target.Get())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
			{
				ASC->RemoveLooseGameplayTag(Pair.Value.MarkTag);
			}
		}
	}
	ShadowMarks.Reset();
}

void USacrificeRuneComponent::ApplyDashChargeBonus(FGuid SourceGuid, const FSacrificeRunePassiveConfig& Config)
{
	if (DashChargeEffectHandles.Contains(SourceGuid) || Config.DashChargeBonus <= 0.0f)
	{
		return;
	}

	UYogAbilitySystemComponent* ASC = GetPlayerASC();
	if (!ASC)
	{
		return;
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UPlayerAttributeSet::GetMaxDashChargeAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Config.DashChargeBonus));
	Effect->Modifiers.Add(Modifier);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(GetOwner(), GetOwner());
	FGameplayEffectSpec Spec(Effect, Context, 1.0f);
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
	if (Handle.IsValid())
	{
		DashChargeEffectHandles.Add(SourceGuid, Handle);
	}
}

void USacrificeRuneComponent::RemoveDashChargeBonus(FGuid SourceGuid)
{
	if (FActiveGameplayEffectHandle* Handle = DashChargeEffectHandles.Find(SourceGuid))
	{
		if (UYogAbilitySystemComponent* ASC = GetPlayerASC())
		{
			ASC->RemoveActiveGameplayEffect(*Handle);
		}
		DashChargeEffectHandles.Remove(SourceGuid);
	}
}

void USacrificeRuneComponent::SpawnMoonlightShadow(const FVector& StartLocation, const FVector& Direction, const FSacrificeRunePassiveConfig& Config)
{
	APlayerCharacterBase* Player = GetPlayerOwner();
	UWorld* World = GetWorld();
	if (!Player || !World)
	{
		return;
	}

	if (ActiveShadow)
	{
		ActiveShadow->Destroy();
		ActiveShadow = nullptr;
	}

	const FRotator Rotation = Direction.IsNearlyZero()
		? Player->GetActorRotation()
		: Direction.GetSafeNormal2D().Rotation();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Player;
	SpawnParams.Instigator = Player;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveShadow = World->SpawnActor<ASacrificeShadowEchoActor>(
		ASacrificeShadowEchoActor::StaticClass(),
		StartLocation,
		Rotation,
		SpawnParams);
	if (ActiveShadow)
	{
		ActiveShadow->InitializeFromPlayer(Player, Config.ShadowAttackCharges, Config.ShadowLifetime);
		UE_LOG(LogTemp, Warning, TEXT("[SacrificeRune][MoonlightShadow] Spawned Location=%s Attacks=%d"),
			*StartLocation.ToCompactString(),
			Config.ShadowAttackCharges);
	}
}

void USacrificeRuneComponent::ReplayOffensiveCardFlowsFromShadow(const FCombatCardResolveResult& Result)
{
	if (!ActiveShadow || Result.ExecutedFlows.IsEmpty())
	{
		return;
	}

	APlayerCharacterBase* Player = GetPlayerOwner();
	if (!Player || !Player->BuffFlowComponent)
	{
		return;
	}

	const FTransform SourceTransform(ActiveShadow->GetActorRotation(), ActiveShadow->GetActorLocation(), FVector::OneVector);
	for (UFlowAsset* FlowAsset : Result.ExecutedFlows)
	{
		if (!FlowHasOffensiveSpawnNode(FlowAsset))
		{
			continue;
		}

		Player->BuffFlowComponent->StartCombatCardFlowWithSourceTransform(
			FlowAsset,
			Result.ConsumedCard,
			Result.ActionContext,
			Result,
			Player,
			SourceTransform,
			true);
	}
}

void USacrificeRuneComponent::QueueShadowAttackDamage(float PlayerDamage)
{
	if (PendingShadowDamageApplications <= 0 || PlayerDamage <= 0.0f)
	{
		return;
	}

	if (const FSacrificeRunePassiveConfig* Config = GetConfig(ESacrificeRunePassiveType::MoonlightShadow))
	{
		PerformShadowAttack(PlayerDamage, *Config);
	}
	PendingShadowDamageApplications = FMath::Max(0, PendingShadowDamageApplications - 1);
}

void USacrificeRuneComponent::PerformShadowAttack(float PlayerDamage, const FSacrificeRunePassiveConfig& Config)
{
	if (!ActiveShadow)
	{
		return;
	}

	const FVector Origin = ActiveShadow->GetActorLocation();
	FVector Forward = ActiveShadow->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal();
	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Config.ShadowAttackConeDegrees, 0.0f, 180.0f) * 0.5f));
	const float Damage = PlayerDamage * FMath::Max(0.0f, Config.ShadowDamageMultiplier);
	for (AActor* Actor : FindEnemyActorsInRadius(Origin, Config.ShadowAttackRange))
	{
		const FVector ToTarget = (Actor->GetActorLocation() - Origin).GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToTarget) < CosThreshold)
		{
			continue;
		}
		ApplyInstantPureDamage(Actor, Damage, TEXT("Rune_Sacrifice_MoonlightShadow"), false);
	}
}

void USacrificeRuneComponent::ApplyDashShadowMarks(const FVector& StartLocation, const FVector& EndLocation, const FSacrificeRunePassiveConfig& Config)
{
	for (AActor* Actor : FindEnemyActorsInRadius(StartLocation, FVector::Dist2D(StartLocation, EndLocation) + Config.DashMarkRadius))
	{
		const float Distance = DistancePointToSegment2D(Actor->GetActorLocation(), StartLocation, EndLocation);
		if (Distance <= Config.DashMarkRadius)
		{
			AddShadowMark(Actor, Config);
		}
	}
}

void USacrificeRuneComponent::AddShadowMark(AActor* Target, const FSacrificeRunePassiveConfig& Config)
{
	if (!IsValidEnemyTarget(Target) || !GetWorld())
	{
		return;
	}

	const FGameplayTag MarkTag = TagOrFallback(Config.ShadowMarkTag, TEXT("Buff.Status.ShadowMark"));
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		ASC->AddLooseGameplayTag(MarkTag);
	}

	FShadowMarkState State;
	State.Target = Target;
	State.ExpireTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, Config.MarkDuration);
	State.MarkTag = MarkTag;
	ShadowMarks.Add(TObjectKey<AActor>(Target), State);
	UE_LOG(LogTemp, Warning, TEXT("[SacrificeRune][ShadowMark] Marked Target=%s Duration=%.2f"),
		*GetNameSafe(Target),
		Config.MarkDuration);
}

bool USacrificeRuneComponent::ConsumeShadowMark(AActor* Target)
{
	if (!Target)
	{
		return false;
	}

	FShadowMarkState State;
	if (!ShadowMarks.RemoveAndCopyValue(TObjectKey<AActor>(Target), State))
	{
		return false;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		ASC->RemoveLooseGameplayTag(State.MarkTag);
	}
	return true;
}

void USacrificeRuneComponent::DetonateShadowMark(AActor* CenterTarget, const FSacrificeRunePassiveConfig& Config)
{
	if (!CenterTarget)
	{
		return;
	}

	const FVector Origin = CenterTarget->GetActorLocation();
	for (AActor* Actor : FindEnemyActorsInRadius(Origin, Config.MarkExplosionRadius))
	{
		ApplyInstantPureDamage(Actor, Config.MarkExplosionDamage, TEXT("Rune_Sacrifice_ShadowMarkExplosion"), false);
	}
}

void USacrificeRuneComponent::TriggerGiantSwing(AActor* Target, const FSacrificeRunePassiveConfig& Config)
{
	APlayerCharacterBase* Player = GetPlayerOwner();
	if (!Player || !IsValidEnemyTarget(Target))
	{
		return;
	}

	static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	if (KnockbackTag.IsValid())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Player;
		Payload.Target = Target;
		Payload.EventMagnitude = Config.KnockbackDistance;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, KnockbackTag, Payload);
	}

	FKnockbackCollisionTrack Track;
	Track.Target = Target;
	Track.RemainingTime = FMath::Max(0.01f, Config.KnockbackCollisionDuration);
	Track.Config = Config;
	Track.DamagedActors.Add(Target);
	Track.DamagedActors.Add(Player);
	KnockbackTracks.Add(MoveTemp(Track));
}

void USacrificeRuneComponent::TickKnockbackCollisionTracks(float DeltaTime)
{
	for (int32 Index = KnockbackTracks.Num() - 1; Index >= 0; --Index)
	{
		FKnockbackCollisionTrack& Track = KnockbackTracks[Index];
		AActor* Target = Track.Target.Get();
		Track.RemainingTime -= DeltaTime;
		Track.TickAccumulator += DeltaTime;
		if (!Target || Track.RemainingTime <= 0.0f)
		{
			KnockbackTracks.RemoveAtSwap(Index);
			continue;
		}

		const float TickInterval = FMath::Max(0.01f, Track.Config.KnockbackCollisionTickInterval);
		if (Track.TickAccumulator < TickInterval)
		{
			continue;
		}
		Track.TickAccumulator = 0.0f;

		for (AActor* Actor : FindEnemyActorsInRadius(Target->GetActorLocation(), Track.Config.KnockbackCollisionRadius, Target))
		{
			if (Track.DamagedActors.Contains(Actor))
			{
				continue;
			}
			Track.DamagedActors.Add(Actor);
			ApplyInstantPureDamage(Actor, Track.Config.KnockbackCollisionDamage, TEXT("Rune_Sacrifice_GiantSwingCollision"), false);
		}
	}
}

TArray<AActor*> USacrificeRuneComponent::FindEnemyActorsInRadius(const FVector& Origin, float Radius, AActor* ExcludeActor) const
{
	TArray<AActor*> Result;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}

	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsOfClass(World, AYogCharacterBase::StaticClass(), Candidates);
	const float RadiusSq = FMath::Square(FMath::Max(0.0f, Radius));
	for (AActor* Actor : Candidates)
	{
		if (!Actor || Actor == ExcludeActor || !IsValidEnemyTarget(Actor))
		{
			continue;
		}
		if (FVector::DistSquared2D(Origin, Actor->GetActorLocation()) <= RadiusSq)
		{
			Result.Add(Actor);
		}
	}
	return Result;
}

bool USacrificeRuneComponent::IsValidEnemyTarget(AActor* Actor) const
{
	if (!Actor || Actor == GetOwner() || Cast<APlayerCharacterBase>(Actor))
	{
		return false;
	}

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor) != nullptr;
}

void USacrificeRuneComponent::ApplyInstantPureDamage(AActor* Target, float Damage, FName DamageLogType, bool bSuppressFeedback)
{
	if (bApplyingSacrificeDamage || Damage <= 0.0f || !Target)
	{
		return;
	}

	UYogAbilitySystemComponent* SourceASC = GetPlayerASC();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	TGuardValue<bool> Guard(bApplyingSacrificeDamage, true);
	if (bSuppressFeedback)
	{
		if (UYogAbilitySystemComponent* TargetYogASC = Cast<UYogAbilitySystemComponent>(TargetASC))
		{
			TargetYogASC->SuppressNextDamageFeedback();
		}
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UDamageAttributeSet::GetDamagePureAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Damage));
	Effect->Modifiers.Add(Modifier);

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(GetOwner(), GetOwner());
	Context.AddSourceObject(this);
	FGameplayEffectSpec Spec(Effect, Context, 1.0f);
	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	SourceASC->LogDamageDealt(Target, Damage, DamageLogType);
}

bool USacrificeRuneComponent::IsPlayerInAttackState() const
{
	const UYogAbilitySystemComponent* ASC = GetPlayerASC();
	if (!ASC)
	{
		return false;
	}

	static const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"), false);
	static const FGameplayTag HeavyAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"), false);
	static const FGameplayTag DashAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.DashAtk"), false);
	return (LightAttackTag.IsValid() && ASC->HasMatchingGameplayTag(LightAttackTag))
		|| (HeavyAttackTag.IsValid() && ASC->HasMatchingGameplayTag(HeavyAttackTag))
		|| (DashAttackTag.IsValid() && ASC->HasMatchingGameplayTag(DashAttackTag));
}

bool USacrificeRuneComponent::FlowHasOffensiveSpawnNode(const UFlowAsset* FlowAsset) const
{
	if (!FlowAsset)
	{
		return false;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value)
			|| Cast<UBFNode_SpawnRuneGroundPathEffect>(Pair.Value))
		{
			return true;
		}
	}
	return false;
}

void USacrificeRuneComponent::HandlePlayerDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage)
{
	if (bApplyingSacrificeDamage || !TargetASC)
	{
		return;
	}

	AActor* TargetActor = TargetASC->GetAvatarActor();
	QueueShadowAttackDamage(Damage);

	if (const FSacrificeRunePassiveConfig* Config = GetConfig(ESacrificeRunePassiveType::ShadowMark))
	{
		if (ConsumeShadowMark(TargetActor))
		{
			DetonateShadowMark(TargetActor, *Config);
		}
	}

	if (const FSacrificeRunePassiveConfig* Config = GetConfig(ESacrificeRunePassiveType::GiantSwing))
	{
		if (IsPlayerInAttackState())
		{
			TriggerGiantSwing(TargetActor, *Config);
		}
	}
}

void USacrificeRuneComponent::HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
	if (!ActiveShadow)
	{
		return;
	}

	ReplayOffensiveCardFlowsFromShadow(Result);
	PendingShadowDamageApplications++;
	ActiveShadow->ConsumeAttackCharge();
}
