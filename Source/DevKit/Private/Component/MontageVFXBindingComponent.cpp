#include "Component/MontageVFXBindingComponent.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

UMontageVFXBindingComponent::UMontageVFXBindingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMontageVFXBindingComponent::RegisterBinding(FName SlotName, const FMontageVFXBindingConfig& Config)
{
	if (SlotName.IsNone())
	{
		return;
	}
	PendingBindings.Add(SlotName, Config);
}

void UMontageVFXBindingComponent::ActivateSlot(FName SlotName)
{
	const FMontageVFXBindingConfig* Config = PendingBindings.Find(SlotName);
	if (!Config)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MontageVFXBinding] ActivateSlot: no pending binding for slot=%s"), *SlotName.ToString());
		return;
	}

	if (FMontageVFXActiveState* Stale = ActiveStates.Find(SlotName))
	{
		TearDownActiveState(*Stale);
		ActiveStates.Remove(SlotName);
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FMontageVFXActiveState State;

	// ── Niagara ──────────────────────────────────────────────────────────────
	if (Config->NiagaraSystem)
	{
		FName ResolvedSocket = NAME_None;
		USceneComponent* AttachComp = ResolveAttachTarget(*Config, ResolvedSocket);

		UNiagaraComponent* NiagaraComp = nullptr;
		if (AttachComp)
		{
			NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Config->NiagaraSystem,
				AttachComp,
				ResolvedSocket,
				Config->LocationOffset,
				Config->RotationOffset,
				EAttachLocation::KeepRelativeOffset,
				false);

			if (NiagaraComp)
			{
				NiagaraComp->SetRelativeScale3D(Config->Scale);
			}
		}
		else if (Config->AttachTarget != EGCNAttachedNiagaraAttachTarget::EquippedWeapon
			|| Config->bFallbackToTargetActorIfWeaponMissing)
		{
			UWorld* World = Owner->GetWorld();
			if (World)
			{
				const FVector SpawnLoc = Owner->GetActorTransform().TransformPosition(Config->LocationOffset);
				const FRotator SpawnRot = (Owner->GetActorRotation() + Config->RotationOffset).GetNormalized();
				NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World, Config->NiagaraSystem, SpawnLoc, SpawnRot, Config->Scale, false);
			}
		}

		if (NiagaraComp)
		{
			NiagaraComp->SetAutoDestroy(false);
			ApplyNiagaraParameterOverrides(NiagaraComp, Config->NiagaraParameterOverrides);
			State.NiagaraComp = NiagaraComp;
		}
	}

	// ── Sound ────────────────────────────────────────────────────────────────
	if (Config->Sound)
	{
		UGameplayStatics::SpawnSoundAttached(Config->Sound, Owner->GetRootComponent());
	}

	// ── Weapon material override ──────────────────────────────────────────────
	if (Config->WeaponMaterialOverride)
	{
		AWeaponInstance* Weapon = ResolveEquippedWeapon();
		if (Weapon)
		{
			TArray<UMeshComponent*> Meshes;
			Weapon->GetComponents<UMeshComponent>(Meshes);
			if (Meshes.IsValidIndex(0) && Meshes[0])
			{
				UMeshComponent* WeaponMesh = Meshes[0];
				const int32 Slot = Config->WeaponMaterialSlot;

				State.WeaponMesh = WeaponMesh;
				State.MaterialSlot = Slot;
				State.OriginalMaterial = WeaponMesh->GetMaterial(Slot);

				UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Config->WeaponMaterialOverride, this);
				if (DynMat)
				{
					ApplyMaterialParams(DynMat, Config->WeaponMaterialParameterOverrides);
					WeaponMesh->SetMaterial(Slot, DynMat);
					State.ActiveDynamicMaterial = DynMat;
				}
			}
		}
	}

	ActiveStates.Add(SlotName, MoveTemp(State));

	UE_LOG(LogTemp, Log, TEXT("[MontageVFXBinding] ActivateSlot slot=%s niagara=%s sound=%s mat=%s"),
		*SlotName.ToString(),
		*GetNameSafe(Config->NiagaraSystem.Get()),
		*GetNameSafe(Config->Sound.Get()),
		*GetNameSafe(Config->WeaponMaterialOverride.Get()));
}

