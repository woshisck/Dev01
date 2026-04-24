#include "DevKitEditor/Rune/RuneManagerWidget.h"
#include "Data/RuneDataAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Subsystems/AssetEditorSubsystem.h"

// ──────────────────────────────────────────────────────────────
//  扫描所有 RuneDataAsset
// ──────────────────────────────────────────────────────────────

TArray<URuneDataAsset*> URuneManagerWidget::GetAllRuneDataAssets()
{
	FAssetRegistryModule& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassPaths.Add(URuneDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths   = true;
	Filter.PackagePaths.Add(FName("/Game"));

	TArray<FAssetData> AssetDataList;
	Registry.Get().GetAssets(Filter, AssetDataList);

	TArray<URuneDataAsset*> Result;
	Result.Reserve(AssetDataList.Num());

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (URuneDataAsset* DA = Cast<URuneDataAsset>(AssetData.GetAsset()))
			Result.Add(DA);
	}

	return Result;
}

// ──────────────────────────────────────────────────────────────
//  复制 DA（从模板新建）
// ──────────────────────────────────────────────────────────────

URuneDataAsset* URuneManagerWidget::DuplicateRuneDA(
	URuneDataAsset* Template,
	const FString& NewName,
	const FString& DestinationPath)
{
	if (!Template)
	{
		UE_LOG(LogTemp, Warning, TEXT("DuplicateRuneDA: Template is null"));
		return nullptr;
	}

	IAssetTools& AssetTools =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	UObject* NewAsset = AssetTools.DuplicateAsset(NewName, DestinationPath, Template);

	if (URuneDataAsset* NewDA = Cast<URuneDataAsset>(NewAsset))
	{
		SyncToBrowserAndSelect(NewDA);
		OpenAssetEditor(NewDA);
		return NewDA;
	}

	return nullptr;
}

// ──────────────────────────────────────────────────────────────
//  Content Browser 交互
// ──────────────────────────────────────────────────────────────

void URuneManagerWidget::SyncToBrowserAndSelect(UObject* Asset)
{
	if (!Asset) return;

	FContentBrowserModule& ContentBrowser =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<UObject*> Assets = { Asset };
	ContentBrowser.Get().SyncBrowserToAssets(Assets);
}

void URuneManagerWidget::OpenAssetEditor(UObject* Asset)
{
	if (!Asset || !GEditor) return;

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
}
