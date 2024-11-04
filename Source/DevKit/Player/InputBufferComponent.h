// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "InputBufferComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
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
	FVector2D GetLastFrameInput(TArray<FVector2D> MoveBuffer);

	UPROPERTY(BlueprintReadWrite, Category="Input buffer")
	TArray<FVector2D> MoveBuffer;
		
	UPROPERTY(BlueprintReadWrite, Category = "Input buffer")
	int32 MAX_MOVE_BUFFER_SIZE = 20;

	
};