void UMontageVFXBindingComponent::DeactivateSlot(FName SlotName)
{
	FMontageVFXActiveState* State = ActiveStates.Find(SlotName);
	if (!State)
	{
		return;
	}
	TearDownActiveState(*State);
	ActiveStates.Remove(SlotName);
	PendingBindings.Remove(SlotName);
}

void UMontageVFXBindingComponent::ClearAllBindings()
{
	for (auto& Pair : ActiveStates)
	{
		TearDownActiveState(Pair.Value);
	}
	ActiveStates.Reset();
	PendingBindings.Reset();
}

void UMontageVFXBindingComponent::TearDownActiveState(FMontageVFXActiveState& State)
{
	if (State.NiagaraComp)
	{
		State.NiagaraComp->Deactivate();
		State.NiagaraComp->DestroyComponent();
		State.NiagaraComp = nullptr;
	}

	if (State.WeaponMesh && State.ActiveDynamicMaterial)
	{
		State.WeaponMesh->SetMaterial(State.MaterialSlot, State.OriginalMaterial);
	}
	State.WeaponMesh = nullptr;
	State.OriginalMaterial = nullptr;
	State.ActiveDynamicMaterial = nullptr;
}

USceneComponent* UMontageVFXBindingComponent::ResolveAttachTarget(const FMontageVFXBindingConfig& Config, FName& OutSocket) const
{
	OutSocket = NAME_None;

	if (Config.AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon)
	{
		USceneComponent* WeaponComp = ResolveWeaponAttachComponent(Config, OutSocket);
		if (WeaponComp)
		{
			return WeaponComp;
		}
		if (!Config.bFallbackToTargetActorIfWeaponMissing)
		{
			return nullptr;
		}
	}

	return ResolveOwnerAttachComponent(Config, OutSocket);
}

USceneComponent* UMontageVFXBindingComponent::ResolveWeaponAttachComponent(const FMontageVFXBindingConfig& Config, FName& OutSocket) const
{
	OutSocket = NAME_None;

	AWeaponInstance* Weapon = ResolveEquippedWeapon();
	if (!Weapon)
	{
		return nullptr;
	}

	// Named scene component lookup (e.g. a dedicated VFX attach point component on the weapon)
	if (USceneComponent* Named = FindNamedSceneComponent(Weapon, Config.WeaponAttachSocketName))
	{
		return Named;
	}
	for (const FName& FallbackName : Config.WeaponAttachSocketFallbackNames)
	{
		if (USceneComponent* Named = FindNamedSceneComponent(Weapon, FallbackName))
		{
			return Named;
		}
	}

	// Mesh socket/bone lookup
	TArray<UMeshComponent*> Meshes;
	Weapon->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh)
		{
			continue;
		}
		if (!TryResolveSocketOrBone(Mesh, Config.WeaponAttachSocketName, OutSocket))
		{
			for (const FName& Fallback : Config.WeaponAttachSocketFallbackNames)
			{
				if (TryResolveSocketOrBone(Mesh, Fallback, OutSocket))
				{
					break;
				}
			}
		}
		if (OutSocket != NAME_None)
		{
			return Mesh;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[MontageVFXBinding] ResolveWeaponAttachComponent: no socket/bone match on weapon=%s socketName=%s, attaching to root"),
		*GetNameSafe(Weapon), *Config.WeaponAttachSocketName.ToString());

	return Weapon->GetRootComponent();
}

