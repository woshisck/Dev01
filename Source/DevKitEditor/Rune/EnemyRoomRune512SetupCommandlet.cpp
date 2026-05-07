#include "Rune/EnemyRoomRune512SetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Data/EnemyData.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace EnemyRoomRune512Setup
{
	const FString SourceRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated");
	const FString TargetRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom");
	const FString ReportFileName = TEXT("512EnemyRoomRuneSetupReport.md");

	struct FEnemyRoomRuneSpec
	{
		FString Key;
		FString SourceAssetName;
		FString TargetAssetName;
		FString DisplayName;
		FString Description;
		ERuneTriggerType TriggerType = ERuneTriggerType::Passive;
		int32 RuneId = 0;
		int32 DefaultDifficultyScore = 1;
		FString UsageNote;
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

	UObject* DuplicateAssetIfMissing(
		const FString& TemplatePackagePath,
		const FString& TargetPackagePath,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UObject* LoadedExisting = FindObject<UObject>(nullptr, *ToObjectPath(TargetPackagePath)))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found existing asset `%s`."), *TargetPackagePath));
			return LoadedExisting;
		}

		if (PackageExists(TargetPackagePath))
		{
			if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ToObjectPath(TargetPackagePath)))
			{
				ReportLines.Add(FString::Printf(TEXT("- Found existing asset `%s`."), *TargetPackagePath));
				return Existing;
			}
		}

		UObject* Template = LoadAssetByPackagePath<UObject>(TemplatePackagePath);
		if (!Template)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing source `%s`; cannot create `%s`."),
				*TemplatePackagePath,
				*TargetPackagePath));
			return nullptr;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s `%s` -> `%s`."),
			bDryRun ? TEXT("Would duplicate") : TEXT("Duplicated"),
			*TemplatePackagePath,
			*TargetPackagePath));

		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const FString TargetName = FPackageName::GetLongPackageAssetName(TargetPackagePath);
		const FString TargetDir = FPackageName::GetLongPackagePath(TargetPackagePath);
		UObject* NewAsset = AssetTools.DuplicateAsset(TargetName, TargetDir, Template);
		if (NewAsset)
		{
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		else
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to duplicate `%s`."), *TargetPackagePath));
		}
		return NewAsset;
	}

	TArray<FEnemyRoomRuneSpec> MakeSpecs()
	{
		return {
			{
				TEXT("Attack"),
				TEXT("DA_Rune512_Attack"),
				TEXT("DA_Rune512_EnemyRoom_Attack"),
				TEXT("512敌人·攻击强化"),
				TEXT("通用敌人/关卡 Buff：敌人生成时获得攻击伤害加成。适合放入 DA_Room.BuffPool 或精英敌人的 EnemyBuffPool。"),
				ERuneTriggerType::Passive,
				51251,
				2,
				TEXT("Passive; stable baseline pressure buff.")
			},
			{
				TEXT("Shield"),
				TEXT("DA_Rune512_Shield"),
				TEXT("DA_Rune512_EnemyRoom_Shield"),
				TEXT("512敌人·护盾"),
				TEXT("通用敌人/关卡 Buff：敌人生成时获得护盾/护甲类保护。适合做死亡守卫、精英房或高难度房间强化。"),
				ERuneTriggerType::Passive,
				51252,
				2,
				TEXT("Passive; replaces old Playtest_GA IronArmor recommendations.")
			},
			{
				TEXT("ReduceDamage"),
				TEXT("DA_Rune512_ReduceDamage"),
				TEXT("DA_Rune512_EnemyRoom_ReduceDamage"),
				TEXT("512敌人·减伤"),
				TEXT("通用敌人/关卡 Buff：敌人生成时获得受伤减免或削弱攻击者的防御型效果。适合耐久型房间。"),
				ERuneTriggerType::Passive,
				51253,
				2,
				TEXT("Passive; defensive room pressure buff.")
			},
			{
				TEXT("Burn"),
				TEXT("DA_Rune512_Burn"),
				TEXT("DA_Rune512_EnemyRoom_Burn"),
				TEXT("512敌人·燃烧攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时触发燃烧效果。适合短房间制造持续掉血压力。"),
				ERuneTriggerType::OnAttackHit,
				51254,
				2,
				TEXT("OnAttackHit; requires enemy hit events.")
			},
			{
				TEXT("Poison"),
				TEXT("DA_Rune512_Poison"),
				TEXT("DA_Rune512_EnemyRoom_Poison"),
				TEXT("512敌人·中毒攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时触发中毒效果。适合慢压制和走位惩罚。"),
				ERuneTriggerType::OnAttackHit,
				51255,
				2,
				TEXT("OnAttackHit; requires enemy hit events.")
			},
			{
				TEXT("Pierce"),
				TEXT("DA_Rune512_Pierce"),
				TEXT("DA_Rune512_EnemyRoom_Pierce"),
				TEXT("512敌人·穿透攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时触发穿透/破盾倾向的攻击效果。适合对抗护盾构筑。"),
				ERuneTriggerType::OnAttackHit,
				51256,
				2,
				TEXT("OnAttackHit; verify current Flow tuning before using in core rooms.")
			},
			{
				TEXT("Split"),
				TEXT("DA_Rune512_Split"),
				TEXT("DA_Rune512_EnemyRoom_Split"),
				TEXT("512敌人·分裂攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时触发分裂/溅射倾向的攻击效果。当前更适合测试房或精英房验证。"),
				ERuneTriggerType::OnAttackHit,
				51257,
				3,
				TEXT("OnAttackHit; prototype/high-risk tuning.")
			},
			{
				TEXT("MoonlightForward"),
				TEXT("DA_Rune512_Moonlight_Forward"),
				TEXT("DA_Rune512_EnemyRoom_Moonlight"),
				TEXT("512敌人·月光攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时释放 512 月光基础效果。适合精英房或少量强敌。"),
				ERuneTriggerType::OnAttackHit,
				51258,
				3,
				TEXT("OnAttackHit; strong visual/projectile pressure.")
			},
			{
				TEXT("MoonlightReversed"),
				TEXT("DA_Rune512_Moonlight_Reversed"),
				TEXT("DA_Rune512_EnemyRoom_Moonlight_Reversed"),
				TEXT("512敌人·反向月光攻击"),
				TEXT("通用敌人/关卡 Buff：敌人命中目标时释放反向月光测试效果。建议只在调试或精英验证房使用。"),
				ERuneTriggerType::OnAttackHit,
				51259,
				4,
				TEXT("OnAttackHit; high-risk elite/test buff.")
			},
		};
	}

	void ConfigureEnemyRoomRune(
		const FEnemyRoomRuneSpec& Spec,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const FString SourcePath = SourceRoot + TEXT("/") + Spec.SourceAssetName;
		const FString TargetPath = TargetRoot + TEXT("/") + Spec.TargetAssetName;
		ReportLines.Add(FString::Printf(TEXT("## `%s`"), *Spec.TargetAssetName));

		URuneDataAsset* SourceRune = LoadAssetByPackagePath<URuneDataAsset>(SourcePath);
		if (!SourceRune)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing source rune `%s`."), *SourcePath));
			return;
		}

		if (!SourceRune->RuneInfo.Flow.FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Source rune `%s` has no FlowAsset."), *SourcePath));
		}

		URuneDataAsset* TargetRune = Cast<URuneDataAsset>(DuplicateAssetIfMissing(
			SourcePath,
			TargetPath,
			bDryRun,
			ReportLines,
			DirtyPackages));

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Would configure Trigger=%s RuneID=%d DefaultDifficulty=%d Note=%s."),
				*UEnum::GetValueAsString(Spec.TriggerType),
				Spec.RuneId,
				Spec.DefaultDifficultyScore,
				*Spec.UsageNote));
			return;
		}

		if (!TargetRune)
		{
			ReportLines.Add(TEXT("- Cannot configure target rune because it was not created or loaded."));
			return;
		}

		TargetRune->Modify();
		FRuneInstance& RuneInfo = TargetRune->RuneInfo;
		RuneInfo.RuneConfig.RuneName = FName(*Spec.DisplayName);
		RuneInfo.RuneConfig.RuneDescription = FText::FromString(Spec.Description);
		RuneInfo.RuneConfig.RuneID = Spec.RuneId;
		RuneInfo.RuneConfig.TriggerType = Spec.TriggerType;
		RuneInfo.Flow.FlowAsset = SourceRune->RuneInfo.Flow.FlowAsset;

		FCombatCardConfig& CombatCard = RuneInfo.CombatCard;
		CombatCard.bIsCombatCard = false;
		CombatCard.CardType = ECombatCardType::Normal;
		CombatCard.CardIdTag = FGameplayTag();
		CombatCard.CardEffectTags.Reset();
		CombatCard.CardTags.Reset();
		CombatCard.BaseFlow = nullptr;
		CombatCard.MatchedFlow = nullptr;
		CombatCard.LinkMode = ECardLinkMode::None;
		CombatCard.LinkConfig = FCombatCardLinkConfig();
		CombatCard.LinkRecipes.Reset();
		CombatCard.bRequiresComboFinisher = false;
		CombatCard.DisplayName = FText::GetEmpty();
		CombatCard.HUDReasonText = FText::GetEmpty();

		TargetRune->MarkPackageDirty();
		DirtyPackages.AddUnique(TargetRune->GetPackage());
		ReportLines.Add(FString::Printf(
			TEXT("- Configured enemy/room rune. Flow=`%s`; Trigger=%s; RuneID=%d; SuggestedDifficultyScore=%d; Note=%s."),
			*GetNameSafe(RuneInfo.Flow.FlowAsset.Get()),
			*UEnum::GetValueAsString(Spec.TriggerType),
			Spec.RuneId,
			Spec.DefaultDifficultyScore,
			*Spec.UsageNote));
	}

	FString GetRunePath(const URuneDataAsset* RuneDA)
	{
		return RuneDA ? RuneDA->GetPathName() : TEXT("None");
	}

	void ScanCurrentPools(TArray<FString>& ReportLines)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Current room/enemy BuffPool scan"));

		FARFilter RoomFilter;
		RoomFilter.ClassPaths.Add(URoomDataAsset::StaticClass()->GetClassPathName());
		RoomFilter.PackagePaths.Add(TEXT("/Game"));
		RoomFilter.bRecursivePaths = true;

		TArray<FAssetData> RoomAssets;
		AssetRegistry.GetAssets(RoomFilter, RoomAssets);
		RoomAssets.Sort([](const FAssetData& A, const FAssetData& B)
		{
			return A.PackageName.LexicalLess(B.PackageName);
		});

		for (const FAssetData& AssetData : RoomAssets)
		{
			URoomDataAsset* Room = Cast<URoomDataAsset>(AssetData.GetAsset());
			if (!Room || Room->BuffPool.IsEmpty())
			{
				continue;
			}

			ReportLines.Add(FString::Printf(TEXT("- Room `%s` BuffPool=%d"), *Room->GetPathName(), Room->BuffPool.Num()));
			for (const FBuffEntry& Entry : Room->BuffPool)
			{
				const FString RunePath = GetRunePath(Entry.RuneDA.Get());
				const bool bIs512EnemyRoom = RunePath.Contains(TEXT("/V2-RuneCard/512Generated/EnemyRoom/"));
				ReportLines.Add(FString::Printf(
					TEXT("  - %s Rune=`%s` Cost=%d Chance=%.2f"),
					bIs512EnemyRoom ? TEXT("[512EnemyRoom]") : TEXT("[Check]"),
					*RunePath,
					Entry.DifficultyScore,
					Entry.ApplyChance));
			}
		}

		FARFilter EnemyFilter;
		EnemyFilter.ClassPaths.Add(UEnemyData::StaticClass()->GetClassPathName());
		EnemyFilter.PackagePaths.Add(TEXT("/Game"));
		EnemyFilter.bRecursivePaths = true;

		TArray<FAssetData> EnemyAssets;
		AssetRegistry.GetAssets(EnemyFilter, EnemyAssets);
		EnemyAssets.Sort([](const FAssetData& A, const FAssetData& B)
		{
			return A.PackageName.LexicalLess(B.PackageName);
		});

		for (const FAssetData& AssetData : EnemyAssets)
		{
			UEnemyData* EnemyData = Cast<UEnemyData>(AssetData.GetAsset());
			if (!EnemyData || EnemyData->EnemyBuffPool.IsEmpty())
			{
				continue;
			}

			ReportLines.Add(FString::Printf(TEXT("- Enemy `%s` EnemyBuffPool=%d"), *EnemyData->GetPathName(), EnemyData->EnemyBuffPool.Num()));
			for (const FBuffEntry& Entry : EnemyData->EnemyBuffPool)
			{
				const FString RunePath = GetRunePath(Entry.RuneDA.Get());
				const bool bIs512EnemyRoom = RunePath.Contains(TEXT("/V2-RuneCard/512Generated/EnemyRoom/"));
				ReportLines.Add(FString::Printf(
					TEXT("  - %s Rune=`%s` Cost=%d Chance=%.2f"),
					bIs512EnemyRoom ? TEXT("[512EnemyRoom]") : TEXT("[Check]"),
					*RunePath,
					Entry.DifficultyScore,
					Entry.ApplyChance));
			}
		}
	}
}

UEnemyRoomRune512SetupCommandlet::UEnemyRoomRune512SetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UEnemyRoomRune512SetupCommandlet::Main(const FString& Params)
{
	using namespace EnemyRoomRune512Setup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bSkipScan = Params.Contains(TEXT("NoScan"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# 512 Enemy/Room Rune Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Source root: `%s`"), *SourceRoot));
	ReportLines.Add(FString::Printf(TEXT("- Target root: `%s`"), *TargetRoot));
	ReportLines.Add(TEXT(""));

	for (const FEnemyRoomRuneSpec& Spec : MakeSpecs())
	{
		ConfigureEnemyRoomRune(Spec, bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	if (!bSkipScan)
	{
		ScanCurrentPools(ReportLines);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(EnemyRoomRune512Setup::ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("512 enemy/room rune setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
