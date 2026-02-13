// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/GameplayTagRelation.h"
#include "DevAssetManager.h"

UGameplayTagRelation::UGameplayTagRelation()
{
}

const UGameplayTagRelation& UGameplayTagRelation::UGameplayTagRelation::Get()
{

	return UDevAssetManager::Get().GetGameplayTagRelation();
	// TODO: insert return statement here
}
