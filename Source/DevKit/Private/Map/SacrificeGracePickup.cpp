#include "Map/SacrificeGracePickup.h"

#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/SacrificeGraceDA.h"

ASacrificeGracePickup::ASacrificeGracePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80.f, 80.f, 80.f));
	RootComponent = CollisionVolume;

	PickupHintWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupHintWidgetComp"));
	PickupHintWidgetComp->SetupAttachment(RootComponent);
	PickupHintWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	PickupHintWidgetComp->SetDrawAtDesiredSize(true);
	PickupHintWidgetComp->SetVisibility(false);
}

void ASacrificeGracePickup::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionVolume)
	{
		CollisionVolume->SetGenerateOverlapEvents(false);
		CollisionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (PickupHintWidgetComp)
	{
		PickupHintWidgetComp->SetVisibility(false);
	}
	SetActorHiddenInGame(true);
}

void ASacrificeGracePickup::SetSacrificeGraceDA(USacrificeGraceDA* DA)
{
	SacrificeGraceDA = DA;
}

void ASacrificeGracePickup::OnPlayerEnterRange(APlayerCharacterBase* Player)
{
	UE_LOG(LogTemp, Verbose, TEXT("[SacrificeGracePickup] Ignored: SacrificeGraceOption is retired."));
}

void ASacrificeGracePickup::OnPlayerLeaveRange(APlayerCharacterBase* Player)
{
	NearbyPlayer = nullptr;
	if (PickupHintWidgetComp)
	{
		PickupHintWidgetComp->SetVisibility(false);
	}
}

void ASacrificeGracePickup::TryPickup(APlayerCharacterBase* Player)
{
	UE_LOG(LogTemp, Warning, TEXT("[SacrificeGracePickup] TryPickup ignored: SacrificeGraceOption is retired. Use RewardPickup/LootSelection instead."));
	if (PickupHintWidgetComp)
	{
		PickupHintWidgetComp->SetVisibility(false);
	}
}

void ASacrificeGracePickup::ConsumeAndDestroy()
{
	Destroy();
}

void ASacrificeGracePickup::ResetForSkip(APlayerCharacterBase* Player)
{
	if (PickupHintWidgetComp)
	{
		PickupHintWidgetComp->SetVisibility(false);
	}
}

bool ASacrificeGracePickup::IsPickupAllowed() const
{
	return false;
}

void ASacrificeGracePickup::ClearNearbyPlayer()
{
	NearbyPlayer = nullptr;
	if (PickupHintWidgetComp)
	{
		PickupHintWidgetComp->SetVisibility(false);
	}
}

void ASacrificeGracePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
}

void ASacrificeGracePickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
