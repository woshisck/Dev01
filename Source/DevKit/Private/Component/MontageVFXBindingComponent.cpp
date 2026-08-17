#include "Component/MontageVFXBindingComponent.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/MontageVFXBindingDataAsset.h"
#include "Data/WhiffVFXData.h"
#include "Item/Weapon/WeaponDefinitionBase.h"
#include "Item/Weapon/WeaponInstance.h"
#include "System/YogSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

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

void UMontageVFXBindingComponent::ActivateSlot(FName SlotName, const FActionData* ActionData,
	float AnnulusPlaneRemainTime)
{
	if (SlotName.IsNone())
	{
		return;
	}

	if (FMontageVFXActiveSlot* Stale = ActiveStates.Find(SlotName))
	{
		TearDownActiveSlot(*Stale);
		ActiveStates.Remove(SlotName);
	}

	if (!GetOwner())
	{
		return;
	}

	// A BuffFlow registration overrides the base look outright, so a one-off rune can bypass
	// the table entirely. Everything below is the fallback chain when no rune claimed this slot.
	const FMontageVFXBindingConfig* BaseConfig = PendingBindings.Find(SlotName);

	FWhiffVFXResolved Resolved;
	if (!BaseConfig)
	{
		const UYogSettings* Settings = UYogSettings::Get();
		if (const UWhiffVFXData* WhiffTable = Settings ? Settings->WhiffVFXData.LoadSynchronous() : nullptr)
		{
			Resolved = WhiffTable->Resolve(SlotName, BuildWhiffTagContext());
			BaseConfig = Resolved.Base;
		}
	}

	if (!BaseConfig && DefaultBindingsAsset)
	{
		BaseConfig = DefaultBindingsAsset->ResolveBinding(SlotName);
	}

	// Lives until the end of this call, which is all the spawn below needs.
	FMontageVFXBindingConfig WeaponFallbackConfig;
	if (!BaseConfig && BuildWeaponFallbackConfig(WeaponFallbackConfig))
	{
		BaseConfig = &WeaponFallbackConfig;
	}

	FMontageVFXActiveSlot NewSlot;

	if (BaseConfig)
	{
		NewSlot.Layers.Add(SpawnLayer(*BaseConfig, ActionData, AnnulusPlaneRemainTime, /*bIsBaseLayer=*/true));
	}

	for (const FMontageVFXBindingConfig* AdditiveConfig : Resolved.Additive)
	{
		if (AdditiveConfig)
		{
			NewSlot.Layers.Add(SpawnLayer(*AdditiveConfig, ActionData, AnnulusPlaneRemainTime, /*bIsBaseLayer=*/false));
		}
	}

	if (Resolved.AdditiveOverflowCount > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MontageVFXBinding] ActivateSlot slot=%s dropped %d additive layer(s) over the cap"),
			*SlotName.ToString(), Resolved.AdditiveOverflowCount);
	}

	if (NewSlot.Layers.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MontageVFXBinding] ActivateSlot: nothing resolved for slot=%s (no BuffFlow binding, no WhiffVFXData row, no default, no weapon fallback)"),
			*SlotName.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[MontageVFXBinding] ActivateSlot slot=%s layers=%d (base=%s additive=%d) niagara=%s sound=%s mat=%s"),
		*SlotName.ToString(),
		NewSlot.Layers.Num(),
		BaseConfig ? TEXT("yes") : TEXT("none"),
		Resolved.Additive.Num(),
		*GetNameSafe(BaseConfig ? BaseConfig->NiagaraSystem.Get() : nullptr),
		*GetNameSafe(BaseConfig ? BaseConfig->Sound.Get() : nullptr),
		*GetNameSafe(BaseConfig ? BaseConfig->WeaponMaterialOverride.Get() : nullptr));

	ActiveStates.Add(SlotName, MoveTemp(NewSlot));
}

FGameplayTagContainer UMontageVFXBindingComponent::BuildWhiffTagContext() const
{
	FGameplayTagContainer Context;

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetOwner());
	if (!Character)
	{
		return Context;
	}

	if (const UWeaponDefinitionBase* WeaponDefinition = Character->GetEffectiveWeaponDefinition())
	{
		if (WeaponDefinition->WeaponIdentityTag.IsValid())
		{
			Context.AddTag(WeaponDefinition->WeaponIdentityTag);
		}
	}

	// Buff/status tags come straight off the ASC, so the look follows whatever is active right now.
	if (const UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		Context.AppendTags(OwnedTags);
	}

	return Context;
}

