#include "Map/RewardPickup.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "UI/RuneRewardFloatWidget.h"

ARewardPickup::ARewardPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionVolume"));
	CollisionVolume->InitBoxExtent(FVector(80, 80, 80));
	RootComponent = CollisionVolume;
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &ARewardPickup::OnOverlapBegin);
	CollisionVolume->OnComponentEndOverlap.AddDynamic(this, &ARewardPickup::OnOverlapEnd);

	RuneInfoWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("RuneInfoWidgetComp"));
	RuneInfoWidgetComp->SetupAttachment(RootComponent);
	RuneInfoWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	RuneInfoWidgetComp->SetVisibility(false);
}

void ARewardPickup::BeginPlay()
{
	Super::BeginPlay();
}

void ARewardPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bPlayerInRange || !NearbyPlayer.IsValid() || !RuneInfoWidgetComp) return;

	// 动态左右偏移：武器在屏幕右半则向左偏，左半则向右偏
	FVector Right = FVector(0.f, 1.f, 0.f);
	if (APlayerController* PC = NearbyPlayer->GetController<APlayerController>())
	{
		if (PC->PlayerCameraManager)
		{
			FVector CamRight = FRotationMatrix(PC->PlayerCameraManager->GetCameraRotation())
				.GetScaledAxis(EAxis::Y);
			CamRight.Z = 0.f;
			if (!CamRight.IsNearlyZero())
				Right = CamRight.GetSafeNormal();
		}
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(GetActorLocation(), ScreenPos, false))
		{
			FVector2D ViewportSize;
			if (GEngine && GEngine->GameViewport)
				GEngine->GameViewport->GetViewportSize(ViewportSize);
			if (ScreenPos.X > ViewportSize.X * 0.5f)
				Right = -Right;
		}
	}
	RuneInfoWidgetComp->SetRelativeLocation(Right * WidgetSideOffset + FVector(0.f, 0.f, WidgetZOffset));
}

void ARewardPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (bPickedUp) return;
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;

	Player->PendingPickup = this;
	NearbyPlayer = Player;
	bPlayerInRange = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(true);
	UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家进入拾取范围，按 E 键拾取"));
}

void ARewardPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor);
	if (!Player) return;

	if (Player->PendingPickup == this)
	{
		Player->PendingPickup = nullptr;
		UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家离开拾取范围"));
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer = nullptr;
		bPlayerInRange = false;
		if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(false);
	}
}

void ARewardPickup::AssignLoot(const TArray<FLootOption>& InLoot)
{
	AssignedLoot = InLoot;

	// 初始化符文浮窗内容
	if (RuneFloatWidgetClass && RuneInfoWidgetComp)
	{
		RuneInfoWidgetComp->SetWidgetClass(RuneFloatWidgetClass);
		RuneInfoWidgetComp->InitWidget();
		if (URuneRewardFloatWidget* FloatWidget = Cast<URuneRewardFloatWidget>(RuneInfoWidgetComp->GetWidget()))
		{
			FloatWidget->SetLootOptions(AssignedLoot);
		}
	}
}

void ARewardPickup::TryPickup(APlayerCharacterBase* Player)
{
	if (bPickedUp || !Player) return;
	bPickedUp = true;

	bPlayerInRange = false;
	NearbyPlayer = nullptr;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(false);

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
