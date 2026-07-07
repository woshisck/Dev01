#include "Projectile/YogBulletManagerSubsystem.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

// ─── SpawnBullet ─────────────────────────────────────────────────────────────

void UYogBulletManagerSubsystem::SpawnBullet(const FYogBulletSpawnParams& Params)
{
	FYogBulletState& State = ActiveBullets.AddDefaulted_GetRef();

	State.Position             = Params.SpawnLocation;
	State.Direction            = Params.Direction.GetSafeNormal();
	State.Speed                = FMath::Max(1.f, Params.Speed);
	State.CollisionRadius      = FMath::Max(1.f, Params.CollisionRadius);
	State.CollisionHalfHeight  = FMath::Max(State.CollisionRadius, Params.CollisionHalfHeight);
	State.Lifetime             = FMath::Max(0.01f, Params.Lifetime);
	State.Elapsed              = 0.f;
	State.bPiercing            = Params.bPiercing;
	State.bDrawDebug           = Params.bDrawDebug;
	State.HitsRemaining        = Params.bPiercing
	                               ? (Params.MaxHits > 0 ? Params.MaxHits : -1)
	                               : 1;
	State.SourceCharacter      = Params.SourceCharacter;
	State.SourceASC            = Params.SourceASC;
	State.EffectMagnitude      = Params.EffectMagnitude;
	State.HitEventTag          = Params.HitEventTag;
	State.ExpireEventTag       = Params.ExpireEventTag;
	State.bSendHitEventToCreator = Params.bSendHitEventToCreator;
	State.HitNiagaraSystem     = Params.HitNiagaraSystem;
	State.HitNiagaraScale      = Params.HitNiagaraScale;
	State.ExpireNiagaraSystem  = Params.ExpireNiagaraSystem;
	State.ExpireNiagaraScale   = Params.ExpireNiagaraScale;

	if (State.Direction.IsNearlyZero())
	{
		State.Direction = FVector::ForwardVector;
	}

	if (Params.TravelNiagaraSystem)
	{
		if (UWorld* World = GetWorld())
		{
			State.TravelNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World, Params.TravelNiagaraSystem,
				State.Position, State.Direction.Rotation(), Params.TravelNiagaraScale,
				/*bAutoDestroy*/ false, /*bAutoActivate*/ true, ENCPoolMethod::None);
		}
	}
}

// ─── Tick ─────────────────────────────────────────────────────────────────────

void UYogBulletManagerSubsystem::Tick(float DeltaTime)
{
	// Collect removal indices in reverse order so swap-remove stays correct.
	TArray<int32> ToRemove;

	for (int32 i = ActiveBullets.Num() - 1; i >= 0; --i)
	{
		FYogBulletState& Bullet = ActiveBullets[i];

		Bullet.Position += Bullet.Direction * Bullet.Speed * DeltaTime;
		Bullet.Elapsed  += DeltaTime;

		if (Bullet.TravelNiagaraComp)
		{
			Bullet.TravelNiagaraComp->SetWorldLocation(Bullet.Position);
		}

		if (Bullet.Elapsed >= Bullet.Lifetime)
		{
			SendExpireEvent(Bullet);
			SpawnBurstNiagara(Bullet.ExpireNiagaraSystem, Bullet.Position, Bullet.ExpireNiagaraScale);
			DestroyBulletVisual(Bullet);
			ActiveBullets.RemoveAtSwap(i);
			continue;
		}

		const bool bHit = CheckBulletHits(Bullet);

		// Non-piercing: remove after first hit.
		// Piercing with finite MaxHits: remove when count reaches zero.
		const bool bExhausted = bHit && (!Bullet.bPiercing || Bullet.HitsRemaining == 0);
		if (bExhausted)
		{
			DestroyBulletVisual(Bullet);
			ActiveBullets.RemoveAtSwap(i);
		}
	}
}

TStatId UYogBulletManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYogBulletManagerSubsystem, STATGROUP_Tickables);
}

// ─── Hit detection ────────────────────────────────────────────────────────────

