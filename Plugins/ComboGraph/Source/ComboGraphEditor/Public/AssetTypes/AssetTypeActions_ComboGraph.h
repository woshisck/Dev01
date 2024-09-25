// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class COMBOGRAPHEDITOR_API FAssetTypeActions_ComboGraph : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_ComboGraph(EAssetTypeCategories::Type InAssetCategory);

	//~ IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;

private:
	EAssetTypeCategories::Type AssetCategory;
};
