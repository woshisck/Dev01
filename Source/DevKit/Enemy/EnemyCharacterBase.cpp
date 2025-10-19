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
	
	//UClass
	static ConstructorHelpers::FObjectFinderOptional<UClass> Ability_Blueprint(TEXT("/Game/Code/Weapon/GA_WeaponAtk"));

	if (Ability_Blueprint.Succeeded())
	{

		Ability_Class = Ability_Blueprint.Get();
	}


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

void AEnemyCharacterBase::SetupAI(UBehaviorTree* bt, UBlackboardData* bb)
{
}

void AEnemyCharacterBase::AssignAbilities()
{
	for (auto& action_row : EnemyData->ActionRows)
	{

		if (!action_row.IsNull())
		{
			FActionData* actionData = action_row.GetRow<FActionData>(__func__);
			
			UYogAbilitySystemComponent* ASC = this->GetASC();
			check(Ability_Class)
			FGameplayAbilitySpec abilitySpec(Ability_Class, 0);
			ASC->GiveAbility(abilitySpec);

		}

	}
}

