#include "AbilitySystem/Abilities/GA_EnemyScriptedSkill.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EnemySkill.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static const FName EnemySkillEvent_MontageEnded(TEXT("MontageEnded"));
static const FName EnemySkillEvent_MontageInterrupted(TEXT("MontageInterrupted"));
static const FName EnemySkillEvent_MoveFinished(TEXT("MoveFinished"));
static const FName EnemySkillEvent_HitPlayer(TEXT("HitPlayer"));
static const FName EnemySkillEvent_HitWall(TEXT("HitWall"));
static const FName EnemySkillEvent_GrabPlayer(TEXT("GrabPlayer"));

// The contact sweep runs on a short timer rather than Tick: the GA is not a ticking object and the
// capsule's own hit events only fire against blocking geometry, which pawns usually are not.
static constexpr float EnemySkill_ContactCheckInterval = 0.02f;

// Floors and slopes report blocking hits too; only roughly vertical surfaces count as walls.
static constexpr float EnemySkill_MaxWallNormalZ = 0.3f;

static FGameplayTag EnemyScriptedSkill_DeadTag()
{
	static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Dead")));
	return Tag;
}

static FGameplayTag EnemyScriptedSkill_StunnedTag()
{
	static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Status.Stunned")));
	return Tag;
}

UGA_EnemyScriptedSkill::UGA_EnemyScriptedSkill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(EnemyScriptedSkill_DeadTag());
}

TSubclassOf<UGA_EnemyScriptedSkill> UGA_EnemyScriptedSkill::GetClassForSlot(const FGameplayTag& SlotTag)
{
	const TSubclassOf<UGA_EnemyScriptedSkill> SlotClasses[] =
	{
		UGA_EnemyScriptedSkill1::StaticClass(),
		UGA_EnemyScriptedSkill2::StaticClass(),
		UGA_EnemyScriptedSkill3::StaticClass(),
		UGA_EnemyScriptedSkill4::StaticClass(),
	};

	for (const TSubclassOf<UGA_EnemyScriptedSkill>& SlotClass : SlotClasses)
	{
		if (SlotClass->GetDefaultObject<UGA_EnemyScriptedSkill>()->AbilityTags.HasTagExact(SlotTag))
		{
			return SlotClass;
		}
	}
	return nullptr;
}

void UGA_EnemyScriptedSkill::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	SkillAsset = Cast<UEnemySkill>(GetCurrentSourceObject());
	if (!SkillAsset || !GetSkillOwner() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyScriptedSkill] %s on %s has no skill asset or failed to commit."),
			*GetName(), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// The skill steers the pawn itself; a live move request or focus would fight it every frame.
	if (AAIController* AIC = Cast<AAIController>(GetSkillOwner()->GetController()))
	{
		SavedFocusActor = AIC->GetFocusActor();
		AIC->StopMovement();
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}

	SkillAsset->OnActivate(this);
}

void UGA_EnemyScriptedSkill::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	StopChargeMove();

	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : DelayedEventTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
	DelayedEventTimers.Reset();

	// Only unbound, not ended: Super::EndAbility destroys the task as an ability end, which is what makes
	// bStopWhenAbilityEnds stop a looping ChargeLoop. EndTask() here would leave it playing forever.
	if (MontageTask)
	{
		MontageTask->OnBlendOut.RemoveAll(this);
		MontageTask->OnCompleted.RemoveAll(this);
		MontageTask->OnInterrupted.RemoveAll(this);
		MontageTask->OnCancelled.RemoveAll(this);
		MontageTask = nullptr;
	}

	if (AEnemyCharacterBase* Owner = GetSkillOwner())
	{
		if (AAIController* AIC = Cast<AAIController>(Owner->GetController()))
		{
			if (SavedFocusActor.IsValid() && !AIC->GetFocusActor())
			{
				AIC->SetFocus(SavedFocusActor.Get());
			}
		}
	}
	SavedFocusActor.Reset();

	// Cleared before OnEnd so a script that calls EndSkill from OnEnd re-enters as a no-op.
	const UEnemySkill* EndingAsset = SkillAsset;
	SkillAsset = nullptr;
	if (EndingAsset)
	{
		EndingAsset->OnEnd(this, bWasCancelled);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AEnemyCharacterBase* UGA_EnemyScriptedSkill::GetSkillOwner() const
{
	return Cast<AEnemyCharacterBase>(GetAvatarActorFromActorInfo());
}

AActor* UGA_EnemyScriptedSkill::GetSkillTarget() const
{
	if (SavedFocusActor.IsValid())
	{
		return SavedFocusActor.Get();
	}
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar ? UGameplayStatics::GetPlayerPawn(Avatar, 0) : nullptr;
}

bool UGA_EnemyScriptedSkill::PlaySkillMontage(FName Section)
{
	if (!SkillAsset || !SkillAsset->Montage)
	{
		return false;
	}

	ReleaseMontageTask();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, SkillAsset->Montage, 1.f, Section, true);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyScriptedSkill::OnMontageBlendOut);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyScriptedSkill::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyScriptedSkill::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyScriptedSkill::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
	return true;
}

