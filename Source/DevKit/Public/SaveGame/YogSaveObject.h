

#pragma once

#include "CoreMinimal.h"
#include "YogSaveObject.generated.h"

USTRUCT()
struct FComponentRecord
{
	GENERATED_BODY()
public:
	//Component name
	UPROPERTY()
	FName Name;

	//Component class name
	UPROPERTY()
	FString ClassName;

	//Serialized data for all UProperties that are 'SaveGame' enabled
	UPROPERTY()
	TArray<uint8> Data;

};

USTRUCT()
struct FObjectRecord
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY()
	FString ClassName;

	// Name of the object
	UPROPERTY()
	FString Name;

	// Path name of the object
	UPROPERTY()
	FString PathName;

	// Path name of the Outer
	UPROPERTY()
	FString OuterPathName;

	UPROPERTY()
	TArray<uint8> Data;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY(NotReplicated)
	TMap<FName, FComponentRecord> Components;

	UPROPERTY()
	FGuid Id;


};

UCLASS()
class DEVKIT_API UYogSaveObject : public UObject
{
	GENERATED_BODY()
	
public:

};

