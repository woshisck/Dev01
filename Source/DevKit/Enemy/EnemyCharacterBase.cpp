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
	static ConstructorHelpers::FClassFinder<AActor> Ability_Blueprint_Class(TEXT("Blueprint'/Game/Code/Weapon/GA_MobAbility'"));
	if (Ability_Blueprint_Class.Succeeded())
	{
		UClass* MyActorClass = Ability_Blueprint_Class.Class.Get();

		Ability_Class = Ability_Blueprint_Class.Class.Get();
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
		//TObjectPtr<UBehaviorTree> EnemyBT;
		//TArray<FDataTableRowHandle> ActionRows;
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
					

					//assign action ability

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float ActDamage = 20;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float ActRange = 400;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float ActResilience = 20;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float ActDmgReduce = 0;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float ActRotateSpeed = 360;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float JumpFrameTime = 0.15;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//float FreezeFrameTime = 0.15;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//TObjectPtr<UAnimMontage> Montage;

					////UPROPERTY(EditAnywhere, BlueprintReadWrite)
					////TSubclassOf<UYogGameplayAbility> Ability;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite)
					//TSubclassOf<UYogGameplayAbility> Ability_Template;

					//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
					//TArray<FHitBoxData> hitbox;

				}
			}
		}




	}

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

