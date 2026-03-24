// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"

#include "YogPlayerControllerBase.generated.h"


class AYogCameraPawn;
class AYogCharacterBase;
class UInputMappingContext;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerHasInput, const FVector, Velocity);

UCLASS()
class DEVKIT_API AYogPlayerControllerBase : public AModularPlayerController
{
	GENERATED_BODY()

public:
	AYogPlayerControllerBase(){}

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

	//UFUNCTION(BlueprintCallable)
	//AYogCharacterBase* GetPossCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TSubclassOf<AYogCameraPawn> CameraPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;


	UFUNCTION(BlueprintCallable)
	void OnInteractTriggered(const AItemSpawner* item);

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Character|Movement")
	FPlayerHasInput OnPlayerHasInput;



	UFUNCTION(BlueprintCallable)
	void ToggleInput(bool bEnable);


	UFUNCTION(BlueprintCallable, Category = "ASC")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;


	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SetEnableRotationRate(FRotator RotationRate, bool isEnable);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SpawnCameraPawn(AYogCharacterBase* PossessedCharacter);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	AYogCharacterBase* GetControlledCharacter();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetPlayerState(EYogCharacterState newState);

};
