// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


#include "Components/ActorComponent.h"
//#include "DevKit/Buff/BuffElement.h"
#include "GameEffectComponent.generated.h"


class UYogGameplayEffect;
class UBuffElement;
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UGameEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGameEffectComponent();

	UFUNCTION(BlueprintCallable)
	TSubclassOf<UBuffElement> GetItemAt(int index);

	UPROPERTY(BlueprintReadOnly)
	int CurrentIndex = 0;

	UFUNCTION(BlueprintCallable)
	void MoveToNextItem();


	UFUNCTION(BlueprintCallable)
	bool RemoveItemByIndex(int index);


	UFUNCTION(BlueprintCallable)
	bool RemoveItem(TSubclassOf<UBuffElement> BuffToRemove);

	UFUNCTION(BlueprintCallable)
	bool AddItem(TSubclassOf<UBuffElement> buff);

	UFUNCTION(BlueprintCallable)
	void ClearAll();

	UFUNCTION(BlueprintCallable)
	int GetBuffCount();

	UPROPERTY(BlueprintReadOnly)
	TArray<TSubclassOf<UBuffElement>> BuffArray;


	// Called when the game starts
	virtual void BeginPlay() override;

	//TODO: Change to the specific game effect for player




		
};
