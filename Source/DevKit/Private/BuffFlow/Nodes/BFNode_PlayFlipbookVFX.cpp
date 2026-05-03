#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"

#include "BuffFlow/Actors/Rune512FlipbookVFXActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UBFNode_PlayFlipbookVFX::UBFNode_PlayFlipbookVFX(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Visual");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PlayFlipbookVFX::ExecuteInput(const FName& PinName)
{
	if (!Texture || !Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayFlipbookVFX] Skip: Texture or Material is null Effect=%s Texture=%s Material=%s"),
			*EffectName.ToString(),
			*GetNameSafe(Texture),
			*GetNameSafe(Material));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor || !TargetActor->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayFlipbookVFX] Skip: target is null Effect=%s TargetSelector=%d"),
			*EffectName.ToString(),
			static_cast<int32>(Target));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	USceneComponent* AttachComp = TargetActor->GetRootComponent();
	FName ResolvedSocket = Socket;
	if (USkeletalMeshComponent* SkelMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		AttachComp = SkelMesh;
		auto HasSocketOrBone = [SkelMesh](const FName& CandidateName)
		{
			return CandidateName != NAME_None
				&& (SkelMesh->DoesSocketExist(CandidateName) || SkelMesh->GetBoneIndex(CandidateName) != INDEX_NONE);
		};

		if (!HasSocketOrBone(ResolvedSocket))
		{
			ResolvedSocket = NAME_None;
			for (const FName& FallbackName : SocketFallbackNames)
			{
				if (HasSocketOrBone(FallbackName))
				{
					ResolvedSocket = FallbackName;
					break;
				}
			}
		}
	}

	FVector SpawnLocation = TargetActor->GetActorLocation() + Offset;
	FRotator SpawnRotation = TargetActor->GetActorRotation();
	if (AttachComp)
	{
		const FTransform AttachTransform = ResolvedSocket != NAME_None
			? AttachComp->GetSocketTransform(ResolvedSocket)
			: AttachComp->GetComponentTransform();
		SpawnLocation = AttachTransform.TransformPosition(Offset);
		SpawnRotation = AttachTransform.GetRotation().Rotator();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = TargetActor;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARune512FlipbookVFXActor* VFXActor = TargetActor->GetWorld()->SpawnActor<ARune512FlipbookVFXActor>(
		ARune512FlipbookVFXActor::StaticClass(),
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (!VFXActor)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (AttachComp)
	{
		VFXActor->AttachToComponent(AttachComp, FAttachmentTransformRules::KeepWorldTransform, ResolvedSocket);
	}

	VFXActor->InitializeFlipbook(
		Texture,
		Material,
		PlaneMesh,
		Rows,
		Columns,
		Duration,
		Size,
		bFaceCamera,
		EmissiveColor,
		AlphaScale);

	if (bDestroyWithFlow)
	{
		ActiveActors.Add(VFXActor);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayFlipbookVFX] Spawned Effect=%s Texture=%s Target=%s Socket=%s Size=%.1f Duration=%.2f"),
		*EffectName.ToString(),
		*GetNameSafe(Texture),
		*GetNameSafe(TargetActor),
		*ResolvedSocket.ToString(),
		Size,
		Duration);

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_PlayFlipbookVFX::Cleanup()
{
	if (bDestroyWithFlow)
	{
		for (ARune512FlipbookVFXActor* Actor : ActiveActors)
		{
			if (Actor && !Actor->IsActorBeingDestroyed())
			{
				Actor->Destroy();
			}
		}
	}
	ActiveActors.Reset();
	Super::Cleanup();
}
