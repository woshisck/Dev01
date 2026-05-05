#include "UI/Tutorial512SetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/DialogContentDA.h"
#include "UI/TutorialRegistryDA.h"
#include "UObject/Package.h"

namespace Tutorial512Setup
{
	const FString TutorialRoot = TEXT("/Game/Docs/UI/Tutorial");
	const FString IllustrationRoot = TEXT("/Game/Docs/UI/TutorialTex/512");
	const FString RegistryPackagePath = TutorialRoot + TEXT("/DA_TutorialRegistry");
	const FString ReportFileName = TEXT("512TutorialSetupReport.md");

	struct FTutorial512PageSpec
	{
		FString Title;
		FString Body;
		FString SubText;
	};

	struct FTutorial512EntrySpec
	{
		FName EventID;
		FString AssetName;
		TArray<FTutorial512PageSpec> Pages;
	};

	struct FTutorial512IllustrationSpec
	{
		FString AssetName;
		FString FileName;
	};

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	template <typename T>
	T* LoadAssetByPackagePath(const FString& PackagePath)
	{
		if (PackagePath.IsEmpty())
		{
			return nullptr;
		}
		if (T* Existing = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}
		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	FTutorialPage MakePage(const FTutorial512PageSpec& Spec)
	{
		FTutorialPage Page;
		Page.Title = FText::FromString(Spec.Title);
		Page.Body = FText::FromString(Spec.Body);
		Page.SubText = FText::FromString(Spec.SubText);
		return Page;
	}

	TArray<FTutorial512IllustrationSpec> MakeIllustrationSpecs()
	{
		return {
			{ TEXT("T_Tutorial512_WeaponPickup"), TEXT("T_Tutorial512_WeaponPickup.png") },
			{ TEXT("T_Tutorial512_WeaponCards"), TEXT("T_Tutorial512_WeaponCards.png") },
			{ TEXT("T_Tutorial512_RewardCard"), TEXT("T_Tutorial512_RewardCard.png") },
			{ TEXT("T_Tutorial512_CardSort"), TEXT("T_Tutorial512_CardSort.png") },
			{ TEXT("T_Tutorial512_LinkCard"), TEXT("T_Tutorial512_LinkCard.png") },
			{ TEXT("T_Tutorial512_DeckReload"), TEXT("T_Tutorial512_DeckReload.png") },
		};
	}

	FString GetIllustrationAssetName(FName EventID, int32 PageIndex)
	{
		if (EventID == TEXT("tutorial_weapon_pickup"))
		{
			return PageIndex == 0 ? TEXT("T_Tutorial512_WeaponPickup") : TEXT("T_Tutorial512_WeaponCards");
		}
		if (EventID == TEXT("tutorial_first_rune"))
		{
			return TEXT("T_Tutorial512_RewardCard");
		}
		if (EventID == TEXT("tutorial_backpack"))
		{
			return PageIndex == 0 ? TEXT("T_Tutorial512_CardSort") : TEXT("T_Tutorial512_LinkCard");
		}
		if (EventID == TEXT("tutorial_card_link"))
		{
			return TEXT("T_Tutorial512_LinkCard");
		}
		if (EventID == TEXT("tutorial_shuffle_hint"))
		{
			return TEXT("T_Tutorial512_DeckReload");
		}
		return FString();
	}

	UTexture2D* LoadIllustrationTexture(const FString& AssetName)
	{
		return LoadAssetByPackagePath<UTexture2D>(IllustrationRoot + TEXT("/") + AssetName);
	}

	void ConfigureImportedTexture(UTexture2D* Texture)
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

	void ImportIllustrations(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/UI/Tutorial512"));
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		ReportLines.Add(TEXT("## Illustration import"));
		for (const FTutorial512IllustrationSpec& IllustrationSpec : MakeIllustrationSpecs())
		{
			const FString SourceFile = FPaths::Combine(SourceDir, IllustrationSpec.FileName);
			if (!FPaths::FileExists(SourceFile))
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing source PNG `%s`."), *SourceFile));
				continue;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> `%s`."),
				bDryRun ? TEXT("Would import/reimport") : TEXT("Imported/reimported"),
				*SourceFile,
				*IllustrationRoot));
			if (bDryRun)
			{
				continue;
			}

			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = IllustrationRoot;
			ImportData->Filenames.Add(SourceFile);
			ImportData->bReplaceExisting = true;

			const TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);
			for (UObject* ImportedAsset : ImportedAssets)
			{
				if (UTexture2D* Texture = Cast<UTexture2D>(ImportedAsset))
				{
					ConfigureImportedTexture(Texture);
				}
				if (ImportedAsset)
				{
					ImportedAsset->MarkPackageDirty();
					DirtyPackages.AddUnique(ImportedAsset->GetPackage());
				}
			}
		}
		ReportLines.Add(TEXT(""));
	}

	TArray<FTutorial512EntrySpec> MakeSpecs()
	{
		return {
			{
				TEXT("tutorial_weapon_pickup"),
				TEXT("DA_Tutorial_WeaponPickup"),
				{
					{
						TEXT("拾起武器"),
						TEXT("靠近发光武器，按 [E] 拾取。武器会装备到角色身上，并把它自带的初始卡牌装入下方 1D 卡组。"),
						TEXT("")
					},
					{
						TEXT("武器自带初始卡牌"),
						TEXT("每把武器都有一组起始卡。拾取后，卡组会按武器配置顺序显示；轻/重攻击不会被卡组阻止，但命中会按顺序消耗卡牌并触发效果。"),
						TEXT("")
					},
				}
			},
			{
				TEXT("tutorial_first_rune"),
				TEXT("DA_Tutorial_FirstRune"),
				{
					{
						TEXT("新的卡牌已入组"),
						TEXT("选择奖励后，它会进入后台背包，并追加到当前战斗卡组末尾。下一轮装填或下一房间，你会在 1D 卡组条里看到它。"),
						TEXT("跳过奖励等于放弃这次成长。")
					},
				}
			},
			{
				TEXT("tutorial_backpack"),
				TEXT("DA_Tutorial_Backpack"),
				{
					{
						TEXT("调整卡牌顺序"),
						TEXT("整理阶段可以在卡组编排列表里拖动卡牌排序。放下后会触发一次短装填，HUD 会按新顺序刷新。"),
						TEXT("鼠标拖动卡牌；手柄 A 拿起/放下，方向键移动位置。")
					},
					{
						TEXT("切换连携方向"),
						TEXT("Link 卡可以正向读取前一张，也可以反向赋能下一张。选中 Link 卡按 R 或手柄左键切换方向。"),
						TEXT("")
					},
				}
			},
			{
				TEXT("tutorial_card_link"),
				TEXT("DA_Tutorial_CardLink"),
				{
					{
						TEXT("连携卡"),
						TEXT("Link 卡会读取相邻卡牌的标签和方向。条件满足时，它会触发连携效果；HUD 会显示连携原因，让你知道为什么这次攻击被强化。"),
						TEXT("例：月光放在不同方向，可以选择不同连携效果。")
					},
				}
			},
			{
				TEXT("tutorial_shuffle_hint"),
				TEXT("DA_Tutorial_ShuffleHint"),
				{
					{
						TEXT("卡组装填中"),
						TEXT("这段空窗仍可攻击，但暂时不会触发卡牌效果。"),
						TEXT("")
					},
				}
			},
		};
	}

	UDialogContentDA* CreateDialogContentAsset(const FString& PackagePath, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UDialogContentDA* Existing = LoadAssetByPackagePath<UDialogContentDA>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create package `%s`."), *PackagePath));
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		UDialogContentDA* NewAsset = NewObject<UDialogContentDA>(
			Package,
			UDialogContentDA::StaticClass(),
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);

		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		else
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to allocate asset `%s`."), *PackagePath));
		}

		return NewAsset;
	}

	UTutorialRegistryDA* CreateRegistryAsset(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UTutorialRegistryDA* Existing = LoadAssetByPackagePath<UTutorialRegistryDA>(RegistryPackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found registry `%s`."), *RegistryPackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s registry `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *RegistryPackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*RegistryPackagePath);
		if (!Package)
		{
			ReportLines.Add(TEXT("- Failed to create registry package."));
			return nullptr;
		}

		UTutorialRegistryDA* NewAsset = NewObject<UTutorialRegistryDA>(
			Package,
			UTutorialRegistryDA::StaticClass(),
			*FPackageName::GetLongPackageAssetName(RegistryPackagePath),
			RF_Public | RF_Standalone | RF_Transactional);

		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		else
		{
			ReportLines.Add(TEXT("- Failed to allocate registry asset."));
		}

		return NewAsset;
	}

	void ConfigureDialogContent(
		const FTutorial512EntrySpec& Spec,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages,
		TMap<FName, UDialogContentDA*>& OutContentByEvent)
	{
		const FString PackagePath = TutorialRoot + TEXT("/") + Spec.AssetName;
		ReportLines.Add(FString::Printf(TEXT("## `%s`"), *Spec.EventID.ToString()));

		UDialogContentDA* DialogContent = CreateDialogContentAsset(PackagePath, bDryRun, ReportLines, DirtyPackages);
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would write %d page(s) to `%s`."), Spec.Pages.Num(), *PackagePath));
			for (int32 PageIndex = 0; PageIndex < Spec.Pages.Num(); ++PageIndex)
			{
				const FString IllustrationAssetName = GetIllustrationAssetName(Spec.EventID, PageIndex);
				if (!IllustrationAssetName.IsEmpty())
				{
					ReportLines.Add(FString::Printf(
						TEXT("- Would set page %d illustration to `%s`."),
						PageIndex + 1,
						*IllustrationAssetName));
				}
			}
			OutContentByEvent.Add(Spec.EventID, nullptr);
			return;
		}

		if (!DialogContent)
		{
			ReportLines.Add(TEXT("- Skipped because the DialogContentDA could not be loaded or created."));
			return;
		}

		DialogContent->Modify();
		DialogContent->Pages.Reset();
		for (const FTutorial512PageSpec& PageSpec : Spec.Pages)
		{
			DialogContent->Pages.Add(MakePage(PageSpec));
		}

		for (int32 PageIndex = 0; PageIndex < DialogContent->Pages.Num(); ++PageIndex)
		{
			const FString IllustrationAssetName = GetIllustrationAssetName(Spec.EventID, PageIndex);
			if (IllustrationAssetName.IsEmpty())
			{
				continue;
			}

			UTexture2D* Illustration = LoadIllustrationTexture(IllustrationAssetName);
			if (Illustration)
			{
				DialogContent->Pages[PageIndex].Illustration = Illustration;
				ReportLines.Add(FString::Printf(
					TEXT("- Page %d illustration set to `%s`."),
					PageIndex + 1,
					*IllustrationAssetName));
			}
			else
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Page %d missing illustration `%s`."),
					PageIndex + 1,
					*IllustrationAssetName));
			}
		}

		DialogContent->MarkPackageDirty();
		DirtyPackages.AddUnique(DialogContent->GetPackage());
		OutContentByEvent.Add(Spec.EventID, DialogContent);

		ReportLines.Add(FString::Printf(TEXT("- Wrote %d page(s) to `%s`."), DialogContent->Pages.Num(), *PackagePath));
	}

	void ConfigureRegistry(
		UTutorialRegistryDA* Registry,
		const TMap<FName, UDialogContentDA*>& ContentByEvent,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Registry"));
		if (bDryRun)
		{
			for (const TPair<FName, UDialogContentDA*>& Pair : ContentByEvent)
			{
				ReportLines.Add(FString::Printf(TEXT("- Would map `%s`."), *Pair.Key.ToString()));
			}
			return;
		}

		if (!Registry)
		{
			ReportLines.Add(TEXT("- Skipped because registry could not be loaded or created."));
			return;
		}

		Registry->Modify();
		for (const TPair<FName, UDialogContentDA*>& Pair : ContentByEvent)
		{
			if (Pair.Value)
			{
				Registry->Entries.Add(Pair.Key, Pair.Value);
				ReportLines.Add(FString::Printf(TEXT("- Mapped `%s` -> `%s`."), *Pair.Key.ToString(), *Pair.Value->GetName()));
			}
		}

		Registry->MarkPackageDirty();
		DirtyPackages.AddUnique(Registry->GetPackage());
	}
}

UTutorial512SetupCommandlet::UTutorial512SetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UTutorial512SetupCommandlet::Main(const FString& Params)
{
	using namespace Tutorial512Setup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	TMap<FName, UDialogContentDA*> ContentByEvent;

	ReportLines.Add(TEXT("# 512 Tutorial Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Tutorial root: `%s`"), *TutorialRoot));
	ReportLines.Add(FString::Printf(TEXT("- Illustration root: `%s`"), *IllustrationRoot));
	ReportLines.Add(TEXT(""));

	ImportIllustrations(bDryRun, ReportLines, DirtyPackages);

	const TArray<FTutorial512EntrySpec> Specs = MakeSpecs();
	for (const FTutorial512EntrySpec& Spec : Specs)
	{
		ConfigureDialogContent(Spec, bDryRun, ReportLines, DirtyPackages, ContentByEvent);
		ReportLines.Add(TEXT(""));
	}

	UTutorialRegistryDA* Registry = CreateRegistryAsset(bDryRun, ReportLines, DirtyPackages);
	ConfigureRegistry(Registry, ContentByEvent, bDryRun, ReportLines, DirtyPackages);

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("512 tutorial setup finished. Report: %s"), *ReportPath);
	return 0;
}
