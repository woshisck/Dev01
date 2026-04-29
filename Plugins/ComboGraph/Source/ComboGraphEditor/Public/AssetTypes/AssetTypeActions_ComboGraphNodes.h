// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

class COMBOGRAPHEDITOR_API FAssetTypeActions_ComboGraphNodeBase : public FAssetTypeActions_Blueprint
{
public:
	FAssetTypeActions_ComboGraphNodeBase(EAssetTypeCategories::Type InAssetCategory);

	//~ IAssetTypeActions interface
	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type AssetCategory;
};

class COMBOGRAPHEDITOR_API FAssetTypeActions_ComboGraphNodeMontage : public FAssetTypeActions_ComboGraphNodeBase
{
public:
	FAssetTypeActions_ComboGraphNodeMontage(EAssetTypeCategories::Type InAssetCategory);

	//~ IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;


};

class COMBOGRAPHEDITOR_API FAssetTypeActions_ComboGraphNodeSequence : public FAssetTypeActions_ComboGraphNodeBase
{
public:
	FAssetTypeActions_ComboGraphNodeSequence(EAssetTypeCategories::Type InAssetCategory);

	//~ IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;

};
