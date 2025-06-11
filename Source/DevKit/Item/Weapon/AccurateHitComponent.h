// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AccurateHitComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DisplayName = "AC_DidItHit")
class DEVKIT_API UAccurateHitComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAccurateHitComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		

	//Boolean used during Tick
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool TriggerTrace = false;

	UFUNCTION()
	void AddHitResult(TArray<FHitResult> HitArrayToAdd);
	

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Socket_Start;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Socket_End;
};
