// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitBoxBufferComponent.generated.h"


USTRUCT(BlueprintType)
struct FHitBoxData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHitBoxData()
		: normalVector(FVector(0, 0, 1)), Location_End(FVector(0, 0, 0)), Location_Start(FVector(0, 0, 0)), HasTriggered(false), Index(0)
	{
	}
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector normalVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location_End;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool HasTriggered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Index;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UHitBoxBufferComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHitBoxBufferComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamageBox")
	TArray<FHitBoxData> array_HitboxBuffer;

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void Clear();

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void Initialize(TArray<FHitBoxData> gaArray);

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void UpdateTrigger(int index, bool trigger);


};
