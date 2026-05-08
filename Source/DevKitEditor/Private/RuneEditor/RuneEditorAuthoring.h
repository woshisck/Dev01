#pragma once

#include "CoreMinimal.h"
#include "Data/RuneDataAsset.h"

class UFlowAsset;
class URuneDataAsset;

struct FRuneEditorCreateRuneRequest
{
	FString DisplayName = TEXT("New Rune");
	FString RuneIdTag = TEXT("Rune.ID.NewRune");
	FString AssetFolder = TEXT("/Game/YogRuneEditor/Runes");
	FString FlowAssetFolder = TEXT("/Game/YogRuneEditor/Flows");
	ERuneType RuneType = ERuneType::Buff;
	ERuneRarity Rarity = ERuneRarity::Common;
	ERuneTriggerType TriggerType = ERuneTriggerType::Passive;
	FText SummaryText = FText::FromString(TEXT("New Yog rune."));
	bool bOpenAfterCreate = true;
};

struct FRuneEditorCreateRuneResult
{
	TWeakObjectPtr<URuneDataAsset> RuneAsset;
	TWeakObjectPtr<UFlowAsset> FlowAsset;
	FText Message;
	bool bSuccess = false;
};

struct FRuneEditorRunRuneResult
{
	FText Message;
	FText DebugText;
	bool bSuccess = false;
};

struct FRuneEditorAssetActionResult
{
	TArray<TWeakObjectPtr<UObject>> Assets;
	FText Message;
	bool bSuccess = false;
};

class FRuneEditorAuthoring
{
public:
	static FString GetDefaultAssetRoot();
	static FString GetDefaultRuneFolder();
	static FString GetDefaultFlowFolder();

	static FRuneEditorCreateRuneResult CreateRuneAuthoringAssets(const FRuneEditorCreateRuneRequest& Request);
	static FText SaveRuneBasicInfo(URuneDataAsset* Rune, const FString& DisplayName, const FString& RuneIdTag, const FText& SummaryText);
	static FRuneEditorRunRuneResult RunRuneOnSelectedActor(URuneDataAsset* Rune);
	static FRuneEditorAssetActionResult DuplicateResource(UObject* SourceAsset);
	static FText RenameResource(UObject* Asset, const FString& NewName);
	static FText DeleteResources(const TArray<UObject*>& Assets);
	static void SyncBrowserToAssets(const TArray<UObject*>& Assets);
	static FString BuildDefaultRuneTagFromName(const FString& DisplayName);
	static FGameplayTag EnsureGameplayTag(const FString& TagString, FText& OutStatus);

private:
	static FString MakeSafeAssetToken(const FString& Value);
	static FString NormalizeAssetFolder(FString Folder);
	static FString GetDefaultFolderForAsset(const UObject* Asset);
	static UObject* DuplicateAssetToFolder(UObject* SourceAsset, const FString& DestinationFolder, FText& OutMessage);
	static void SyncBrowserAndMaybeOpen(UObject* Asset, bool bOpen);
};
