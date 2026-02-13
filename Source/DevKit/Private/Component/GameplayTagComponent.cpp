// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/GameplayTagComponent.h"
#include "GameplayEffect.h"

#include "Data/GameplayTagRelation.h"
#include "DevAssetManager.h"
#include "DevAssetManager.h"


// Sets default values for this component's properties
UGameplayTagComponent::UGameplayTagComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UGameplayTagComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UGameplayTagComponent::TryAddGameplayTag(FGameplayTag tag)
{
	const FGameTagRelationConfig* tagConfig = UGameplayTagRelation::Get().GameTagRelationRow.Find(tag);
	
	//tagConfig->Priority

	//const int32& Priority = UGameplayTagRelation::Get()


	//const FGameTagRelationConfig* const_game_tag_relation = FindTagRelationInData(tag);	
}

void UGameplayTagComponent::TryRemoveGameplayTag(FGameplayTag tag)
{
	//const FGameTagRelationConfig* const_game_tag_relation = FindTagRelationInData(tag);
	//AYogCharacterBase* owner = Cast<AYogCharacterBase>(GetOwner());

	//if (owner)
	//{
	//	UYogAbilitySystemComponent* asc = owner->GetASC();
	//}
}

bool UGameplayTagComponent::HasTag(FGameplayTag tag)
{
	return false;
}


//const FGameTagRelationConfig* UGameplayTagComponent::FindTagRelationInData(FGameplayTag tag)
//{
//	UDevAssetManager* DevAssetManager = UDevAssetManager::Get();
//	const UGameplayTagRelation& GameTagRelation = DevAssetManager->GetGameplayTagRelation();
//	const FGameTagRelationConfig* const_game_tag_relation = GameTagRelation.GameTagRelationRow.Find(tag);
//	return const_game_tag_relation;
//}