void UGA_EnemyScriptedSkill::JumpToSection(FName Section)
{
	if (GetCurrentMontage())
	{
		MontageJumpToSection(Section);
	}
}

void UGA_EnemyScriptedSkill::FaceTarget()
{
	AEnemyCharacterBase* Owner = GetSkillOwner();
	const AActor* Target = GetSkillTarget();
	if (!Owner || !Target)
	{
		return;
	}

	const FVector ToTarget = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator FaceRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
	Owner->SetActorRotation(FaceRotation);
	if (AController* Controller = Owner->GetController())
	{
		Controller->SetControlRotation(FaceRotation);
	}
}

bool UGA_EnemyScriptedSkill::StartChargeMove(float Speed, float Distance, float WallHitDot, float HitPadding, bool bPinPlayer)
{
	AEnemyCharacterBase* Owner = GetSkillOwner();
	if (!Owner || Speed <= KINDA_SMALL_NUMBER || Distance <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyScriptedSkill] %s StartChargeMove rejected: Speed=%.0f Distance=%.0f (both must be > 0)."),
			*GetNameSafe(Owner), Speed, Distance);
		return false;
	}

	StopChargeMove();

	// Locked once here so the charge is dodgeable; it does not home in on the target afterwards.
	// const AActor* Target = GetSkillTarget();
	// ChargeDir = Target
	// 	? (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D()
	// 	: Owner->GetActorForwardVector().GetSafeNormal2D();
	ChargeDir = Owner->GetActorForwardVector().GetSafeNormal2D();
	if (ChargeDir.IsNearlyZero())
	{
		ChargeDir = Owner->GetActorForwardVector().GetSafeNormal2D();
	}
	//Owner->SetActorRotation(FRotator(0.f, ChargeDir.Rotation().Yaw, 0.f));

	ChargeWallHitDot = WallHitDot;
	ChargeHitPadding = FMath::Max(HitPadding, 0.f);
	bChargePinPlayer = bPinPlayer;

	// The target point lies past the player so a missed charge overshoots rather than stopping where the player was.
	const FVector TargetLocation = Owner->GetActorLocation() + ChargeDir * Distance;

	// Walking is forced as GA_Knockback does: a pawn left in MOVE_None would skip the root motion source.
	ChargeTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this,
		NAME_None,
		TargetLocation,
		Distance / Speed,
		true,
		EMovementMode::MOVE_Walking,
		true,
		nullptr,
		ERootMotionFinishVelocityMode::SetVelocity,
		FVector::ZeroVector,
		0.f);
	ChargeTask->OnTimedOut.AddDynamic(this, &UGA_EnemyScriptedSkill::OnChargeMoveFinished);
	ChargeTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_EnemyScriptedSkill::OnChargeMoveFinished);
	ChargeTask->ReadyForActivation();

	SetMontageRootMotionSuppressed(true);

	UE_LOG(LogTemp, Log, TEXT("[EnemyScriptedSkill] %s charge started: Dir=%s Speed=%.0f Distance=%.0f"),
		*Owner->GetName(), *ChargeDir.ToString(), Speed, Distance);

	if (UCapsuleComponent* Capsule = Owner->GetCapsuleComponent())
	{
		Capsule->OnComponentHit.AddDynamic(this, &UGA_EnemyScriptedSkill::OnOwnerCapsuleHit);
	}
	Owner->OnCharacterMovementUpdated.AddDynamic(this, &UGA_EnemyScriptedSkill::OnOwnerMovementUpdated);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ChargeContactTimer,
			FTimerDelegate::CreateUObject(this, &UGA_EnemyScriptedSkill::CheckChargePlayerContact),
			EnemySkill_ContactCheckInterval, true);
	}

	bChargeArmed = true;
	return true;
}