bool UYogBulletManagerSubsystem::CheckBulletHits(FYogBulletState& Bullet)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(YogBulletManagerSweep), false);
	if (Bullet.SourceCharacter.IsValid())
	{
		QueryParams.AddIgnoredActor(Bullet.SourceCharacter.Get());
	}

	// The capsule's local axis is +Z; align it to the flight direction so the
	// long axis runs along travel.
	const FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, Bullet.Direction);
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Bullet.CollisionRadius, Bullet.CollisionHalfHeight);

#if ENABLE_DRAW_DEBUG
	if (Bullet.bDrawDebug)
	{
		DrawDebugCapsule(World, Bullet.Position, Bullet.CollisionHalfHeight, Bullet.CollisionRadius,
			CapsuleRot, FColor::Green, /*bPersistentLines*/ false, /*LifeTime*/ -1.f, /*DepthPriority*/ 0, /*Thickness*/ 1.f);
	}
#endif

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		Bullet.Position,
		CapsuleRot,
		ECC_Pawn,
		Capsule,
		QueryParams);

	bool bHitAny = false;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (Bullet.HitsRemaining == 0)
		{
			break;
		}

		AActor* Target = Overlap.GetActor();
		if (!Target)
		{
			continue;
		}

		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
		{
			continue;
		}

		SendHitEvent(Bullet, Target);
		SpawnBurstNiagara(Bullet.HitNiagaraSystem, Bullet.Position, Bullet.HitNiagaraScale);
		bHitAny = true;

		if (Bullet.HitsRemaining > 0)
		{
			--Bullet.HitsRemaining;
		}

		if (!Bullet.bPiercing)
		{
			break;
		}
	}

	return bHitAny;
}

// ─── Event helpers ────────────────────────────────────────────────────────────

void UYogBulletManagerSubsystem::SendHitEvent(const FYogBulletState& Bullet, AActor* Target) const
{
	if (!Bullet.HitEventTag.IsValid() || !Target)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Bullet.SourceASC.Get();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	UAbilitySystemComponent* EventASC = Bullet.bSendHitEventToCreator ? SourceASC : TargetASC;
	if (!EventASC || !SourceASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag      = Bullet.HitEventTag;
	Payload.Instigator    = Bullet.SourceCharacter.Get();
	Payload.Target        = Target;
	Payload.EventMagnitude = Bullet.EffectMagnitude;
	Payload.ContextHandle  = SourceASC->MakeEffectContext();
	Payload.ContextHandle.AddInstigator(Bullet.SourceCharacter.Get(), Bullet.SourceCharacter.Get());

	EventASC->HandleGameplayEvent(Bullet.HitEventTag, &Payload);
}

void UYogBulletManagerSubsystem::SendExpireEvent(const FYogBulletState& Bullet) const
{
	if (!Bullet.ExpireEventTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Bullet.SourceASC.Get();
	if (!SourceASC)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag       = Bullet.ExpireEventTag;
	Payload.Instigator     = Bullet.SourceCharacter.Get();
	Payload.Target         = Bullet.SourceCharacter.Get();
	Payload.EventMagnitude = Bullet.EffectMagnitude;
	Payload.ContextHandle  = SourceASC->MakeEffectContext();
	Payload.ContextHandle.AddInstigator(Bullet.SourceCharacter.Get(), Bullet.SourceCharacter.Get());

	SourceASC->HandleGameplayEvent(Bullet.ExpireEventTag, &Payload);
}

void UYogBulletManagerSubsystem::SpawnBurstNiagara(
	UNiagaraSystem* System, const FVector& Location, const FVector& Scale) const
{
	if (!System)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, Location, FRotator::ZeroRotator, Scale, true, true);
}

void UYogBulletManagerSubsystem::DestroyBulletVisual(FYogBulletState& Bullet) const
{
	if (UNiagaraComponent* Comp = Bullet.TravelNiagaraComp.Get())
	{
		Comp->Deactivate();
		Comp->DestroyComponent();
	}
	Bullet.TravelNiagaraComp = nullptr;
}

void UYogBulletManagerSubsystem::Deinitialize()
{
	for (FYogBulletState& Bullet : ActiveBullets)
	{
		DestroyBulletVisual(Bullet);
	}
	ActiveBullets.Reset();

	Super::Deinitialize();
}
