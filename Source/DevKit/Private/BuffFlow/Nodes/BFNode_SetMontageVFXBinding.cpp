#include "BuffFlow/Nodes/BFNode_SetMontageVFXBinding.h"

#include "Character/PlayerCharacterBase.h"
#include "Component/MontageVFXBindingComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

UBFNode_SetMontageVFXBinding::UBFNode_SetMontageVFXBinding(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Combat");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultPlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (DefaultPlaneMesh.Succeeded())
	{
		AnnulusPlaneMesh = DefaultPlaneMesh.Object;
	}
}

void UBFNode_SetMontageVFXBinding::ExecuteBuffFlowInput(const FName& PinName)
{
	if (SlotName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SetMontageVFXBinding] SlotName is None, skipping."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetBuffOwner());
	if (!Player || !Player->MontageVFXBindingComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SetMontageVFXBinding] No PlayerCharacterBase or MontageVFXBindingComponent found."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	FMontageVFXBindingConfig Config;
	Config.NiagaraSystem = NiagaraSystem;
	Config.AttachTarget = AttachTarget;
	Config.bAttachToSkeletalMesh = bAttachToSkeletalMesh;
	Config.AttachSocketName = AttachSocketName;
	Config.AttachSocketFallbackNames = AttachSocketFallbackNames;
	Config.WeaponAttachSocketName = WeaponAttachSocketName;
	Config.WeaponAttachSocketFallbackNames = WeaponAttachSocketFallbackNames;
	Config.bFallbackToTargetActorIfWeaponMissing = bFallbackToTargetActorIfWeaponMissing;
	Config.LocationOffset = LocationOffset;
	Config.RotationOffset = RotationOffset;
	Config.Scale = Scale;
	Config.NiagaraParameterOverrides = NiagaraParameterOverrides;
	Config.Sound = Sound;
	Config.WeaponMaterialOverride = WeaponMaterialOverride;
	Config.WeaponMaterialParameterOverrides = WeaponMaterialParameterOverrides;
	Config.WeaponMaterialSlot = WeaponMaterialSlot;
	Config.bSpawnAnnulusPlane = bSpawnAnnulusPlane;
	Config.AnnulusPlaneMesh = AnnulusPlaneMesh;
	Config.AnnulusPlaneMaterial = AnnulusPlaneMaterial;
	Config.AnnulusPlaneZOffset = AnnulusPlaneZOffset;
	Config.AnnulusPlaneMeshSize = AnnulusPlaneMeshSize;
	Config.AnnulusPlaneTint = AnnulusPlaneTint;
	Config.AnnulusPlaneMaterialParameterOverrides = AnnulusPlaneMaterialParameterOverrides;

	Player->MontageVFXBindingComponent->RegisterBinding(SlotName, Config);

	TriggerOutput(TEXT("Out"), true);
}