void UGA_EnemyScriptedSkill::StopChargeMove()
{
	DisarmChargeDetection();
	ReleasePin();
	SetMontageRootMotionSuppressed(false);

	if (ChargeTask)
	{
		ChargeTask->EndTask();
		ChargeTask = nullptr;
	}
}

FActiveGameplayEffectHandle UGA_EnemyScriptedSkill::ApplyEffectToActor(AActor* Target, TSubclassOf<UGameplayEffect> Effect, float Level)
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC || !Effect)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Effect, Level, EffectContext);
	return Spec.IsValid() ? SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC) : FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UGA_EnemyScriptedSkill::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> Effect, float Level)
{
	return ApplyEffectToActor(GetAvatarActorFromActorInfo(), Effect, Level);
}

void UGA_EnemyScriptedSkill::RaiseSkillEvent(FName EventName, AActor* Other)
{
	if (SkillAsset && IsActive())
	{
		UE_LOG(LogTemp, Log, TEXT("[EnemyScriptedSkill] %s event '%s' Other=%s"),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *EventName.ToString(), *GetNameSafe(Other));
		SkillAsset->OnEvent(this, EventName, Other);
	}
}

void UGA_EnemyScriptedSkill::RaiseSkillEventAfter(FName EventName, float Delay)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (Delay <= KINDA_SMALL_NUMBER)
	{
		RaiseSkillEvent(EventName, nullptr);
		return;
	}

	FTimerHandle& Handle = DelayedEventTimers.AddDefaulted_GetRef();
	World->GetTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &UGA_EnemyScriptedSkill::RaiseSkillEvent, EventName, static_cast<AActor*>(nullptr)),
		Delay, false);
}

float UGA_EnemyScriptedSkill::GetSectionLength(FName Section) const
{
	const UAnimMontage* Montage = SkillAsset ? SkillAsset->Montage.Get() : nullptr;
	if (!Montage)
	{
		return 0.f;
	}

	const int32 SectionIndex = Montage->GetSectionIndex(Section);
	return SectionIndex == INDEX_NONE ? 0.f : Montage->GetSectionLength(SectionIndex);
}

void UGA_EnemyScriptedSkill::EndSkill()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_EnemyScriptedSkill::OnMontageBlendOut()
{
	ReleaseMontageTask();
	RaiseSkillEvent(EnemySkillEvent_MontageEnded, nullptr);
}

void UGA_EnemyScriptedSkill::OnMontageInterrupted()
{
	ReleaseMontageTask();
	RaiseSkillEvent(EnemySkillEvent_MontageInterrupted, nullptr);
}

void UGA_EnemyScriptedSkill::OnChargeMoveFinished()
{
	if (!bChargeArmed)
	{
		return;
	}

	AActor* Carried = PinnedPlayer.Get();
	StopChargeMove();
	RaiseSkillEvent(EnemySkillEvent_MoveFinished, Carried);
}

void UGA_EnemyScriptedSkill::OnOwnerCapsuleHit(UPrimitiveComponent* /*HitComponent*/, AActor* OtherActor,
                                               UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/,
                                               const FHitResult& Hit)
{
	if (!bChargeArmed)
	{
		return;
	}

	// A player that blocks the capsule is still a player hit, not a wall.
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		if (bChargePinPlayer)
		{
			BeginPin(Player);
		}
		else
		{
			ResolveChargeHit(EnemySkillEvent_HitPlayer, Player);
		}
		return;
	}

	if (OtherActor && OtherActor->IsA<APawn>())
	{
		return;
	}

	const FVector Normal = Hit.ImpactNormal;
	if (FMath::Abs(Normal.Z) >= EnemySkill_MaxWallNormalZ)
	{
		return;
	}

	if (FVector::DotProduct(Normal.GetSafeNormal2D(), ChargeDir) < ChargeWallHitDot)
	{
		ResolveChargeHit(EnemySkillEvent_HitWall, OtherActor);
	}
}

