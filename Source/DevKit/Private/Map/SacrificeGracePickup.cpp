#include "Map/SacrificeGracePickup.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Data/SacrificeGraceDA.h"
#include "GameModes/YogGameMode.h"
#include "GameModes/LevelFlowTypes.h"
#include "Kismet/GameplayStatics.h"

ASacrificeGracePickup::ASacrificeGracePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80, 80, 80));
	RootComponent = CollisionVolume;
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ASacrificeGracePickup::OnOverlapBegin);
	CollisionVolume->OnComponentEndOverlap.AddDynamic(this, &ASacrificeGracePickup::OnOverlapEnd);

	PickupHintWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupHintWidgetComp"));
	PickupHintWidgetComp->SetupAttachment(RootComponent);
	PickupHintWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	PickupHintWidgetComp->SetVisibility(false);
}

void ASacrificeGracePickup::BeginPlay()
{
	Super::BeginPlay();
}

void ASacrificeGracePickup::SetSacrificeGraceDA(USacrificeGraceDA* DA)
{
	SacrificeGraceDA = DA;
}

void ASacrificeGracePickup::OnPlayerEnterRange(APlayerCharacterBase* Player)
{
	if (!Player) return;
	Player->PendingSacrificePickup = this;
	NearbyPlayer = Player;
	if (PickupHintWidgetComp) PickupHintWidgetComp->SetVisibility(true);
	UE_LOG(LogTemp, Log, TEXT("SacrificeGracePickup: 玩家进入拾取范围，按 E 键拾取"));
}

void ASacrificeGracePickup::OnPlayerLeaveRange(APlayerCharacterBase* Player)
{
	if (!Player) return;
	if (Player->PendingSacrificePickup == this)
	{
		Player->PendingSacrificePickup = nullptr;
		UE_LOG(LogTemp, Log, TEXT("SacrificeGracePickup: 玩家离开拾取范围"));
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer = nullptr;
		if (PickupHintWidgetComp) PickupHintWidgetComp->SetVisibility(false);
	}
}

void ASacrificeGracePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (bPickedUp) return;
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;
	OnPlayerEnterRange(Player);
}

void ASacrificeGracePickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;
	OnPlayerLeaveRange(Player);
}

void ASacrificeGracePickup::TryPickup(APlayerCharacterBase* Player)
{
	if (bPickedUp || !Player) return;

	if (!IsPickupAllowed())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SacrificeGracePickup] TryPickup ignored outside Arrangement phase."));
		ClearNearbyPlayer();
		return;
	}

	if (!SacrificeGraceDA)
	{
		UE_LOG(LogTemp, Error, TEXT("[SacrificeGracePickup] TryPickup 失败：SacrificeGraceDA 未注入"));
		return;
	}

	bPickedUp = true;
	if (Player->PendingSacrificePickup == this)
	{
		Player->PendingSacrificePickup = nullptr;
	}
	if (PickupHintWidgetComp) PickupHintWidgetComp->SetVisibility(false);

	Player->AcquireSacrificeGrace(SacrificeGraceDA);
	Destroy();
}

bool ASacrificeGracePickup::IsPickupAllowed() const
{
	if (const AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->CurrentPhase == ELevelPhase::Arrangement;
	}
	return false;
}

void ASacrificeGracePickup::ClearNearbyPlayer()
{
	if (APlayerCharacterBase* Player = NearbyPlayer.Get())
	{
		if (Player->PendingSacrificePickup == this)
		{
			Player->PendingSacrificePickup = nullptr;
		}
	}
	NearbyPlayer = nullptr;
	if (PickupHintWidgetComp) PickupHintWidgetComp->SetVisibility(false);
}
