// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/EnemyTrait.h"

void UEnemyTrait::ActivateTrait(AEnemyCharacterBase* Owner) const
{
	K2_OnActivated(Owner);
}

void UEnemyTrait::DeactivateTrait(AEnemyCharacterBase* Owner) const
{
	K2_OnDeactivated(Owner);
}
