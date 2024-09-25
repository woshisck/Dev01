// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "FileHelpers.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CreationMenu/GSCCreationMenu.h"
#include "CreationMenu/GSCGameplayEffectCreationMenu.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FGSCCreationMenuSpec, "GASCompanion.Editor.GSCCreationMenu", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	const UGSCGameplayEffectCreationMenu* EffectCreationMenu = nullptr;

	static constexpr const TCHAR* TargetPackagePath = TEXT("/Game/Developers/GASCompanion/GSCCreationMenu");

	FString TestContentDir = FPaths::ProjectContentDir() / TEXT("Developers/GASCompanion");

	TArray<FAssetData> AssetsToDelete;

	bool TestContains(const TCHAR* What, const FString& Actual, const FString& Expected)
	{
		if (!Actual.Contains(Expected))
		{
			AddError(FString::Printf(TEXT("Expected '%s' to contain \"%s\", but it was \"%s\"."), What, *Expected, *Actual), 1);
			return false;
		}
		
		AddInfo(FString::Printf(TEXT("Successfully found '%s' to contain \"%s\", it was \"%s\"."), What, *Expected, *Actual), 1);
		return true;
	}

END_DEFINE_SPEC(FGSCCreationMenuSpec)

void FGSCCreationMenuSpec::Define()
{
	Describe(TEXT("Gameplay Effects"), [this]()
	{
		BeforeEach([this]()
		{
			EffectCreationMenu = GetDefault<UGSCGameplayEffectCreationMenu>();
			AssetsToDelete.Empty();
		});

		It(TEXT("has definitions"), [this]()
		{
			TestFalse(TEXT("Definitions not empty"), EffectCreationMenu->Definitions.IsEmpty());

			for (const FGSCGameplayEffectCreationData& Definition : EffectCreationMenu->Definitions)
			{
				AddInfo(FString::Printf(TEXT("Checking against definition: %s"), *Definition.ToString()));
				TestFalse(TEXT("MenuPath Ok"), Definition.MenuPath.IsEmpty());
				TestFalse(TEXT("BaseName Ok"), Definition.BaseName.IsEmpty());
				TestTrue(TEXT("ParentClass Ok"), Definition.ParentClass != nullptr);
				TestTrue(TEXT("ParentClass Ok"), Definition.ParentClass->IsChildOf(UGSCTemplate_GameplayEffectDefinition::StaticClass()));
				TestFalse(TEXT("AssetPrefix Ok"), Definition.AssetPrefix.IsEmpty());
				TestFalse(TEXT("TooltipText Ok"), Definition.TooltipText.IsEmpty());
			}
		});

		It(TEXT("should create Gameplay Effect blueprint"), [this]()
		{
			TestFalse(TEXT("Definitions not empty"), EffectCreationMenu->Definitions.IsEmpty());

			if (!EffectCreationMenu->Definitions.IsValidIndex(0))
			{
				return AddError(TEXT("Invalid index for definitions"));
			}

			const FGSCGameplayEffectCreationData Definition = EffectCreationMenu->Definitions[0];
			AddInfo(FString::Printf(TEXT("About to create BP from definition: %s"), *Definition.ToString()));

			const TSharedPtr<FGSCMenuItem> MenuItem = MakeShared<FGSCMenuItem>();
			MenuItem->AssetPrefix = Definition.AssetPrefix;
			MenuItem->DefaultAssetName = Definition.BaseName;
			MenuItem->TooltipText = Definition.TooltipText;
			MenuItem->ParentClass = Definition.ParentClass;

			const TArray<FString> SelectedPaths = { TargetPackagePath };
			FGSCMenuItem::CreateBlueprintAction(MenuItem, SelectedPaths);

			FString AssetName = MenuItem->AssetPrefix + MenuItem->DefaultAssetName;

			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			TArray<FAssetData> AssetsData;
			const FName PackageName = FName(*(TargetPackagePath / AssetName));
			if (!AssetRegistryModule.Get().GetAssetsByPackageName(PackageName, AssetsData))
			{
				return AddError(FString::Printf(TEXT("Unable to get asset data for %s"), *PackageName.ToString()));
			}

			TestEqual(TEXT("Assets data expected size"), AssetsData.Num(), 1);

			if (!AssetsData.IsValidIndex(0))
			{
				return AddError(FString::Printf(TEXT("Unable to get asset data for %s"), *PackageName.ToString()));
			}

			FAssetData AssetData = AssetsData[0];
			FString String = AssetData.ToSoftObjectPath().ToString();
			TestEqual(
				TEXT("Asset data path ok"),
				AssetData.ToSoftObjectPath().ToString(),
				// eg. /Game/Developers/GASCompanion/GSCCreationMenu/GE_Damage.GE_Damage
				FString::Printf(TEXT("%s/%s.%s"), TargetPackagePath, *AssetName, *AssetName)
			);

			FString GeneratedClass;
			AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClass);
			TestContains(
				TEXT("Blueprint GeneratedClass"),
				GeneratedClass,
				FString::Printf(TEXT("BlueprintGeneratedClass'%s_C'"), *AssetData.ToSoftObjectPath().ToString())
			);
			
			FString NativeParentClassPath;
			AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath);
			TestContains(
				TEXT("Blueprint NativeParentClassPath"),
				NativeParentClassPath,
				TEXT("Class'/Script/GameplayAbilities.GameplayEffect'")
			);
			
			FString ParentClassPath;
			AssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPath);
			TestContains(
				TEXT("Blueprint ParentClass"),
				ParentClassPath,
				TEXT("Class'/Script/GameplayAbilities.GameplayEffect'")
			);
			
			FString BlueprintPathWithinPackage;
			AssetData.GetTagValue(FBlueprintTags::BlueprintPathWithinPackage, BlueprintPathWithinPackage);
			TestEqual(TEXT("Blueprint BlueprintPath"), BlueprintPathWithinPackage, AssetName);

			AssetData.PrintAssetData();

			// Save for teardown (and not having dirty save packages dialog pop up on subsequent run or when closing editor)
			FString Filename = TestContentDir / TEXT("GSCCreationMenu") / AssetData.AssetName.ToString();
			AddInfo(FString::Printf(TEXT("About to save BP for filename: %s (%s)"), *Filename, *AssetData.PackageName.ToString()));
			
			TArray<UPackage*> PackagesToSave;
			PackagesToSave.Add(AssetData.GetPackage());
			UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
			
			AssetsToDelete.Add(AssetData);
		});

		AfterEach([this]()
		{
			EffectCreationMenu = nullptr;

			if (!AssetsToDelete.IsEmpty())
			{
				if (ObjectTools::DeleteAssets(AssetsToDelete, false) != AssetsToDelete.Num())
				{
					return AddError(TEXT("Failed to delete assets"));
				}

				if (!IFileManager::Get().DeleteDirectory(*TestContentDir, false, true))
				{
					return AddError(FString::Printf(TEXT("Unable to delete directory %s"), *TestContentDir));
				}
			}
		});
	});
}
