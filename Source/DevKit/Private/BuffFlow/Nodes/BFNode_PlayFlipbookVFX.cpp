#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"

#include "BuffFlow/Actors/Rune512FlipbookVFXActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/YogCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
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

void UBFNode_PlayFlipbookVFX::ExecuteBuffFlowInput(const FName& PinName)
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

	if (bProjectToVisibleSurface)
	{
		SpawnLocation = ProjectToVisibleSurface(TargetActor, AttachComp, SpawnLocation);
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
		Lifetime,
		bLoop,
		Size,
		bFaceCamera,
		EmissiveColor,
		AlphaScale);

	if (bDestroyWithFlow)
	{
		ActiveActors.Add(VFXActor);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayFlipbookVFX] Spawned Effect=%s Texture=%s Target=%s Socket=%s Surface=%d Size=%.1f Duration=%.2f Lifetime=%.2f Loop=%d"),
		*EffectName.ToString(),
		*GetNameSafe(Texture),
		*GetNameSafe(TargetActor),
		*ResolvedSocket.ToString(),
		bProjectToVisibleSurface ? 1 : 0,
		Size,
		Duration,
		Lifetime > 0.f ? Lifetime : Duration,
		bLoop ? 1 : 0);

	TriggerOutput(TEXT("Out"), true);
}

FVector UBFNode_PlayFlipbookVFX::ProjectToVisibleSurface(
	AActor* TargetActor,
	USceneComponent* AttachComp,
	const FVector& BaseLocation) const
{
	if (!TargetActor || !TargetActor->GetWorld())
	{
		return BaseLocation;
	}

	const APlayerController* PC = TargetActor->GetWorld()->GetFirstPlayerController();
	const APlayerCameraManager* CameraManager = PC ? PC->PlayerCameraManager : nullptr;
	const FVector CameraLocation = CameraManager
		? CameraManager->GetCameraLocation()
		: BaseLocation - TargetActor->GetActorForwardVector() * 500.f;

	const FVector ToCamera = CameraLocation - BaseLocation;
	if (ToCamera.IsNearlyZero())
	{
		return BaseLocation;
	}

	const FVector SurfaceNormal = ToCamera.GetSafeNormal();
	const FVector TraceEnd = BaseLocation - SurfaceNormal * FMath::Max(1.f, SurfaceTraceExtraDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(Rune512FlipbookSurfaceTrace), false);
	if (AActor* OwnerActor = GetBuffOwner())
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}
	if (AActor* GiverActor = ResolveTarget(EBFTargetSelector::BuffGiver))
	{
		QueryParams.AddIgnoredActor(GiverActor);
	}

	FHitResult Hit;
	if (TargetActor->GetWorld()->LineTraceSingleByChannel(
		Hit,
		CameraLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams))
	{
		AActor* HitActor = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		const AActor* HitOwner = HitComponent ? HitComponent->GetOwner() : nullptr;
		if (HitActor == TargetActor || HitOwner == TargetActor)
		{
			return Hit.ImpactPoint + Hit.ImpactNormal.GetSafeNormal() * SurfaceOffset;
		}
	}

	USceneComponent* BoundsComp = AttachComp ? AttachComp : TargetActor->GetRootComponent();
	if (!BoundsComp)
	{
		return BaseLocation + SurfaceNormal * SurfaceOffset;
	}

	const FBoxSphereBounds Bounds = BoundsComp->Bounds;
	const FVector Extent = Bounds.BoxExtent;
	const float ProjectedRadius =
		FMath::Abs(SurfaceNormal.X) * Extent.X
		+ FMath::Abs(SurfaceNormal.Y) * Extent.Y
		+ FMath::Abs(SurfaceNormal.Z) * Extent.Z;
	const float FallbackDistance = FMath::Clamp(
		ProjectedRadius * FMath::Max(0.f, SurfaceFallbackRadiusScale),
		0.f,
		ProjectedRadius + SurfaceOffset);

	return BaseLocation + SurfaceNormal * (FallbackDistance + SurfaceOffset);
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
