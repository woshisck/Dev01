#include "DevKitEditor/Rune/RuneCardBatchGeneratorCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Data/RuneDataAsset.h"
#include "Engine/Texture2D.h"
#include "AutomatedAssetImportData.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace Rune512Batch
{
	const FString GeneratedRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated");
	const FString GeneratedFlowRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow");
	const FString IconRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons");
	const FString AttackTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_AttackUp_01");
	const FString MoonlightTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_MoonBlade_01");
	const FString MoonlightBaseTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Base");
	const FString MoonlightForwardTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Forward");
	const FString MoonlightReversedTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Backward");
	const FString AttackTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Rune_AttackUp_01");

	struct FIconImportSpec
	{
		FString AssetName;
		FString FileName;
	};

	struct FLinkRecipeSpec
	{
		FString NeighborEffectTag;
		FString Suffix;
		ECombatCardLinkOrientation Direction = ECombatCardLinkOrientation::Forward;
		float Multiplier = 1.f;
	};

	struct FCardSpec
	{
		FString Key;
		FString DisplayName;
		FString Description;
		FString CardIdTag;
		TArray<FString> EffectTags;
		ECombatCardType CardType = ECombatCardType::Normal;
		ECombatCardLinkOrientation DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;
		ERuneType RuneType = ERuneType::Buff;
		FString TargetAssetName;
		FString TemplateDAPath;
		FString IconAssetName;
		FString BaseFlowTemplatePath;
		FString BaseFlowTargetName;
		TArray<FLinkRecipeSpec> LinkRecipes;
		TArray<FString> ManualTodos;
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

	FGameplayTag RequestTag(const FString& TagName, TArray<FString>& ReportLines)
	{
		const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*TagName), false);
		if (!Tag.IsValid())
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing GameplayTag: `%s`"), *TagName));
		}
		return Tag;
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

		UObject* Template = PackageExists(TemplatePackagePath)
			? StaticLoadObject(UObject::StaticClass(), nullptr, *ToObjectPath(TemplatePackagePath))
			: nullptr;
		if (!Template)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing template `%s`; cannot create `%s`."),
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

	TArray<FIconImportSpec> MakeIconSpecs()
	{
		return {
			{ TEXT("T_Rune512_Burn"), TEXT("T_Rune512_Burn.png") },
			{ TEXT("T_Rune512_Poison"), TEXT("T_Rune512_Poison.png") },
			{ TEXT("T_Rune512_Moonlight"), TEXT("T_Rune512_Moonlight.png") },
			{ TEXT("T_Rune512_Split"), TEXT("T_Rune512_Split.png") },
			{ TEXT("T_Rune512_Shield"), TEXT("T_Rune512_Shield.png") },
			{ TEXT("T_Rune512_Pierce"), TEXT("T_Rune512_Pierce.png") },
			{ TEXT("T_Rune512_Attack"), TEXT("T_Rune512_Attack.png") },
			{ TEXT("T_Rune512_ReduceDamage"), TEXT("T_Rune512_ReduceDamage.png") },
		};
	}

	void ImportIcons(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/512RuneIcons"));
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		ReportLines.Add(TEXT("## Icon import"));
		for (const FIconImportSpec& IconSpec : MakeIconSpecs())
		{
			const FString SourceFile = FPaths::Combine(SourceDir, IconSpec.FileName);
			const FString IconPackagePath = IconRoot + TEXT("/") + IconSpec.AssetName;
			if (!FPaths::FileExists(SourceFile))
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing source PNG `%s`."), *SourceFile));
				continue;
			}

			if (LoadAssetByPackagePath<UTexture2D>(IconPackagePath))
			{
				ReportLines.Add(FString::Printf(TEXT("- Found icon `%s`."), *IconPackagePath));
				continue;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> `%s`."),
				bDryRun ? TEXT("Would import") : TEXT("Imported"),
				*SourceFile,
				*IconRoot));

			if (bDryRun)
			{
				continue;
			}

			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = IconRoot;
			ImportData->Filenames.Add(SourceFile);
			ImportData->bReplaceExisting = true;
			const TArray<UObject*> ImportedAssets = AssetTools.ImportAssetsAutomated(ImportData);
			for (UObject* ImportedAsset : ImportedAssets)
			{
				if (ImportedAsset)
				{
					ImportedAsset->MarkPackageDirty();
					DirtyPackages.AddUnique(ImportedAsset->GetPackage());
				}
			}
		}
	}

	TArray<FLinkRecipeSpec> MakeMoonlightRecipes(ECombatCardLinkOrientation Direction)
	{
		const FString DirectionPrefix = Direction == ECombatCardLinkOrientation::Forward ? TEXT("Forward") : TEXT("Reversed");
		const TArray<TPair<FString, FString>> Effects = {
			{ TEXT("Card.Effect.Burn"), TEXT("Burn") },
			{ TEXT("Card.Effect.Poison"), TEXT("Poison") },
			{ TEXT("Card.Effect.Split"), TEXT("Split") },
			{ TEXT("Card.Effect.Shield"), TEXT("Shield") },
			{ TEXT("Card.Effect.Pierce"), TEXT("Pierce") },
			{ TEXT("Card.Effect.Attack"), TEXT("Attack") },
			{ TEXT("Card.Effect.Defense.ReduceDamage"), TEXT("ReduceDamage") },
		};

		TArray<FLinkRecipeSpec> Recipes;
		for (const TPair<FString, FString>& Effect : Effects)
		{
			FLinkRecipeSpec Recipe;
			Recipe.NeighborEffectTag = Effect.Key;
			Recipe.Suffix = DirectionPrefix + TEXT("_") + Effect.Value;
			Recipe.Direction = Direction;
			Recipe.Multiplier = 1.f;
			Recipes.Add(Recipe);
		}
		return Recipes;
	}

	FCardSpec MakeNormalCard(
		const FString& Key,
		const FString& DisplayName,
		const FString& Description,
		const FString& CardIdTag,
		const TArray<FString>& EffectTags,
		const FString& IconAssetName,
		const FString& BaseFlowTemplatePath,
		const FString& BaseFlowTargetName,
		ERuneType RuneType,
		const TArray<FString>& ManualTodos)
	{
		FCardSpec Spec;
		Spec.Key = Key;
		Spec.DisplayName = DisplayName;
		Spec.Description = Description;
		Spec.CardIdTag = CardIdTag;
		Spec.EffectTags = EffectTags;
		Spec.CardType = ECombatCardType::Normal;
		Spec.RuneType = RuneType;
		Spec.TargetAssetName = FString::Printf(TEXT("DA_Rune512_%s"), *Key);
		Spec.TemplateDAPath = AttackTemplateDA;
		Spec.IconAssetName = IconAssetName;
		Spec.BaseFlowTemplatePath = BaseFlowTemplatePath;
		Spec.BaseFlowTargetName = BaseFlowTargetName;
		Spec.ManualTodos = ManualTodos;
		return Spec;
	}

	TArray<FCardSpec> MakeCardSpecs()
	{
		TArray<FCardSpec> Specs;
		Specs.Add(MakeNormalCard(
			TEXT("Burn"),
			TEXT("燃烧"),
			TEXT("对攻击的敌人造成燃烧。燃烧每秒对自身与附近敌人造成持续掉血，不可叠加；护甲会使燃烧持续时间减半。"),
			TEXT("Card.ID.Burn"),
			{ TEXT("Card.Effect.Burn") },
			TEXT("T_Rune512_Burn"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Burn"),
			TEXT("FA_Rune512_Burn_Base"),
			ERuneType::Debuff,
			{ TEXT("确认燃烧 GE 的持续时间、周围伤害半径和护甲减半逻辑。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Poison"),
			TEXT("中毒"),
			TEXT("对攻击的敌人造成中毒。3 秒后按层数毒性爆发；移动留下毒液路径，踩中路径的敌人获得固定一层中毒。"),
			TEXT("Card.ID.Poison"),
			{ TEXT("Card.Effect.Poison") },
			TEXT("T_Rune512_Poison"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Poison"),
			TEXT("FA_Rune512_Poison_Base"),
			ERuneType::Debuff,
			{ TEXT("补毒液路径 Niagara/碰撞体与 3 秒爆发 Execution。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Split"),
			TEXT("溅射/分裂"),
			TEXT("近战武器为溅射，攻击时对受伤敌人周围目标造成本次攻击 20% 伤害；远程武器为分裂，额外发射 2 个独立弹道。"),
			TEXT("Card.ID.Split"),
			{ TEXT("Card.Effect.Split") },
			TEXT("T_Rune512_Split"),
			MoonlightBaseTemplateFlow,
			TEXT("FA_Rune512_Split_Base"),
			ERuneType::Buff,
			{ TEXT("按武器类型分近战溅射/远程分裂，现阶段 Flow 需要手工细化。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Shield"),
			TEXT("护盾"),
			TEXT("触发后获得 20 点护甲。月光连携可获得 50 点月光护甲并反弹敌人攻击伤害。"),
			TEXT("Card.ID.Shield"),
			{ TEXT("Card.Effect.Shield") },
			TEXT("T_Rune512_Shield"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/IronArmor/FA_Rune_IronArmor"),
			TEXT("FA_Rune512_Shield_Base"),
			ERuneType::Buff,
			{ TEXT("确认护甲属性字段、反弹比例和护盾破碎反馈。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Pierce"),
			TEXT("穿透"),
			TEXT("对护甲造成额外 25% 伤害并造成击退。月光连携可使月刃不被敌人消耗，碰撞静态物体时消失。"),
			TEXT("Card.ID.Pierce"),
			{ TEXT("Card.Effect.Pierce") },
			TEXT("T_Rune512_Pierce"),
			MoonlightBaseTemplateFlow,
			TEXT("FA_Rune512_Pierce_Base"),
			ERuneType::Buff,
			{ TEXT("在月刃节点中启用 BonusArmorDamageMultiplier / bDestroyOnWorldStaticHit / bForcePureDamage。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Attack"),
			TEXT("攻击"),
			TEXT("攻击伤害加成。月光连携可转为更快更强的竖向月光斩，或低速大月刃持续多段伤害。"),
			TEXT("Card.ID.AttackUp"),
			{ TEXT("Card.Effect.Attack"), TEXT("Card.Effect.Buff.AttackUp") },
			TEXT("T_Rune512_Attack"),
			AttackTemplateFlow,
			TEXT("FA_Rune512_Attack_Base"),
			ERuneType::Buff,
			{ TEXT("确认加成数值与战斗属性快照恢复逻辑。") }));

		Specs.Add(MakeNormalCard(
			TEXT("ReduceDamage"),
			TEXT("减伤"),
			TEXT("攻击过程中受到的伤害减少 15%。月光连携可提供短时自身减伤或降低敌人攻速、移速。"),
			TEXT("Card.ID.ReduceDamage"),
			{ TEXT("Card.Effect.Defense.ReduceDamage") },
			TEXT("T_Rune512_ReduceDamage"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/Weakness_Unveiled/FA_Rune_WeaknessUnveiled"),
			TEXT("FA_Rune512_ReduceDamage_Base"),
			ERuneType::Buff,
			{ TEXT("补临时减伤 GE 与敌方攻速/移速削弱 GE。") }));

		FCardSpec MoonlightForward;
		MoonlightForward.Key = TEXT("Moonlight_Forward");
		MoonlightForward.DisplayName = TEXT("月光 Forward");
		MoonlightForward.Description = TEXT("玩家攻击时生成一道月光远程弹道。此测试卡默认正向连携，读取上一张卡的效果 Tag。");
		MoonlightForward.CardIdTag = TEXT("Card.ID.Moonlight");
		MoonlightForward.EffectTags = { TEXT("Card.Effect.Moonlight") };
		MoonlightForward.CardType = ECombatCardType::Link;
		MoonlightForward.DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;
		MoonlightForward.TargetAssetName = TEXT("DA_Rune512_Moonlight_Forward");
		MoonlightForward.TemplateDAPath = MoonlightTemplateDA;
		MoonlightForward.IconAssetName = TEXT("T_Rune512_Moonlight");
		MoonlightForward.BaseFlowTemplatePath = MoonlightBaseTemplateFlow;
		MoonlightForward.BaseFlowTargetName = TEXT("FA_Rune512_Moonlight_Base");
		MoonlightForward.LinkRecipes.Append(MakeMoonlightRecipes(ECombatCardLinkOrientation::Forward));
		MoonlightForward.LinkRecipes.Append(MakeMoonlightRecipes(ECombatCardLinkOrientation::Reversed));
		MoonlightForward.ManualTodos = {
			TEXT("Forward/Reversed 配方 Flow 已按模板复制，复杂节点连接与 Niagara 参数需要按配置文档检查。")
		};
		Specs.Add(MoonlightForward);

		FCardSpec MoonlightReversed = MoonlightForward;
		MoonlightReversed.Key = TEXT("Moonlight_Reversed");
		MoonlightReversed.DisplayName = TEXT("月光 Reversed");
		MoonlightReversed.Description = TEXT("玩家攻击时生成一道月光远程弹道。此测试卡默认反向连携，强化下一张满足条件的卡。");
		MoonlightReversed.DefaultLinkOrientation = ECombatCardLinkOrientation::Reversed;
		MoonlightReversed.TargetAssetName = TEXT("DA_Rune512_Moonlight_Reversed");
		Specs.Add(MoonlightReversed);

		return Specs;
	}

	UFlowAsset* EnsureFlowAsset(
		const FString& TemplatePath,
		const FString& TargetName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (TemplatePath.IsEmpty() || TargetName.IsEmpty())
		{
			return nullptr;
		}
		const FString TargetPath = GeneratedFlowRoot + TEXT("/") + TargetName;
		UObject* FlowObject = DuplicateAssetIfMissing(TemplatePath, TargetPath, bDryRun, ReportLines, DirtyPackages);
		return Cast<UFlowAsset>(FlowObject);
	}

	void ApplyCardSpec(
		const FCardSpec& Spec,
		int32 RuneId,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const FString TargetPath = GeneratedRoot + TEXT("/") + Spec.TargetAssetName;
		ReportLines.Add(FString::Printf(TEXT("## Card `%s`"), *Spec.TargetAssetName));

		RequestTag(Spec.CardIdTag, ReportLines);
		for (const FString& EffectTag : Spec.EffectTags)
		{
			RequestTag(EffectTag, ReportLines);
		}

		UFlowAsset* BaseFlow = EnsureFlowAsset(
			Spec.BaseFlowTemplatePath,
			Spec.BaseFlowTargetName,
			bDryRun,
			ReportLines,
			DirtyPackages);

		URuneDataAsset* RuneDA = Cast<URuneDataAsset>(DuplicateAssetIfMissing(
			Spec.TemplateDAPath,
			TargetPath,
			bDryRun,
			ReportLines,
			DirtyPackages));

		if (bDryRun)
		{
			for (const FLinkRecipeSpec& LinkSpec : Spec.LinkRecipes)
			{
				const FString TemplateFlow = LinkSpec.Direction == ECombatCardLinkOrientation::Forward
					? MoonlightForwardTemplateFlow
					: MoonlightReversedTemplateFlow;
				const FString FlowName = TEXT("FA_Rune512_Moonlight_") + LinkSpec.Suffix;
				EnsureFlowAsset(TemplateFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			}
			ReportLines.Add(TEXT("- Dry run: no DA fields were changed."));
			for (const FString& Todo : Spec.ManualTodos)
			{
				ReportLines.Add(FString::Printf(TEXT("- Manual check: %s"), *Todo));
			}
			return;
		}

		if (!RuneDA)
		{
			ReportLines.Add(TEXT("- Cannot update DA because target asset was not loaded or created."));
			return;
		}

		UTexture2D* Icon = LoadAssetByPackagePath<UTexture2D>(IconRoot + TEXT("/") + Spec.IconAssetName);
		if (!Icon)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing icon asset `%s/%s`."), *IconRoot, *Spec.IconAssetName));
		}

		FRuneInstance& RuneInfo = RuneDA->RuneInfo;
		RuneInfo.RuneConfig.RuneName = FName(*Spec.DisplayName);
		RuneInfo.RuneConfig.RuneDescription = FText::FromString(Spec.Description);
		RuneInfo.RuneConfig.RuneIcon = Icon;
		RuneInfo.RuneConfig.RuneID = RuneId;
		RuneInfo.RuneConfig.RuneType = Spec.RuneType;
		RuneInfo.Flow.FlowAsset = BaseFlow;

		FCombatCardConfig& CombatCard = RuneInfo.CombatCard;
		CombatCard.bIsCombatCard = true;
		CombatCard.CardType = Spec.CardType;
		CombatCard.CardIdTag = RequestTag(Spec.CardIdTag, ReportLines);
		CombatCard.CardEffectTags.Reset();
		for (const FString& EffectTag : Spec.EffectTags)
		{
			const FGameplayTag Tag = RequestTag(EffectTag, ReportLines);
			if (Tag.IsValid())
			{
				CombatCard.CardEffectTags.AddTag(Tag);
			}
		}
		CombatCard.RequiredAction = ECardRequiredAction::Any;
		CombatCard.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
		CombatCard.BaseFlow = BaseFlow;
		CombatCard.LinkRecipes.Reset();
		CombatCard.DefaultLinkOrientation = Spec.DefaultLinkOrientation;
		CombatCard.DisplayName = FText::FromString(Spec.DisplayName);
		CombatCard.HUDReasonText = FText::FromString(Spec.Description);

		for (const FLinkRecipeSpec& LinkSpec : Spec.LinkRecipes)
		{
			const FString TemplateFlow = LinkSpec.Direction == ECombatCardLinkOrientation::Forward
				? MoonlightForwardTemplateFlow
				: MoonlightReversedTemplateFlow;
			const FString FlowName = TEXT("FA_Rune512_Moonlight_") + LinkSpec.Suffix;
			UFlowAsset* LinkFlow = EnsureFlowAsset(TemplateFlow, FlowName, bDryRun, ReportLines, DirtyPackages);

			FCombatCardLinkRecipe Recipe;
			Recipe.Direction = LinkSpec.Direction;
			const FGameplayTag NeighborTag = RequestTag(LinkSpec.NeighborEffectTag, ReportLines);
			if (NeighborTag.IsValid())
			{
				Recipe.Condition.RequiredNeighborEffectTags.AddTag(NeighborTag);
			}
			Recipe.LinkFlow = LinkFlow;
			Recipe.Multiplier = LinkSpec.Multiplier;
			Recipe.ReasonText = FText::FromString(LinkSpec.Suffix);
			CombatCard.LinkRecipes.Add(Recipe);
		}

		RuneDA->MarkPackageDirty();
		DirtyPackages.AddUnique(RuneDA->GetPackage());
		ReportLines.Add(TEXT("- Updated RuneInfo -> CombatCard."));

		for (const FString& Todo : Spec.ManualTodos)
		{
			ReportLines.Add(FString::Printf(TEXT("- Manual check: %s"), *Todo));
		}
	}
}

URuneCardBatchGeneratorCommandlet::URuneCardBatchGeneratorCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URuneCardBatchGeneratorCommandlet::Main(const FString& Params)
{
	using namespace Rune512Batch;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bImportIcons = !Params.Contains(TEXT("NoImportIcons"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# 512 Rune Card Batch Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Generated DA root: `%s`"), *GeneratedRoot));
	ReportLines.Add(FString::Printf(TEXT("- Generated Flow root: `%s`"), *GeneratedFlowRoot));
	ReportLines.Add(TEXT(""));

	if (bImportIcons)
	{
		ImportIcons(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	int32 RuneId = 51201;
	for (const FCardSpec& Spec : MakeCardSpecs())
	{
		ApplyCardSpec(Spec, RuneId++, bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	ReportLines.Add(TEXT("## Skipped design-only items"));
	ReportLines.Add(TEXT("- Bleed(X), Rend(X), Bloodvine(X): documented only; no assets generated in this pass."));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Niagara/manual VFX todos"));
	ReportLines.Add(TEXT("- Burn / Poison / Shield / Pierce / ReduceDamage require Niagara or GameplayCue parameter passes after Flow graph review."));
	ReportLines.Add(TEXT("- Moonlight projectile visuals reuse BP_SlashWaveProjectile; split, cone, repeated-hit and wall-expire parameters are now exposed on BFNode_SpawnSlashWaveProjectile."));
	ReportLines.Add(TEXT("- Any Flow copied from a template must be opened once and checked against the 512 design doc before gameplay signoff."));

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("512RuneCardBatchReport.md"));
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("512 rune card batch finished. Report: %s"), *ReportPath);
	return 0;
}
