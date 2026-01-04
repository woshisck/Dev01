// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "Components/ActorComponent.h"
#include "DevKit/Buff/PlayEffectDefinition.h"
#include "GameEffectComponent.generated.h"


class UYogGameplayEffect;
class UPlayEffectDefinition;
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UGameEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameEffectComponent();

	// Called when the game starts
	virtual void BeginPlay() override;


	UFUNCTION()
	void HealthCheck();


	UFUNCTION()
	void AttackCheck();

	UFUNCTION()
	void MoveSpeedCheck();




	UFUNCTION(BlueprintCallable)
	UAttributeStatComponent* GetOwnerAttributeStateComp();
		
};
