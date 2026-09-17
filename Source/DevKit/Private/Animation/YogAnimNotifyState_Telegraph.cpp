#include "Animation/YogAnimNotifyState_Telegraph.h"

#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Actors/YogTelegraphZoneActor.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimSequenceBase.h"
#include "Character/YogCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/MontageAttackDataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

namespace
{
	void Telegraph_LogResolvedShape(const TCHAR* Source, const FYogTelegraphShape& Shape)
	{
		UE_LOG(LogTemp, Log,
			TEXT("ANS Pre-Attack Telegraph resolved from %s: Shape=%s Radius=%.1f HalfAngle=%.1f "
				 "InnerRadius=%.1f HalfWidth=%.1f Offset=%s"),
			Source,
			*UEnum::GetValueAsString(Shape.ShapeType),
			Shape.Radius, Shape.HalfAngle, Shape.InnerRadius, Shape.HalfWidth,
			*Shape.LocalOffset.ToString());
	}

	/**
	 * The melee damage notify this telegraph warns about: the earliest one triggering at or after
	 * the window opens. Picking by time rather than "first in the montage" is what lets a
	 * multi-hit combo montage telegraph each swing with that swing's own reach.
	 *
	 * Anchored on the window START deliberately. Anchoring on its end looks tidier but silently
	 * rejects the swing whenever the telegraph overlaps past the damage frame, which is a normal
	 * way to author it.
	 */
	const UAN_MeleeDamage* Telegraph_FindDamageNotifyFrom(const UAnimSequenceBase* Animation, float WindowStartTime)
	{
		if (!Animation)
		{
			return nullptr;
		}

		const UAN_MeleeDamage* Best = nullptr;
		float BestTime = TNumericLimits<float>::Max();
		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			const UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(Event.Notify);
			if (!DamageNotify)
			{
				continue;
			}

			const float TriggerTime = Event.GetTriggerTime();
			if (TriggerTime < WindowStartTime - KINDA_SMALL_NUMBER)
			{
				continue;
			}

			if (TriggerTime < BestTime)
			{
				BestTime = TriggerTime;
				Best = DamageNotify;
			}
		}

		return Best;
	}

	/** Trigger times of every melee damage notify on this animation, for diagnostics. */
	FString Telegraph_DescribeDamageNotifies(const UAnimSequenceBase* Animation)
	{
		if (!Animation)
		{
			return TEXT("<null anim>");
		}

		FString Out;
		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			if (Cast<UAN_MeleeDamage>(Event.Notify))
			{
				Out += FString::Printf(TEXT("t=%.3f "), Event.GetTriggerTime());
			}
		}

		return Out.IsEmpty() ? FString(TEXT("<none on this animation>")) : Out;
	}

	bool Telegraph_ResolveShapeFromActionData(const FActionData& ActionData, FYogTelegraphShape& OutShape)
	{
		const TArray<FYogHitboxType>& Hitboxes = ActionData.hitboxTypes;
		if (Hitboxes.IsEmpty())
		{
			return false;
		}

		// Verbatim, matching IsTargetHit: ActRange already carries the character's AttackRange
		// and the zone must cover exactly ActRange + AttackRange.
		const float ActRange = ActionData.ActRange;

		const FYogHitboxType& Hitbox = Hitboxes[0];
		FYogTelegraphShape Resolved;
		Resolved.ShapeType = Hitbox.hitboxType;
		Resolved.Radius = ActRange;

		switch (Hitbox.hitboxType)
		{
		case EHitBoxType::Annulus:
		{
			const FHitboxAnnulus& Annulus = Hitbox.AnnulusHitbox;
			const float InnerR = FMath::Max(Annulus.inner_radius, 0.f);

			// Mirror YogTargetType_Melee::IsInAnnulus exactly: bAutoOffset moves the centre back
			// by InnerR, and the outer radius stays ActRange with no compensation.
			Resolved.Radius = ActRange;
			Resolved.InnerRadius = InnerR;
			Resolved.HalfAngle = Annulus.degree * 0.5f;
			Resolved.LocalOffset.X = Annulus.bAutoOffset ? -InnerR : Annulus.OffsetCore;
			break;
		}

		case EHitBoxType::Triangle:
			if (Hitbox.HitboxTriangles.IsEmpty())
			{
				return false;
			}
			Resolved.HalfAngle = Hitbox.HitboxTriangles[0].Degree * 0.5f;
			break;

		case EHitBoxType::Square:
			if (Hitbox.HitboxSquares.IsEmpty())
			{
				return false;
			}
			Resolved.HalfWidth = Hitbox.HitboxSquares[0].Width * 0.5f;
			break;

		case EHitBoxType::Circle:
			Resolved.HalfAngle = 180.f;
			if (!Hitbox.HitboxCircles.IsEmpty())
			{
				Resolved.LocalOffset = Hitbox.HitboxCircles[0].Offset;
			}
			break;

		default:
			return false;
		}

		// A zero-degree arc renders nothing, so treat it as unauthored and keep the defaults.
		const bool bNeedsAngle = Hitbox.hitboxType == EHitBoxType::Annulus
			|| Hitbox.hitboxType == EHitBoxType::Triangle;
		if (bNeedsAngle && Resolved.HalfAngle <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		OutShape = Resolved;
		return true;
	}
}