bool UMontageVFXBindingComponent::BuildWeaponFallbackConfig(FMontageVFXBindingConfig& OutConfig) const
{
	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetOwner());
	const UWeaponDefinitionBase* WeaponDefinition = Character ? Character->GetEffectiveWeaponDefinition() : nullptr;
	if (!WeaponDefinition)
	{
		return false;
	}

	if (!WeaponDefinition->WhiffVFX && !WeaponDefinition->WhiffSound)
	{
		return false;
	}

	OutConfig.NiagaraSystem = WeaponDefinition->WhiffVFX;
	OutConfig.Sound = WeaponDefinition->WhiffSound;
	OutConfig.SoundVolume = WeaponDefinition->WhiffSoundVolume;
	OutConfig.SoundPitch = WeaponDefinition->WhiffSoundPitch;
	OutConfig.AttachTarget = EGCNAttachedNiagaraAttachTarget::EquippedWeapon;
	OutConfig.bFallbackToTargetActorIfWeaponMissing = true;

	return true;
}

FMontageVFXActiveState UMontageVFXBindingComponent::SpawnLayer(const FMontageVFXBindingConfig& Config,
	const FActionData* ActionData, float AnnulusPlaneRemainTime, bool bIsBaseLayer)
{
	FMontageVFXActiveState State;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return State;
	}

	// ── Niagara ──────────────────────────────────────────────────────────────
	if (Config.NiagaraSystem)
	{
		FName ResolvedSocket = NAME_None;
		USceneComponent* AttachComp = ResolveAttachTarget(Config, ResolvedSocket);

		UNiagaraComponent* NiagaraComp = nullptr;
		if (AttachComp)
		{
			NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Config.NiagaraSystem,
				AttachComp,
				ResolvedSocket,
				Config.LocationOffset,
				Config.RotationOffset,
				EAttachLocation::KeepRelativeOffset,
				false);

			if (NiagaraComp)
			{
				NiagaraComp->SetRelativeScale3D(Config.Scale);
			}
		}
		else if (Config.AttachTarget != EGCNAttachedNiagaraAttachTarget::EquippedWeapon
			|| Config.bFallbackToTargetActorIfWeaponMissing)
		{
			UWorld* World = Owner->GetWorld();
			if (World)
			{
				const FVector SpawnLoc = Owner->GetActorTransform().TransformPosition(Config.LocationOffset);
				const FRotator SpawnRot = (Owner->GetActorRotation() + Config.RotationOffset).GetNormalized();
				NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					World, Config.NiagaraSystem, SpawnLoc, SpawnRot, Config.Scale, false);
			}
		}

		if (NiagaraComp)
		{
			NiagaraComp->SetAutoDestroy(false);
			ApplyNiagaraParameterOverrides(NiagaraComp, Config.NiagaraParameterOverrides);
			State.NiagaraComp = NiagaraComp;
		}
	}

	// ── Sound ────────────────────────────────────────────────────────────────
	if (Config.Sound)
	{
		UGameplayStatics::SpawnSoundAttached(Config.Sound, Owner->GetRootComponent(), NAME_None,
			FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true,
			Config.SoundVolume, Config.SoundPitch);
	}

	// ── Weapon material override ──────────────────────────────────────────────
	if (Config.WeaponMaterialOverride)
	{
		if (!bIsBaseLayer)
		{
			// Each state caches one OriginalMaterial, so letting two layers patch the same weapon
			// slot would make their restores stomp each other and strand the override.
			UE_LOG(LogTemp, Warning,
				TEXT("[MontageVFXBinding] Additive layer requested weapon material %s; ignored (only the base layer may override weapon materials)"),
				*GetNameSafe(Config.WeaponMaterialOverride.Get()));
		}
		else if (AWeaponInstance* Weapon = ResolveEquippedWeapon())
		{
			TArray<UMeshComponent*> Meshes;
			Weapon->GetComponents<UMeshComponent>(Meshes);
			if (Meshes.IsValidIndex(0) && Meshes[0])
			{
				UMeshComponent* WeaponMesh = Meshes[0];
				const int32 Slot = Config.WeaponMaterialSlot;

				State.WeaponMesh = WeaponMesh;
				State.MaterialSlot = Slot;
				State.OriginalMaterial = WeaponMesh->GetMaterial(Slot);

				UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Config.WeaponMaterialOverride, this);
				if (DynMat)
				{
					ApplyMaterialParams(DynMat, Config.WeaponMaterialParameterOverrides);
					WeaponMesh->SetMaterial(Slot, DynMat);
					State.ActiveDynamicMaterial = DynMat;
				}
			}
		}
	}

	// ── Annulus plane ────────────────────────────────────────────────────────
	if (ActionData)
	{
		SpawnAnnulusPlanes(Config, *ActionData, State, AnnulusPlaneRemainTime);
	}

	return State;
}

