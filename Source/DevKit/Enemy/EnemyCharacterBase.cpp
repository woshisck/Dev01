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

	UEnemyData* enemyData = Cast<UEnemyData>(CharacterData);
	if (enemyData)
	{
		InitEnemyData(enemyData);
	}

	//if (enemyData)
	//{
	//	UBehaviorTree* behaviour_tree = enemyData->EnemyBT;
	//	AAIController* controller = Cast<AAIController>(this->GetController());
	//	if (controller)
	//	{
	//		controller->RunBehaviorTree(behaviour_tree);
	//	}
	//}
}

void AEnemyCharacterBase::SetupAI(UBehaviorTree* bt, UBlackboardData* bb)
{
}

void AEnemyCharacterBase::InitEnemyData(UEnemyData* enemy_data)
{
	for (FDataTableRowHandle& action_row : enemy_data->ActionRows)
	{

		if (!action_row.IsNull())
		{
			FActionData* actionData = action_row.GetRow<FActionData>(__func__);
			ensure(actionData && actionData->Ability_Template);
			//check(Ability_Class)
			
			UYogAbilitySystemComponent* ASC = this->GetASC();
			
			FGameplayAbilitySpec abilitySpec(actionData->Ability_Template,0);
			FGameplayAbilitySpecHandle ability_handle = ASC->GiveAbility(abilitySpec);
			
			if (!ability_handle.IsValid())
			{
				return;
			}

			if (UYogGameplayAbility* GrantedAbility = Cast<UYogGameplayAbility>(ASC->FindAbilitySpecFromHandle(ability_handle)->Ability))
			{
				//GrantedAbility->SetupActionData(*actionData);

				GrantedAbility->ActDamage = actionData->ActDamage;
				GrantedAbility->ActRange = actionData->ActRange;
				GrantedAbility->ActResilience = actionData->ActResilience;
				GrantedAbility->ActRotateSpeed = actionData->ActRotateSpeed;
				GrantedAbility->JumpFrameTime = actionData->JumpFrameTime;
				GrantedAbility->FreezeFrameTime = actionData->FreezeFrameTime;
				GrantedAbility->Montage = actionData->Montage;

				GrantedAbility->hitbox.SetNumUninitialized(actionData->hitbox.Num());
				FMemory::Memcpy(GrantedAbility->hitbox.GetData(), actionData->hitbox.GetData(), actionData->hitbox.Num() * sizeof(FHitBoxData));

			}
		}
	}
}
