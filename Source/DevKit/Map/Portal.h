// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogMapDefinition.h"
#include "Portal.generated.h"




UCLASS()
class DEVKIT_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortal();

	UFUNCTION(BlueprintImplementableEvent)
	void EnablePortal();

	UFUNCTION(BlueprintImplementableEvent)
	void DisablePortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Index;

	UPROPERTY(BlueprintReadOnly)
	TArray<FNextMapNode> NextLevels;

};
