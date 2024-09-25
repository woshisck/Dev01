// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/BlueprintFactory.h"
#include "ComboGraphNodeFactories.generated.h"

class UComboGraphNodeMontage;

class FComboGraphNodeFactoryUtils
{
public:
	static UObject* FactoryCreateNew(UClass* Class, UClass* ParentClass, UClass* BlueprintClassType, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext);
};

UCLASS(HideCategories=Object, MinimalAPI)
class UComboGraphNodeMontageFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UComboGraphNodeMontageFactory();

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface
};

UCLASS(HideCategories=Object, MinimalAPI)
class UComboGraphNodeSequenceFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UComboGraphNodeSequenceFactory();

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface
};
