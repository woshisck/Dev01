#include "Story/Encounter/StoryEncounterTrigger.h"

#include "Components/BoxComponent.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

AStoryEncounterTrigger::AStoryEncounterTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AStoryEncounterTrigger::OnTriggerBeginOverlap);
}

void AStoryEncounterTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UStoryEncounterRuntimeSubsystem* Runtime = GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
		{
			if (EncounterPoint)
			{
				Runtime->TriggerEncounterPoint(EncounterPoint, OtherActor);
			}
			else if (EncounterMap && !NodeId.IsNone())
			{
				Runtime->TriggerEncounterNode(EncounterMap, NodeId, OtherActor);
			}
		}
	}
}
