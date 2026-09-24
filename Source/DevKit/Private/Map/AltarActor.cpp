#include "Map/AltarActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/InteractPromptComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameModes/YogGameMode.h"
#include "NiagaraComponent.h"
#include "UI/AltarMenuWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UI/YogUIManagerSubsystem.h"
#include "Character/PlayerCharacterBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UYogUIManagerSubsystem* GetAltarUIManagerForPlayer(const APlayerCharacterBase* Player)
{
	const APlayerController* PC = Player ? Player->GetController<APlayerController>() : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
}

APlayerCharacterBase* GetFirstLocalPlayerCharacter(const UWorld* World)
{
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
}

bool IsPlayerWithinAltarFallbackRange(const UBoxComponent* Box, const APlayerCharacterBase* Player, float& OutDist2D)
{
	OutDist2D = TNumericLimits<float>::Max();
	if (!Box || !Player)
	{
		return false;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	const FVector BoxLocation = Box->GetComponentLocation();
	const FVector Extent = Box->GetScaledBoxExtent();
	OutDist2D = FVector::Dist2D(PlayerLocation, BoxLocation);

	const FBox ExpandedBox = Box->Bounds.GetBox().ExpandBy(FVector(160.f, 160.f, 140.f));
	const bool bInsideExpandedBox = ExpandedBox.IsInsideOrOn(PlayerLocation);
	const bool bNearCenter = OutDist2D <= FMath::Max(Extent.X, Extent.Y) + 180.f
		&& FMath::Abs(PlayerLocation.Z - BoxLocation.Z) <= Extent.Z + 240.f;
	return bInsideExpandedBox || bNearCenter;
}
}

AAltarActor::AAltarActor()
{
	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	RootComponent = InteractBox;
	InteractBox->InitBoxExtent(FVector(110.f, 110.f, 110.f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractBox->SetGenerateOverlapEvents(true);
	InteractBox->OnComponentBeginOverlap.AddDynamic(this, &AAltarActor::OnInteractBoxBeginOverlap);
	InteractBox->OnComponentEndOverlap.AddDynamic(this, &AAltarActor::OnInteractBoxEndOverlap);

	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));
	AltarMesh->SetupAttachment(RootComponent);
	AltarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultAltarMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultAltarMesh.Succeeded())
	{
		AltarMesh->SetStaticMesh(DefaultAltarMesh.Object);
	}

	InteractPromptComp = CreateDefaultSubobject<UInteractPromptComponent>(TEXT("InteractPromptComp"));

	IdleVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleVFXComponent"));
	IdleVFXComponent->SetupAttachment(RootComponent);
	IdleVFXComponent->SetAutoActivate(true);
}

void AAltarActor::BeginPlay()
{
	Super::BeginPlay();

	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->OnPhaseChanged.AddDynamic(this, &AAltarActor::OnPhaseChanged);
		// Sync immediately in case OnPhaseChanged already fired before this actor's BeginPlay
		OnPhaseChanged(GM->CurrentPhase);
	}

	QueueRefreshCurrentPlayerOverlap();
}

void AAltarActor::OnPhaseChanged(ELevelPhase NewPhase)
{
	SetAltarActive(NewPhase == ELevelPhase::Arrangement);
}

void AAltarActor::SetAltarActive(bool bInActive)
{
	bIsActive = bInActive && !bSacrificeRewardConsumed;
	if (!bIsActive && AltarMenuWidget && AltarMenuWidget->IsActivated())
	{
		if (UYogUIManagerSubsystem* UIManager = GetAltarUIManagerForPlayer(NearbyPlayer.Get()))
		{
			UIManager->PopScreen(EYogUIScreenId::AltarMenu);
		}
	}
	if (!bIsActive && SacrificeWidget && SacrificeWidget->IsActivated())
	{
		if (UYogUIManagerSubsystem* UIManager = GetAltarUIManagerForPlayer(NearbyPlayer.Get()))
		{
			UIManager->PopScreen(EYogUIScreenId::SacrificeSelection);
		}
	}

	if (NearbyPlayer.IsValid())
	{
		if (bIsActive)
		{
			NearbyPlayer->RegisterInteractable(this);
		}
		else
		{
			NearbyPlayer->UnregisterInteractable(this);
		}
		SetInteractPromptVisible(bIsActive);
	}
	else if (bIsActive && InteractBox)
	{
		RefreshCurrentPlayerOverlap();
		QueueRefreshCurrentPlayerOverlap();
	}
	else
	{
		SetInteractPromptVisible(false);
	}
}

