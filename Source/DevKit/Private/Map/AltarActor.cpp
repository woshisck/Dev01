#include "Map/AltarActor.h"
#include "Components/BoxComponent.h"
#include "GameModes/YogGameMode.h"
#include "UI/AltarMenuWidget.h"

AAltarActor::AAltarActor()
{
	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	RootComponent = InteractBox;
}

void AAltarActor::BeginPlay()
{
	Super::BeginPlay();

	if (AYogGameMode* GM = Cast<AYogGameMode>(GetWorld()->GetAuthGameMode()))
		GM->OnPhaseChanged.AddDynamic(this, &AAltarActor::OnPhaseChanged);

	if (AltarMenuWidgetClass)
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			AltarMenuWidget = CreateWidget<UAltarMenuWidget>(PC, AltarMenuWidgetClass);
			if (AltarMenuWidget)
				AltarMenuWidget->AddToViewport(10);
		}
}

void AAltarActor::OnPhaseChanged(ELevelPhase NewPhase)
{
	bIsActive = (NewPhase == ELevelPhase::Arrangement);
	if (!bIsActive && AltarMenuWidget && AltarMenuWidget->IsActivated())
		AltarMenuWidget->DeactivateWidget();
}

void AAltarActor::TryInteract(APlayerCharacterBase* Player)
{
	if (!bIsActive || !AltarMenuWidget) return;
	AltarMenuWidget->SetupAltar(AltarData, Player);
	AltarMenuWidget->ActivateWidget();
}

void AAltarActor::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	OnPlayerNearby(Player, true);
}

void AAltarActor::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	OnPlayerNearby(Player, false);
	if (AltarMenuWidget && AltarMenuWidget->IsActivated())
		AltarMenuWidget->DeactivateWidget();
}
