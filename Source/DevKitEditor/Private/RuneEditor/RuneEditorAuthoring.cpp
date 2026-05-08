#include "RuneEditor/RuneEditorAuthoring.h"

#include "Asset/FlowAssetFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "BuffFlow/YogRuneFlowAsset.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Factories/DataAssetFactory.h"
#include "FlowAsset.h"
#include "GameplayTagsEditorModule.h"
#include "GameplayTagsManager.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "RuneEditorAuthoring"

namespace
{
	const TCHAR* YogRuneEditorRoot = TEXT("/Game/YogRuneEditor");
	const TCHAR* YogRuneEditorRuneFolder = TEXT("/Game/YogRuneEditor/Runes");
	const TCHAR* YogRuneEditorFlowFolder = TEXT("/Game/YogRuneEditor/Flows");

	FString NormalizePackageFolder(FString Folder)
	{
		Folder.TrimStartAndEndInline();
		if (Folder.IsEmpty())
		{
			return YogRuneEditorRuneFolder;
		}

		Folder.ReplaceInline(TEXT("\\"), TEXT("/"));
		if (!Folder.StartsWith(TEXT("/Game")))
		{
			Folder = FString::Printf(TEXT("/Game/%s"), *Folder.TrimStartAndEnd());
		}
		return Folder;
	}

	FString StripAssetPrefix(FString Token)
	{
		Token.RemoveFromStart(TEXT("DA_Rune_"), ESearchCase::IgnoreCase);
		Token.RemoveFromStart(TEXT("DA_"), ESearchCase::IgnoreCase);
		Token.RemoveFromStart(TEXT("Rune_"), ESearchCase::IgnoreCase);
		return Token;
	}

	FText JoinRunFeedbackLines(const TArray<FText>& Lines)
	{
		TArray<FString> Strings;
		Strings.Reserve(Lines.Num());
		for (const FText& Line : Lines)
		{
			Strings.Add(Line.ToString());
		}
		return FText::FromString(FString::Join(Strings, TEXT("\n")));
	}

	FRuneEditorRunRuneResult MakeRunResult(const bool bSuccess, const FText& Message, const TArray<FText>& Lines)
	{
		FRuneEditorRunRuneResult Result;
		Result.bSuccess = bSuccess;
		Result.Message = Message;
		Result.DebugText = JoinRunFeedbackLines(Lines);
		return Result;
	}
}

FString FRuneEditorAuthoring::GetDefaultAssetRoot()
{
	return YogRuneEditorRoot;
}

FString FRuneEditorAuthoring::GetDefaultRuneFolder()
{
	return YogRuneEditorRuneFolder;
}

FString FRuneEditorAuthoring::GetDefaultFlowFolder()
{
	return YogRuneEditorFlowFolder;
}

