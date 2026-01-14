// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponInstance.h"
#include "DevKit/SaveGame/YogSaveGame.h"

// Sets default values
AWeaponInstance::AWeaponInstance()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

}

void AWeaponInstance::EquipWeaponToCharacter(APlayerCharacterBase* ReceivingChar)
{
	//TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
	//FName Socket = WeaponActorInst.AttachSocket;
	//FTransform Transform = WeaponActorInst.AttachTransform;


	this->SetActorRelativeTransform(Relative_Transform);
	this->AttachToComponent(ReceivingChar->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
	
}
