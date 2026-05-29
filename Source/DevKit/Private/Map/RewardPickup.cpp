#include "Map/RewardPickup.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Containers/Ticker.h"
#include "Component/BackpackGridComponent.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "Misc/App.h"
#include "UI/RuneRewardFloatWidget.h"
#include "UI/YogHUD.h"
#include "Visual/TimeDilationVisualSubsystem.h"

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

	if (bUseFixedLootOptions && !FixedLootOptions.IsEmpty() && AssignedLoot.IsEmpty())
	{
		AssignLoot(FixedLootOptions);
	}

	RefreshPickupAvailability();
}

void ARewardPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickSpawnFocusCue(DeltaTime);

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

void ARewardPickup::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetSpawnFocusHighlightActive(false);
	RestoreSpawnFocusTimeDilation();

	Super::EndPlay(EndPlayReason);
}

void ARewardPickup::OnPlayerEnterRange(APlayerCharacterBase* Player)
{
	if (bPickedUp || !Player) return;
	if (!IsPickupAllowed()) return;

	Player->PendingPickup = this;
	NearbyPlayer = Player;
	bPlayerInRange = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(true);
	UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家进入拾取范围，按 A 键（手柄）拾取"));
}

void ARewardPickup::OnPlayerLeaveRange(APlayerCharacterBase* Player)
{
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
		if (RuneInfoWidgetComp && !bSpawnFocusCueActive) RuneInfoWidgetComp->SetVisibility(false);
	}
}

void ARewardPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerEnterRange(Player);
	}
}

void ARewardPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerLeaveRange(Player);
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

void ARewardPickup::RefreshPickupAvailability()
{
	const bool bAllowed = IsPickupAllowed();
	if (CollisionVolume)
	{
		CollisionVolume->SetGenerateOverlapEvents(bAllowed);
	}
	SetActorEnableCollision(bAllowed);
	SetActorHiddenInGame(!bAllowed);
}

void ARewardPickup::PlaySpawnFocusCue()
{
	if (!bEnableSpawnFocusCue || bPickedUp)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SetSpawnFocusHighlightActive(true);

	const float SafeScale = FMath::Clamp(SpawnFocusDilationScale, 0.01f, 1.f);
	const float SafeDuration = FMath::Max(0.05f, SpawnFocusDilationDuration);

	if (SpawnFocusDilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SpawnFocusDilationTickerHandle);
		SpawnFocusDilationTickerHandle.Reset();
	}

	if (!bSpawnFocusTimeDilationActive)
	{
		PreviousSpawnFocusGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
		UTimeDilationVisualSubsystem::BeginTimeDilationVisual(this);
		bSpawnFocusTimeDilationActive = true;
	}

	UGameplayStatics::SetGlobalTimeDilation(World, SafeScale);

	TWeakObjectPtr<ARewardPickup> WeakThis(this);
	SpawnFocusDilationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float) -> bool
		{
			if (ARewardPickup* Pickup = WeakThis.Get())
			{
				Pickup->RestoreSpawnFocusTimeDilation();
			}
			return false;
		}),
		SafeDuration);
}

bool ARewardPickup::IsAvailableForCameraFocus() const
{
	return !bPickedUp
		&& !IsHidden()
		&& !IsActorBeingDestroyed()
		&& IsPickupAllowed();
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
		return;  // 不修改任何状态，pickup 仍可被再次按 A（手柄）
	}

	// 此时所有依赖都可用，安全修改自身状态
	bPickedUp = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(false);
	if (Player->PendingPickup == this) Player->PendingPickup = nullptr;

	if (bUseFixedLootOptions && !FixedLootOptions.IsEmpty() && AssignedLoot.IsEmpty())
	{
		AssignLoot(FixedLootOptions);
	}

	TArray<FLootOption> Options = (AssignedLoot.Num() > 0)
		? AssignedLoot
		: GM->GenerateIndependentLootOptions();

	if (ShouldGrantImmediately(Options))
	{
		if (GrantImmediateLoot(Player, Options))
		{
			ConsumeAndDestroy();
		}
		else
		{
			bPickedUp = false;
			if (bPlayerInRange && RuneInfoWidgetComp)
			{
				RuneInfoWidgetComp->SetVisibility(true);
			}
		}
		return;
	}

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
	if (bAllowPickupOutsideArrangement)
	{
		return true;
	}

	if (const AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		return GM->CurrentPhase == ELevelPhase::Arrangement;
	}

	return false;
}

bool ARewardPickup::ShouldGrantImmediately(const TArray<FLootOption>& Options) const
{
	return ShouldGrantLootImmediatelyForOptions(Options);
}

bool ARewardPickup::ShouldGrantLootImmediatelyForOptions(const TArray<FLootOption>& Options)
{
	if (Options.IsEmpty())
	{
		return false;
	}

	for (const FLootOption& Option : Options)
	{
		if (Option.LootType == ELootType::Rune && Option.RuneAsset)
		{
			return false;
		}
	}

	return true;
}

FGameplayTag ARewardPickup::ResolveMaterialCurrencyTag(FGameplayTag ConfiguredTag)
{
	if (ConfiguredTag.IsValid())
	{
		return ConfiguredTag;
	}

	return FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.Common.A"), false);
}

