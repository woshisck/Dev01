// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DevKit/Card/CardData.h"
#include "CardComponent.generated.h"

class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardPopSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardShuffleSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardAddSignature, FCardProperty, cardProperty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardFillPoolSignature, int, CardNum);

//DECLARE_DYNAMIC_DELEGATE_OneParam(FStringParamCallback, const FString&, Value);



UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCardComponent();


	UPROPERTY(BlueprintAssignable)
	FCardPopSignature Event_OnCardPopSignature;

	UPROPERTY(BlueprintAssignable)
	FOnCardShuffleSignature Event_OnCardShuffleSignature;

	UPROPERTY(BlueprintAssignable)
	FOnCardAddSignature Event_OnCardAddSignature;

	UPROPERTY(BlueprintAssignable)
	FOnCardFillPoolSignature Event_OnCardFillPoolSignature;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCardData> CardData;

	UFUNCTION(BlueprintCallable)
	FCardProperty GetWeightedRandomCard(const TArray<FCardProperty>& card_properties);

	UFUNCTION(BlueprintCallable)
	void Additem();

	UFUNCTION(BlueprintCallable)
	void RemoveItem();

	UFUNCTION(BlueprintCallable)
	void InitDeck();

	UFUNCTION(BlueprintCallable)
	FCardProperty PopAtFirst();

	UFUNCTION(BlueprintCallable)
	void Shuffle(UPARAM(ref) TArray<FCardProperty>& cards);

	UFUNCTION(BlueprintCallable)
	void FillPool(UCardData* card_data_pool);

	UFUNCTION(BlueprintCallable)
	void FillPoolWithRareDistribution(UCardData* card_data_pool);

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

