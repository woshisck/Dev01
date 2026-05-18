#include "World/HubFacilityActor.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "UI/YogHUD.h"

AHubFacilityActor::AHubFacilityActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetBoxExtent(FVector(120.f, 120.f, 90.f));
	InteractBox->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = InteractBox;
}

void AHubFacilityActor::BeginPlay()
{
	Super::BeginPlay();
	InteractBox->OnComponentBeginOverlap.AddDynamic(this, &AHubFacilityActor::HandleBeginOverlap);
	InteractBox->OnComponentEndOverlap.AddDynamic(this,   &AHubFacilityActor::HandleEndOverlap);
}

void AHubFacilityActor::Interact(APlayerCharacterBase* Player)
{
	BP_OnInteract(Player);

	if (!WidgetClass) return;

	APlayerController* PC = Player ? Player->GetController<APlayerController>() : nullptr;
	if (!PC) return;

	if (UCommonActivatableWidget* Widget = CreateWidget<UCommonActivatableWidget>(PC, WidgetClass))
	{
		Widget->ActivateWidget();
	}
}

void AHubFacilityActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                            AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                            int32 OtherBodyIndex, bool bFromSweep,
                                            const FHitResult& SweepResult)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		Player->PendingFacility = this;
	}
}

void AHubFacilityActor::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent,
                                          AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                          int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		if (Player->PendingFacility == this)
		{
			Player->PendingFacility = nullptr;
		}
	}
}
