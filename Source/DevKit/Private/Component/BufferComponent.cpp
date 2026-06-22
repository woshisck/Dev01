// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BufferComponent.h"
#include "AbilitySystem/GameplayEffect/YogGameplayEffect.h"

// Sets default values for this component's properties
UBufferComponent::UBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UBufferComponent::RecordAttack()
{
	PushCommand(FInputCommand(EInputCommandType::Attack, GetWorld()->GetTimeSeconds()));
}

void UBufferComponent::RecordWeaponSkill()
{
	PushCommand(FInputCommand(EInputCommandType::WeaponSkill, GetWorld()->GetTimeSeconds()));
}

void UBufferComponent::RecordDash()
{
	PushCommand(FInputCommand(EInputCommandType::Dash, GetWorld()->GetTimeSeconds()));
}

void UBufferComponent::RecordSpecial()
{
	PushCommand(FInputCommand(EInputCommandType::Special, GetWorld()->GetTimeSeconds()));
}

void UBufferComponent::RecordMove(const FVector2D& Direction)
{
	PushCommand(FInputCommand(EInputCommandType::Move, Direction, GetWorld()->GetTimeSeconds()));
}

bool UBufferComponent::HasBufferedInput(EInputCommandType Type, float TimeWindow) const
{
	const float Now = GetWorld()->GetTimeSeconds();
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		const FInputCommand& Cmd = InputCommandHistory[i];
		if (Cmd.CommandType == Type && (Now - Cmd.Timestamp) <= TimeWindow)
		{
			return true;
		}
	}
	return false;
}

bool UBufferComponent::HasBufferedInputSince(EInputCommandType Type, float SinceTime) const
{
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		const FInputCommand& Cmd = InputCommandHistory[i];
		if (Cmd.CommandType == Type && Cmd.Timestamp > SinceTime)
		{
			return true;
		}
	}
	return false;
}

bool UBufferComponent::ConsumeBufferedInput(EInputCommandType Type)
{
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		FInputCommand& Cmd = InputCommandHistory[i];
		if (Cmd.CommandType == Type)
		{
			// 标记为极旧时间戳使其在时间窗口外，等效"消耗"
			Cmd.Timestamp = -9999.0f;
			return true;
		}
	}
	return false;
}

bool UBufferComponent::ConsumeBufferedInputSince(EInputCommandType Type, float SinceTime)
{
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		FInputCommand& Cmd = InputCommandHistory[i];
		if (Cmd.CommandType == Type && Cmd.Timestamp > SinceTime)
		{
			Cmd.Timestamp = -9999.0f;
			return true;
		}
	}
	return false;
}

bool UBufferComponent::ConsumeLatestAttackInputSince(float SinceTime, EInputCommandType& OutType)
{
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		FInputCommand& Cmd = InputCommandHistory[i];
		if (Cmd.CommandType == EInputCommandType::Attack && Cmd.Timestamp > SinceTime)
		{
			OutType = Cmd.CommandType;
			Cmd.Timestamp = -9999.0f;
			return true;
		}
	}
	return false;
}

bool UBufferComponent::ConsumeLatestActionInputSince(float SinceTime, EInputCommandType& OutType)
{
	for (int32 i = InputCommandHistory.Num() - 1; i >= 0; --i)
	{
		FInputCommand& Cmd = InputCommandHistory[i];
		const bool bIsAction =
			Cmd.CommandType == EInputCommandType::Attack ||
			Cmd.CommandType == EInputCommandType::WeaponSkill ||
			Cmd.CommandType == EInputCommandType::Dash ||
			Cmd.CommandType == EInputCommandType::Special;
		if (bIsAction && Cmd.Timestamp > SinceTime)
		{
			OutType = Cmd.CommandType;
			Cmd.Timestamp = -9999.0f;
			return true;
		}
	}
	return false;
}

void UBufferComponent::ClearBuffer()
{
	InputCommandHistory.Empty();
}


// Called when the game starts
void UBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

UYogGameplayEffect* UBufferComponent::GetItemAt(int index)
{
	return BufferArray[index];
}

void UBufferComponent::MoveToNextItem()
{

	if (CurrentIndex == BufferArray.Num())
	{
		CurrentIndex = 0;
	}
	else
	{
		CurrentIndex++;
	}
}

void UBufferComponent::PushCommand(const FInputCommand& Command)
{
	// If we already have 20 elements, remove the oldest one (index 0)
	if (InputCommandHistory.Num() >= 20)
	{
		InputCommandHistory.RemoveAt(0);
	}

	// Add the new command at the end
	InputCommandHistory.Add(Command);
}

// Called every frame
void UBufferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

FString UBufferComponent::CommandToString(const FInputCommand& Command)
{
	switch (Command.CommandType)
	{
	case EInputCommandType::Attack:
		return TEXT("Attack");
	case EInputCommandType::WeaponSkill:
		return TEXT("WeaponSkill");
	case EInputCommandType::Dash:
		return TEXT("Dash");
	case EInputCommandType::Special:
		return TEXT("Special");
	case EInputCommandType::Move:
		return FString::Printf(TEXT("Move: X=%f, Y=%f"), Command.MoveDirection.X, Command.MoveDirection.Y);
	default:
		return TEXT("Unknown");
	}
}

