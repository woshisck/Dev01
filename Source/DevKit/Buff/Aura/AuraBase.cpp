// Fill out your copyright notice in the Description page of Project Settings.


#include "AuraBase.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

// Sets default values
AAuraBase::AAuraBase(const FObjectInitializer& ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;



	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	//InventoryManagerComponent = ObjectInitializer.CreateDefaultSubobject<UInventoryManagerComponent>(this, TEXT("InventoryComponent"));
	//HitboxbuffComponent = ObjectInitializer.CreateDefaultSubobject<UHitBoxBufferComponent>(this, TEXT("HitBoxBufferComponent"));
	//UCapsuleComponent* CapsuleComp = GetCapsuleComponent();

	//check(CapsuleComp);
	//CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	//HealthSet = CreateDefaultSubobject<UYogHealthSet>(TEXT("HealthSet"));
	//AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));




}

// Called when the game starts or when spawned
void AAuraBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAuraBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

