#include "Animation/YogAnimNotifyState_Telegraph.h"

#include "Actors/YogTelegraphZoneActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UYogAnimNotifyState_Telegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || !TelegraphClass)
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

	const FTransform& OwnerTransform = Owner->GetActorTransform();
	const FVector SpawnLocation = OwnerTransform.TransformPosition(Offset);
	const FRotator SpawnRotation = OwnerTransform.Rotator();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AYogTelegraphZoneActor* Zone = World->SpawnActor<AYogTelegraphZoneActor>(
		TelegraphClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!Zone)
	{
		return;
	}

	// Follow the enemy through the windup (e.g. a lunge) while keeping the spawn offset/facing.
	Zone->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
	Zone->Show(Radius, HalfAngle, Color);

	SpawnedByMesh.Add(TObjectKey<USkeletalMeshComponent>(MeshComp), Zone);
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

FString UYogAnimNotifyState_Telegraph::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Pre-Attack Telegraph | R:%.0f A:%.0f"), Radius, HalfAngle);
}
