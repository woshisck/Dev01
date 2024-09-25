// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "ComboGraphAbilityTypes.generated.h"

USTRUCT(BlueprintType)
struct FComboGraphGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> CueParamsObjects;

	UPROPERTY()
	TArray<FSoftObjectPath> CueParamsObjectsPaths;

public:
	TArray<TWeakObjectPtr<UObject>> GetCueParamsObjects() const { return CueParamsObjects; }
	void SetCueParamsObjects(const TArray<TWeakObjectPtr<UObject>> InCueParamsObjects) { CueParamsObjects = InCueParamsObjects; }

	TArray<FSoftObjectPath> GetCueParamsObjectsPaths() const { return CueParamsObjectsPaths; }
	void SetCueParamsObjectsPath(const TArray<FSoftObjectPath> InCueParamsObjectsPaths) { CueParamsObjectsPaths = InCueParamsObjectsPaths; }

	/** Returns the actual struct used for serialization, subclasses must override this! */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	/** Creates a copy of this context, used to duplicate for later modifications */
	virtual FComboGraphGameplayEffectContext* Duplicate() const override
	{
		FComboGraphGameplayEffectContext* NewContext = new FComboGraphGameplayEffectContext();
		*NewContext = *this;
		NewContext->AddActors(Actors);
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
};

template <>
struct TStructOpsTypeTraits<FComboGraphGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FComboGraphGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true // Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};
