// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GameplayTagContainer.h"
#include "YogBuffDefinition.generated.h"

/**
 * 
 */

class UObject;
class USoundBase;


UENUM(BlueprintType)
enum class EBuffType : uint8
{
	Increase,
	Decrease
};


UENUM(BlueprintType)
enum class EBuffDurition : uint8
{
	Instant,
	Period
};


UENUM(BlueprintType)
enum class EBuffUnique : uint8
{
	Unique,
	Common
};

UENUM(BlueprintType)
enum class EStackType : uint8
{
	Refresh,
	Stack,
	Stop
};

UENUM(BlueprintType)
enum class EStackReduceType : uint8
{
	AllAtOnce,
	OneByOne
};



USTRUCT(BlueprintType)
struct FYogBuffDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// The sound to play
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundBase> Sound;

	// The icon to display	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true", AllowedClasses = "Texture,MaterialInterface,SlateTextureAtlasInterface", DisallowedClasses = "MediaTexture"))
	TSoftObjectPtr<UObject> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBuffType Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBuffDurition Duration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EBuffUnique UniqueType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStackReduceType StackReduceType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStackType StackType;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer BufferTags;

};


UCLASS()
class DEVKIT_API UYogBuffDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// The sound to play
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<USoundBase> Sound;

	// The icon to display	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true", AllowedClasses = "Texture,MaterialInterface,SlateTextureAtlasInterface", DisallowedClasses = "MediaTexture"))
	TSoftObjectPtr<UObject> Icon;

};