FRuneEditorCreateRuneResult FRuneEditorAuthoring::CreateRuneAuthoringAssets(const FRuneEditorCreateRuneRequest& Request)
{
	FRuneEditorCreateRuneResult Result;

	const FString AssetToken = MakeSafeAssetToken(Request.DisplayName);
	if (AssetToken.IsEmpty())
	{
		Result.Message = LOCTEXT("InvalidRuneName", "Rune name is empty.");
		return Result;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const FString RuneFolder = NormalizeAssetFolder(Request.AssetFolder);
	const FString FlowFolder = NormalizeAssetFolder(Request.FlowAssetFolder);
	const FString RuneBaseName = FString::Printf(TEXT("DA_Rune_%s"), *StripAssetPrefix(AssetToken));
	const FString FlowBaseName = FString::Printf(TEXT("FA_Rune_%s"), *StripAssetPrefix(AssetToken));

	FString RunePackageName;
	FString RuneAssetName;
	AssetTools.CreateUniqueAssetName(RuneFolder / RuneBaseName, TEXT(""), RunePackageName, RuneAssetName);

	FString FlowPackageName;
	FString FlowAssetName;
	AssetTools.CreateUniqueAssetName(FlowFolder / FlowBaseName, TEXT(""), FlowPackageName, FlowAssetName);

	UFlowAssetFactory* FlowFactory = NewObject<UFlowAssetFactory>();
	FlowFactory->AssetClass = UYogRuneFlowAsset::StaticClass();
	UFlowAsset* FlowAsset = Cast<UFlowAsset>(AssetTools.CreateAsset(
		FlowAssetName,
		FlowFolder,
		UYogRuneFlowAsset::StaticClass(),
		FlowFactory));

	UDataAssetFactory* RuneFactory = NewObject<UDataAssetFactory>();
	RuneFactory->DataAssetClass = URuneDataAsset::StaticClass();
	URuneDataAsset* RuneAsset = Cast<URuneDataAsset>(AssetTools.CreateAsset(
		RuneAssetName,
		RuneFolder,
		URuneDataAsset::StaticClass(),
		RuneFactory));

	if (!RuneAsset || !FlowAsset)
	{
		Result.Message = LOCTEXT("AssetCreateFailed", "Failed to create RuneDataAsset or FlowAsset.");
		return Result;
	}

	FText TagStatus;
	const FString RequestedTag = Request.RuneIdTag.IsEmpty()
		? BuildDefaultRuneTagFromName(Request.DisplayName)
		: Request.RuneIdTag;
	const FGameplayTag RuneIdTag = EnsureGameplayTag(RequestedTag, TagStatus);

	RuneAsset->Modify();
	RuneAsset->RuneInfo.RuneConfig.RuneName = FName(*Request.DisplayName);
	RuneAsset->RuneInfo.RuneConfig.RuneIdTag = RuneIdTag;
	RuneAsset->RuneInfo.RuneConfig.RuneType = Request.RuneType;
	RuneAsset->RuneInfo.RuneConfig.Rarity = Request.Rarity;
	RuneAsset->RuneInfo.RuneConfig.TriggerType = Request.TriggerType;
	RuneAsset->RuneInfo.RuneConfig.HUDSummaryText = Request.SummaryText;
	RuneAsset->RuneInfo.RuneConfig.RuneDescription = Request.SummaryText;
	RuneAsset->RuneInfo.Shape.Cells.Reset();
	RuneAsset->RuneInfo.Flow.FlowAsset = FlowAsset;
	RuneAsset->MarkPackageDirty();
	FlowAsset->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(RuneAsset);
	FAssetRegistryModule::AssetCreated(FlowAsset);

	SyncBrowserAndMaybeOpen(RuneAsset, Request.bOpenAfterCreate);

	Result.RuneAsset = RuneAsset;
	Result.FlowAsset = FlowAsset;
	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT("CreatedRune", "Created {0} with {1}. {2}"),
		FText::FromString(RuneAsset->GetName()),
		FText::FromString(FlowAsset->GetName()),
		TagStatus);
	return Result;
}

FText FRuneEditorAuthoring::SaveRuneBasicInfo(URuneDataAsset* Rune, const FString& DisplayName, const FString& RuneIdTag, const FText& SummaryText)
{
	if (!Rune)
	{
		return LOCTEXT("SaveNoRune", "No rune selected.");
	}

	const FScopedTransaction Transaction(LOCTEXT("SaveRuneBasicInfo", "Save Rune Basic Info"));
	FText TagStatus;
	const FGameplayTag NewTag = EnsureGameplayTag(RuneIdTag, TagStatus);

	Rune->Modify();
	Rune->RuneInfo.RuneConfig.RuneName = FName(*DisplayName);
	Rune->RuneInfo.RuneConfig.RuneIdTag = NewTag;
	Rune->RuneInfo.RuneConfig.HUDSummaryText = SummaryText;
	Rune->RuneInfo.RuneConfig.RuneDescription = SummaryText;
	Rune->MarkPackageDirty();

	return FText::Format(
		LOCTEXT("SavedRuneBasicInfo", "Saved {0}. {1}"),
		FText::FromString(Rune->GetName()),
		TagStatus);
}