void AAltarActor::ConsumeSacrificeReward()
{
	bSacrificeRewardConsumed = true;
	if (IdleVFXComponent)
	{
		IdleVFXComponent->Deactivate();
		IdleVFXComponent->SetVisibility(false);
	}
	SetAltarActive(false);
}

void AAltarActor::SetAltarData(UAltarDataAsset* InData)
{
	AltarData = InData;
	SetInteractPromptVisible(NearbyPlayer.IsValid() && bIsActive);
	QueueRefreshCurrentPlayerOverlap();
}

void AAltarActor::SetOpenSacrificeDirectly(bool bInOpenDirectly)
{
	bOpenSacrificeDirectly = bInOpenDirectly;
	SetInteractPromptVisible(NearbyPlayer.IsValid() && bIsActive);
	QueueRefreshCurrentPlayerOverlap();
}

void AAltarActor::EnsureInteractBoxMinimumExtent(FVector MinimumExtent)
{
	if (!InteractBox)
	{
		return;
	}

	const FVector CurrentExtent = InteractBox->GetUnscaledBoxExtent();
	const FVector NewExtent(
		FMath::Max(CurrentExtent.X, MinimumExtent.X),
		FMath::Max(CurrentExtent.Y, MinimumExtent.Y),
		FMath::Max(CurrentExtent.Z, MinimumExtent.Z));
	if (!CurrentExtent.Equals(NewExtent))
	{
		InteractBox->SetBoxExtent(NewExtent, true);
	}
	QueueRefreshCurrentPlayerOverlap();
}

void AAltarActor::TryInteract(APlayerCharacterBase* Player)
{
	if (!bIsActive || bSacrificeRewardConsumed || !Player || !AltarData)
	{
		return;
	}

	if (bOpenSacrificeDirectly)
	{
		if (APlayerController* PC = Player->GetController<APlayerController>())
		{
			TSubclassOf<USacrificeSelectionWidget> WidgetClass = SacrificeWidgetClass;
			if (!WidgetClass)
			{
				WidgetClass = USacrificeSelectionWidget::StaticClass();
			}
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
				{
					UIManager->SetWidgetClassOverride(EYogUIScreenId::SacrificeSelection, WidgetClass);
					SacrificeWidget = Cast<USacrificeSelectionWidget>(UIManager->EnsureWidget(EYogUIScreenId::SacrificeSelection));
				}
			}
		}

		if (!SacrificeWidget)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[AltarActor] TryInteract failed to create sacrifice widget. WidgetClass=%s Player=%s Actor=%s"),
				*GetNameSafe(SacrificeWidgetClass.Get()),
				*GetNameSafe(Player),
				*GetNameSafe(this));
			return;
		}

		SacrificeWidget->Setup(AltarData, Player, this);
		if (APlayerController* PC = Player->GetController<APlayerController>())
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
				{
					UIManager->PushScreen(EYogUIScreenId::SacrificeSelection);
					return;
				}
			}
		}
		UE_LOG(LogTemp, Warning,
			TEXT("[AltarActor] TryInteract could not push sacrifice screen. Player=%s Actor=%s"),
			*GetNameSafe(Player),
			*GetNameSafe(this));
		return;
	}

	if (APlayerController* PC = Player->GetController<APlayerController>())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				if (AltarMenuWidgetClass)
				{
					UIManager->SetWidgetClassOverride(EYogUIScreenId::AltarMenu, AltarMenuWidgetClass);
				}
				AltarMenuWidget = Cast<UAltarMenuWidget>(UIManager->EnsureWidget(EYogUIScreenId::AltarMenu));
			}
		}
	}

	if (!AltarMenuWidget)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AltarActor] TryInteract failed to create altar menu widget. WidgetClass=%s Player=%s Actor=%s"),
			*GetNameSafe(AltarMenuWidgetClass.Get()),
			*GetNameSafe(Player),
			*GetNameSafe(this));
		return;
	}
	AltarMenuWidget->SetupAltar(AltarData, Player);
	if (APlayerController* PC = Player->GetController<APlayerController>())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				UIManager->PushScreen(EYogUIScreenId::AltarMenu);
				return;
			}
		}
	}
	UE_LOG(LogTemp, Warning,
		TEXT("[AltarActor] TryInteract could not push altar menu screen. Player=%s Actor=%s"),
		*GetNameSafe(Player),
		*GetNameSafe(this));
}

