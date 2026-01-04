// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEffectComponent.h"

#include "DevKit/AbilitySystem/GameplayEffect/YogGameplayEffect.h"
#include "DevKit/Character/YogCharacterBase.h"

// Sets default values for this component's properties
UGameEffectComponent::UGameEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UGameEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UGameEffectComponent::HealthCheck()
{
	AYogCharacterBase* owner = Cast<AYogCharacterBase>(GetOwner());
	if (owner)
	{
		UAttributeStatComponent* attribute_comp = Cast<UAttributeStatComponent>(owner->GetComponentByClass(UAttributeStatComponent::StaticClass()));

	}
}

void UGameEffectComponent::AttackCheck()
{

}

void UGameEffectComponent::MoveSpeedCheck()
{

}

UAttributeStatComponent* UGameEffectComponent::GetOwnerAttributeStateComp()
{
	AYogCharacterBase* owner = Cast<AYogCharacterBase>(GetOwner());
	if (owner)
	{
		UAttributeStatComponent* attribute_comp = Cast<UAttributeStatComponent>(owner->GetComponentByClass(UAttributeStatComponent::StaticClass()));
		return attribute_comp;
	}
	else
	{
		return nullptr;
	}

}