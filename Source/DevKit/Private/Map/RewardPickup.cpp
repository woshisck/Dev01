#include "Map/RewardPickup.h"
#include "Components/BoxComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

ARewardPickup::ARewardPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80, 80, 80));
	RootComponent = CollisionVolume;
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ARewardPickup::OnOverlapBegin);
}

void ARewardPickup::BeginPlay()
{
	Super::BeginPlay();
}

void ARewardPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (bPickedUp) return;
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;

	bPickedUp = true;

	// 通知 GameMode 生成战利品选项并广播给 UI
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->GenerateLootOptions();
	}

	// 拾取后销毁自身
	Destroy();
}
