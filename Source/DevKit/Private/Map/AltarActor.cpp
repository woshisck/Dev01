#include "Map/AltarActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameModes/YogGameMode.h"
#include "NiagaraComponent.h"
#include "UI/AltarMenuWidget.h"
#include "UI/InteractPromptWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "Character/PlayerCharacterBase.h"
#include "UObject/ConstructorHelpers.h"

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

	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));
	AltarMesh->SetupAttachment(RootComponent);
	AltarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultAltarMesh(
		TEXT("/Game/Art/EnvironmentAsset/Prox_Box/SM_GothicAltar01.SM_GothicAltar01"));
	if (DefaultAltarMesh.Succeeded())
	{
		AltarMesh->SetStaticMesh(DefaultAltarMesh.Object);
	}

	InteractPromptWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidgetComp"));
	InteractPromptWidgetComp->SetupAttachment(RootComponent);
	InteractPromptWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	InteractPromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	InteractPromptWidgetComp->SetDrawAtDesiredSize(true);
	InteractPromptWidgetComp->SetVisibility(false);
	InteractPromptWidgetComp->SetWidgetClass(UInteractPromptWidget::StaticClass());

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

	if (AltarMenuWidgetClass)
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			AltarMenuWidget = CreateWidget<UAltarMenuWidget>(PC, AltarMenuWidgetClass);
			if (AltarMenuWidget)
				AltarMenuWidget->AddToViewport(10);
		}

	ConfigureInteractPrompt();
}

void AAltarActor::OnPhaseChanged(ELevelPhase NewPhase)
{
	SetAltarActive(NewPhase == ELevelPhase::Arrangement);
}

void AAltarActor::SetAltarActive(bool bInActive)
{
	bIsActive = bInActive && !bSacrificeRewardConsumed;
	if (!bIsActive && AltarMenuWidget && AltarMenuWidget->IsActivated())
		AltarMenuWidget->DeactivateWidget();
	if (!bIsActive && SacrificeWidget && SacrificeWidget->IsActivated())
		SacrificeWidget->DeactivateWidget();

	if (NearbyPlayer.IsValid())
	{
		NearbyPlayer->PendingAltar = bIsActive ? this : nullptr;
		SetInteractPromptVisible(bIsActive);
	}
	else if (bIsActive && InteractBox)
	{
		// Player may already be inside the box when the altar becomes active (e.g. phase change
		// fires after the player walked in). Re-trigger the overlap logic so PendingAltar is set.
		TArray<AActor*> OverlappingActors;
		InteractBox->GetOverlappingActors(OverlappingActors, APlayerCharacterBase::StaticClass());
		for (AActor* Actor : OverlappingActors)
		{
			if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Actor))
			{
				OnPlayerBeginOverlap(Player);
				break;
			}
		}
		if (!NearbyPlayer.IsValid())
			SetInteractPromptVisible(false);
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
	ConfigureInteractPrompt();
	SetInteractPromptVisible(NearbyPlayer.IsValid() && bIsActive);
}

void AAltarActor::SetOpenSacrificeDirectly(bool bInOpenDirectly)
{
	bOpenSacrificeDirectly = bInOpenDirectly;
	ConfigureInteractPrompt();
}

void AAltarActor::TryInteract(APlayerCharacterBase* Player)
{
	if (!bIsActive || bSacrificeRewardConsumed || !Player || !AltarData) return;

	if (bOpenSacrificeDirectly)
	{
		if (!SacrificeWidget || !SacrificeWidget->IsInViewport())
		{
			TSubclassOf<USacrificeSelectionWidget> WidgetClass = SacrificeWidgetClass;
			if (!WidgetClass)
			{
				WidgetClass = USacrificeSelectionWidget::StaticClass();
			}
			if (APlayerController* PC = Player->GetController<APlayerController>())
			{
				SacrificeWidget = CreateWidget<USacrificeSelectionWidget>(PC, WidgetClass);
				if (SacrificeWidget)
				{
					SacrificeWidget->AddToViewport(16);
				}
			}
		}

		if (!SacrificeWidget)
		{
			return;
		}

		SacrificeWidget->Setup(AltarData, Player, this);
		SacrificeWidget->ActivateWidget();
		return;
	}

	if (!AltarMenuWidget) return;
	AltarMenuWidget->SetupAltar(AltarData, Player);
	AltarMenuWidget->ActivateWidget();
}

void AAltarActor::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	NearbyPlayer = Player;
	if (Player && bIsActive)
	{
		Player->PendingAltar = this;
	}
	SetInteractPromptVisible(Player && bIsActive);
	OnPlayerNearby(Player, true);
}

void AAltarActor::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	if (Player && Player->PendingAltar == this)
	{
		Player->PendingAltar = nullptr;
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer.Reset();
	}
	SetInteractPromptVisible(false);
	OnPlayerNearby(Player, false);
	if (AltarMenuWidget && AltarMenuWidget->IsActivated())
		AltarMenuWidget->DeactivateWidget();
	if (SacrificeWidget && SacrificeWidget->IsActivated())
		SacrificeWidget->DeactivateWidget();
}

void AAltarActor::ConfigureInteractPrompt()
{
	if (!InteractPromptWidgetComp)
	{
		return;
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		InteractPromptWidgetComp->SetOwnerPlayer(PC->GetLocalPlayer());
	}

	InteractPromptWidgetComp->SetWidgetClass(UInteractPromptWidget::StaticClass());
	InteractPromptWidgetComp->InitWidget();
	if (UInteractPromptWidget* PromptWidget = Cast<UInteractPromptWidget>(InteractPromptWidgetComp->GetWidget()))
	{
		PromptWidget->SetPromptLabel(bOpenSacrificeDirectly
			? NSLOCTEXT("InteractPrompt", "Sacrifice", "献祭")
			: NSLOCTEXT("InteractPrompt", "UseAltar", "使用祭坛"));
	}
}

void AAltarActor::SetInteractPromptVisible(bool bVisible)
{
	if (InteractPromptWidgetComp)
	{
		InteractPromptWidgetComp->SetVisibility(bVisible && bIsActive && !bSacrificeRewardConsumed);
	}
}
