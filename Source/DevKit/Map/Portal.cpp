// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "DevKit/Player/PlayerCharacterBase.h"
#include "Components/BillboardComponent.h"




APortal::APortal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)

{
	PrimaryActorTick.bCanEverTick = false;


	// Create the Billboard component
	//BillBoard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root_BillBoard"));
	//RootComponent = BillBoard;

	BillBoard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Root_BillBoard"));
	RootComponent = BillBoard;  // Now this works!


	//RootComponent = CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	
	
	//CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(10,10,10));
	CollisionVolume->SetupAttachment(RootComponent);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);


}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
}

//void UGameplayStatics::OpenLevelBySoftObjectPtr(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute, FString Options)
//{
//	const FName LevelName = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));
//	UGameplayStatics::OpenLevel(WorldContextObject, LevelName, bAbsolute, Options);
//}



void APortal::YogOpenLevel(FName level_name)
{
	UGameplayStatics::OpenLevel(
		GetWorld(),
		level_name,
		true,
		"?game=/Game/Code/Core/B_GameMode.B_GameMode_C"
	);
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Entry Portal OnOverlapBegin Happens"));
	APlayerCharacterBase* OverlappingPawn = Cast<APlayerCharacterBase>(OtherActor);

	if (OverlappingPawn != nullptr)
	{
		EnterPortal(OverlappingPawn);

		//UWorld* world = GetWorld();
		//if (world)
		//{
		//	UYogBlueprintFunctionLibrary::GiveWeaponToCharacter(this, OverlappingPawn, WeaponDefinition);
		//}

	}
}
