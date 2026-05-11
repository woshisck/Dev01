#include "LevelFlow/LevelEventTrigger.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "FlowComponent.h"

ALevelEventTrigger::ALevelEventTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->InitBoxExtent(FVector(150.f, 150.f, 100.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALevelEventTrigger::OnOverlapBegin);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ALevelEventTrigger::OnOverlapEnd);
	RootComponent = TriggerVolume;

	LevelFlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("LevelFlowComp"));
}

void ALevelEventTrigger::BeginPlay()
{
	Super::BeginPlay();
	bTriggered = false;
	UE_LOG(LogTemp, Log, TEXT("[LevelEventTrigger] BeginPlay: %s bTriggerOnce=%d Flow=%s"),
		*GetNameSafe(this), (int32)bTriggerOnce, *GetNameSafe(LevelFlow));
}

void ALevelEventTrigger::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;
	if (bTriggerOnce && bTriggered) return;
	if (!LevelFlow) { UE_LOG(LogTemp, Warning, TEXT("[LevelEventTrigger] LevelFlow 未赋值！")); return; }

	bTriggered = true;
	UE_LOG(LogTemp, Log, TEXT("[LevelEventTrigger] 触发 Flow: %s"), *LevelFlow->GetName());

	LevelFlowComp->FinishRootFlow(LevelFlow, EFlowFinishPolicy::Keep);
	LevelFlowComp->RootFlow = LevelFlow;
	LevelFlowComp->StartRootFlow();
}

void ALevelEventTrigger::OnOverlapEnd(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;
	OnPlayerExited.Broadcast();
}
