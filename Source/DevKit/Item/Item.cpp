// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"

#include "ItemSpawner.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Itemdefinition.h"
#include "../Character/PlayerCharacterBase.h"
#include "../AbilitySystem/YogAbilitySystemComponent.h"
#include "ItemDefinition.h"


// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;



	RootComponent = PlayerInteractVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Player_Interact_Volume"));

	PlayerInteractVolume->InitCapsuleSize(100.f, 100.f);
	//PlayerInteractVolume->OnComponentBeginOverlap.AddDynamic(this, &AItemSpawner::OnOverlapBegin);
	//PlayerInteractVolume->OnComponentEndOverlap.AddDynamic(this, &AItemSpawner::OnOverlapEnd);


	BlockVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlockVolume"));
	BlockVolume->InitCapsuleSize(60.f, 60.f);
	BlockVolume->SetupAttachment(RootComponent);


	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
}

void AItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItem::OnConstruction(const FTransform& Transform)
{
	if (ItemDefinition != nullptr && ItemDefinition->DisplayMesh != nullptr)
	{	//Visual stat
		ItemMesh->SetStaticMesh(ItemDefinition->DisplayMesh);
		ItemMesh->SetRelativeLocation(ItemDefinition->ItemMeshOffset);
		ItemMesh->SetRelativeScale3D(ItemDefinition->ItemMeshScale);

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NO FOUND ITEM DEFINITION!, name: %s"), *GetNameSafe(this));
	}
}

