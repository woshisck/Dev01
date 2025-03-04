// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSpawner.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Itemdefinition.h"
#include "../Character/YogBaseCharacter.h"
#include "../AbilitySystem/YogAbilitySystemComponent.h"

//#include "GameFramework/Pawn.h"
//#include "NiagaraFunctionLibrary.h"
//#include "NiagaraSystem.h"
//#include "TimerManager.h"
//#include "GameplayEffect.h"
//#include "GameFramework/Pawn.h"


AItemSpawner::AItemSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CollisionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionVolume"));

	CollisionVolume->InitCapsuleSize(80.f, 80.f);
	CollisionVolume->OnComponentBeginOverlap.AddDynamic(this, &AItemSpawner::OnOverlapBegin);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	ItemMesh->SetupAttachment(RootComponent);

}

void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AItemSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void AItemSpawner::Tick(float DeltaTime)
{
}

void AItemSpawner::OnConstruction(const FTransform& Transform)
{	


	if (ItemDefinition != nullptr && ItemDefinition->DisplayMesh != nullptr)
	{	//Visual stat
		ItemMesh->SetStaticMesh(ItemDefinition->DisplayMesh);
		ItemMesh->SetRelativeLocation(ItemDefinition->ItemMeshOffset);
		ItemMesh->SetRelativeScale3D(ItemDefinition->ItemMeshScale);
		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NO FOUND ITEM DEFINITION!, name: %s"), *GetNameSafe(this));
	}
}

void AItemSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{

	AYogBaseCharacter* OverlappingCharacter = Cast<AYogBaseCharacter>(OtherActor);
	if (OverlappingCharacter != nullptr)
	{


		if (this->ItemDefinition->GrantEffectContainerMap.Num() > 0)
		{
			UYogAbilitySystemComponent* ASC = OverlappingCharacter->GetYogAbilitySystemComponent();

			TArray<FGameplayAbilitySpec>& ActiviableAbilities = ASC->GetActivatableAbilities();

			for (FGameplayAbilitySpec& GameplayAbilitySpec : ActiviableAbilities)
			{
				
				if (GameplayAbilitySpec.Ability)
				{
					UYogGameplayAbility* ability = Cast<UYogGameplayAbility>(GameplayAbilitySpec.Ability);

					if (ability)
					{
						ability->IncrementInternalNum();
					}
				}

			}
			
			//for (TPair<FGameplayTag, FYogGameplayEffectContainer>& pair : this->ItemDefinition->GrantEffectContainerMap)
			//{
			//	

			//}
		}

		UE_LOG(LogTemp, Warning, TEXT("ItemSpawner::OnOverlapBegin"));
		this->Destroy();

	}

}
