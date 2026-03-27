// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BufferComponent.generated.h"


class UYogGameplayEffect;

UENUM(BlueprintType)
enum class EInputCommandType : uint8
{
	LightAttack,
	HeavyAttack,
	Dash,
	Move
};

USTRUCT(BlueprintType)
struct FInputCommand
{
	GENERATED_BODY()

	UPROPERTY()
	EInputCommandType CommandType;

	// Optional data (only used for Move)
	UPROPERTY()
	FVector2D MoveDirection;

	FInputCommand()
		: CommandType(EInputCommandType::LightAttack), MoveDirection(FVector2D::ZeroVector)
	{
	}

	FInputCommand(EInputCommandType InType)
		: CommandType(InType), MoveDirection(FVector2D::ZeroVector)
	{
	}

	FInputCommand(EInputCommandType InType, const FVector2D& InDirection)
		: CommandType(InType), MoveDirection(InDirection)
	{
	}

	FString ToString() const
	{
		switch (CommandType)
		{
		case EInputCommandType::LightAttack:
			return TEXT("LightAttack");
		case EInputCommandType::HeavyAttack:
			return TEXT("HeavyAttack");
		case EInputCommandType::Dash:
			return TEXT("Dash");
		case EInputCommandType::Move:
			return FString::Printf(TEXT("Move: X=%f, Y=%f"), MoveDirection.X, MoveDirection.Y);
		default:
			return TEXT("Unknown");
		}
	}
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

	void PushCommand(const FInputCommand& Command);
	FString CommandToString(const FInputCommand& Command);


	//TODO: Change to the specific game effect for player
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UYogGameplayEffect*> BufferArray;



private:
	TArray<FInputCommand> InputCommandHistory;


};
