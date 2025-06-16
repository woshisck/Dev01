// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitBoxBufferComponent.generated.h"

struct FHitBoxData;

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
	void FlushHitBoxArray();

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void SyncHitBoxArray(TArray<FHitBoxData> gaArray);

	UFUNCTION(BlueprintCallable, Category = "Buffer")
	void UpdateTrigger(int index, bool trigger);


};
