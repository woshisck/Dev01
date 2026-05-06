#include "AbilitySystem/GameplayCue/GCN_AttachedNiagara.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AGCN_AttachedNiagara::AGCN_AttachedNiagara()
{
	bAutoDestroyOnRemove = true;
	bAllowMultipleOnActiveEvents = false;
	bAllowMultipleWhileActiveEvents = false;
}

bool AGCN_AttachedNiagara::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	Super::OnActive_Implementation(Target, Parameters);
	SpawnNiagara(Target, false);
	return true;
}

bool AGCN_AttachedNiagara::WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	Super::WhileActive_Implementation(Target, Parameters);
	SpawnNiagara(Target, false);
	return true;
}

bool AGCN_AttachedNiagara::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	Super::OnExecute_Implementation(Target, Parameters);
	if (bSpawnOnExecute)
	{
		SpawnNiagara(Target, true);
	}
	return true;
}

bool AGCN_AttachedNiagara::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters)
{
	StopNiagara();
	return Super::OnRemove_Implementation(Target, Parameters);
}

void AGCN_AttachedNiagara::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopNiagara();
	Super::EndPlay(EndPlayReason);
}

UNiagaraComponent* AGCN_AttachedNiagara::SpawnNiagara(AActor* Target, bool bAutoDestroy)
{
	if (ActiveNiagaraComponent)
	{
		return ActiveNiagaraComponent;
	}

	if (!NiagaraSystem || !Target)
	{
		return nullptr;
	}

	UWorld* World = Target->GetWorld();
	if (!World || (bSkipDedicatedServer && World->GetNetMode() == NM_DedicatedServer))
	{
		return nullptr;
	}

	FName ResolvedSocketName = NAME_None;
	USceneComponent* AttachComponent = ResolveAttachComponent(Target, ResolvedSocketName);
	UNiagaraComponent* SpawnedComponent = nullptr;

	if (AttachComponent)
	{
		SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			AttachComponent,
			ResolvedSocketName,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			bAutoDestroy,
			true,
			ENCPoolMethod::None,
			false);

		if (SpawnedComponent)
		{
			SpawnedComponent->SetRelativeScale3D(Scale);
		}
	}
	else
	{
		const FTransform TargetTransform = Target->GetActorTransform();
		SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			NiagaraSystem,
			TargetTransform.TransformPosition(LocationOffset),
			(Target->GetActorRotation() + RotationOffset).GetNormalized(),
			Scale,
			bAutoDestroy,
			true,
			ENCPoolMethod::None,
			false);
	}

	if (!bAutoDestroy)
	{
		ActiveNiagaraComponent = SpawnedComponent;
		if (ActiveNiagaraComponent)
		{
			ActiveNiagaraComponent->SetAutoDestroy(false);
		}
	}

	return SpawnedComponent;
}

void AGCN_AttachedNiagara::StopNiagara()
{
	if (!ActiveNiagaraComponent)
	{
		return;
	}

	ActiveNiagaraComponent->Deactivate();
	ActiveNiagaraComponent->DestroyComponent();
	ActiveNiagaraComponent = nullptr;
}

USceneComponent* AGCN_AttachedNiagara::ResolveAttachComponent(AActor* Target, FName& OutSocketName) const
{
	OutSocketName = NAME_None;
	if (!Target)
	{
		return nullptr;
	}

	if (bAttachToSkeletalMesh)
	{
		if (USkeletalMeshComponent* MeshComponent = Target->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (HasSocketOrBone(MeshComponent, AttachSocketName))
			{
				OutSocketName = AttachSocketName;
			}
			else
			{
				for (const FName& FallbackSocketName : AttachSocketFallbackNames)
				{
					if (HasSocketOrBone(MeshComponent, FallbackSocketName))
					{
						OutSocketName = FallbackSocketName;
						break;
					}
				}
			}

			return MeshComponent;
		}
	}

	return Target->GetRootComponent();
}

bool AGCN_AttachedNiagara::HasSocketOrBone(
	const USkeletalMeshComponent* MeshComponent,
	const FName SocketOrBoneName) const
{
	return MeshComponent
		&& SocketOrBoneName != NAME_None
		&& (MeshComponent->DoesSocketExist(SocketOrBoneName) ||
			MeshComponent->GetBoneIndex(SocketOrBoneName) != INDEX_NONE);
}
