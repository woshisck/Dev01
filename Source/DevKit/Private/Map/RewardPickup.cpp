#include "Map/RewardPickup.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Component/InteractPromptComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Containers/Ticker.h"
#include "Component/BackpackGridComponent.h"
#include "Data/RewardLootTableDataAsset.h"
#include "Data/WeaponSkillDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "System/YogLootSubsystem.h"
#include "System/YogSettings.h"
#include "UI/LootTypeChoiceWidget.h"
#include "UI/RuneRewardFloatWidget.h"
#include "UI/YogHUD.h"
#include "UI/YogUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
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

	InteractPromptComp = CreateDefaultSubobject<UInteractPromptComponent>(TEXT("InteractPromptComp"));
	InteractPromptComp->PromptHeight = 60.f;
}

void ARewardPickup::BeginPlay()
{
	Super::BeginPlay();

	if (bStopAutoActivatedAudioOnBeginPlay)
	{
		TArray<UAudioComponent*> AudioComponents;
		GetComponents(AudioComponents);
		for (UAudioComponent* AudioComponent : AudioComponents)
		{
			if (!AudioComponent)
			{
				continue;
			}

			AudioComponent->SetAutoActivate(false);
			AudioComponent->Stop();
		}
	}

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
	if (!bPickupEnabled) return;

	Player->RegisterInteractable(this);
	NearbyPlayer = Player;
	bPlayerInRange = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(true);
	ShowInteractPrompt(true);
	UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家进入拾取范围，按 A 键（手柄）拾取"));
}

void ARewardPickup::OnPlayerLeaveRange(APlayerCharacterBase* Player)
{
	if (!Player) return;

	// Cancel while the player is still registered as nearby so ResetForSkip restores state;
	// the unregister below then removes the interactable again.
	if (ULootTypeChoiceWidget* ChoiceWidget = OpenLootTypeChoiceWidget.Get())
	{
		if (ChoiceWidget->IsActivated())
		{
			ChoiceWidget->CancelChoice();
		}
	}
	OpenLootTypeChoiceWidget.Reset();

	Player->UnregisterInteractable(this);
	UE_LOG(LogTemp, Log, TEXT("RewardPickup: 玩家离开拾取范围"));
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer = nullptr;
		bPlayerInRange = false;
		ShowInteractPrompt(false);
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

void ARewardPickup::SetPickupEnabled(bool bEnabled)
{
	bPickupEnabled = bEnabled;
	RefreshPickupAvailability();
}

ARewardPickup* ARewardPickup::SpawnWithLoot(UObject* WorldContext, TSubclassOf<ARewardPickup> PickupClass,
	const FVector& Location, const TArray<FLootOption>& Options, bool bPlayFocusCue)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Options.IsEmpty())
	{
		return nullptr;
	}

	if (!PickupClass)
	{
		PickupClass = UYogSettings::Get()->DefaultRewardPickupClass.LoadSynchronous();
	}
	if (!PickupClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] SpawnWithLoot: no pickup class. Set DefaultRewardPickupClass in Project Settings > Yog."));
		return nullptr;
	}

	ARewardPickup* Pickup = World->SpawnActor<ARewardPickup>(PickupClass, Location, FRotator::ZeroRotator);
	if (!Pickup)
	{
		return nullptr;
	}

	Pickup->AssignLoot(Options);
	if (bPlayFocusCue)
	{
		Pickup->PlaySpawnFocusCue();
	}
	return Pickup;
}

void ARewardPickup::RefreshPickupAvailability()
{
	const bool bAllowed = bPickupEnabled;
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
	const float CurrentDilation = UGameplayStatics::GetGlobalTimeDilation(World);

	if (!bSpawnFocusTimeDilationActive && CurrentDilation < 0.99f)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[RewardPickup] Spawn focus highlight only; global time dilation is already active (Current=%.3f)."),
			CurrentDilation);
		return;
	}

	if (SpawnFocusDilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SpawnFocusDilationTickerHandle);
		SpawnFocusDilationTickerHandle.Reset();
	}

	if (!bSpawnFocusTimeDilationActive)
	{
		PreviousSpawnFocusGlobalTimeDilation = CurrentDilation;
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
		&& bPickupEnabled;
}

