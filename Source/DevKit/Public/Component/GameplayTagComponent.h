// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GameplayTagComponent.generated.h"

struct FGameplayTag;



UCLASS(BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UGameplayTagComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameplayTagComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


public:

	UFUNCTION(BlueprintCallable)
	void TryAddGameplayTag(FGameplayTag tag);
	
	UFUNCTION(BlueprintCallable)
	void TryRemoveGameplayTag(FGameplayTag tag);

	bool HasTag(FGameplayTag tag);

private:

};