void UMontageVFXBindingComponent::TearDownActiveSlot(FMontageVFXActiveSlot& Slot)
{
	// Reverse order so the base layer's weapon-material restore is the last write.
	for (int32 Index = Slot.Layers.Num() - 1; Index >= 0; --Index)
	{
		TearDownActiveState(Slot.Layers[Index]);
	}
	Slot.Layers.Reset();
}

void UMontageVFXBindingComponent::DeactivateSlot(FName SlotName)
{
	FMontageVFXActiveSlot* Slot = ActiveStates.Find(SlotName);
	if (!Slot)
	{
		return;
	}
	TearDownActiveSlot(*Slot);
	ActiveStates.Remove(SlotName);
	PendingBindings.Remove(SlotName);
}

void UMontageVFXBindingComponent::SetAnnulusPlaneProgress(FName SlotName, float Progress)
{
	FMontageVFXActiveSlot* Slot = ActiveStates.Find(SlotName);
	if (!Slot)
	{
		return;
	}

	const float ClampedProgress = FMath::Clamp(Progress, 0.f, 1.f);
	for (FMontageVFXActiveState& State : Slot->Layers)
	{
		for (UMaterialInstanceDynamic* Material : State.AnnulusPlaneMaterials)
		{
			if (Material)
			{
				Material->SetScalarParameterValue(TEXT("ChargeProgress"), ClampedProgress);
				Material->SetScalarParameterValue(TEXT("Progress"), ClampedProgress);
				Material->SetScalarParameterValue(TEXT("FillPercent"), ClampedProgress);
			}
		}
	}
}

