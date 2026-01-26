// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponInstance.h"
#include "WeaponSpawner.h"
#include "DevKit/SaveGame/YogSaveGame.h"

// Sets default values
AWeaponInstance::AWeaponInstance()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

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
		//WeaponAbilities
	}


}

void AWeaponInstance::EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar)
{
	//TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
	//FName Socket = WeaponActorInst.AttachSocket;
	//FTransform Transform = WeaponActorInst.AttachTransform;


	this->SetActorRelativeTransform(AttachTransform);
	this->AttachToComponent(ReceivingChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
	
	if (WeaponLayer)
	{
		ReceivingChar->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(this->WeaponLayer);
	}

}
