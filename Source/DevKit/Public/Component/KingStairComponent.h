// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystem/GameplayEffect/YogGameplayEffect.h"


#include "KingStairComponent.generated.h"

class UGameplayEffect;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCardPopSignature);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardShuffleSignature);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardAddSignature, FCardProperty, cardProperty);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardFillPoolSignature, int, CardNum);

//DECLARE_DYNAMIC_DELEGATE_OneParam(FStringParamCallback, const FString&, Value);

USTRUCT(BlueprintType)
struct FKingStairNode
{
	GENERATED_BODY()
public:
	FKingStairNode()
	{
		//LevelPackageName = "DefaultNode";
		//LevelMapSoftPath = FSoftObjectPath(TEXT("/Game/Maps/Dungeon/RootNode.RootNode"));

		//NodeType = ESublevelType::Default;

	};

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//ESublevelType NodeType;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsSelected;



	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayEffect> Gameplayeffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level;


	void setNodeEffect(TSubclassOf<UYogGameplayEffect> gameplayeffect, int32 level)
	{
		this->Gameplayeffect = gameplayeffect;
		this->Level = level;
	}

};


USTRUCT(BlueprintType)
struct FKingStairRow
{
	GENERATED_BODY()

public:


	FKingStairRow() {};

	FKingStairRow(int32 x, int32 y)
		:row_length(x)
	{

		for (int i = 0; i < x; i++)
		{
			FKingStairNode node = FKingStairNode();
			node.x = i;
			node.y = y;
			NodeItems.Add(node);
		}
		//NodeItems.Init(node, row_length);
	};


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FKingStairNode> NodeItems;

	int32 row_length;



	FKingStairNode& operator [] (int32 i)
	{
		return NodeItems[i];
	}

	void Add(FKingStairNode level2Dnode)
	{
		NodeItems.Add(level2Dnode);
	}

	int Num()
	{
		return NodeItems.Num();
	}

};

USTRUCT(BlueprintType)
struct FSelectedNode
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int x;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int y;
};


UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEVKIT_API UKingStairComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UKingStairComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="KingStairGrid")
	int GridRow = 3;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="KingStairGrid")
	int GridCol = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="KingStairGrid")
	TArray<FKingStairRow> KingStairGrid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FSelectedNode> SelectedNode;

	UFUNCTION(BlueprintCallable)
	void SetNodeEffect(int x, int y, TSubclassOf<UYogGameplayEffect> gameplayeffect, int32 level);

	UFUNCTION(BlueprintCallable)
	void SelectNode(int x, int y);

	UFUNCTION(BlueprintCallable)
	void ClearGrid();

	UFUNCTION(BlueprintCallable)
	void InitGrid();


};

