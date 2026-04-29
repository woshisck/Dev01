// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphGameplayTasksComponent.h"
#include "ComboGraphSystemComponent.generated.h"

/**
 * Gameplay Task Actor Component designed to be added in Blueprints to Actors that wants to run Start Graph Gameplay Task outside of the regular GAS route and Gameplay Abilities.
 *
 * It is meant to be used for Actors not implementing IAbilitySystemInterface. On BeginPlay, this component will check owners for a valid Ability System Component
 * (either via the interface, or through component search) and if none is found, will spawn and register an ASC at runtime.
 *
 * Once done, this component will grant a generic internal ability UComboGraphNativeAbility (which is also not Blueprintable) that the gameplay task can use to run a specific combo graph.
 * UComboGraphNativeAbility provides a basic implementation of a typical gameplay ability to run a combo graph via the ability task, pulling the combo graph asset from Gameplay Event Data.
 */
UCLASS(ClassGroup=ComboGraph, hidecategories=(Object, LOD, Lighting, Transform, Sockets, TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class COMBOGRAPH_API UComboGraphSystemComponent : public UComboGraphGameplayTasksComponent
{
	GENERATED_BODY()

public:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

protected:
	virtual void OnRegister() override;

	/** Setup an Ability System Component on Owner if missing */
	virtual void SetupAbilitySystemComponent();
};
