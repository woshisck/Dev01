// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BufferComponent.generated.h"


class UYogGameplayEffect;

UENUM(BlueprintType)
enum class InputCommand : uint8
{
	LightAttack,
	HeavyAttack,
	Dash
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBufferComponent();

	void RecordLightAttack();
	void RecordHeavyAttack();
	void RecordDash();
	void RecordMove(const FVector2D& Direction);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int CurrentIndex;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	UYogGameplayEffect* GetItemAt(int index);

	UFUNCTION(BlueprintCallable)
	void MoveToNextItem();

	//TODO: Change to the specific game effect for player
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UYogGameplayEffect*> BufferArray;

private:
	TArray<FString> InputCommandHistory;
};