void UGA_EnemyScriptedSkill::CheckChargePlayerContact()
{
	AEnemyCharacterBase* Owner = GetSkillOwner();
	UWorld* World = GetWorld();
	if (!bChargeArmed || !Owner || !World)
	{
		return;
	}

	// Pinned on every contact check so nothing outside the skill (StateTree face snaps, focus) can re-aim the charge.
	Owner->SetActorRotation(FRotator(0.f, ChargeDir.Rotation().Yaw, 0.f));

	if (PinnedPlayer.IsValid())
	{
		return;
	}

	const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	const float Radius = (Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f) + ChargeHitPadding;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemySkillChargeContact), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation() + ChargeDir * Radius * 0.5f,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Overlap.GetActor()))
		{
			if (bChargePinPlayer)
			{
				BeginPin(Player);
			}
			else
			{
				ResolveChargeHit(EnemySkillEvent_HitPlayer, Player);
			}
			return;
		}
	}
}

void UGA_EnemyScriptedSkill::ResolveChargeHit(FName EventName, AActor* Other)
{
	// Stopped before the event so the script sees a pawn that is no longer moving and cannot be re-hit.
	// A pinned player is reported as Other so the script can stun the one it was carrying.
	AActor* Carried = PinnedPlayer.Get();
	StopChargeMove();
	RaiseSkillEvent(EventName, Carried ? Carried : Other);
}

void UGA_EnemyScriptedSkill::BeginPin(APlayerCharacterBase* Player)
{
	AEnemyCharacterBase* Owner = GetSkillOwner();
	if (!Owner || !Player || PinnedPlayer.IsValid())
	{
		return;
	}

	const UCapsuleComponent* OwnerCapsule = Owner->GetCapsuleComponent();
	const UCapsuleComponent* PlayerCapsule = Player->GetCapsuleComponent();
	const float ContactDistance = (OwnerCapsule ? OwnerCapsule->GetScaledCapsuleRadius() : 40.f)
		+ (PlayerCapsule ? PlayerCapsule->GetScaledCapsuleRadius() : 40.f);

	// The player is pulled onto the charge line just ahead of the owner so the carry reads as being rammed.
	const FVector ToPlayer = Player->GetActorLocation() - Owner->GetActorLocation();
	PinDistance = FMath::Max(FVector::DotProduct(ToPlayer, ChargeDir), ContactDistance);

	// Each capsule ignores the other, or the player would block the root motion it is being carried by.
	Owner->MoveIgnoreActorAdd(Player);
	Player->MoveIgnoreActorAdd(Owner);

	// The stun tag makes the player lock its own movement and input. Loose, so it is held exactly as long as the
	// carry and composes with a GE_Stun landing on top: movement only comes back once both are gone.
	if (UAbilitySystemComponent* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player))
	{
		PlayerASC->AddLooseGameplayTag(EnemyScriptedSkill_StunnedTag());
	}

	PinnedPlayer = Player;
	RaiseSkillEvent(EnemySkillEvent_GrabPlayer, Player);
}

void UGA_EnemyScriptedSkill::ReleasePin()
{
	APlayerCharacterBase* Player = PinnedPlayer.Get();
	PinnedPlayer.Reset();
	if (!Player)
	{
		return;
	}

	if (AEnemyCharacterBase* Owner = GetSkillOwner())
	{
		Owner->MoveIgnoreActorRemove(Player);
		Player->MoveIgnoreActorRemove(Owner);
	}

	if (UAbilitySystemComponent* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player))
	{
		PlayerASC->RemoveLooseGameplayTag(EnemyScriptedSkill_StunnedTag());
	}
}