FYogTelegraphShape UYogAnimNotifyState_Telegraph::ResolveShape(USkeletalMeshComponent* MeshComp,
	const UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) const
{
	FYogTelegraphShape Shape;

	// Primary source: the ability driving this montage. GetAbilityActionData resolves the whole
	// combo chain (active combo config -> combo data -> cached notify -> montage config) and
	// applies AttackDataOverride, so it is the same data the hit itself will use. Scanning the
	// montage for a notify bypasses all of that - ANS_MontageVFXBinding uses this order too.
	FString AbilityDiagnostic(TEXT("no active UYogGameplayAbility"));
	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
	{
		if (UYogAbilitySystemComponent* ASC = Character->GetASC())
		{
			if (const UYogGameplayAbility* ActiveAbility = ASC->GetCurrentAbilityInstance())
			{
				FActionData ActionData = ActiveAbility->GetAbilityActionData();
				ActionData.ActRange = Character->ResolveEffectiveAttackRange(ActionData.ActRange);
				if (Telegraph_ResolveShapeFromActionData(ActionData, Shape))
				{
					Telegraph_LogResolvedShape(*ActiveAbility->GetClass()->GetName(), Shape);
					return Shape;
				}

				AbilityDiagnostic = FString::Printf(TEXT("%s gave ActRange=%.1f with %d hitbox(es)"),
					*ActiveAbility->GetClass()->GetName(), ActionData.ActRange, ActionData.hitboxTypes.Num());
			}
		}
	}

	// Fallback: the next melee damage notify on this montage, for attacks whose GA exposes no
	// action data.
	const FAnimNotifyEvent* SelfEvent = EventReference.GetNotify();
	const float WindowStartTime = SelfEvent ? SelfEvent->GetTriggerTime() : 0.f;
	if (SelfEvent)
	{
		if (const UAN_MeleeDamage* DamageNotify = Telegraph_FindDamageNotifyFrom(Animation, WindowStartTime))
		{
			FActionData ActionData = DamageNotify->BuildActionData();
			if (const AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner()))
			{
				ActionData.ActRange = Character->ResolveEffectiveAttackRange(ActionData.ActRange);
			}

			if (Telegraph_ResolveShapeFromActionData(ActionData, Shape))
			{
				Telegraph_LogResolvedShape(TEXT("AN_MeleeDamage fallback"), Shape);
				return Shape;
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("ANS Pre-Attack Telegraph on %s could not resolve a hitbox; using default shape. "
			 "Ability: %s. Window opens at t=%.3f. Melee damage notifies on this animation: %s"),
		Animation ? *Animation->GetName() : TEXT("<null anim>"),
		*AbilityDiagnostic,
		WindowStartTime,
		*Telegraph_DescribeDamageNotifies(Animation));
	return Shape;
}

TSubclassOf<AYogTelegraphZoneActor> UYogAnimNotifyState_Telegraph::ResolveZoneClass(EHitBoxType ShapeType) const
{
	if (const TSubclassOf<AYogTelegraphZoneActor>* Found = ZoneClassByShape.Find(ShapeType))
	{
		if (*Found)
		{
			return *Found;
		}
	}

	// A triangle hitbox is an annulus with no inner cull, so it reuses the fan zone.
	if (ShapeType == EHitBoxType::Triangle)
	{
		if (const TSubclassOf<AYogTelegraphZoneActor>* Fan = ZoneClassByShape.Find(EHitBoxType::Annulus))
		{
			if (*Fan)
			{
				return *Fan;
			}
		}
	}

	return TelegraphClass;
}

void UYogAnimNotifyState_Telegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();
	// Skip animation-editor preview worlds so scrubbing a montage does not spawn stray actors.
	if (!Owner || !World || !World->IsGameWorld())
	{
		return;
	}

	const FYogTelegraphShape Shape = ResolveShape(MeshComp, Animation, EventReference);
	const TSubclassOf<AYogTelegraphZoneActor> ZoneClass = ResolveZoneClass(Shape.ShapeType);
	if (!ZoneClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ANS Pre-Attack Telegraph on %s resolved shape %s but has no zone class for it. "
				 "Add it to ZoneClassByShape, or set TelegraphClass as the fallback."),
			Animation ? *Animation->GetName() : TEXT("<null anim>"),
			*UEnum::GetValueAsString(Shape.ShapeType));
		return;
	}

	// Drop to the feet: a character's actor location is the capsule centre, and this zone is a
	// ground plane.
	FVector LocalOffset = Shape.LocalOffset;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		if (const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			LocalOffset.Z -= Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FTransform& OwnerTransform = Owner->GetActorTransform();
	const FVector SpawnLocation = OwnerTransform.TransformPosition(LocalOffset);
	const FRotator SpawnRotation = OwnerTransform.Rotator();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AYogTelegraphZoneActor* Zone = World->SpawnActor<AYogTelegraphZoneActor>(
		ZoneClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Zone)
	{
		return;
	}

	// Follow the enemy through the windup (e.g. a lunge) while keeping the spawn offset/facing.
	Zone->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
	Zone->Show(Shape, Color);

	// TotalDuration is the window length in animation time, so the fill finishes exactly when the
	// window closes regardless of montage play rate (attack speed).
	Zone->StartProgress(TotalDuration);

	SpawnedByMesh.Add(TObjectKey<USkeletalMeshComponent>(MeshComp), Zone);
}

void UYogAnimNotifyState_Telegraph::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (TWeakObjectPtr<AYogTelegraphZoneActor>* Found = SpawnedByMesh.Find(TObjectKey<USkeletalMeshComponent>(MeshComp)))
	{
		if (AYogTelegraphZoneActor* Zone = Found->Get())
		{
			Zone->AdvanceProgress(FrameDeltaTime);
		}
	}
}

void UYogAnimNotifyState_Telegraph::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	const TObjectKey<USkeletalMeshComponent> Key(MeshComp);
	if (TWeakObjectPtr<AYogTelegraphZoneActor>* Found = SpawnedByMesh.Find(Key))
	{
		if (AYogTelegraphZoneActor* Zone = Found->Get())
		{
			Zone->Hide();
			Zone->Destroy();
		}
		SpawnedByMesh.Remove(Key);
	}
}
