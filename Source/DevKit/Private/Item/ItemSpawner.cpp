// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemSpawner.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Item/ItemDefinition.h"
#include "Character/PlayerCharacterBase.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"


AItemSpawner::AItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;



	RootComponent = PlayerInteractVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Player_Interact_Volume"));

	PlayerInteractVolume->InitCapsuleSize(100.f, 100.f);


	BlockVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlockVolume"));
	BlockVolume->InitCapsuleSize(60.f, 60.f);
	BlockVolume->SetupAttachment(RootComponent);


	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	ItemMesh->SetupAttachment(RootComponent);

}

void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void AItemSpawner::Tick(float DeltaTime)
{
}

void AItemSpawner::OnConstruction(const FTransform& Transform)
{	


	//if (ItemDefinition != nullptr && ItemDefinition->DisplayMesh != nullptr)
	//{	//Visual stat
	//	ItemMesh->SetStaticMesh(ItemDefinition->DisplayMesh);
	//	ItemMesh->SetRelativeLocation(ItemDefinition->ItemMeshOffset);
	//	ItemMesh->SetRelativeScale3D(ItemDefinition->ItemMeshScale);
	//	
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("NO FOUND ITEM DEFINITION!, name: %s"), *GetNameSafe(this));
	//}
}



void AItemSpawner::GrantItem_Implementation(AYogCharacterBase* ReceivingChar)
{
	UE_LOG(LogTemp, Warning, TEXT("GrantItem_Implementation"));
}