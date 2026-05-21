#pragma once

#include "AssetTypeActions_Base.h"
#include "CoreMinimal.h"

class FAssetTypeActions_GameplayAbilityComboGraph : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_GameplayAbilityComboGraph(EAssetTypeCategories::Type InAssetCategory);

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type AssetCategory;
};