void AAltarActor::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	NearbyPlayer = Player;
	if (Player && bIsActive)
	{
		Player->RegisterInteractable(this);
	}
	const bool bWasActive = bIsActive;
	OnPlayerNearby(Player, true);
	// If the BP event called SetAltarActive(false) as part of an intro animation,
	// re-activate so the prompt appears on the first box entry. Skip re-activation
	// only when the sacrifice has already been consumed (a legitimate deactivation).
	if (bWasActive && !bIsActive && !bSacrificeRewardConsumed)
	{
		SetAltarActive(true);
	}
	else
	{
		SetInteractPromptVisible(Player && bIsActive);
	}
}

void AAltarActor::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	if (Player)
	{
		Player->UnregisterInteractable(this);
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer.Reset();
	}
	SetInteractPromptVisible(false);
	OnPlayerNearby(Player, false);
	if (AltarMenuWidget && AltarMenuWidget->IsActivated())
	{
		if (UYogUIManagerSubsystem* UIManager = GetAltarUIManagerForPlayer(Player))
		{
			UIManager->PopScreen(EYogUIScreenId::AltarMenu);
		}
	}
	if (SacrificeWidget && SacrificeWidget->IsActivated())
	{
		if (UYogUIManagerSubsystem* UIManager = GetAltarUIManagerForPlayer(Player))
		{
			UIManager->PopScreen(EYogUIScreenId::SacrificeSelection);
		}
	}
}

void AAltarActor::OnInteractBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerBeginOverlap(Player);
	}
}

void AAltarActor::OnInteractBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		OnPlayerEndOverlap(Player);
	}
}

void AAltarActor::SetInteractPromptVisible(bool bVisible)
{
	ShowInteractPrompt(bVisible && bIsActive && !bSacrificeRewardConsumed);
}

bool AAltarActor::IsInteractPromptShowing() const
{
	return InteractPromptComp && InteractPromptComp->IsPromptShowing();
}

void AAltarActor::RefreshCurrentPlayerOverlap()
{
	if (!bIsActive || !InteractBox)
	{
		return;
	}

	InteractBox->UpdateOverlaps();

	TArray<AActor*> OverlappingActors;
	InteractBox->GetOverlappingActors(OverlappingActors, APlayerCharacterBase::StaticClass());
	APlayerCharacterBase* FallbackPlayer = GetFirstLocalPlayerCharacter(GetWorld());
	float FallbackDist2D = TNumericLimits<float>::Max();
	const bool bFallbackPlayerInRange = IsPlayerWithinAltarFallbackRange(InteractBox, FallbackPlayer, FallbackDist2D);
	for (AActor* Actor : OverlappingActors)
	{
		if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Actor))
		{
			if (NearbyPlayer.Get() != Player || !Player->OverlappingInteractables.Contains(this) || !IsInteractPromptShowing())
			{
				OnPlayerBeginOverlap(Player);
			}
			return;
		}
	}

	if (bFallbackPlayerInRange)
	{
		if (NearbyPlayer.Get() != FallbackPlayer
			|| !FallbackPlayer->OverlappingInteractables.Contains(this)
			|| !IsInteractPromptShowing())
		{
			OnPlayerBeginOverlap(FallbackPlayer);
		}
		return;
	}

	if (!NearbyPlayer.IsValid())
	{
		SetInteractPromptVisible(false);
	}
}

void AAltarActor::QueueRefreshCurrentPlayerOverlap()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			RefreshCurrentPlayerOverlap();
		}));
}
