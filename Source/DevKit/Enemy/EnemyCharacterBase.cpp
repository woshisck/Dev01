// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyCharacterBase.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "Character/YogCharacterMovementComponent.h"
#include "DevKit/Data/EnemyData.h"
#include "DevKit/Controller/YogAIController.h"

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	EnemyAttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));
	

	//static ConstructorHelpers::FClassFinder<UYogGameplayAbility> Ability_Blueprint_Class(TEXT("Blueprint'/Game/Code/Weapon/GA_MobAbility'"));
	//if (Ability_Blueprint_Class.Succeeded())
	//{
	//	UClass* MyActorClass = Ability_Blueprint_Class.Class.Get();
	//	Ability_Class = Ability_Blueprint_Class.Class.Get();
	//	UE_LOG(LogTemp, Warning, TEXT("class name:%s"), *Ability_Class->GetName());
	//}
	 

	//Script / Engine.Blueprint'/Game/Code/Weapon/GA_MobAbility.GA_MobAbility'
	//static ConstructorHelpers::FClassFinder<AActor> BlueprintClassFinder(TEXT("/Game/Code/Weapon/GA_WeaponAtk.GA_WeaponAtk_C"));
	//if (BlueprintClassFinder.Succeeded())
	//{
	//	Ability_Class = BlueprintClassFinder.Class;
	//}


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

void AEnemyCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}


