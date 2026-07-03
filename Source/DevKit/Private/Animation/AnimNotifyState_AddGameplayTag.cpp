#include "Animation/AnimNotifyState_AddGameplayTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	void DestroyNiagaraForMesh(
		TMap<TObjectKey<USkeletalMeshComponent>, TWeakObjectPtr<UNiagaraComponent>>& ActiveComponents,
		USkeletalMeshComponent* MeshComp)
	{
		if (!MeshComp)
		{
			return;
		}

		const TObjectKey<USkeletalMeshComponent> MeshKey(MeshComp);
		TWeakObjectPtr<UNiagaraComponent>* Found = ActiveComponents.Find(MeshKey);
		UNiagaraComponent* NiagaraComp = Found ? Found->Get() : nullptr;
		if (NiagaraComp)
		{
			NiagaraComp->Deactivate();
			NiagaraComp->DestroyComponent();
		}

		ActiveComponents.Remove(MeshKey);
	}
}

UAbilitySystemComponent* UAnimNotifyState_AddGameplayTag::GetASC(const USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* Owner = MeshComp->GetOwner();
	return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
}

void UAnimNotifyState_AddGameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!Tags.IsEmpty())
	{
		if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
		{
			ASC->AddLooseGameplayTags(Tags);
		}
	}

	SpawnScreenNiagara(MeshComp);
	SpawnAttachedNiagara(MeshComp);
}

void UAnimNotifyState_AddGameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	DestroyScreenNiagara(MeshComp);
	DestroyAttachedNiagara(MeshComp);

	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!Tags.IsEmpty())
	{
		if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
		{
			ASC->RemoveLooseGameplayTags(Tags);
		}
	}
}

void UAnimNotifyState_AddGameplayTag::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (bFollowOwnerScreenPosition)
	{
		UpdateScreenNiagara(MeshComp);
	}
}

FString UAnimNotifyState_AddGameplayTag::GetNotifyName_Implementation() const
{
	const bool bHasVFX = ScreenNiagaraSystem || AttachedNiagaraSystem;
	if (Tags.IsEmpty())
	{
		if (ScreenNiagaraSystem && AttachedNiagaraSystem)
		{
			return TEXT("Tag Window: Screen+Attach VFX");
		}
		if (ScreenNiagaraSystem)
		{
			return TEXT("Tag Window: Screen VFX");
		}
		return AttachedNiagaraSystem ? TEXT("Tag Window: Attach VFX") : TEXT("Tag Window: (none)");
	}

	const TCHAR* VFXSuffix = bHasVFX ? TEXT(" + VFX") : TEXT("");
	if (Tags.Num() == 1)
	{
		return FString::Printf(TEXT("Tag Window: %s%s"), *Tags.First().ToString(), VFXSuffix);
	}

	return FString::Printf(TEXT("Tag Window: %s +%d%s"), *Tags.First().ToString(), Tags.Num() - 1, VFXSuffix);
}

bool UAnimNotifyState_AddGameplayTag::ResolveScreenNiagaraTransform(const USkeletalMeshComponent* MeshComp,
	FVector& OutLocation, FRotator& OutRotation) const
{
	const AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!Owner || !World)
	{
		return false;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	FVector2D ScreenPosition;
	const FVector ProjectWorldPosition = Owner->GetActorLocation() + ScreenProjectionWorldOffset;
	if (!PC->ProjectWorldLocationToScreen(ProjectWorldPosition, ScreenPosition, false))
	{
		return false;
	}
	ScreenPosition += ScreenOffset;

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldOrigin, WorldDirection))
	{
		return false;
	}

	const float SafeDistance = FMath::Max(ScreenNiagaraDistance, 1.f);
	OutLocation = WorldOrigin + WorldDirection * SafeDistance;
	OutRotation = PC->PlayerCameraManager->GetCameraRotation();
	return true;
}

void UAnimNotifyState_AddGameplayTag::SpawnScreenNiagara(USkeletalMeshComponent* MeshComp)
{
	if (!ScreenNiagaraSystem || !MeshComp)
	{
		return;
	}

	DestroyScreenNiagara(MeshComp);

	FVector SpawnLocation;
	FRotator SpawnRotation;
	if (!ResolveScreenNiagaraTransform(MeshComp, SpawnLocation, SpawnRotation))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		ScreenNiagaraSystem.Get(),
		SpawnLocation,
		SpawnRotation,
		ScreenNiagaraScale,
		false,
		false);
	if (!NiagaraComp)
	{
		return;
	}

	NiagaraComp->SetAutoDestroy(false);
	NiagaraComp->Activate(true);
	ActiveScreenNiagaraComponents.Add(TObjectKey<USkeletalMeshComponent>(MeshComp), NiagaraComp);
}

void UAnimNotifyState_AddGameplayTag::UpdateScreenNiagara(USkeletalMeshComponent* MeshComp)
{
	TWeakObjectPtr<UNiagaraComponent>* Found = MeshComp
		? ActiveScreenNiagaraComponents.Find(TObjectKey<USkeletalMeshComponent>(MeshComp))
		: nullptr;
	UNiagaraComponent* NiagaraComp = Found ? Found->Get() : nullptr;
	if (!NiagaraComp)
	{
		return;
	}

	FVector NewLocation;
	FRotator NewRotation;
	if (ResolveScreenNiagaraTransform(MeshComp, NewLocation, NewRotation))
	{
		NiagaraComp->SetWorldLocationAndRotation(NewLocation, NewRotation);
	}
}

void UAnimNotifyState_AddGameplayTag::DestroyScreenNiagara(USkeletalMeshComponent* MeshComp)
{
	DestroyNiagaraForMesh(ActiveScreenNiagaraComponents, MeshComp);
}

void UAnimNotifyState_AddGameplayTag::SpawnAttachedNiagara(USkeletalMeshComponent* MeshComp)
{
	if (!AttachedNiagaraSystem || !MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	DestroyAttachedNiagara(MeshComp);

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		AttachedNiagaraSystem.Get(),
		MeshComp,
		AttachedNiagaraSocketName,
		AttachedNiagaraLocationOffset,
		AttachedNiagaraRotationOffset,
		EAttachLocation::KeepRelativeOffset,
		false,
		false);
	if (!NiagaraComp)
	{
		return;
	}

	NiagaraComp->SetRelativeScale3D(AttachedNiagaraScale);
	NiagaraComp->SetAutoDestroy(false);
	NiagaraComp->Activate(true);
	ActiveAttachedNiagaraComponents.Add(TObjectKey<USkeletalMeshComponent>(MeshComp), NiagaraComp);
}

void UAnimNotifyState_AddGameplayTag::DestroyAttachedNiagara(USkeletalMeshComponent* MeshComp)
{
	DestroyNiagaraForMesh(ActiveAttachedNiagaraComponents, MeshComp);
}
