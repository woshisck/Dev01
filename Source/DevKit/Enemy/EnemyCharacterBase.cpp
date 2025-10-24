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
	

	static ConstructorHelpers::FClassFinder<UYogGameplayAbility> Ability_Blueprint_Class(TEXT("Blueprint'/Game/Code/Weapon/GA_MobAbility'"));
	if (Ability_Blueprint_Class.Succeeded())
	{
		UClass* MyActorClass = Ability_Blueprint_Class.Class.Get();
		Ability_Class = Ability_Blueprint_Class.Class.Get();
		UE_LOG(LogTemp, Warning, TEXT("class name:%s"), *Ability_Class->GetName());
	}
	 

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

	if (EnemyData) 
	{
		InitEnemyData(EnemyData);
	}


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

	if (CharacterData)
	{
		const FMovementData& moveData = CharacterData->GetMovementData();
		const FYogBaseAttributeData& characterData = CharacterData->GetBaseAttributeData();



		BaseAttributeSet->Init(CharacterData);
	}

	if (AbilitySystemComponent)
	{

		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);
	}



	if (EnemyData)
	{

		UBehaviorTree* behaviour_tree = EnemyData->EnemyBT;

		AAIController* controller = Cast<AAIController>(this->GetController());
		if (controller)
		{
			controller->RunBehaviorTree(behaviour_tree);
		}

		for (FDataTableRowHandle& data_row : EnemyData->ActionRows)
		{
			if (!data_row.IsNull())
			{
				FActionData* action_data = data_row.GetRow<FActionData>(__func__);
				if (action_data)
				{

				}
			}
		}




	}

}

void AEnemyCharacterBase::SetupAI(UBehaviorTree* bt, UBlackboardData* bb)
{
}

void AEnemyCharacterBase::InitEnemyData(UEnemyData* enemy_data)
{
	for (auto action_row : enemy_data->ActionRows)
	{

		if (!action_row.IsNull())
		{
			FActionData* actionData = action_row.GetRow<FActionData>(__func__);


			ensure(actionData);
			UYogAbilitySystemComponent* ASC = this->GetASC();
			check(Ability_Class)
			FGameplayAbilitySpec abilitySpec(Ability_Class,0);
			FGameplayAbilitySpecHandle ability_handle = ASC->GiveAbility(abilitySpec);
			
			if (ability_handle.IsValid())
			{
				if (UYogGameplayAbility* GrantedAbility = Cast<UYogGameplayAbility>(ASC->FindAbilitySpecFromHandle(ability_handle)->Ability))
				{
					GrantedAbility->SetupActionData(*actionData);
				}
			}

			//if (abilitySpec.Ability && abilitySpec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
			//{
			//	UYogGameplayAbility* AbilityInstance = Cast<UYogGameplayAbility>(abilitySpec.GetPrimaryInstance());
			//	ensure(AbilityInstance);
			//	//if (ensureMsgf(AbilityInstance, TEXT("NOT Fount correct abilityInstance from EnemyData")))
			//	AbilityInstance->SetupActionData(*actionData);
			//}


		}

	}
}

UPROPERTY()
float ActDamage = 20;

UPROPERTY()
float ActRange = 400;

UPROPERTY()
float ActResilience = 20;

UPROPERTY()
float ActDmgReduce = 0;

UPROPERTY()
float ActRotateSpeed = 360;

UPROPERTY()
float JumpFrameTime = 0.15;

UPROPERTY()
float FreezeFrameTime = 0.15;

UPROPERTY()
TObjectPtr<UAnimMontage> Montage;