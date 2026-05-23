#include "UI/FirstRunLoadingScreenSetupCommandlet.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "WidgetBlueprint.h"

namespace FirstRunLoadingScreenSetup
{
	const FString SourceFile = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("SourceArt/UI/Loading/T_FirstRunTutorial_Loading.png"));
	const FString DestinationPath = TEXT("/Game/UI/Loading");
	const FString TexturePath = TEXT("/Game/UI/Loading/T_FirstRunTutorial_Loading.T_FirstRunTutorial_Loading");
	const FString LoadingWidgetPath = TEXT("/Game/UI/UI_LoadingScreen.UI_LoadingScreen");
	const FString ReportPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Docs/GeneratedReports/CommandletReports/FirstRunLoadingScreenSetupReport.md"));

	void ConfigureTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->NeverStream = true;
		Texture->SRGB = true;
		Texture->PostEditChange();
	}

	UTexture2D* ImportTexture(const bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Loading texture"));
		if (!FPaths::FileExists(SourceFile))
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing source file `%s`."), *SourceFile));
			return nullptr;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s `%s` -> `%s`."),
			bDryRun ? TEXT("Would import") : TEXT("Imported"),
			*SourceFile,
			*DestinationPath));
		if (bDryRun)
		{
			return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(SourceFile);
		ImportData->bReplaceExisting = true;
		ImportData->Factory = NewObject<UTextureFactory>();

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);
		for (UObject* ImportedAsset : ImportedAssets)
		{
			if (UTexture2D* Texture = Cast<UTexture2D>(ImportedAsset))
			{
				ConfigureTexture(Texture);
				Texture->MarkPackageDirty();
				DirtyPackages.AddUnique(Texture->GetPackage());
				return Texture;
			}
		}

		return Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexturePath));
	}

	bool ApplyTextureToLoadingWidget(UTexture2D* Texture, const bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Loading widget"));
		if (!Texture)
		{
			ReportLines.Add(TEXT("- Skipped widget update because the texture could not be loaded."));
			return false;
		}

		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(
			StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *LoadingWidgetPath));
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing loading widget `%s`."), *LoadingWidgetPath));
			return false;
		}

		TArray<UWidget*> Widgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);

		UImage* TargetImage = nullptr;
		for (UWidget* Widget : Widgets)
		{
			if (UImage* Image = Cast<UImage>(Widget))
			{
				const FString Name = Image->GetName();
				if (Name.Contains(TEXT("Loading"), ESearchCase::IgnoreCase)
					|| Name.Contains(TEXT("Background"), ESearchCase::IgnoreCase)
					|| Name.Contains(TEXT("Image"), ESearchCase::IgnoreCase))
				{
					TargetImage = Image;
					break;
				}

				if (!TargetImage)
				{
					TargetImage = Image;
				}
			}
		}

		if (!TargetImage)
		{
			ReportLines.Add(TEXT("- No Image widget was found; imported texture is ready for manual binding."));
			return false;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s loading image widget `%s` to `%s`."),
			bDryRun ? TEXT("Would bind") : TEXT("Bound"),
			*TargetImage->GetName(),
			*TexturePath));
		if (bDryRun)
		{
			return true;
		}

		WidgetBlueprint->Modify();
		TargetImage->Modify();
		TargetImage->SetBrushFromTexture(Texture, true);
		WidgetBlueprint->MarkPackageDirty();
		DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		return true;
	}
}

UFirstRunLoadingScreenSetupCommandlet::UFirstRunLoadingScreenSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFirstRunLoadingScreenSetupCommandlet::Main(const FString& Params)
{
	const bool bDryRun = Params.Contains(TEXT("-DryRun"));

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# First Run Loading Screen Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- DryRun: `%s`"), bDryRun ? TEXT("true") : TEXT("false")));
	ReportLines.Add(TEXT(""));

	UTexture2D* Texture = FirstRunLoadingScreenSetup::ImportTexture(bDryRun, ReportLines, DirtyPackages);
	FirstRunLoadingScreenSetup::ApplyTextureToLoadingWidget(Texture, bDryRun, ReportLines, DirtyPackages);

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FirstRunLoadingScreenSetup::ReportPath), true);
	FFileHelper::SaveStringArrayToFile(ReportLines, *FirstRunLoadingScreenSetup::ReportPath);

	for (const FString& Line : ReportLines)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}

	return 0;
}