bool UMontageVFXBindingComponent::TryResolveSocketOrBone(UMeshComponent* Mesh, FName SocketOrBoneName, FName& OutSocket)
{
	if (!Mesh || SocketOrBoneName == NAME_None)
	{
		return false;
	}

	if (USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Mesh))
	{
		if (Skel->DoesSocketExist(SocketOrBoneName) || Skel->GetBoneIndex(SocketOrBoneName) != INDEX_NONE)
		{
			OutSocket = SocketOrBoneName;
			return true;
		}
	}
	else if (Mesh->DoesSocketExist(SocketOrBoneName))
	{
		OutSocket = SocketOrBoneName;
		return true;
	}

	return false;
}

USceneComponent* UMontageVFXBindingComponent::FindNamedSceneComponent(AActor* OwnerActor, FName ComponentName)
{
	if (!OwnerActor || ComponentName == NAME_None)
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	OwnerActor->GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* Comp : SceneComponents)
	{
		if (Comp && Comp->GetFName() == ComponentName)
		{
			return Comp;
		}
	}

	return nullptr;
}

USceneComponent* UMontageVFXBindingComponent::ResolveOwnerAttachComponent(const FMontageVFXBindingConfig& Config, FName& OutSocket) const
{
	OutSocket = NAME_None;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (Config.bAttachToSkeletalMesh)
	{
		if (USkeletalMeshComponent* SkelMesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
		{
			auto HasSocketOrBone = [SkelMesh](const FName& Name) -> bool
			{
				return Name != NAME_None
					&& (SkelMesh->DoesSocketExist(Name) || SkelMesh->GetBoneIndex(Name) != INDEX_NONE);
			};

			if (HasSocketOrBone(Config.AttachSocketName))
			{
				OutSocket = Config.AttachSocketName;
			}
			else
			{
				for (const FName& Fallback : Config.AttachSocketFallbackNames)
				{
					if (HasSocketOrBone(Fallback))
					{
						OutSocket = Fallback;
						break;
					}
				}
			}
			return SkelMesh;
		}
	}

	return Owner->GetRootComponent();
}

AWeaponInstance* UMontageVFXBindingComponent::ResolveEquippedWeapon() const
{
	const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwner());
	return Player ? Player->EquippedWeaponInstance : nullptr;
}

void UMontageVFXBindingComponent::ApplyNiagaraParameterOverrides(UNiagaraComponent* Comp, const TArray<FGCNNiagaraParamOverride>& Overrides) const
{
	if (!Comp)
	{
		return;
	}
	for (const FGCNNiagaraParamOverride& Override : Overrides)
	{
		if (Override.ParameterName.IsNone())
		{
			continue;
		}
		switch (Override.ParamType)
		{
		case EGCNNiagaraParamType::Float:
			Comp->SetVariableFloat(Override.ParameterName, Override.FloatValue);
			break;
		case EGCNNiagaraParamType::Vector:
			Comp->SetVariableVec3(Override.ParameterName, Override.VectorValue);
			break;
		case EGCNNiagaraParamType::Color:
			Comp->SetVariableLinearColor(Override.ParameterName, Override.ColorValue);
			break;
		case EGCNNiagaraParamType::Bool:
			Comp->SetVariableBool(Override.ParameterName, Override.bBoolValue);
			break;
		case EGCNNiagaraParamType::Int:
			Comp->SetVariableInt(Override.ParameterName, Override.IntValue);
			break;
		case EGCNNiagaraParamType::Material:
			Comp->SetVariableMaterial(Override.ParameterName, Override.MaterialValue.Get());
			break;
		}
	}
}

void UMontageVFXBindingComponent::ApplyMaterialParams(UMaterialInstanceDynamic* DynMat, const TArray<FGCNMaterialParamOverride>& Params) const
{
	if (!DynMat)
	{
		return;
	}
	for (const FGCNMaterialParamOverride& Param : Params)
	{
		if (Param.ParameterName.IsNone())
		{
			continue;
		}
		switch (Param.ParamType)
		{
		case EGCNMaterialParamType::Scalar:
			DynMat->SetScalarParameterValue(Param.ParameterName, Param.ScalarValue);
			break;
		case EGCNMaterialParamType::Vector:
			DynMat->SetVectorParameterValue(Param.ParameterName, Param.VectorValue);
			break;
		}
	}
}
