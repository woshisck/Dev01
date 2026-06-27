#include "Animation/AnimNotifyState_AddGameplayTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

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
}

void UAnimNotifyState_AddGameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	DestroyScreenNiagara(MeshComp);

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
	if (Tags.IsEmpty())
	{
		return ScreenNiagaraSystem ? TEXT("Tag Window: Screen VFX") : TEXT("Tag Window: (none)");
	}

	if (Tags.Num() == 1)
	{
		return FString::Printf(TEXT("Tag Window: %s"), *Tags.First().ToString());
	}

	return FString::Printf(TEXT("Tag Window: %s +%d"), *Tags.First().ToString(), Tags.Num() - 1);
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

	const TObjectKey<USkeletalMeshComponent> MeshKey(MeshComp);
	if (TWeakObjectPtr<UNiagaraComponent>* Existing = ActiveScreenNiagaraComponents.Find(MeshKey))
	{
		if (UNiagaraComponent* ExistingComp = Existing->Get())
		{
			ExistingComp->Deactivate();
			ExistingComp->DestroyComponent();
		}
		ActiveScreenNiagaraComponents.Remove(MeshKey);
	}

	FVector SpawnLocation;
	FRotator SpawnRotation;
	if (!ResolveScreenNiagaraTransform(MeshComp, SpawnLocation, SpawnRotation))
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World)
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
	ActiveScreenNiagaraComponents.Add(MeshKey, NiagaraComp);
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
	if (!MeshComp)
	{
		return;
	}

	const TObjectKey<USkeletalMeshComponent> MeshKey(MeshComp);
	TWeakObjectPtr<UNiagaraComponent>* Found = ActiveScreenNiagaraComponents.Find(MeshKey);
	UNiagaraComponent* NiagaraComp = Found ? Found->Get() : nullptr;
	if (NiagaraComp)
	{
		NiagaraComp->Deactivate();
		NiagaraComp->DestroyComponent();
	}

	ActiveScreenNiagaraComponents.Remove(MeshKey);
}
