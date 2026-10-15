// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Weapon/WeaponInstance.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "SaveGame/YogSaveGame.h"
#include "Character/PlayerCharacterBase.h"
#include "NiagaraComponent.h"

// Sets default values
AWeaponInstance::AWeaponInstance()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(root);

	VFXAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("VFXAttachPoint"));
	VFXAttachPoint->SetMobility(EComponentMobility::Movable);
	VFXAttachPoint->SetupAttachment(root);

}

void AWeaponInstance::BeginPlay()
{
	Super::BeginPlay();
}

void AWeaponInstance::PostActorCreated()
{
	Super::PostActorCreated();

	
}


void AWeaponInstance::InitializeWeapon()
{
	AActor* owner = this->GetOwner();
	if (Cast<AWeaponSpawner>(owner))
	{
		//AttachSocket = 
		//AttachTransform
		//WeaponLayer
	}


}

void AWeaponInstance::EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar)
{
	this->SetActorRelativeTransform(AttachTransform);
	this->AttachToComponent(ReceivingChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);

	if (WeaponLayer)
	{
		ReceivingChar->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(this->WeaponLayer);
	}
}

void AWeaponInstance::PlayJustComboVFX(UNiagaraSystem* System)
{
	if (!System || !VFXAttachPoint)
	{
		return;
	}

	if (!JustComboVFXComponent)
	{
		JustComboVFXComponent = NewObject<UNiagaraComponent>(this);
		JustComboVFXComponent->SetAsset(System);
		// Both must be set before registration: auto-activate would fire the burst on equip,
		// and auto-destroy would tear the component down after the first proc.
		JustComboVFXComponent->bAutoActivate = false;
		JustComboVFXComponent->SetAutoDestroy(false);
		JustComboVFXComponent->SetupAttachment(VFXAttachPoint);
		JustComboVFXComponent->RegisterComponent();
	}

	JustComboVFXComponent->Activate(true);
}
