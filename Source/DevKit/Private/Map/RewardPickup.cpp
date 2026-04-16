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
	CollisionVolume->OnComponentEndOverlap.AddDynamic(this, &ARewardPickup::OnOverlapEnd);
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
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;

	// 进入范围时登记，等待玩家主动按 E 拾取
	Player->PendingPickup = this;
	UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家进入拾取范围，按 E 键拾取"));
}

void ARewardPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;

	// 离开范围时清除登记
	if (Player->PendingPickup == this)
	{
		Player->PendingPickup = nullptr;
		UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家离开拾取范围"));
	}
}

void ARewardPickup::AssignLoot(const TArray<FLootOption>& InLoot)
{
	AssignedLoot = InLoot;
}

void ARewardPickup::TryPickup(APlayerCharacterBase* Player)
{
	if (bPickedUp || !Player) return;
	bPickedUp = true;

	Player->PendingPickup = nullptr;

	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (AssignedLoot.Num() > 0)
		{
			// 使用预分配的选项：多个拾取物互不干扰
			GM->ShowLootOptions(AssignedLoot);
		}
		else
		{
			// 兜底：GameMode 即时生成（单拾取物场景 / 旧逻辑兼容）
			GM->GenerateLootOptions();
		}
	}

	Destroy();
}
