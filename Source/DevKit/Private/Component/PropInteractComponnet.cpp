// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/PropInteractComponnet.h"
#include "Item/ItemSpawner.h"
#include "Components/CapsuleComponent.h"

#include "Components/WidgetComponent.h"
#include "Character/PlayerCharacterBase.h"

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
	if (APlayerCharacterBase* Character = Cast<APlayerCharacterBase>(GetOwner()))
	{
		InteractIconWidget->SetVisibility(true);
	}



	if (OtherActor && OtherActor != GetOwner())
	{

		UE_LOG(LogTemp, Log, TEXT("Overlapped with: %s"), *OtherActor->GetName());
		// You can store reference to item here
	}
}

void UPropInteractComponnet::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Character = Cast<APlayerCharacterBase>(GetOwner()))
	{

		
		InteractIconWidget->SetVisibility(false);
		
	}

	if (OtherActor && OtherActor != GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("Stopped overlapping with: %s"), *OtherActor->GetName());
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

		if (!InteractIconWidget)
		{
			InteractIconWidget = NewObject<UWidgetComponent>(GetOwner());
			InteractIconWidget->RegisterComponent();
			InteractIconWidget->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			InteractIconWidget->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
			InteractIconWidget->SetWidgetSpace(EWidgetSpace::Screen);
			InteractIconWidget->SetVisibility(false);
			InteractIconWidget->SetWidgetClass(InteractIconWidgetClass);
		}
	}

	// ...
	
}


// Called every frame
void UPropInteractComponnet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

