// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PropInteractComponnet.h"
#include "Item/ItemSpawner.h"
#include "Components/CapsuleComponent.h"

#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/PlayerInteraction.h"
#include "Map/RewardPickup.h"

// Sets default values for this component's properties
UPropInteractComponnet::UPropInteractComponnet()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UPropInteractComponnet::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner()) return;

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwner());
	if (!Player) return;

	if (IPlayerInteraction* Interactable = Cast<IPlayerInteraction>(OtherActor))
	{
		Interactable->OnPlayerBeginOverlap(Player);
	}
	else if (OtherActor->IsA(ARewardPickup::StaticClass()))
	{
		Player->GetWidgetcomponent()->SetVisibility(true);
	}
}

//JUNKYARD
//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
//if (PlayerController && InventoryWidgetClass)
//{
//	UUserWidget* HUDWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);

//	if (HUDWidget)
//	{
//		HUDWidget->AddToViewport();

//		// Show mouse cursor
//		PlayerController->bShowMouseCursor = true;

//		// Set input mode to UI only (disables movement)
//		FInputModeUIOnly InputMode;
//		InputMode.SetWidgetToFocus(HUDWidget->TakeWidget());
//		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
//		PlayerController->SetInputMode(InputMode);
//	}
//}

void UPropInteractComponnet::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor == GetOwner()) return;

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwner());
	if (!Player) return;

	if (IPlayerInteraction* Interactable = Cast<IPlayerInteraction>(OtherActor))
	{
		Interactable->OnPlayerEndOverlap(Player);
	}
	else if (OtherActor->IsA(ARewardPickup::StaticClass()))
	{
		Player->GetWidgetcomponent()->SetVisibility(false);
	}
}

void UPropInteractComponnet::Interact()
{

	UE_LOG(LogTemp, Log, TEXT("Interacted with item!"));
}

// Called when the game starts
void UPropInteractComponnet::BeginPlay()
{
	Super::BeginPlay();


	if (AActor* Owner = GetOwner())
	{
		if (UCapsuleComponent* Capsule = Owner->FindComponentByClass<UCapsuleComponent>())
		{

			Capsule->OnComponentBeginOverlap.AddDynamic(this, &UPropInteractComponnet::OnOverlapBegin);
			Capsule->OnComponentEndOverlap.AddDynamic(this, &UPropInteractComponnet::OnOverlapEnd);
			// Position above the head

		}
		if (UWidgetComponent* widgetComp = Owner->FindComponentByClass<UWidgetComponent>())
		{
			widgetComp->SetWidgetClass(InventoryWidgetClass);
		}


		AYogCharacterBase* PlayerOwner = Cast<AYogCharacterBase>(GetOwner());
		PlayerOwner->GetWidgetcomponent()->SetWidgetClass(InventoryWidgetClass);
		PlayerOwner->GetWidgetcomponent()->SetWidgetSpace(EWidgetSpace::Screen);
		PlayerOwner->GetWidgetcomponent()->SetDrawAtDesiredSize(true);
		PlayerOwner->GetWidgetcomponent()->SetVisibility(false);
	}

	// ...
	
}


// Called every frame
void UPropInteractComponnet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