void ARewardPickup::TryPickup(APlayerCharacterBase* Player)
{
	if (bPickedUp || !Player) return;

	// Validate every dependency before touching state, so a failed pickup can be interacted with again.
	if (!bPickupEnabled)
	{
		ClearNearbyPlayer();
		return;
	}
	APlayerController* PC = Player->GetController<APlayerController>();
	AYogHUD* HUD = PC ? Cast<AYogHUD>(PC->GetHUD()) : nullptr;
	UYogLootSubsystem* Loot = GetWorld() ? GetWorld()->GetSubsystem<UYogLootSubsystem>() : nullptr;

	if (!PC || !HUD || !Loot)
	{
		UE_LOG(LogTemp, Error, TEXT("[RewardPickup] TryPickup failed: PC=%s HUD=%s Loot=%s"),
			PC ? TEXT("OK") : TEXT("NULL"),
			HUD ? TEXT("OK") : TEXT("NULL"),
			Loot ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	// 此时所有依赖都可用，安全修改自身状态
	bPickedUp = true;
	if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(false);
	Player->UnregisterInteractable(this);

	if (bUseFixedLootOptions && !FixedLootOptions.IsEmpty() && AssignedLoot.IsEmpty())
	{
		AssignLoot(FixedLootOptions);
	}

	if (bOfferLootTypeChoice && !LootTypeChoices.IsEmpty() && TryOpenLootTypeChoice(Player, PC))
	{
		return;
	}

	TArray<FLootOption> Options = (AssignedLoot.Num() > 0)
		? AssignedLoot
		: Loot->GenerateRuneOptions(LootTable);

	if (ShouldGrantImmediately(Options))
	{
		if (GrantImmediateLoot(Player, Options))
		{
			ConsumeAndDestroy();
		}
		else
		{
			// Registration was dropped optimistically above; without restoring it the player
			// keeps pressing E with no effect until they leave and re-enter the trigger.
			bPickedUp = false;
			if (bPlayerInRange)
			{
				Player->RegisterInteractable(this);
				if (RuneInfoWidgetComp)
				{
					RuneInfoWidgetComp->SetVisibility(true);
				}
			}
		}
		return;
	}

	// 入队到 HUD（HUD 内部决定立即弹还是排队），不再直接 Destroy 自己
	HUD->QueueLootSelection(Options, this);
}

bool ARewardPickup::TryOpenLootTypeChoice(APlayerCharacterBase* Player, APlayerController* PC)
{
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UYogUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
	ULootTypeChoiceWidget* ChoiceWidget = UIManager
		? Cast<ULootTypeChoiceWidget>(UIManager->EnsureWidget(EYogUIScreenId::LootTypeChoice))
		: nullptr;

	if (!ChoiceWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] LootTypeChoice screen unavailable (missing UI Registry row or wrong widget class); falling back to the normal pickup flow."));
		return false;
	}

	ChoiceWidget->Setup(this, Player);
	OpenLootTypeChoiceWidget = ChoiceWidget;
	UIManager->PushScreen(EYogUIScreenId::LootTypeChoice);
	return true;
}

void ARewardPickup::ResolveLootTypeChoice(int32 Index, APlayerCharacterBase* Player)
{
	OpenLootTypeChoiceWidget.Reset();

	if (!LootTypeChoices.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] ResolveLootTypeChoice: invalid index %d (count=%d)."), Index, LootTypeChoices.Num());
		ResetForSkip(Player);
		return;
	}

	const FLootTypeChoice& Choice = LootTypeChoices[Index];

	if (Choice.LootType == ELootType::Rune)
	{
		UYogLootSubsystem* Loot = GetWorld() ? GetWorld()->GetSubsystem<UYogLootSubsystem>() : nullptr;
		APlayerController* PC = Player ? Player->GetController<APlayerController>() : nullptr;
		AYogHUD* HUD = PC ? Cast<AYogHUD>(PC->GetHUD()) : nullptr;
		if (!Loot || !HUD)
		{
			UE_LOG(LogTemp, Error, TEXT("[RewardPickup] ResolveLootTypeChoice(Rune) failed: Loot=%s HUD=%s"),
				Loot ? TEXT("OK") : TEXT("NULL"),
				HUD ? TEXT("OK") : TEXT("NULL"));
			ResetForSkip(Player);
			return;
		}

		// Prefer the options assigned at spawn so the floating panel and the cards agree;
		// only roll when none were assigned.
		bool bAssignedHasRune = false;
		for (const FLootOption& Option : AssignedLoot)
		{
			bAssignedHasRune |= Option.LootType == ELootType::Rune;
		}

		TArray<FLootOption> RuneOptions = bAssignedHasRune ? AssignedLoot : Loot->GenerateRuneOptions(LootTable);
		if (RuneOptions.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] ResolveLootTypeChoice(Rune): no rune options available."));
			ResetForSkip(Player);
			return;
		}

		HUD->QueueLootSelection(RuneOptions, this);
		return;
	}

	FLootOption Option;
	Option.LootType = Choice.LootType;
	Option.Amount = Choice.Amount;
	Option.MetaCurrencyTag = Choice.MetaCurrencyTag;
	Option.WeaponSkillAsset = Choice.WeaponSkillAsset;
	Option.WeaponSkillCharges = Choice.WeaponSkillCharges;
	Option.DisplayName = Choice.DisplayName;
	Option.Icon = Choice.Icon;

	if (GrantImmediateLoot(Player, { Option }))
	{
		ConsumeAndDestroy();
		return;
	}

	ResetForSkip(Player);
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

	// 仅当玩家仍在范围内才重新登记交互 + 显示浮窗
	if (Player && bPlayerInRange && NearbyPlayer.Get() == Player)
	{
		if (!bPickupEnabled) return;

		Player->RegisterInteractable(this);
		if (RuneInfoWidgetComp) RuneInfoWidgetComp->SetVisibility(true);
	}
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
		if (Option.LootType == ELootType::Rune)
		{
			return false;
		}
	}

	return true;
}

