// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"

#include "GameplayTagRelation.generated.h"


USTRUCT(BlueprintType)
struct FGameTagRelationConfig
{
	GENERATED_BODY()

public:
	FGameTagRelationConfig() {}


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Priority;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> ExclusiveTags;


};



UCLASS()
class DEVKIT_API UGameplayTagRelation : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Action|General")
	TMap<FGameplayTag, FGameTagRelationConfig> TagRelations;


	//TMap<FYogTagContainerWrapper, FActionData> AbilityMap;
};
