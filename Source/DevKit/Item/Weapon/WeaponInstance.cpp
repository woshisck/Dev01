// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponInstance.h"


// Sets default values
AWeaponInstance::AWeaponInstance()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);


	point_DamageStart = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("DamageBox Start"));
	point_DamageEnd = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("DamageBox End"));
	
	
	point_DamageStart->SetupAttachment(RootComponent);
	point_DamageEnd->SetupAttachment(RootComponent);

	//if (WeaponAttributeTable)
	//{
	//	static const FString ContextString(TEXT("Character movement Data Lookup"));
	//	FName RowName(TEXT("TripleC_Lvl_1")); // Name of the row you want to access
	//	FWeaponAttributeData* AttributeData = this->WeaponAttributeTable->FindRow<FWeaponAttributeData>(FName(TEXT("defaultAttribute")), ContextString, true);

	//	if (AttributeData)
	//	{
	//		AttributeData->AttackPower = this->AttackPower;
	//		AttributeData->AttackSpeed = this->AttackSpeed;
	//		AttributeData->AttackRange = this->AttackRange;
	//		AttributeData->PickedUpEffect = this->PickedUpEffect;
	//		AttributeData->CrticalRate = this->CrticalRate;
	//		AttributeData->CriticalDamage = this->CriticalDamage;
	//		AttributeData->Actions = this->Actions;
	//	}


	//}

	//if (Mesh)
	//{
	//	Mesh->AlwaysLoadOnClient = true;
	//	Mesh->AlwaysLoadOnServer = true;
	//	Mesh->bOwnerNoSee = false;
	//	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	//	Mesh->bCastDynamicShadow = true;
	//	Mesh->bAffectDynamicIndirectLighting = true;
	//	Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	//	Mesh->SetupAttachment(CapsuleComponent);
	//	static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
	//	Mesh->SetCollisionProfileName(MeshCollisionProfileName);
	//	Mesh->SetGenerateOverlapEvents(false);
	//	Mesh->SetCanEverAffectNavigation(false);
	//}

}

// Called when the game starts or when spawned
void AWeaponInstance::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponInstance::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponInstance::ClearArray()
{
	IgnoreActorList.Empty();
}

void AWeaponInstance::AddIgnoreActor(AActor* actor)
{
	IgnoreActorList.AddUnique(actor);

}

void AWeaponInstance::Initialize()
{
	Array_damageBox.Empty();
	IgnoreActorList.Empty();
}




