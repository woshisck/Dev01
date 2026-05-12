#include "AbilitySystem/GameplayCue/GCN_AttachedNiagara.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Item/Weapon/WeaponInstance.h"
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
	if (!AttachComponent
		&& AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon
		&& !bFallbackToTargetActorIfWeaponMissing)
	{
		return nullptr;
	}

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

	if (AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon)
	{
		if (USceneComponent* WeaponAttachComponent = ResolveWeaponAttachComponent(Target, OutSocketName))
		{
			return WeaponAttachComponent;
		}

		if (!bFallbackToTargetActorIfWeaponMissing)
		{
			return nullptr;
		}
	}

	return ResolveTargetActorAttachComponent(Target, OutSocketName);
}

USceneComponent* AGCN_AttachedNiagara::ResolveTargetActorAttachComponent(AActor* Target, FName& OutSocketName) const
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
			if (!TryResolveSocketOrBone(MeshComponent, AttachSocketName, OutSocketName))
			{
				for (const FName& FallbackSocketName : AttachSocketFallbackNames)
				{
					if (TryResolveSocketOrBone(MeshComponent, FallbackSocketName, OutSocketName))
					{
						break;
					}
				}
			}

			return MeshComponent;
		}
	}

	return Target->GetRootComponent();
}

USceneComponent* AGCN_AttachedNiagara::ResolveWeaponAttachComponent(AActor* Target, FName& OutSocketName) const
{
	OutSocketName = NAME_None;

	AWeaponInstance* Weapon = ResolveEquippedWeapon(Target);
	if (!Weapon)
	{
		return nullptr;
	}

	if (USceneComponent* NamedComponent = FindNamedSceneComponent(Weapon, WeaponAttachSocketName))
	{
		return NamedComponent;
	}

	for (const FName& FallbackComponentName : WeaponAttachSocketFallbackNames)
	{
		if (USceneComponent* NamedComponent = FindNamedSceneComponent(Weapon, FallbackComponentName))
		{
			return NamedComponent;
		}
	}

	TArray<UMeshComponent*> MeshComponents;
	Weapon->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if (!TryResolveSocketOrBone(MeshComponent, WeaponAttachSocketName, OutSocketName))
		{
			for (const FName& FallbackSocketName : WeaponAttachSocketFallbackNames)
			{
				if (TryResolveSocketOrBone(MeshComponent, FallbackSocketName, OutSocketName))
				{
					break;
				}
			}
		}

		if (OutSocketName != NAME_None)
		{
			return MeshComponent;
		}
	}

	return Weapon->GetRootComponent();
}

USceneComponent* AGCN_AttachedNiagara::FindNamedSceneComponent(AActor* OwnerActor, const FName ComponentName) const
{
	if (!OwnerActor || ComponentName == NAME_None)
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	OwnerActor->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == ComponentName)
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

AWeaponInstance* AGCN_AttachedNiagara::ResolveEquippedWeapon(AActor* Target) const
{
	if (!Target)
	{
		return nullptr;
	}

	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Target))
	{
		return Player->EquippedWeaponInstance;
	}

	return nullptr;
}

bool AGCN_AttachedNiagara::TryResolveSocketOrBone(
	USceneComponent* Component,
	const FName SocketOrBoneName,
	FName& OutSocketName) const
{
	if (!Component || SocketOrBoneName == NAME_None)
	{
		return false;
	}

	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component))
	{
		if (SkeletalMeshComponent->DoesSocketExist(SocketOrBoneName) ||
			SkeletalMeshComponent->GetBoneIndex(SocketOrBoneName) != INDEX_NONE)
		{
			OutSocketName = SocketOrBoneName;
			return true;
		}
	}
	else if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Component))
	{
		if (MeshComponent->DoesSocketExist(SocketOrBoneName))
		{
			OutSocketName = SocketOrBoneName;
			return true;
		}
	}
	else if (Component->DoesSocketExist(SocketOrBoneName))
	{
		OutSocketName = SocketOrBoneName;
		return true;
	}

	return false;
}
