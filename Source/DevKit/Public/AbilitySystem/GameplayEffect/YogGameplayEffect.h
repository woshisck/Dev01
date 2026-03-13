// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "YogGameplayEffect.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EGameEffectedOnActor : uint8
{
	OnTarget				UMETA(DisplayName = "OnTarget"),
	OnSelf					UMETA(DisplayName = "OnSelf")

};




UCLASS()
class DEVKIT_API UYogGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()
	
public:

	UYogGameplayEffect();

	UFUNCTION(BlueprintCallable)
	void CreateOwnEffectContext(AActor* TargetActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGameEffectedOnActor EffectType = EGameEffectedOnActor::OnTarget;


protected:
	// Store the effect context.  Crucial for passing it along.
	FGameplayEffectContextHandle EffectContextHandle;



};
