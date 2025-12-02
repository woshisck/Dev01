// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardData.h"
#include "CardComponent.generated.h"

class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardPopSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardShuffleSignature);




USTRUCT(BlueprintType)
struct FCardProperty
{
	GENERATED_BODY()

	// Transform in pivot space (*not* texture space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UGameplayEffect>> CardEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	CardEffectTarget Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DisplayName;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARDRUNTIME_API UCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCardComponent();


	UPROPERTY(BlueprintAssignable)
	FCardPopSignature Event_OnCardPopSignature;

	UPROPERTY(BlueprintAssignable)
	FOnCardShuffleSignature Event_OnCardShuffleSignature;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardProperty> CardPresent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardProperty> CardDeck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardProperty> CardPool;


	UFUNCTION()
	void Additem();

	UFUNCTION()
	void RemoveItem();

	UFUNCTION()
	void InitDeck();

	UFUNCTION()
	void Pop();

	UFUNCTION()
	void Shuffle();



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


};