FRuneEditorRunRuneResult FRuneEditorAuthoring::RunRuneOnSelectedActor(URuneDataAsset* Rune)
{
	if (!Rune)
	{
		const FText Message = LOCTEXT("RunNoRune", "No rune selected.");
		return MakeRunResult(false, Message, { Message });
	}

	UFlowAsset* FlowAsset = Rune->GetFlowAsset();
	if (!FlowAsset)
	{
		const FText Message = LOCTEXT("RunNoFlow", "Selected rune has no FlowAsset.");
		return MakeRunResult(false, Message, {
			FText::Format(LOCTEXT("RunFeedbackRune", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
			LOCTEXT("RunFeedbackFlowMissing", "FlowAssetName: None"),
			Message });
	}

	if (!GEditor)
	{
		const FText Message = LOCTEXT("RunNoEditor", "GEditor is not available.");
		return MakeRunResult(false, Message, {
			FText::Format(LOCTEXT("RunFeedbackRuneNoEditor", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
			FText::Format(LOCTEXT("RunFeedbackFlowNoEditor", "FlowAssetName: {0}"), FText::FromString(FlowAsset->GetName())),
			Message });
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	AActor* TargetActor = nullptr;
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		TargetActor = Cast<AActor>(*It);
		if (TargetActor)
		{
			break;
		}
	}

	if (!TargetActor)
	{
		const FText Message = LOCTEXT("RunNoSelectedActor", "Select an actor with BuffFlowComponent in the editor or PIE world first.");
		return MakeRunResult(false, Message, {
			FText::Format(LOCTEXT("RunFeedbackRuneNoActor", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
			FText::Format(LOCTEXT("RunFeedbackFlowNoActor", "FlowAssetName: {0}"), FText::FromString(FlowAsset->GetName())),
			LOCTEXT("RunFeedbackActorMissing", "SelectedActorName: None"),
			Message });
	}

	UWorld* World = TargetActor->GetWorld();
	if (!World || !World->GetGameInstance())
	{
		const FText Message = LOCTEXT("RunNeedsPIE", "Rune run requires PIE or a world with GameInstance.");
		return MakeRunResult(false, Message, {
			FText::Format(LOCTEXT("RunFeedbackRuneNeedsPie", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
			FText::Format(LOCTEXT("RunFeedbackFlowNeedsPie", "FlowAssetName: {0}"), FText::FromString(FlowAsset->GetName())),
			FText::Format(LOCTEXT("RunFeedbackActorNeedsPie", "SelectedActorName: {0}"), FText::FromString(TargetActor->GetName())),
			Message });
	}

	UBuffFlowComponent* BuffFlowComponent = TargetActor->FindComponentByClass<UBuffFlowComponent>();
	if (!BuffFlowComponent)
	{
		const FText Message = FText::Format(
			LOCTEXT("RunNoBuffFlowComponent", "{0} has no BuffFlowComponent."),
			FText::FromString(TargetActor->GetName()));
		return MakeRunResult(false, Message, {
			FText::Format(LOCTEXT("RunFeedbackRuneNoComponent", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
			FText::Format(LOCTEXT("RunFeedbackFlowNoComponent", "FlowAssetName: {0}"), FText::FromString(FlowAsset->GetName())),
			FText::Format(LOCTEXT("RunFeedbackActorNoComponent", "SelectedActorName: {0}"), FText::FromString(TargetActor->GetName())),
			LOCTEXT("RunFeedbackComponentMissing", "BuffFlowComponentName: None"),
			Message });
	}

	BuffFlowComponent->StartBuffFlowWithRune(FlowAsset, FGuid::NewGuid(), Rune, TargetActor, true);
	const FText Message = FText::Format(
		LOCTEXT("RunStarted", "Started {0} on {1}."),
		FText::FromName(Rune->GetRuneName()),
		FText::FromString(TargetActor->GetName()));
	return MakeRunResult(true, Message, {
		FText::Format(LOCTEXT("RunFeedbackRuneStarted", "Rune: {0}"), FText::FromName(Rune->GetRuneName())),
		FText::Format(LOCTEXT("RunFeedbackFlowStarted", "FlowAssetName: {0}"), FText::FromString(FlowAsset->GetName())),
		FText::Format(LOCTEXT("RunFeedbackActorStarted", "SelectedActorName: {0}"), FText::FromString(TargetActor->GetName())),
		FText::Format(LOCTEXT("RunFeedbackComponentStarted", "BuffFlowComponentName: {0}"), FText::FromString(BuffFlowComponent->GetName())),
		LOCTEXT("RunFeedbackStartCall", "StartBuffFlowWithRune called with a new runtime guid.") });
}

FRuneEditorAssetActionResult FRuneEditorAuthoring::DuplicateResource(UObject* SourceAsset)
{
	FRuneEditorAssetActionResult Result;
	if (!SourceAsset)
	{
		Result.Message = LOCTEXT("DuplicateNoAsset", "No resource selected to copy.");
		return Result;
	}

	FText DuplicateMessage;
	UObject* NewAsset = DuplicateAssetToFolder(SourceAsset, GetDefaultFolderForAsset(SourceAsset), DuplicateMessage);
	if (!NewAsset)
	{
		Result.Message = DuplicateMessage;
		return Result;
	}

	Result.Assets.Add(NewAsset);

	if (URuneDataAsset* NewRune = Cast<URuneDataAsset>(NewAsset))
	{
		if (const URuneDataAsset* SourceRune = Cast<URuneDataAsset>(SourceAsset))
		{
			if (UFlowAsset* SourceFlow = SourceRune->GetFlowAsset())
			{
				FText FlowDuplicateMessage;
				UObject* NewFlowObject = DuplicateAssetToFolder(SourceFlow, GetDefaultFlowFolder(), FlowDuplicateMessage);
				if (UFlowAsset* NewFlow = Cast<UFlowAsset>(NewFlowObject))
				{
					NewRune->Modify();
					NewRune->RuneInfo.Flow.FlowAsset = NewFlow;
					NewRune->MarkPackageDirty();
					Result.Assets.Add(NewFlow);
				}
			}
		}
	}

	TArray<UObject*> ObjectsToSync;
	for (const TWeakObjectPtr<UObject>& Asset : Result.Assets)
	{
		if (Asset.IsValid())
		{
			ObjectsToSync.Add(Asset.Get());
		}
	}
	SyncBrowserToAssets(ObjectsToSync);

	Result.bSuccess = true;
	Result.Message = FText::Format(
		LOCTEXT("DuplicateSucceeded", "Copied {0} resource(s)."),
		FText::AsNumber(Result.Assets.Num()));
	return Result;
}

FText FRuneEditorAuthoring::RenameResource(UObject* Asset, const FString& NewName)
{
	if (!Asset)
	{
		return LOCTEXT("RenameNoAsset", "No resource selected to rename.");
	}

	const FString SafeName = MakeSafeAssetToken(NewName);
	if (SafeName.IsEmpty())
	{
		return LOCTEXT("RenameEmptyName", "New resource name is empty.");
	}

	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TArray<FAssetRenameData> RenameData;
	RenameData.Emplace(Asset, PackagePath, SafeName);

	const bool bRenamed = AssetTools.RenameAssets(RenameData);
	if (bRenamed)
	{
		SyncBrowserToAssets({ Asset });
	}

	return bRenamed
		? FText::Format(LOCTEXT("RenameSucceeded", "Renamed resource to {0}."), FText::FromString(SafeName))
		: LOCTEXT("RenameFailed", "Resource rename failed.");
}

FText FRuneEditorAuthoring::DeleteResources(const TArray<UObject*>& Assets)
{
	TArray<UObject*> ValidAssets;
	for (UObject* Asset : Assets)
	{
		if (!Asset)
		{
			continue;
		}

		ValidAssets.Add(Asset);

		// When deleting a RuneDataAsset, also collect its FlowAsset to avoid orphans.
		if (const URuneDataAsset* Rune = Cast<URuneDataAsset>(Asset))
		{
			if (UFlowAsset* Flow = Rune->GetFlowAsset())
			{
				ValidAssets.AddUnique(Flow);
			}
		}
	}

	if (ValidAssets.Num() == 0)
	{
		return LOCTEXT("DeleteNoAsset", "No resource selected to delete.");
	}

	const int32 DeletedCount = ObjectTools::DeleteObjects(ValidAssets, true);
	return FText::Format(
		LOCTEXT("DeleteResourcesStatus", "Deleted {0} of {1} resource(s) (including associated FlowAssets)."),
		FText::AsNumber(DeletedCount),
		FText::AsNumber(ValidAssets.Num()));
}

void FRuneEditorAuthoring::SyncBrowserToAssets(const TArray<UObject*>& Assets)
{
	TArray<UObject*> ValidAssets;
	for (UObject* Asset : Assets)
	{
		if (Asset)
		{
			ValidAssets.Add(Asset);
		}
	}

	if (ValidAssets.Num() == 0 || !FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
	{
		return;
	}

	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.Get().SyncBrowserToAssets(ValidAssets);
}

FString FRuneEditorAuthoring::BuildDefaultRuneTagFromName(const FString& DisplayName)
{
	const FString Token = MakeSafeAssetToken(DisplayName);
	return Token.IsEmpty() ? TEXT("Rune.ID.NewRune") : FString::Printf(TEXT("Rune.ID.%s"), *StripAssetPrefix(Token));
}

FString FRuneEditorAuthoring::MakeSafeAssetToken(const FString& Value)
{
	FString Token = Value;
	Token.TrimStartAndEndInline();
	if (Token.IsEmpty())
	{
		return FString();
	}

	for (TCHAR& Char : Token)
	{
		if (!FChar::IsAlnum(Char))
		{
			Char = TEXT('_');
		}
	}

	while (Token.Contains(TEXT("__")))
	{
		Token.ReplaceInline(TEXT("__"), TEXT("_"));
	}

	Token.RemoveFromStart(TEXT("_"));
	Token.RemoveFromEnd(TEXT("_"));
	return Token;
}

FString FRuneEditorAuthoring::NormalizeAssetFolder(FString Folder)
{
	return NormalizePackageFolder(MoveTemp(Folder));
}

FString FRuneEditorAuthoring::GetDefaultFolderForAsset(const UObject* Asset)
{
	if (Asset && Asset->IsA<URuneDataAsset>())
	{
		return GetDefaultRuneFolder();
	}

	if (Asset && Asset->IsA<UFlowAsset>())
	{
		return GetDefaultFlowFolder();
	}

	return GetDefaultAssetRoot();
}

UObject* FRuneEditorAuthoring::DuplicateAssetToFolder(UObject* SourceAsset, const FString& DestinationFolder, FText& OutMessage)
{
	if (!SourceAsset)
	{
		OutMessage = LOCTEXT("DuplicateMissingSource", "No source resource selected.");
		return nullptr;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const FString Folder = NormalizeAssetFolder(DestinationFolder);
	const FString BaseName = FString::Printf(TEXT("%s_Copy"), *StripAssetPrefix(SourceAsset->GetName()));

	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(Folder / BaseName, TEXT(""), PackageName, AssetName);

	UObject* NewAsset = AssetTools.DuplicateAsset(AssetName, Folder, SourceAsset);
	if (!NewAsset)
	{
		OutMessage = FText::Format(
			LOCTEXT("DuplicateFailed", "Failed to copy {0}."),
			FText::FromString(SourceAsset->GetName()));
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	NewAsset->MarkPackageDirty();
	OutMessage = FText::Format(
		LOCTEXT("DuplicateAssetSucceeded", "Copied {0} to {1}."),
		FText::FromString(SourceAsset->GetName()),
		FText::FromString(NewAsset->GetName()));
	return NewAsset;
}

FGameplayTag FRuneEditorAuthoring::EnsureGameplayTag(const FString& TagString, FText& OutStatus)
{
	FString NormalizedTag = TagString;
	NormalizedTag.TrimStartAndEndInline();
	if (NormalizedTag.IsEmpty())
	{
		OutStatus = LOCTEXT("EmptyTagStatus", "RuneIdTag is empty.");
		return FGameplayTag();
	}

	FGameplayTag ExistingTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*NormalizedTag), false);
	if (ExistingTag.IsValid())
	{
		OutStatus = FText::Format(LOCTEXT("ExistingTagStatus", "Using existing tag {0}."), FText::FromString(NormalizedTag));
		return ExistingTag;
	}

	const bool bAddedTag = IGameplayTagsEditorModule::Get().AddNewGameplayTagToINI(
		NormalizedTag,
		TEXT("Created by Rune Editor"),
		NAME_None);
	if (bAddedTag)
	{
		UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
	}

	FGameplayTag NewTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*NormalizedTag), false);
	OutStatus = NewTag.IsValid()
		? FText::Format(LOCTEXT("CreatedTagStatus", "Created tag {0}."), FText::FromString(NormalizedTag))
		: FText::Format(LOCTEXT("InvalidTagStatus", "Tag {0} was written but is not loaded yet; restart editor if it remains invalid."), FText::FromString(NormalizedTag));
	return NewTag;
}

void FRuneEditorAuthoring::SyncBrowserAndMaybeOpen(UObject* Asset, bool bOpen)
{
	if (!Asset)
	{
		return;
	}

	if (FModuleManager::Get().IsModuleLoaded(TEXT("ContentBrowser")))
	{
		SyncBrowserToAssets({ Asset });
	}

	if (bOpen && GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(Asset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
