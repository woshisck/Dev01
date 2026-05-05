#include "Actors/SacrificeShadowEchoActor.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

ASacrificeShadowEchoActor::ASacrificeShadowEchoActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
	ShadowMesh->SetupAttachment(SceneRoot);
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMesh->SetGenerateOverlapEvents(false);
	ShadowMesh->SetCastShadow(false);
	SetActorEnableCollision(false);
}

void ASacrificeShadowEchoActor::InitializeFromPlayer(APlayerCharacterBase* Player, int32 InAttackCharges, float InLifetime)
{
	RemainingAttackCharges = FMath::Max(0, InAttackCharges);

	if (Player && Player->GetMesh() && ShadowMesh)
	{
		USkeletalMeshComponent* PlayerMesh = Player->GetMesh();
		ShadowMesh->SetRelativeTransform(PlayerMesh->GetRelativeTransform());
		ShadowMesh->SetSkeletalMesh(PlayerMesh->GetSkeletalMeshAsset());
		ShadowMesh->SetLeaderPoseComponent(PlayerMesh);
		ShadowMesh->SetVisibility(true, true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimer);
		if (InLifetime > KINDA_SMALL_NUMBER)
		{
			World->GetTimerManager().SetTimer(
				LifetimeTimer,
				this,
				&ASacrificeShadowEchoActor::Expire,
				InLifetime,
				false);
		}
	}
}

bool ASacrificeShadowEchoActor::ConsumeAttackCharge()
{
	if (RemainingAttackCharges <= 0)
	{
		return false;
	}

	--RemainingAttackCharges;
	if (RemainingAttackCharges <= 0)
	{
		Destroy();
	}
	return true;
}

void ASacrificeShadowEchoActor::Expire()
{
	Destroy();
}
