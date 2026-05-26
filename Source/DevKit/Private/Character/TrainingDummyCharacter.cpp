#include "Character/TrainingDummyCharacter.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/YogCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameModes/YogGameMode.h"

ATrainingDummyCharacter::ATrainingDummyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCountsForLevelClear = false;
}

void ATrainingDummyCharacter::BeginPlay()
{
	// Run full enemy initialization (stats, abilities, AI, super armor, etc.)
	Super::BeginPlay();

	// Immediately undo the kill-count registration — dummy deaths must not trigger room completion.
	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->UnregisterEnemy(this);
	}
}

void ATrainingDummyCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	// Call grandparent directly: plays death animation and sends Action.Dead event,
	// but skips EnemyCharacterBase::Die() which would call UpdateFinishLevel + SetLifeSpan.
	AYogCharacterBase::Die();
}

void ATrainingDummyCharacter::FinishDying()
{
	if (!bResetOnDeath)
	{
		Super::FinishDying();
		return;
	}

	// Broadcast death so StoryEncounterRuntimeSubsystem can spawn loot pickups,
	// but skip Super::FinishDying() which would call Destroy().
	OnCharacterDied.Broadcast(this);
	OnCharacterDiedNative.Broadcast(this);

	GetWorldTimerManager().ClearTimer(ResetTimerHandle);
	GetWorldTimerManager().SetTimer(ResetTimerHandle, this, &ATrainingDummyCharacter::ResetDummy, ResetDelay, false);
}

void ATrainingDummyCharacter::ResetDummy()
{
	bIsDead = false;
	ResetDeathStartedBroadcast();

	// Restore capsule collision (disabled by YogCharacterBase::Die).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// Restore full health.
	if (BaseAttributeSet)
	{
		BaseAttributeSet->SetHealth(BaseAttributeSet->GetMaxHealth());
	}

	// Unfreeze mesh — GA_Dead's OnDeathMontageBlendOut sets bPauseAnims = true.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->bPauseAnims = false;
	}
}
