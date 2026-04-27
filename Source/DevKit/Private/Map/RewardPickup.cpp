#include "Map/RewardPickup.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "UI/RuneRewardFloatWidget.h"
#include "UI/YogHUD.h"

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

	if (CollisionVolume)
	{
		CollisionVolume->SetGenerateOverlapEvents(IsPickupAllowed());
	}
	SetActorHiddenInGame(!IsPickupAllowed());
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
	if (!IsPickupAllowed()) return;

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

	// 先验证依赖链完整性，再修改自身状态
	AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this));
	if (!IsPickupAllowed())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] TryPickup ignored outside Arrangement phase."));
		ClearNearbyPlayer();
		return;
	}
	APlayerController* PC = Player->GetController<APlayerController>();
	AYogHUD* HUD = PC ? Cast<AYogHUD>(PC->GetHUD()) : nullptr;

	if (!GM || !PC || !HUD)
	{
		UE_LOG(LogTemp, Error, TEXT("[RewardPickup] TryPickup 失败：GM=%s PC=%s HUD=%s"),
			GM ? TEXT("OK") : TEXT("NULL"),
			PC ? TEXT("OK") : TEXT("NULL"),
			HUD ? TEXT("OK") : TEXT("NULL"));
		return;  // 不修改任何状态，pickup 仍可被再次按 E
	}

	// 此时所有依赖都可用，安全修改自身状态
	bPickedUp = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(false);
	if (Player->PendingPickup == this) Player->PendingPickup = nullptr;

	TArray<FLootOption> Options = (AssignedLoot.Num() > 0)
		? AssignedLoot
		: GM->GenerateIndependentLootOptions();

	// 入队到 HUD（HUD 内部决定立即弹还是排队），不再直接 Destroy 自己
	HUD->QueueLootSelection(Options, this);
}

void ARewardPickup::ConsumeAndDestroy()
{
	UE_LOG(LogTemp, Log, TEXT("[RewardPickup] ConsumeAndDestroy"));
	Destroy();
}

void ARewardPickup::ResetForSkip(APlayerCharacterBase* Player)
{
	UE_LOG(LogTemp, Log, TEXT("[RewardPickup] ResetForSkip — Player=%s InRange=%d"),
		Player ? *Player->GetName() : TEXT("null"), bPlayerInRange);

	bPickedUp = false;

	// 仅当玩家仍在范围内才重新挂回 PendingPickup + 显示浮窗
	if (Player && bPlayerInRange && NearbyPlayer.Get() == Player)
	{
		if (!IsPickupAllowed()) return;

		Player->PendingPickup = this;
		if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(true);
	}
}

bool ARewardPickup::IsPickupAllowed() const
{
	if (const AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->CurrentPhase == ELevelPhase::Arrangement;
	}

	return false;
}

void ARewardPickup::ClearNearbyPlayer()
{
	if (APlayerCharacterBase* Player = NearbyPlayer.Get())
	{
		if (Player->PendingPickup == this)
		{
			Player->PendingPickup = nullptr;
		}
	}

	NearbyPlayer = nullptr;
	bPlayerInRange = false;
	if (RuneInfoWidgetComp)
	{
		RuneInfoWidgetComp->SetVisibility(false);
	}
}
