// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemSpawner.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Item/ItemDefinition.h"
#include "Character/PlayerCharacterBase.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"


AItemSpawner::AItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = PlayerInteractVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Player_Interact_Volume"));
	PlayerInteractVolume->InitCapsuleSize(100.f, 100.f);
	PlayerInteractVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PlayerInteractVolume->SetGenerateOverlapEvents(true);
	PlayerInteractVolume->OnComponentBeginOverlap.AddDynamic(this, &AItemSpawner::OnOverlapBegin);
	PlayerInteractVolume->OnComponentEndOverlap.AddDynamic(this, &AItemSpawner::OnOverlapEnd);

	BlockVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BlockVolume"));
	BlockVolume->InitCapsuleSize(60.f, 60.f);
	BlockVolume->SetupAttachment(RootComponent);
	BlockVolume->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BlockVolume->SetGenerateOverlapEvents(false);

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
}

void AItemSpawner::GrantItem_Implementation(AYogCharacterBase* ReceivingChar)
{
	UE_LOG(LogTemp, Warning, TEXT("GrantItem_Implementation"));
}

void AItemSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerBeginOverlap(Player);
	}
}

void AItemSpawner::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerEndOverlap(Player);
	}
}

void AItemSpawner::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	if (!Player) return;
	Player->OverlappingSpawner = this;
	if (UWidgetComponent* WC = Player->GetWidgetcomponent())
	{
		WC->SetVisibility(true);
	}
}

void AItemSpawner::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	if (!Player) return;
	if (Player->OverlappingSpawner == this)
	{
		Player->OverlappingSpawner = nullptr;
	}
	if (UWidgetComponent* WC = Player->GetWidgetcomponent())
	{
		WC->SetVisibility(false);
	}
}