void UMontageVFXBindingComponent::ClearAllBindings()
{
	for (auto& Pair : ActiveStates)
	{
		TearDownActiveSlot(Pair.Value);
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

	for (UStaticMeshComponent* PlaneComponent : State.AnnulusPlaneComponents)
	{
		if (PlaneComponent)
		{
			PlaneComponent->DestroyComponent();
		}
	}
	State.AnnulusPlaneComponents.Reset();
	State.AnnulusPlaneMaterials.Reset();
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
	// Goes through AYogCharacterBase rather than casting to the player subclass, so enemies
	// resolve their spawned weapon actor too.
	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetOwner());
	return Character ? Character->GetEquippedWeaponActor() : nullptr;
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

void UMontageVFXBindingComponent::SpawnAnnulusPlanes(const FMontageVFXBindingConfig& Config,
	const FActionData& ActionData, FMontageVFXActiveState& State, float AnnulusPlaneRemainTime) const
{
	AActor* Owner = GetOwner();
	UStaticMesh* PlaneMesh = Config.AnnulusPlaneMesh.Get();
	if (!PlaneMesh)
	{
		PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	}

	if (!Owner || !Config.bSpawnAnnulusPlane || !PlaneMesh || !Config.AnnulusPlaneMaterial)
	{
		return;
	}

	const float OuterR = ActionData.ActRange > 0.f ? ActionData.ActRange : 400.f;
	if (OuterR <= 0.f)
	{
		return;
	}

	const float SafeMeshSize = FMath::Max(Config.AnnulusPlaneMeshSize, 1.f);
	const float SafeRemainTime = FMath::Max(AnnulusPlaneRemainTime, 0.f);
	UWorld* World = Owner->GetWorld();
	const bool bUseRemainTime = SafeRemainTime > KINDA_SMALL_NUMBER && World;
	const FVector Loc = Owner->GetActorLocation();
	const float Yaw = Owner->GetActorRotation().Yaw;
	const float YawRad = FMath::DegreesToRadians(Yaw);
	const FVector Forward(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f);

	for (const FYogHitboxType& HB : ActionData.hitboxTypes)
	{
		if (HB.hitboxType != EHitBoxType::Annulus)
		{
			continue;
		}

		const FHitboxAnnulus& Ann = HB.AnnulusHitbox;
		const float InnerR = FMath::Max(Ann.inner_radius, 0.f);
		const float EffectiveOffset = Ann.bAutoOffset ? -InnerR : Ann.OffsetCore;
		const FVector CenterLoc = Loc + Forward * EffectiveOffset + FVector(0.f, 0.f, Config.AnnulusPlaneZOffset);
		const float EffectiveOuterR = (Ann.bAutoOffset && InnerR > 0.f) ? OuterR + InnerR : OuterR;
		if (EffectiveOuterR <= 0.f)
		{
			continue;
		}

		UStaticMeshComponent* PlaneComponent = NewObject<UStaticMeshComponent>(Owner);
		if (!PlaneComponent)
		{
			continue;
		}

		PlaneComponent->SetStaticMesh(PlaneMesh);
		PlaneComponent->SetMobility(EComponentMobility::Movable);
		PlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PlaneComponent->SetGenerateOverlapEvents(false);
		PlaneComponent->SetCastShadow(false);
		PlaneComponent->SetReceivesDecals(false);
		PlaneComponent->SetHiddenInGame(false);
		PlaneComponent->RegisterComponent();

		const float DiameterScale = (EffectiveOuterR * 2.f) / SafeMeshSize;
		PlaneComponent->SetWorldLocation(CenterLoc);
		PlaneComponent->SetWorldRotation(FRotator(0.f, Yaw, 0.f));
		PlaneComponent->SetWorldScale3D(FVector(DiameterScale, DiameterScale, 1.f));

		if (UMaterialInstanceDynamic* MID =
			PlaneComponent->CreateDynamicMaterialInstance(0, Config.AnnulusPlaneMaterial))
		{
			const float InnerRadius01 = FMath::Clamp(InnerR / EffectiveOuterR, 0.f, 1.f);
			const float Degree = FMath::Clamp(Ann.degree, 0.f, 360.f);
			MID->SetScalarParameterValue(TEXT("InnerRadius01"), InnerRadius01);
			MID->SetScalarParameterValue(TEXT("OuterRadius01"), 1.f);
			MID->SetScalarParameterValue(TEXT("InnerRadius"), InnerR);
			MID->SetScalarParameterValue(TEXT("OuterRadius"), EffectiveOuterR);
			MID->SetScalarParameterValue(TEXT("Degree"), Degree);
			MID->SetScalarParameterValue(TEXT("Degree01"), Degree / 360.f);
			MID->SetScalarParameterValue(TEXT("AngleOffsetDeg"), 0.f);
			MID->SetScalarParameterValue(TEXT("WorldYaw"), Yaw);
			MID->SetScalarParameterValue(TEXT("Opacity"), Config.AnnulusPlaneTint.A);
			MID->SetScalarParameterValue(TEXT("RemainTime"), SafeRemainTime);
			MID->SetScalarParameterValue(TEXT("SpawnTime"), World ? World->GetTimeSeconds() : 0.f);
			MID->SetScalarParameterValue(TEXT("ChargeProgress"), 0.f);
			MID->SetScalarParameterValue(TEXT("Progress"), 0.f);
			MID->SetScalarParameterValue(TEXT("FillPercent"), 0.f);
			MID->SetVectorParameterValue(TEXT("Tint"), Config.AnnulusPlaneTint);
			ApplyMaterialParams(MID, Config.AnnulusPlaneMaterialParameterOverrides);
			State.AnnulusPlaneMaterials.Add(MID);
		}

		if (!bUseRemainTime)
		{
			State.AnnulusPlaneComponents.Add(PlaneComponent);
		}

		if (bUseRemainTime)
		{
			TWeakObjectPtr<UStaticMeshComponent> WeakPlaneComponent(PlaneComponent);
			FTimerHandle DestroyTimerHandle;
			World->GetTimerManager().SetTimer(
				DestroyTimerHandle,
				FTimerDelegate::CreateLambda([WeakPlaneComponent]()
				{
					if (UStaticMeshComponent* Plane = WeakPlaneComponent.Get())
					{
						Plane->DestroyComponent();
					}
				}),
				SafeRemainTime,
				false);
		}
	}
}