bool ARewardPickup::GrantImmediateLoot(APlayerCharacterBase* Player, const TArray<FLootOption>& Options)
{
	if (!Player)
	{
		return false;
	}

	bool bGrantedAny = false;
	for (const FLootOption& Option : Options)
	{
		const int32 Amount = FMath::Max(0, Option.Amount);
		if (Amount <= 0)
		{
			continue;
		}

		switch (Option.LootType)
		{
		case ELootType::Gold:
			if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
			{
				Backpack->AddGold(Amount);
				bGrantedAny = true;
				K2_OnGoldLootGranted(Amount);
				K2_OnImmediateLootGranted(ELootType::Gold, Amount, Option.MetaCurrencyTag);
				UE_LOG(LogTemp, Log, TEXT("[RewardPickup] Granted gold: %d"), Amount);
			}
			break;

		case ELootType::Material:
			{
				const FGameplayTag MaterialCurrencyTag = ResolveMaterialCurrencyTag(Option.MetaCurrencyTag);
				if (!MaterialCurrencyTag.IsValid())
				{
					UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] Material reward has no valid currency tag; amount=%d."), Amount);
					break;
				}

				if (UYogMetaProgressionSubsystem* Meta = UGameInstance::GetSubsystem<UYogMetaProgressionSubsystem>(GetGameInstance()))
				{
					Meta->AddCurrency(MaterialCurrencyTag, Amount);
					bGrantedAny = true;
					K2_OnMaterialLootGranted(MaterialCurrencyTag, Amount);
					K2_OnImmediateLootGranted(ELootType::Material, Amount, MaterialCurrencyTag);
					UE_LOG(LogTemp, Log, TEXT("[RewardPickup] Granted material: %s x%d"), *MaterialCurrencyTag.ToString(), Amount);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] Material reward failed: MetaProgression subsystem missing. Tag=%s Amount=%d"),
						*MaterialCurrencyTag.ToString(),
						Amount);
				}
			}
			break;

		default:
			break;
		}
	}

	return bGrantedAny;
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

void ARewardPickup::SetSpawnFocusHighlightActive(bool bActive)
{
	if (bActive && bSpawnFocusCueActive)
	{
		SpawnFocusHighlightElapsed = 0.f;
		if (RuneInfoWidgetComp)
		{
			RuneInfoWidgetComp->SetVisibility(true);
			if (URuneRewardFloatWidget* FloatWidget = Cast<URuneRewardFloatWidget>(RuneInfoWidgetComp->GetWidget()))
			{
				FloatWidget->PlayPromptHighlightPulse(SpawnFocusHighlightDuration);
			}
		}
		return;
	}

	if (!bActive && !bSpawnFocusCueActive)
	{
		return;
	}

	bSpawnFocusCueActive = bActive;
	SpawnFocusHighlightElapsed = 0.f;

	if (bActive)
	{
		SpawnFocusPrimitiveStates.Reset();

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		GetComponents(PrimitiveComponents);
		for (UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!Component)
			{
				continue;
			}

			FPrimitiveHighlightState State;
			State.Component = Component;
			State.bRenderCustomDepth = Component->bRenderCustomDepth;
			State.CustomDepthStencilValue = Component->CustomDepthStencilValue;
			SpawnFocusPrimitiveStates.Add(State);

			Component->SetRenderCustomDepth(true);
			Component->SetCustomDepthStencilValue(FMath::Clamp(SpawnFocusStencilValue, 0, 255));
		}

		if (RuneInfoWidgetComp)
		{
			RuneInfoWidgetComp->SetVisibility(true);
			if (URuneRewardFloatWidget* FloatWidget = Cast<URuneRewardFloatWidget>(RuneInfoWidgetComp->GetWidget()))
			{
				FloatWidget->PlayPromptHighlightPulse(SpawnFocusHighlightDuration);
			}
		}

		K2_OnSpawnFocusCueStarted();
		return;
	}

	for (const FPrimitiveHighlightState& State : SpawnFocusPrimitiveStates)
	{
		if (UPrimitiveComponent* Component = State.Component.Get())
		{
			Component->SetRenderCustomDepth(State.bRenderCustomDepth);
			Component->SetCustomDepthStencilValue(State.CustomDepthStencilValue);
		}
	}
	SpawnFocusPrimitiveStates.Reset();

	if (RuneInfoWidgetComp && !bPlayerInRange)
	{
		RuneInfoWidgetComp->SetVisibility(false);
	}

	K2_OnSpawnFocusCueEnded();
}

void ARewardPickup::TickSpawnFocusCue(float DeltaTime)
{
	if (!bSpawnFocusCueActive)
	{
		return;
	}

	SpawnFocusHighlightElapsed += FApp::GetDeltaTime() > 0.f ? FApp::GetDeltaTime() : DeltaTime;
	if (SpawnFocusHighlightElapsed >= FMath::Max(0.05f, SpawnFocusHighlightDuration))
	{
		SetSpawnFocusHighlightActive(false);
	}
}

void ARewardPickup::RestoreSpawnFocusTimeDilation()
{
	if (SpawnFocusDilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SpawnFocusDilationTickerHandle);
		SpawnFocusDilationTickerHandle.Reset();
	}

	if (!bSpawnFocusTimeDilationActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FMath::Max(0.01f, PreviousSpawnFocusGlobalTimeDilation));
	}

	UTimeDilationVisualSubsystem::EndTimeDilationVisual(this);
	bSpawnFocusTimeDilationActive = false;
}
