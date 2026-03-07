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
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer  OverwriteListTags;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer  BlackListTags;


};



UCLASS(BlueprintType, Const, Meta = (DisplayName = "game tag relation Data", ShortTooltip = "game tag relation in global "))
class DEVKIT_API UGameplayTagRelation : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

public:

	UGameplayTagRelation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ForceInlineRow), Category = "Action|General")
	TMap<FGameplayTag, FGameTagRelationConfig> GameTagRelationRow;


	static const UGameplayTagRelation& Get();

	//TMap<FYogTagContainerWrapper, FActionData> AbilityMap;
};
