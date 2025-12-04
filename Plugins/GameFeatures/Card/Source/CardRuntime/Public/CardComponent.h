// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardData.h"
#include "CardComponent.generated.h"

class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardPopSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardShuffleSignature);


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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int CardPoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int DeckSize;


	UFUNCTION(BlueprintCallable)
	void Additem();

	UFUNCTION(BlueprintCallable)
	void RemoveItem();

	UFUNCTION(BlueprintCallable)
	void InitDeck();

	UFUNCTION(BlueprintCallable)
	void Pop();

	UFUNCTION(BlueprintCallable)
	void Shuffle(UPARAM(ref) TArray<FCardProperty>& cards);

	UFUNCTION(BlueprintCallable)
	void FillPool(UCardData* card_data_pool);


	UFUNCTION(BlueprintCallable)
	void MoveCardAtIndex(UPARAM(ref) TArray<FCardProperty>& Source, UPARAM(ref) TArray<FCardProperty>& Dest, int32 index);

	UFUNCTION(BlueprintCallable)
	void PrintCardPool();

	UFUNCTION(BlueprintCallable)
	void PrintDeck();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


};


template<typename T>
void MoveItemAtIndex(UPARAM(ref) TArray<T>& Source, UPARAM(ref) TArray<T>& Dest, int32 index)
{
	if (Source.IsValidIndex(index))
	{
		Dest.Add(Source[index]);
		Source.RemoveAtSwap(index);      // Faster: swaps with last element
	}
}