void UGA_EnemyScriptedSkill::OnOwnerMovementUpdated(float /*DeltaSeconds*/, FVector /*OldLocation*/, FVector /*OldVelocity*/)
{
	AEnemyCharacterBase* Owner = GetSkillOwner();
	APlayerCharacterBase* Player = PinnedPlayer.Get();
	if (!bChargeArmed || !Owner || !Player)
	{
		return;
	}

	FVector Target = Owner->GetActorLocation() + ChargeDir * PinDistance;
	Target.Z = Player->GetActorLocation().Z;

	FHitResult Hit;
	Player->SetActorLocation(Target, true, &Hit);
	if (!Hit.bBlockingHit)
	{
		return;
	}

	// Another pawn in the way is shoved through rather than ending the charge; anything static is the wall.
	if (Hit.GetActor() && Hit.GetActor()->IsA<APawn>())
	{
		Player->SetActorLocation(Target, false, nullptr, ETeleportType::None);
		return;
	}

	// Floors and ramps grazed by the sweep only shorten this frame's carry; the next frame catches up.
	if (FMath::Abs(Hit.ImpactNormal.Z) >= EnemySkill_MaxWallNormalZ)
	{
		return;
	}

	CrushPinnedPlayerIntoWall();
}

void UGA_EnemyScriptedSkill::CrushPinnedPlayerIntoWall()
{
	// The owner stops where it is: one pin distance behind the player, who is now flush against the wall.
	ResolveChargeHit(EnemySkillEvent_HitWall, PinnedPlayer.Get());
}

void UGA_EnemyScriptedSkill::DisarmChargeDetection()
{
	bChargeArmed = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeContactTimer);
	}

	if (AEnemyCharacterBase* Owner = GetSkillOwner())
	{
		if (UCapsuleComponent* Capsule = Owner->GetCapsuleComponent())
		{
			Capsule->OnComponentHit.RemoveDynamic(this, &UGA_EnemyScriptedSkill::OnOwnerCapsuleHit);
		}
		Owner->OnCharacterMovementUpdated.RemoveDynamic(this, &UGA_EnemyScriptedSkill::OnOwnerMovementUpdated);
	}
}

void UGA_EnemyScriptedSkill::SetMontageRootMotionSuppressed(bool bSuppressed)
{
	if (!bSuppressed)
	{
		UAnimInstance* AnimInstance = RootMotionDisabledAnimInstance.Get();
		if (AnimInstance && RootMotionDisabledMontage.IsValid())
		{
			if (FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(RootMotionDisabledMontage.Get()))
			{
				Instance->PopDisableRootMotion();
			}
		}
		RootMotionDisabledAnimInstance.Reset();
		RootMotionDisabledMontage.Reset();
		return;
	}

	if (RootMotionDisabledMontage.IsValid())
	{
		return;
	}

	// A montage counts as root motion if ANY of its clips has it (e.g. the Windup roar), and anim root motion
	// overrides root motion sources, so an in-place run section would pin the pawn for the whole charge.
	const AEnemyCharacterBase* Owner = GetSkillOwner();
	USkeletalMeshComponent* Mesh = Owner ? Owner->GetMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = GetCurrentMontage();
	FAnimMontageInstance* Instance = (AnimInstance && Montage) ? AnimInstance->GetActiveInstanceForMontage(Montage) : nullptr;
	if (!Instance)
	{
		return;
	}

	Instance->PushDisableRootMotion();
	RootMotionDisabledAnimInstance = AnimInstance;
	RootMotionDisabledMontage = Montage;
}

void UGA_EnemyScriptedSkill::ReleaseMontageTask()
{
	if (!MontageTask)
	{
		return;
	}

	// Unbound first so ending the task here does not feed back a MontageInterrupted event.
	MontageTask->OnBlendOut.RemoveAll(this);
	MontageTask->OnCompleted.RemoveAll(this);
	MontageTask->OnInterrupted.RemoveAll(this);
	MontageTask->OnCancelled.RemoveAll(this);
	MontageTask->EndTask();
	MontageTask = nullptr;
}

UGA_EnemyScriptedSkill1::UGA_EnemyScriptedSkill1()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Enemy.Skill.Skill1"))));
}

UGA_EnemyScriptedSkill2::UGA_EnemyScriptedSkill2()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Enemy.Skill.Skill2"))));
}

UGA_EnemyScriptedSkill3::UGA_EnemyScriptedSkill3()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Enemy.Skill.Skill3"))));
}

UGA_EnemyScriptedSkill4::UGA_EnemyScriptedSkill4()
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("Enemy.Skill.Skill4"))));
}