bool ARewardPickup::GrantImmediateLoot(APlayerCharacterBase* Player, const TArray<FLootOption>& Options)
{
	UYogLootSubsystem* Loot = GetWorld() ? GetWorld()->GetSubsystem<UYogLootSubsystem>() : nullptr;
	if (!Player || !Loot)
	{
		return false;
	}

	bool bGrantedAny = false;
	for (const FLootOption& Option : Options)
	{
		if (!Loot->GrantLoot(Player, Option))
		{
			UE_LOG(LogTemp, Warning, TEXT("[RewardPickup] Grant failed. Type=%d Amount=%d Skill=%s"),
				static_cast<int32>(Option.LootType), Option.Amount, *GetNameSafe(Option.WeaponSkillAsset));
			continue;
		}

		bGrantedAny = true;
		switch (Option.LootType)
		{
		case ELootType::Gold:
			K2_OnGoldLootGranted(FMath::Max(1, Option.Amount));
			K2_OnImmediateLootGranted(ELootType::Gold, FMath::Max(1, Option.Amount), Option.MetaCurrencyTag);
			break;

		case ELootType::Material:
			{
				const FGameplayTag CurrencyTag = UYogLootSubsystem::ResolveMaterialCurrencyTag(Option.MetaCurrencyTag);
				K2_OnMaterialLootGranted(CurrencyTag, FMath::Max(1, Option.Amount));
				K2_OnImmediateLootGranted(ELootType::Material, FMath::Max(1, Option.Amount), CurrencyTag);
			}
			break;

		case ELootType::WeaponSkill:
			K2_OnImmediateLootGranted(ELootType::WeaponSkill, Option.WeaponSkillCharges, FGameplayTag());
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
		Player->UnregisterInteractable(this);
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
