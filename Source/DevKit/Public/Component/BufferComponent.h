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

	// World time when this command was recorded (GetWorld()->GetTimeSeconds())
	UPROPERTY()
	float Timestamp = 0.0f;

	FInputCommand()
		: CommandType(EInputCommandType::LightAttack), MoveDirection(FVector2D::ZeroVector), Timestamp(0.0f)
	{
	}

	FInputCommand(EInputCommandType InType, float InTimestamp = 0.0f)
		: CommandType(InType), MoveDirection(FVector2D::ZeroVector), Timestamp(InTimestamp)
	{
	}

	FInputCommand(EInputCommandType InType, const FVector2D& InDirection, float InTimestamp = 0.0f)
		: CommandType(InType), MoveDirection(InDirection), Timestamp(InTimestamp)
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

	/**
	 * 在时间窗口内是否存在指定类型的缓存输入。
	 * @param Type      要检查的输入类型
	 * @param TimeWindow 向前追溯的秒数（默认 0.3s）
	 */
	UFUNCTION(BlueprintCallable, Category = "InputBuffer")
	bool HasBufferedInput(EInputCommandType Type, float TimeWindow = 0.3f) const;

	/**
	 * 是否存在在 SinceTime 之后记录的指定类型输入。
	 * 典型用途：CanCombo 窗口检查时，只接受本次能力激活之后的预输入，
	 * 避免触发当前能力的那次按键被误当成连击预输入。
	 * @param Type      要检查的输入类型
	 * @param SinceTime 只接受 Timestamp > SinceTime 的输入（传入能力激活时刻）
	 */
	UFUNCTION(BlueprintCallable, Category = "InputBuffer")
	bool HasBufferedInputSince(EInputCommandType Type, float SinceTime) const;

	/**
	 * 消耗（移除）最近一次匹配的缓存输入。
	 * 典型用途：连击触发时调用，防止同一次输入触发两次。
	 * @return 是否成功消耗（缓存中存在该输入则为 true）
	 */
	UFUNCTION(BlueprintCallable, Category = "InputBuffer")
	bool ConsumeBufferedInput(EInputCommandType Type);

	/** 清空所有缓存输入（整理阶段开始时调用，避免残留输入） */
	UFUNCTION(BlueprintCallable, Category = "InputBuffer")
	void ClearBuffer();

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
