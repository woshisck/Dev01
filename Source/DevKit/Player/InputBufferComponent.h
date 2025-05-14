// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "InputBufferComponent.generated.h"



UENUM(BlueprintType)
enum class EPlayerActionInput : uint8
{
	HeavyAttack,
	LightAttack,
	Dash,
	Projectile
};


UCLASS(Blueprintable)
class DEVKIT_API UInputBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInputBufferComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void UpdateMoveBuffer(FVector2D move);

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void UpdateActionBuffer(EPlayerActionInput actionInput);

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	FVector2D GetLastMoveInput(FVector2D Movement);

		

	//BUFFER ARRAY DEFINE
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<EPlayerActionInput> ActionBuffer;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input buffer")
	TArray<FVector2D> MovementBuffer;




	UPROPERTY(BlueprintReadOnly, Category = "Input buffer")
	int32 MAX_BUFFER_SIZE = 10;

	UFUNCTION(BlueprintCallable, Category = "Act Buff")
	void ClearActionBuffer();


	UFUNCTION(BlueprintCallable, Category = "Move Buff")
	void ClearMovementBuffer();
	
	UFUNCTION(BlueprintCallable)
	void DebugPrintAction();

	UFUNCTION(BlueprintCallable)
	void DebugPrintMovement();

	template<typename T>
	void ClearBuffer(TArray<T> TargetArray)
	{
		return TargetArray.Empty();
	}


	template<typename T>
	T GetLastItem(TArray<T> TargetArray)
	{
		return TargetArray.Pop();
	}


	template<typename T>
	T GetFirstItem(TArray<T> TargetArray)
	{
		return TargetArray.Pop();
	}

};
