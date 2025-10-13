// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacterBase.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "Character/YogCharacterMovementComponent.h"
#include "DevKit/Controller/YogAIController.h"

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();



}

void AEnemyCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AEnemyCharacterBase::Die()
{
	Super::Die();

}

void AEnemyCharacterBase::SetupAI(UBehaviorTree* bt, UBlackboardData* bb)
{
	AYogAIController* controller = Cast<AYogAIController>(GetController());

	controller
}

