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




