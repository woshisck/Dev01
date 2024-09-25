// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EnhancedInputComponent.h"
#include "InputTriggers.h"
#include "Components/PawnComponent.h"
#include "GSCPlayerControlsComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UInputAction;

/**
 * Modular pawn component for adding input actions and an optional input mapping to a pawn
 *
 * If your Pawn is dealing with Abilities, use GSCAbilityInputBindingComponent instead.
 *
 * Not meant to be added in Blueprints but rather use as a base class (parent of GSCAbilityInputBindingComponent)
 */
UCLASS(ClassGroup="GASCompanion", Category = "Input")
class GASCOMPANION_API UGSCPlayerControlsComponent : public UPawnComponent
{
	GENERATED_BODY()
public:
	//~ Begin UActorComponent interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent interface

	/** Input mapping to add to the input system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Controls")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	/** Priority to bind mapping context with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Controls")
	int32 InputPriority = 0;

protected:

	/** Native/BP Event to set up player controls */
	UFUNCTION(BlueprintNativeEvent, Category = "Player Controls")
	void SetupPlayerControls(UEnhancedInputComponent* PlayerInputComponent);

	/** Native/BP Event to undo control setup */
	UFUNCTION(BlueprintNativeEvent, Category = "Player Controls")
	void TeardownPlayerControls(UEnhancedInputComponent* PlayerInputComponent);

	/** Wrapper function for binding to this input component */
	template<class UserClass, typename FuncType>
	bool BindInputAction(const UInputAction* Action, const ETriggerEvent EventType, UserClass* Object, FuncType Func)
	{
		if (ensure(InputComponent != nullptr) && ensure(Action != nullptr))
		{
			InputComponent->BindAction(Action, EventType, Object, Func);
			return true;
		}

		return false;
	}

	/** Called when pawn restarts, bound to dynamic delegate */
	UFUNCTION()
	virtual void OnPawnRestarted(APawn* Pawn);

	/** Called when pawn restarts, bound to dynamic delegate */
	UFUNCTION()
	virtual void OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	virtual void SetupInputComponent(APawn* Pawn);
	virtual void ReleaseInputComponent(AController* OldController = nullptr);
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem(AController* OldController = nullptr) const;

	/** The bound input component. */
	UPROPERTY(transient)
	TObjectPtr<UEnhancedInputComponent> InputComponent;
};
