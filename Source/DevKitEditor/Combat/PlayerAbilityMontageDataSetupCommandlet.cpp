#include "DevKitEditor/Combat/PlayerAbilityMontageDataSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimMontage.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Data/AbilityData.h"
#include "FileHelpers.h"
#include "GameplayTagContainer.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace PlayerAbilityMontageDataSetup
{
	const FString DefaultDirectory = TEXT("/Game/Docs/Data/Character/ExplicitActions");
	const FString DefaultWeaponDefinitionPath = TEXT("/Game/Developers/sunchuankai/LootTest/DA_WPN_THSword_Test");
	const FString ReportFileName = TEXT("PlayerAbilityMontageDataSetupReport.md");

	struct FMontageKeySpec
	{
		const TCHAR* TagName;
		const TCHAR* MontagePath;
	};

	struct FAbilityDataAssetSpec
	{
		const TCHAR* AssetName;
		UClass* AssetClass;
		TArray<FMontageKeySpec> MontageKeys;
		TArray<const TCHAR*> PassiveKeys;
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

	UAbilityData* LoadAbilityData(const FString& PackagePath)
	{
		if (UAbilityData* Existing = FindObject<UAbilityData>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<UAbilityData>(StaticLoadObject(UAbilityData::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	UWeaponDefinition* LoadWeaponDefinition(const FString& PackagePath)
	{
		if (UWeaponDefinition* Existing = FindObject<UWeaponDefinition>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<UWeaponDefinition>(StaticLoadObject(UWeaponDefinition::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	UAbilityData* CreateAbilityData(const FString& PackagePath, UClass* AssetClass)
	{
		if (!AssetClass || !AssetClass->IsChildOf(UAbilityData::StaticClass()))
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UAbilityData* NewAsset = NewObject<UAbilityData>(
			Package,
			AssetClass,
			*FPackageName::GetLongPackageAssetName(PackagePath),
			RF_Public | RF_Standalone | RF_Transactional);

		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
			NewAsset->MarkPackageDirty();
		}

		return NewAsset;
	}

	UAnimMontage* LoadMontage(const TCHAR* PackagePath)
	{
		if (!PackagePath || !*PackagePath)
		{
			return nullptr;
		}

		const FString MontagePackagePath(PackagePath);
		if (UAnimMontage* Existing = FindObject<UAnimMontage>(nullptr, *ToObjectPath(MontagePackagePath)))
		{
			return Existing;
		}

		return Cast<UAnimMontage>(StaticLoadObject(UAnimMontage::StaticClass(), nullptr, *ToObjectPath(MontagePackagePath)));
	}

	bool EnsureMontageEntry(
		UAbilityData* AbilityData,
		const FMontageKeySpec& KeySpec,
		bool bForceMontageAssignment,
		bool& bOutAddedKey,
		bool& bOutAssignedMontage,
		FString& OutFailure)
	{
		bOutAddedKey = false;
		bOutAssignedMontage = false;
		OutFailure.Reset();

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(KeySpec.TagName), false);
		if (!AbilityData || !Tag.IsValid())
		{
			OutFailure = FString::Printf(TEXT("invalid tag `%s`"), KeySpec.TagName ? KeySpec.TagName : TEXT("(null)"));
			return false;
		}

		TObjectPtr<UAnimMontage>* ExistingMontage = AbilityData->MontageMap.Find(Tag);
		if (!ExistingMontage)
		{
			ExistingMontage = &AbilityData->MontageMap.Add(Tag, nullptr);
			bOutAddedKey = true;
		}

		if (KeySpec.MontagePath && *KeySpec.MontagePath && (bForceMontageAssignment || ExistingMontage->Get() == nullptr))
		{
			UAnimMontage* Montage = LoadMontage(KeySpec.MontagePath);
			if (!Montage)
			{
				OutFailure = FString::Printf(TEXT("failed to load montage `%s` for `%s`"), KeySpec.MontagePath, KeySpec.TagName);
			}
			else if (ExistingMontage->Get() != Montage)
			{
				*ExistingMontage = Montage;
				bOutAssignedMontage = true;
			}
		}

		return bOutAddedKey || bOutAssignedMontage;
	}

	bool EnsurePassiveEntry(
		UAbilityData* AbilityData,
		const TCHAR* TagName,
		bool& bOutAddedKey,
		FString& OutFailure)
	{
		bOutAddedKey = false;
		OutFailure.Reset();

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (!AbilityData || !Tag.IsValid())
		{
			OutFailure = FString::Printf(TEXT("invalid passive tag `%s`"), TagName ? TagName : TEXT("(null)"));
			return false;
		}

		if (!AbilityData->PassiveMap.Contains(Tag))
		{
			AbilityData->PassiveMap.Add(Tag, FPassiveActionData());
			bOutAddedKey = true;
		}

		return bOutAddedKey;
	}

	void AddAssetSpecReport(
		TArray<FString>& ReportLines,
		const FString& PackagePath,
		UAbilityData* AbilityData,
		bool bCreated,
		int32 AddedKeys,
		int32 AssignedMontages,
		int32 AddedPassiveKeys)
	{
		ReportLines.Add(FString::Printf(
			TEXT("- `%s`: %s `%s`, added missing montage keys: `%d`, assigned montages: `%d`, added missing passive keys: `%d`"),
			*PackagePath,
			bCreated ? TEXT("created") : TEXT("loaded"),
			*GetNameSafe(AbilityData),
			AddedKeys,
			AssignedMontages,
			AddedPassiveKeys));
	}

	bool AssignWeaponAbilityData(
		UWeaponDefinition* WeaponDefinition,
		const TMap<FString, UAbilityData*>& AbilityDataByName,
		TArray<FString>& ReportLines)
	{
		if (!WeaponDefinition)
		{
			return false;
		}

		bool bChanged = false;
		WeaponDefinition->Modify();

		auto AssignField = [&bChanged, &ReportLines, WeaponDefinition](
			TObjectPtr<UAbilityData>& Field,
			const TCHAR* FieldName,
			UAbilityData* NewValue)
		{
			if (!NewValue)
			{
				ReportLines.Add(FString::Printf(TEXT("  - `%s`: skipped, source AbilityData missing."), FieldName));
				return;
			}

			if (Field.Get() != NewValue)
			{
				Field = NewValue;
				bChanged = true;
			}

			ReportLines.Add(FString::Printf(
				TEXT("  - `%s`: `%s`"),
				FieldName,
				*GetNameSafe(Field.Get())));
		};

		ReportLines.Add(FString::Printf(TEXT("- Weapon definition `%s` typed AbilityData:"), *GetNameSafe(WeaponDefinition)));
		AssignField(WeaponDefinition->AttackAbilityData, TEXT("AttackAbilityData"), AbilityDataByName.FindRef(TEXT("DA_WeaponAttack")));
		AssignField(WeaponDefinition->WeaponSkillAbilityData, TEXT("WeaponSkillAbilityData"), AbilityDataByName.FindRef(TEXT("DA_WeaponSkill")));
		AssignField(WeaponDefinition->SpecialAbilityData, TEXT("SpecialAbilityData"), AbilityDataByName.FindRef(TEXT("DA_Special")));

		if (bChanged)
		{
			WeaponDefinition->MarkPackageDirty();
		}

		return bChanged;
	}
}

UPlayerAbilityMontageDataSetupCommandlet::UPlayerAbilityMontageDataSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UPlayerAbilityMontageDataSetupCommandlet::Main(const FString& Params)
{
	using namespace PlayerAbilityMontageDataSetup;

	FString TargetDirectory = DefaultDirectory;
	FParse::Value(*Params, TEXT("Directory="), TargetDirectory);

	FString WeaponDefinitionPath = DefaultWeaponDefinitionPath;
	FParse::Value(*Params, TEXT("WeaponDefinition="), WeaponDefinitionPath);
	const bool bSkipWeaponDefinition = FParse::Param(*Params, TEXT("SkipWeaponDefinition"));
	const bool bForceMontageAssignment = FParse::Param(*Params, TEXT("ForceMontageAssignments"));

	static const FAbilityDataAssetSpec Specs[] = {
		{
			TEXT("DA_WeaponAttack"),
			UWeaponAttackAbilityMontageData::StaticClass(),
			{
				{ TEXT("PlayerState.AbilityCast.Attack.Combo1"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_01_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Attack.Combo2"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_02_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Attack.Combo3"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_03_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Attack.Combo4"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_04_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Dash.Combo1"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash.Combo2"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash.Combo3"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash.Combo4"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash.Dash1"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.Dash.DashATK1"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
				{ TEXT("PlayerState.AbilityCast.DashAtk"), TEXT("/YogAnimSource/ElianAnim/Common/AM_Common_Dash_Seq_01") },
			},
			{},
		},
		{
			TEXT("DA_WeaponSkill"),
			UWeaponSkillAbilityMontageData::StaticClass(),
			{
				{ TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Guard_01_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_06_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_07_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_08_Seq_Montage") },
			},
			{
				TEXT("Action.HitReact.Blocked"),
			},
		},
		{
			TEXT("DA_Special"),
			USpecialAbilityMontageData::StaticClass(),
			{
				{ TEXT("PlayerState.AbilityCast.Special.Combo1"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_09_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Special.Combo2"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_10_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Special.Combo3"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_11_Seq_Montage") },
				{ TEXT("PlayerState.AbilityCast.Special.Combo4"), TEXT("/Game/Animation/1H-2HSword/Montage/1H_Attack_12_Seq_Montage") },
			},
			{},
		},
	};

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Player Ability Montage Data Setup"));
	ReportLines.Add(FString::Printf(TEXT("- Target directory: `%s`"), *TargetDirectory));
	ReportLines.Add(FString::Printf(TEXT("- Force montage assignments: `%s`"), bForceMontageAssignment ? TEXT("yes") : TEXT("no")));

	TArray<UPackage*> PackagesToSave;
	TMap<FString, UAbilityData*> AbilityDataByName;
	bool bHadFailure = false;

	for (const FAbilityDataAssetSpec& Spec : Specs)
	{
		const FString PackagePath = TargetDirectory / Spec.AssetName;
		bool bCreated = false;
		UAbilityData* AbilityData = LoadAbilityData(PackagePath);
		if (!AbilityData)
		{
			AbilityData = CreateAbilityData(PackagePath, Spec.AssetClass);
			bCreated = AbilityData != nullptr;
		}

		if (!AbilityData)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s`: failed to create or load."), *PackagePath));
			bHadFailure = true;
			continue;
		}

		if (!AbilityData->IsA(Spec.AssetClass))
		{
			ReportLines.Add(FString::Printf(
				TEXT("- `%s`: existing asset class `%s` does not match expected `%s`."),
				*PackagePath,
				*GetNameSafe(AbilityData->GetClass()),
				*GetNameSafe(Spec.AssetClass)));
			bHadFailure = true;
			continue;
		}

		int32 AddedKeys = 0;
		int32 AssignedMontages = 0;
		int32 AddedPassiveKeys = 0;
		bool bAbilityDataChanged = bCreated;
		for (const FMontageKeySpec& KeySpec : Spec.MontageKeys)
		{
			bool bAddedKey = false;
			bool bAssignedMontage = false;
			FString Failure;
			if (EnsureMontageEntry(AbilityData, KeySpec, bForceMontageAssignment, bAddedKey, bAssignedMontage, Failure))
			{
				bAbilityDataChanged = true;
			}

			AddedKeys += bAddedKey ? 1 : 0;
			AssignedMontages += bAssignedMontage ? 1 : 0;
			if (!Failure.IsEmpty())
			{
				ReportLines.Add(FString::Printf(TEXT("  - `%s`: %s"), KeySpec.TagName, *Failure));
				bHadFailure = true;
			}
		}
		for (const TCHAR* PassiveKey : Spec.PassiveKeys)
		{
			bool bAddedPassiveKey = false;
			FString Failure;
			if (EnsurePassiveEntry(AbilityData, PassiveKey, bAddedPassiveKey, Failure))
			{
				bAbilityDataChanged = true;
			}

			AddedPassiveKeys += bAddedPassiveKey ? 1 : 0;
			if (!Failure.IsEmpty())
			{
				ReportLines.Add(FString::Printf(TEXT("  - `%s`: %s"), PassiveKey ? PassiveKey : TEXT("(null)"), *Failure));
				bHadFailure = true;
			}
		}

		if (bAbilityDataChanged)
		{
			AbilityData->MarkPackageDirty();
			PackagesToSave.AddUnique(AbilityData->GetPackage());
		}

		AbilityDataByName.Add(Spec.AssetName, AbilityData);
		AddAssetSpecReport(ReportLines, PackagePath, AbilityData, bCreated, AddedKeys, AssignedMontages, AddedPassiveKeys);
	}

	if (!bSkipWeaponDefinition)
	{
		UWeaponDefinition* WeaponDefinition = LoadWeaponDefinition(WeaponDefinitionPath);
		if (!WeaponDefinition)
		{
			ReportLines.Add(FString::Printf(TEXT("- Weapon definition `%s`: failed to load."), *WeaponDefinitionPath));
			bHadFailure = true;
		}
		else if (AssignWeaponAbilityData(WeaponDefinition, AbilityDataByName, ReportLines))
		{
			PackagesToSave.AddUnique(WeaponDefinition->GetPackage());
		}
	}

	const bool bSaved = PackagesToSave.IsEmpty() || UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	ReportLines.Add(FString::Printf(TEXT("- Saved packages: `%s`"), bSaved ? TEXT("yes") : TEXT("no")));

	FString SavedReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, SavedReportPath, SharedReportPath);
	UE_LOG(LogTemp, Display, TEXT("PlayerAbilityMontageDataSetup complete. Report: %s"), *SharedReportPath);

	return (!bHadFailure && bSaved) ? 0 : 1;
}
