#include "LevelFlow/LevelEventTrigger.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "FlowComponent.h"
#include "FlowSubsystem.h"

ALevelEventTrigger::ALevelEventTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->InitBoxExtent(FVector(150.f, 150.f, 100.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALevelEventTrigger::OnOverlapBegin);
	RootComponent = TriggerVolume;

	LevelFlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("LevelFlowComp"));
}

void ALevelEventTrigger::BeginPlay()
{
	Super::BeginPlay();
}

void ALevelEventTrigger::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;
	if (bTriggerOnce && bTriggered) return;
	if (!LevelFlow) return;

	bTriggered = true;

	if (UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>())
	{
		FlowSubsystem->StartRootFlow(LevelFlowComp, LevelFlow, false);
	}
}
