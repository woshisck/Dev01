// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GSCAttributesGenSettings.generated.h"

USTRUCT()
struct FGSCAttributeDefinition
{
	GENERATED_BODY()

public:
	/** The GameplayAttribute name you want to generate */
	UPROPERTY(EditAnywhere, Category = "Attributes")
	FString AttributeName;

	/** The default value for this attribute before getting initialized by a GameplayEffect */
	UPROPERTY(EditAnywhere, Category = "Attributes")
	float DefaultValue = 0.f;

	/** The UPROPERTY Category specifier for this attribute */
	UPROPERTY(EditAnywhere, Category = "Attributes")
	FString Category;

	/** GameplayAttributes are replicated by default, works for Single Player and Multiplayer */
	UPROPERTY(VisibleAnywhere, Category = "Attributes")
	bool bReplicated = true;
};

USTRUCT()
struct FGSCAttributesSettings
{
	GENERATED_BODY()

public:

	/** Define here any number of GameplayAttributes you want to generate (at least one) */
	UPROPERTY(EditAnywhere, Category = "Attributes")
	TArray<FGSCAttributeDefinition> Attributes;

	bool HasAnyAttributes() const
	{
		return Attributes.Num() > 0;
	}
};


/**
 *
 */
UCLASS()
class UGSCAttributesGenSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "GASCompanion Attributes Generation", Meta = (ShowOnlyInnerProperties))
	FGSCAttributesSettings Settings;

	static UGSCAttributesGenSettings* Get()
    {
    	// This is a singleton, use default object
        return GetMutableDefault<UGSCAttributesGenSettings>();
    }
};
