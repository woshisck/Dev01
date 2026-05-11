#include "DevKitEditor/Rune/RuneCardBatchGeneratorCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
#include "AbilitySystem/GameplayEffect/GE_RuneBurn.h"
#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "BuffFlow/Nodes/BFNode_CalcRuneGroundPathTransform.h"
#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "BuffFlow/Nodes/BFNode_OnDamageDealt.h"
#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"
#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/Nodes/BFNode_PlayRuneVFXProfile.h"
#include "BuffFlow/Nodes/BFNode_GrantSacrificePassive.h"
#include "BuffFlow/Nodes/BFNode_ApplyRuneEffectProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRangedProjectiles.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneAreaProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneGroundPathEffect.h"
#include "BuffFlow/Nodes/BFNode_SpawnRuneProjectileProfile.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/RuneCardEffectProfileDA.h"
#include "Data/RuneDataAsset.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Texture2D.h"
#include "Engine/Blueprint.h"
#include "AutomatedAssetImportData.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInterface.h"
#include "MaterialDomain.h"
#include "NiagaraSystem.h"
#include "Nodes/FlowNode.h"
#include "Projectile/MusketBullet.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace Rune512Batch
{
	const FString GeneratedRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated");
	const FString GeneratedFlowRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow");
	const FString GeneratedProfileRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Profile");
	const FString SacrificeGeneratedRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice");
	const FString SacrificeGeneratedFlowRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Sacrifice/Flow");
	const FString IconRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons");
	const FString VfxTextureRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures");
	const FString VfxMaterialRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials");
	const FString FlipbookMaterialPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials/M_Rune512_FlipbookSprite");
	const FString PoisonHitTexturePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures/T_Rune512_VFX_Poison_Hit");
	const FString PoisonSpreadTexturePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures/T_Rune512_VFX_Poison_Spread");
	const FString BurnNiagaraPath = TEXT("/Game/Art/EnvironmentAsset/VFX/Niagara/Fire/NS_Fire_Floor");
	const FString PoisonHitNiagaraPath = TEXT("/Game/Art/EnvironmentAsset/VFX/Niagara/Smoke/NS_Smoke_7_acid");
	const FString PoisonSpreadNiagaraPath = TEXT("/Game/Art/EnvironmentAsset/VFX/Niagara/Smoke/NS_Smoke_7_acid");
	const FString MoonlightBurnHitEventTag = TEXT("Action.Rune.MoonlightBurnHit");
	const FString MoonlightPoisonHitEventTag = TEXT("Action.Rune.MoonlightPoisonHit");
	const FString MoonlightPoisonExpireEventTag = TEXT("Action.Rune.MoonlightPoisonExpired");
	const FString PoisonEffectPath = TEXT("/Game/Code/GAS/Abilities/Shared/GE_Poison");
	const FString PoisonSplashEffectPath = TEXT("/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/GE_PoisonSplash");
	const FString PoisonGroundPathDecalMaterialPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials/M_Rune512_GroundPath_Poison_Decal");
	const FString BurnGroundPathDecalMaterialPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials/M_Rune512_GroundPath_Burn_Fan_Decal");
	const FString AttackTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_AttackUp_01");
	const FString MoonlightTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_MoonBlade_01");
	const FString MoonlightBaseTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Base");
	const FString MoonlightForwardTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Forward");
	const FString MoonlightReversedTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Backward");
	const FString AttackTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Rune_AttackUp_01");
	const FString ProductionTHSwordWeapon = TEXT("/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword");
	const FString ProductionRedSwordWeapon = TEXT("/Game/Code/Weapon/GreatSword/DA_WPN_RedSword");
	const FString ProductionHarquebusWeapon = TEXT("/Game/Code/Weapon/Harquebus/DA_WPN_Harquebus");
	const FString THSwordComboGraph = TEXT("/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test");
	const FString MusketBulletBlueprintPath = TEXT("/Game/Code/Weapon/BP_MusketBullet");
	const FString DamageBasicSetByCallerPath = TEXT("/Game/Code/GAS/GameplayEffects/GE_Damage_Basic_SetByCaller");

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
		ECombatCardTriggerTiming TriggerTiming = ECombatCardTriggerTiming::OnHit;
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

	struct FSacrificeSpec
	{
		FString Key;
		FString DisplayName;
		FString Description;
		FString Summary;
		FString TargetAssetName;
		FString FlowTargetName;
		FString IconAssetName;
		FSacrificeRunePassiveConfig Config;
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

	template <typename T>
	TSubclassOf<T> LoadBlueprintClassByPackagePath(const FString& PackagePath)
	{
		if (PackagePath.IsEmpty())
		{
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		const FString ClassPath = PackagePath + TEXT(".") + AssetName + TEXT("_C");
		UClass* LoadedClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), nullptr, *ClassPath));
		return LoadedClass && LoadedClass->IsChildOf(T::StaticClass()) ? LoadedClass : nullptr;
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

	UGameplayEffect* GetGameplayEffectDefaultObject(const FString& PackagePath)
	{
		TSubclassOf<UGameplayEffect> EffectClass = LoadBlueprintClassByPackagePath<UGameplayEffect>(PackagePath);
		return EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	}

	UPackage* GetGameplayEffectAssetPackage(const FString& PackagePath)
	{
		TSubclassOf<UGameplayEffect> EffectClass = LoadBlueprintClassByPackagePath<UGameplayEffect>(PackagePath);
		if (!EffectClass)
		{
			return nullptr;
		}

		if (UBlueprint* Blueprint = Cast<UBlueprint>(EffectClass->ClassGeneratedBy))
		{
			return Blueprint->GetOutermost();
		}

		return EffectClass->GetOutermost();
	}

	void ConfigurePoisonGameplayEffect(
		const FString& PackagePath,
		const TCHAR* Label,
		const float Duration,
		const float Period,
		const bool bExecuteOnApplication,
		const int32 StackLimit,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const FGameplayTag PoisonedTag = RequestTag(TEXT("Buff.Status.Poisoned"), ReportLines);
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Would configure `%s`: duration=%.1fs period=%.1fs stackLimit=%d execution=GEExec_PoisonDamage."),
				*PackagePath,
				Duration,
				Period,
				StackLimit));
			return;
		}

		UGameplayEffect* EffectCDO = GetGameplayEffectDefaultObject(PackagePath);
		if (!EffectCDO)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing poison GE `%s`; cannot configure %s."), *PackagePath, Label));
			return;
		}

		EffectCDO->Modify();
		EffectCDO->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		EffectCDO->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		EffectCDO->Period = FScalableFloat(Period);
		EffectCDO->bExecutePeriodicEffectOnApplication = bExecuteOnApplication;

		EffectCDO->Modifiers.Reset();
		EffectCDO->Executions.Reset();
		FGameplayEffectExecutionDefinition ExecDef;
		ExecDef.CalculationClass = UGEExec_PoisonDamage::StaticClass();
		EffectCDO->Executions.Add(ExecDef);

		if (PoisonedTag.IsValid())
		{
			FInheritedTagContainer GrantedTagChanges;
			GrantedTagChanges.Added.AddTag(PoisonedTag);
			UTargetTagsGameplayEffectComponent& TargetTagsComponent =
				EffectCDO->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
			TargetTagsComponent.SetAndApplyTargetTagChanges(GrantedTagChanges);
		}

		EffectCDO->StackingType = EGameplayEffectStackingType::AggregateByTarget;
		EffectCDO->StackLimitCount = FMath::Max(1, StackLimit);
		EffectCDO->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
		EffectCDO->StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
		EffectCDO->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

		EffectCDO->PostEditChange();
		EffectCDO->MarkPackageDirty();
		if (UPackage* Package = GetGameplayEffectAssetPackage(PackagePath))
		{
			DirtyPackages.AddUnique(Package);
		}
		else
		{
			DirtyPackages.AddUnique(EffectCDO->GetOutermost());
		}

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: removed old modifiers, added GEExec_PoisonDamage, duration=%.1fs period=%.1fs executeOnApply=%s stackLimit=%d grantedTag=Buff.Status.Poisoned."),
			*PackagePath,
			Duration,
			Period,
			bExecuteOnApplication ? TEXT("true") : TEXT("false"),
			StackLimit));
	}

	void ConfigurePoisonGameplayEffectAssets(
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Poison GameplayEffect assets"));
		RequestTag(TEXT("Data.Damage"), ReportLines);
		RequestTag(TEXT("Data.Poison.PercentPerStack"), ReportLines);
		RequestTag(TEXT("Data.Poison.ArmorPercentPerStack"), ReportLines);

		ConfigurePoisonGameplayEffect(
			PoisonEffectPath,
			TEXT("primary poison"),
			5.f,
			1.f,
			false,
			20,
			bDryRun,
			ReportLines,
			DirtyPackages);

		ConfigurePoisonGameplayEffect(
			PoisonSplashEffectPath,
			TEXT("secondary poison splash"),
			3.f,
			1.f,
			true,
			10,
			bDryRun,
			ReportLines,
			DirtyPackages);
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
			{ TEXT("T_Rune512_Splash"), TEXT("T_Rune512_Splash.png") },
			{ TEXT("T_Rune512_Split"), TEXT("T_Rune512_Split.png") },
			{ TEXT("T_Rune512_Shield"), TEXT("T_Rune512_Shield.png") },
			{ TEXT("T_Rune512_Pierce"), TEXT("T_Rune512_Pierce.png") },
			{ TEXT("T_Rune512_Attack"), TEXT("T_Rune512_Attack.png") },
			{ TEXT("T_Rune512_ReduceDamage"), TEXT("T_Rune512_ReduceDamage.png") },
			{ TEXT("T_Rune512_Sacrifice_MoonlightShadow"), TEXT("T_Rune512_Sacrifice_MoonlightShadow.png") },
			{ TEXT("T_Rune512_Sacrifice_ShadowMark"), TEXT("T_Rune512_Sacrifice_ShadowMark.png") },
			{ TEXT("T_Rune512_Sacrifice_GiantSwing"), TEXT("T_Rune512_Sacrifice_GiantSwing.png") },
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

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> `%s`."),
				bDryRun ? TEXT("Would import/reimport") : TEXT("Imported/reimported"),
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

	TArray<FIconImportSpec> MakeVfxTextureSpecs()
	{
		return {
			{ TEXT("T_Rune512_VFX_MoonBlade_Flight"), TEXT("T_Rune512_VFX_MoonBlade_Flight.png") },
			{ TEXT("T_Rune512_VFX_Moonlight_Hit"), TEXT("T_Rune512_VFX_Moonlight_Hit.png") },
			{ TEXT("T_Rune512_VFX_Poison_Hit"), TEXT("T_Rune512_VFX_Poison_Hit.png") },
			{ TEXT("T_Rune512_VFX_Poison_Spread"), TEXT("T_Rune512_VFX_Poison_Spread.png") },
			{ TEXT("T_Rune512_VFX_Burn_Hit"), TEXT("T_Rune512_VFX_Burn_Hit.png") },
			{ TEXT("T_Rune512_VFX_ShieldPierce"), TEXT("T_Rune512_VFX_ShieldPierce.png") },
		};
	}

	template <typename TExpression>
	TExpression* AddMaterialExpression(UMaterial* Material, int32 NodeX, int32 NodeY)
	{
		if (!Material || !Material->GetEditorOnlyData())
		{
			return nullptr;
		}

		TExpression* Expression = NewObject<TExpression>(Material);
		Expression->Material = Material;
		Expression->MaterialExpressionEditorX = NodeX;
		Expression->MaterialExpressionEditorY = NodeY;
		Material->GetEditorOnlyData()->ExpressionCollection.AddExpression(Expression);
		return Expression;
	}

	UMaterialExpressionScalarParameter* AddScalarParameter(
		UMaterial* Material,
		const FName ParameterName,
		float DefaultValue,
		int32 NodeX,
		int32 NodeY)
	{
		UMaterialExpressionScalarParameter* Parameter = AddMaterialExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
		if (Parameter)
		{
			Parameter->ParameterName = ParameterName;
			Parameter->DefaultValue = DefaultValue;
			Parameter->Group = TEXT("Rune512 Flipbook");
		}
		return Parameter;
	}

	UMaterial* EnsureGroundPathDecalMaterial(
		const FString& MaterialPath,
		const FLinearColor& DefaultColor,
		float DefaultOpacity,
		float DefaultEdgeSoftness,
		float DefaultFanMask,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterial* ExistingMaterial = LoadAssetByPackagePath<UMaterial>(MaterialPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found ground path decal material `%s`."), *MaterialPath));
			return ExistingMaterial;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s ground path decal material `%s`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*MaterialPath));
		if (bDryRun)
		{
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(MaterialPath);
		UPackage* Package = CreatePackage(*MaterialPath);
		UMaterial* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Material || !Material->GetEditorOnlyData())
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create ground path decal material `%s`: material editor data unavailable."), *MaterialPath));
			return nullptr;
		}

		Material->MaterialDomain = MD_DeferredDecal;
		Material->BlendMode = BLEND_Translucent;
		Material->DecalBlendMode = DBM_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;

		UMaterialExpressionTextureCoordinate* UV = AddMaterialExpression<UMaterialExpressionTextureCoordinate>(Material, -760, -80);
		if (UV)
		{
			UV->UTiling = 1.f;
			UV->VTiling = 1.f;
		}

		UMaterialExpressionVectorParameter* ColorParam =
			AddMaterialExpression<UMaterialExpressionVectorParameter>(Material, -760, -300);
		if (ColorParam)
		{
			ColorParam->ParameterName = TEXT("Color");
			ColorParam->DefaultValue = DefaultColor;
			ColorParam->Group = TEXT("Rune512 Ground Path");
		}

		UMaterialExpressionScalarParameter* OpacityParam =
			AddScalarParameter(Material, TEXT("Opacity"), DefaultOpacity, -760, 90);
		if (OpacityParam)
		{
			OpacityParam->Group = TEXT("Rune512 Ground Path");
		}

		UMaterialExpressionScalarParameter* EdgeSoftnessParam =
			AddScalarParameter(Material, TEXT("EdgeSoftness"), DefaultEdgeSoftness, -520, 90);
		if (EdgeSoftnessParam)
		{
			EdgeSoftnessParam->Group = TEXT("Rune512 Ground Path");
		}
		UMaterialExpressionScalarParameter* FanMaskParam =
			AddScalarParameter(Material, TEXT("FanMask"), DefaultFanMask, -280, 90);
		if (FanMaskParam)
		{
			FanMaskParam->Group = TEXT("Rune512 Ground Path");
		}

		UMaterialExpressionCustom* MaskNode = AddMaterialExpression<UMaterialExpressionCustom>(Material, -260, 0);
		if (MaskNode)
		{
			MaskNode->Description = TEXT("Rune512 Ground Path Mask");
			MaskNode->OutputType = CMOT_Float1;
			MaskNode->Code = TEXT(
				"float2 p = abs(UV - 0.5) * 2.0;\n"
				"float edge = min(1.0 - p.x, 1.0 - p.y);\n"
				"float rect = saturate(edge / max(EdgeSoftness, 0.001));\n"
				"float y = saturate(UV.y);\n"
				"float fanHalfWidth = lerp(0.10, 1.0, y);\n"
				"float fanSide = 1.0 - smoothstep(fanHalfWidth, saturate(fanHalfWidth + max(EdgeSoftness, 0.001) * 0.45), p.x);\n"
				"float fanLength = smoothstep(0.02, max(EdgeSoftness, 0.001), y) * (1.0 - smoothstep(1.0 - max(EdgeSoftness, 0.001), 0.995, y));\n"
				"float soft = lerp(rect, saturate(fanSide * fanLength), saturate(FanMask));\n"
				"float vein = 0.5 + 0.5 * sin((UV.y * 7.0 + UV.x * 2.5) * 6.2831853);\n"
				"float core = saturate(1.0 - abs(UV.x - 0.5) * 2.0);\n"
				"return saturate(Opacity * soft * lerp(0.55, 1.0, vein) * lerp(0.75, 1.0, core));");

			auto AddCustomInput = [MaskNode](const FName InputName, UMaterialExpression* InputExpression)
			{
				FCustomInput& Input = MaskNode->Inputs.AddDefaulted_GetRef();
				Input.InputName = InputName;
				if (InputExpression)
				{
					Input.Input.Connect(0, InputExpression);
				}
			};

			AddCustomInput(TEXT("UV"), UV);
			AddCustomInput(TEXT("Opacity"), OpacityParam);
			AddCustomInput(TEXT("EdgeSoftness"), EdgeSoftnessParam);
			AddCustomInput(TEXT("FanMask"), FanMaskParam);
		}

		UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
		if (ColorParam)
		{
			EditorOnlyData->BaseColor.Connect(0, ColorParam);
			EditorOnlyData->EmissiveColor.Connect(0, ColorParam);
		}
		if (MaskNode)
		{
			EditorOnlyData->Opacity.Connect(0, MaskNode);
		}

		FAssetRegistryModule::AssetCreated(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		DirtyPackages.AddUnique(Material->GetPackage());
		return Material;
	}

	UMaterial* EnsureFlipbookMaterial(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UMaterial* ExistingMaterial = LoadAssetByPackagePath<UMaterial>(FlipbookMaterialPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found flipbook material `%s`."), *FlipbookMaterialPath));
			return ExistingMaterial;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s flipbook material `%s` with Custom Node include `/Project/Rune512FlipbookSprite.ush`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*FlipbookMaterialPath));
		if (bDryRun)
		{
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(FlipbookMaterialPath);
		UPackage* Package = CreatePackage(*FlipbookMaterialPath);
		UMaterial* Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Material || !Material->GetEditorOnlyData())
		{
			ReportLines.Add(TEXT("- Failed to create flipbook material: material editor data unavailable."));
			return nullptr;
		}

		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bDisableDepthTest = false;

		UTexture2D* DefaultTexture = LoadAssetByPackagePath<UTexture2D>(PoisonHitTexturePath);

		UMaterialExpressionTextureObjectParameter* TextureParam =
			AddMaterialExpression<UMaterialExpressionTextureObjectParameter>(Material, -880, -280);
		if (TextureParam)
		{
			TextureParam->ParameterName = TEXT("FlipbookTexture");
			TextureParam->Texture = DefaultTexture;
			TextureParam->Group = TEXT("Rune512 Flipbook");
		}

		UMaterialExpressionTextureCoordinate* UV = AddMaterialExpression<UMaterialExpressionTextureCoordinate>(Material, -880, -80);
		if (UV)
		{
			UV->UTiling = 1.f;
			UV->VTiling = 1.f;
		}

		UMaterialExpressionScalarParameter* TimeParam = AddScalarParameter(Material, TEXT("Time"), 0.f, -880, 90);
		UMaterialExpressionScalarParameter* RowsParam = AddScalarParameter(Material, TEXT("Rows"), 4.f, -680, 90);
		UMaterialExpressionScalarParameter* ColumnsParam = AddScalarParameter(Material, TEXT("Columns"), 4.f, -480, 90);
		UMaterialExpressionScalarParameter* DurationParam = AddScalarParameter(Material, TEXT("Duration"), 0.45f, -280, 90);
		UMaterialExpressionVectorParameter* EmissiveParam =
			AddMaterialExpression<UMaterialExpressionVectorParameter>(Material, -480, -280);
		if (EmissiveParam)
		{
			EmissiveParam->ParameterName = TEXT("EmissiveColor");
			EmissiveParam->DefaultValue = FLinearColor::White;
			EmissiveParam->Group = TEXT("Rune512 Flipbook");
		}
		UMaterialExpressionScalarParameter* AlphaParam = AddScalarParameter(Material, TEXT("AlphaScale"), 1.f, -280, -80);

		UMaterialExpressionCustom* CustomNode = AddMaterialExpression<UMaterialExpressionCustom>(Material, 40, -120);
		if (CustomNode)
		{
			CustomNode->Description = TEXT("Rune512 Flipbook Sprite");
			CustomNode->OutputType = CMOT_Float4;
			CustomNode->IncludeFilePaths.Add(TEXT("/Project/Rune512FlipbookSprite.ush"));
			CustomNode->Code = TEXT("return Rune512FlipbookSpriteMain(FlipbookTexture, FlipbookTextureSampler, UV, Time, Rows, Columns, Duration, EmissiveColor, AlphaScale);");

			auto AddCustomInput = [CustomNode](const FName InputName, UMaterialExpression* InputExpression)
			{
				FCustomInput& Input = CustomNode->Inputs.AddDefaulted_GetRef();
				Input.InputName = InputName;
				if (InputExpression)
				{
					Input.Input.Connect(0, InputExpression);
				}
			};

			AddCustomInput(TEXT("FlipbookTexture"), TextureParam);
			AddCustomInput(TEXT("UV"), UV);
			AddCustomInput(TEXT("Time"), TimeParam);
			AddCustomInput(TEXT("Rows"), RowsParam);
			AddCustomInput(TEXT("Columns"), ColumnsParam);
			AddCustomInput(TEXT("Duration"), DurationParam);
			AddCustomInput(TEXT("EmissiveColor"), EmissiveParam);
			AddCustomInput(TEXT("AlphaScale"), AlphaParam);

			UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
			EditorOnlyData->EmissiveColor.Connect(0, CustomNode);
			EditorOnlyData->EmissiveColor.SetMask(1, 1, 1, 1, 0);
			EditorOnlyData->Opacity.Connect(0, CustomNode);
			EditorOnlyData->Opacity.SetMask(1, 0, 0, 0, 1);
		}

		FAssetRegistryModule::AssetCreated(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		DirtyPackages.AddUnique(Material->GetPackage());
		return Material;
	}

	void ImportVfxTextures(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("SourceArt/512RuneVFX/PreviewFrames"));
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

		ReportLines.Add(TEXT("## 512 flipbook VFX texture import"));
		for (const FIconImportSpec& TextureSpec : MakeVfxTextureSpecs())
		{
			const FString SourceFile = FPaths::Combine(SourceDir, TextureSpec.FileName);
			const FString TexturePackagePath = VfxTextureRoot + TEXT("/") + TextureSpec.AssetName;
			if (!FPaths::FileExists(SourceFile))
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing source PNG `%s`."), *SourceFile));
				continue;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> `%s`."),
				bDryRun ? TEXT("Would import/reimport") : TEXT("Imported/reimported"),
				*SourceFile,
				*VfxTextureRoot));

			if (bDryRun)
			{
				continue;
			}

			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = VfxTextureRoot;
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

		EnsureFlipbookMaterial(bDryRun, ReportLines, DirtyPackages);
		EnsureGroundPathDecalMaterial(
			PoisonGroundPathDecalMaterialPath,
			FLinearColor(0.04f, 0.9f, 0.22f, 1.0f),
			0.58f,
			0.38f,
			0.0f,
			bDryRun,
			ReportLines,
			DirtyPackages);
		EnsureGroundPathDecalMaterial(
			BurnGroundPathDecalMaterialPath,
			FLinearColor(1.0f, 0.22f, 0.04f, 1.0f),
			0.66f,
			0.22f,
			1.0f,
			bDryRun,
			ReportLines,
			DirtyPackages);
	}

	TArray<FLinkRecipeSpec> MakeMoonlightRecipes(ECombatCardLinkOrientation Direction)
	{
		const FString DirectionPrefix = Direction == ECombatCardLinkOrientation::Forward ? TEXT("Forward") : TEXT("Reversed");
		TArray<TPair<FString, FString>> Effects = {
			{ TEXT("Card.Effect.Burn"), TEXT("Burn") },
			{ TEXT("Card.Effect.Poison"), TEXT("Poison") },
			{ TEXT("Card.Effect.Shield"), TEXT("Shield") },
			{ TEXT("Card.Effect.Pierce"), TEXT("Pierce") },
			{ TEXT("Card.Effect.Attack"), TEXT("Attack") },
			{ TEXT("Card.Effect.Defense.ReduceDamage"), TEXT("ReduceDamage") },
		};
		if (Direction == ECombatCardLinkOrientation::Reversed)
		{
			Effects.Emplace(TEXT("Card.Effect.SplashSplit"), TEXT("Split"));
		}

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
		if (Key == TEXT("Split"))
		{
			Spec.DisplayName = TEXT("\u5206\u88c2");
			Spec.Description = TEXT("\u8fdc\u7a0b\u6b66\u5668\u5361\u3002\u5f00\u706b\u65f6\u989d\u5916\u53d1\u5c04 2 \u4e2a\u72ec\u7acb\u5f39\u9053\uff0c\u9ed8\u8ba4\u89d2\u5ea6\u4e3a -8/+8 \u5ea6\uff0c\u4e0d\u4f1a\u751f\u6210\u6708\u5203\u3002");
			Spec.BaseFlowTemplatePath = AttackTemplateFlow;
			Spec.TriggerTiming = ECombatCardTriggerTiming::OnCommit;
			Spec.ManualTodos = {
				TEXT("Ranged weapon reward/deck pools should use Split. Split base flow only spawns extra musket projectiles; Moonlight split exists only as Moonlight reversed LinkFlow.")
			};
		}
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
			{ TEXT("补毒液路径碰撞体与 3 秒爆发 Execution；命中表现使用小尺寸 Play Niagara 节点。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Bleed"),
			TEXT("流血"),
			TEXT("命中敌人后赋予流血。流血每秒造成固定伤害；目标有护甲时不应触发。"),
			TEXT("Card.ID.Bleed"),
			{ TEXT("Card.Effect.Bleed") },
			TEXT("T_Rune512_Burn"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Bleed"),
			TEXT("FA_Rune512_Bleed_Base"),
			ERuneType::Debuff,
			{ TEXT("护甲目标不触发流血：如 FA_Effect_Bleed 内未做判断，需要在触发前增加 HasTag(Buff.Status.Armored) 阻断。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Rend"),
			TEXT("撕裂"),
			TEXT("命中敌人后赋予撕裂。目标移动时按移动距离持续掉血，原地 2 秒后自动消失。"),
			TEXT("Card.ID.Rend"),
			{ TEXT("Card.Effect.Rend") },
			TEXT("T_Rune512_Pierce"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Rend"),
			TEXT("FA_Rune512_Rend_Base"),
			ERuneType::Debuff,
			{ TEXT("护甲目标触发概率下降 25%：基础 GA_Rend 已处理移动掉血和静止消失，概率门槛应在 FA 触发前配置。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Wound"),
			TEXT("伤口"),
			TEXT("命中敌人后赋予伤口。目标再次受到攻击时额外扣血。"),
			TEXT("Card.ID.Wound"),
			{ TEXT("Card.Effect.Wound") },
			TEXT("T_Rune512_Attack"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Wound"),
			TEXT("FA_Rune512_Wound_Base"),
			ERuneType::Debuff,
			{ TEXT("护甲目标触发概率下降 25%；伤口额外伤害由 GA_Wound 监听 Ability.Event.Damaged。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Knockback"),
			TEXT("击退"),
			TEXT("命中敌人后赋予击退状态。目标受到攻击时会被击退；有护甲时额外造成护甲伤害。"),
			TEXT("Card.ID.Knockback"),
			{ TEXT("Card.Effect.Knockback") },
			TEXT("T_Rune512_Split"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Knockback"),
			TEXT("FA_Rune512_Knockback_Base"),
			ERuneType::Debuff,
			{ TEXT("已有 GA_KnockbackDebuff 处理 15% 额外护甲伤害；护甲时击退距离减少需要在 GA_Knockback 或专用节点中参数化。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Fear"),
			TEXT("恐惧"),
			TEXT("命中敌人后赋予恐惧。敌人逃离触发点；2 秒内未离开 800 单位时受到惩罚伤害。"),
			TEXT("Card.ID.Fear"),
			{ TEXT("Card.Effect.Fear") },
			TEXT("T_Rune512_ReduceDamage"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Fear"),
			TEXT("FA_Rune512_Fear_Base"),
			ERuneType::Debuff,
			{ TEXT("玩家不应获得恐惧：确认 FA_Effect_Fear 的目标筛选为敌人，或在卡牌触发层排除玩家。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Freeze"),
			TEXT("冻结"),
			TEXT("命中敌人后赋予冻结预警。3 秒内未离开 800 单位时进入冻结眩晕并受到伤害。"),
			TEXT("Card.ID.Freeze"),
			{ TEXT("Card.Effect.Freeze") },
			TEXT("T_Rune512_Shield"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Freeze"),
			TEXT("FA_Rune512_Freeze_Base"),
			ERuneType::Debuff,
			{ TEXT("确认 FrozenStunEffect 已授予 Buff.Status.Frozen 和 Character.State.Stunned。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Stun"),
			TEXT("眩晕"),
			TEXT("命中敌人后尝试赋予眩晕。眩晕期间不能主动移动和攻击；霸体目标免疫。"),
			TEXT("Card.ID.Stun"),
			{ TEXT("Card.Effect.Stun") },
			TEXT("T_Rune512_Shield"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Stun"),
			TEXT("FA_Rune512_Stun_Base"),
			ERuneType::Debuff,
			{ TEXT("需要重点验收霸体交互：霸体免疫眩晕，获得霸体时驱散已有眩晕。") }));

		Specs.Add(MakeNormalCard(
			TEXT("Curse"),
			TEXT("诅咒"),
			TEXT("命中敌人后赋予诅咒。每个负面效果降低最大生命值。"),
			TEXT("Card.ID.Curse"),
			{ TEXT("Card.Effect.Curse") },
			TEXT("T_Rune512_Poison"),
			TEXT("/Game/Docs/BuffDocs/Playtest_GA/RuneBaseEffect/FA_Effect_Curse"),
			TEXT("FA_Rune512_Curse_Base"),
			ERuneType::Debuff,
			{ TEXT("当前 Generic Curse 需要确认是否为“每个负面效果 -7% MaxHealth”；若仍是死亡诅咒版本，需要补负面状态计数逻辑。") }));

		FCardSpec Splash = MakeNormalCard(
			TEXT("Splash"),
			TEXT("\u6e85\u5c04"),
			TEXT("\u8fd1\u6218\u6b66\u5668\u5361\u3002\u653b\u51fb\u547d\u4e2d\u65f6\uff0c\u5bf9\u53d7\u4f24\u654c\u4eba\u5468\u56f4 300cm \u5185\u5176\u4ed6\u654c\u4eba\u9020\u6210\u672c\u6b21\u653b\u51fb 20% \u7684\u6e85\u5c04\u4f24\u5bb3\uff1b\u4e3b\u76ee\u6807\u4e0d\u91cd\u590d\u53d7\u5230\u6e85\u5c04\u3002"),
			TEXT("Card.ID.SplashSplit"),
			{ TEXT("Card.Effect.SplashSplit"), TEXT("Card.Effect.Splash") },
			TEXT("T_Rune512_Splash"),
			AttackTemplateFlow,
			TEXT("FA_Rune512_Splash_Base"),
			ERuneType::Buff,
			{ TEXT("Melee weapon reward/deck pools should use Splash instead of Split. Splash flow uses OnDamageDealt -> MathFloat(0.2) -> ApplyGEInRadius.") });
		Splash.TriggerTiming = ECombatCardTriggerTiming::OnHit;
		Specs.Add(Splash);

		Specs.Add(MakeNormalCard(
			TEXT("Split"),
			TEXT("溅射/分裂"),
			TEXT("近战武器为溅射，攻击时对受伤敌人周围目标造成本次攻击 20% 伤害；远程武器为分裂，额外发射 2 个独立弹道。"),
			TEXT("Card.ID.SplashSplit"),
			{ TEXT("Card.Effect.SplashSplit"), TEXT("Card.Effect.Split") },
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
			TEXT("Forward/Reversed 配方 Flow 已按模板复制，复杂节点连接与表现节点参数需要按配置文档检查。")
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

	TArray<FSacrificeSpec> MakeSacrificeSpecs()
	{
		TArray<FSacrificeSpec> Specs;

		FSacrificeSpec MoonlightShadow;
		MoonlightShadow.Key = TEXT("MoonlightShadow");
		MoonlightShadow.DisplayName = TEXT("\u6708\u5149\u4e4b\u5f71");
		MoonlightShadow.Description = TEXT("\u732e\u796d\u88ab\u52a8\u7b26\u6587\u3002\u51b2\u523a\u540e\u5728\u539f\u5730\u7559\u4e0b\u6697\u5f71\uff1b\u6697\u5f71\u4e0d\u79fb\u52a8\uff0c\u4f1a\u6a21\u4eff\u73a9\u5bb6\u653b\u51fb\u5e76\u540c\u6b65\u6708\u5149\u7b49\u5361\u724c\u589e\u5f3a\uff0c\u9ed8\u8ba4 4 \u6b21\u653b\u51fb\u540e\u6d88\u5931\u3002");
		MoonlightShadow.Summary = TEXT("\u51b2\u523a\u540e\u7559\u4e0b\u6697\u5f71\uff1b\u6697\u5f71\u6a21\u4eff\u653b\u51fb\u5e76\u540c\u6b65\u6708\u5149\u7b49\u653b\u51fb\u5361\u589e\u5f3a\uff0c4 \u6b21\u653b\u51fb\u540e\u6d88\u5931\u3002");
		MoonlightShadow.TargetAssetName = TEXT("DA_Rune512_Sacrifice_MoonlightShadow");
		MoonlightShadow.FlowTargetName = TEXT("FA_Rune512_Sacrifice_MoonlightShadow");
		MoonlightShadow.IconAssetName = TEXT("T_Rune512_Sacrifice_MoonlightShadow");
		MoonlightShadow.Config.PassiveType = ESacrificeRunePassiveType::MoonlightShadow;
		MoonlightShadow.Config.ShadowAttackCharges = 4;
		MoonlightShadow.Config.ShadowLifetime = 10.0f;
		MoonlightShadow.Config.ShadowDamageMultiplier = 0.45f;
		MoonlightShadow.Config.ShadowAttackRange = 380.0f;
		MoonlightShadow.Config.ShadowAttackConeDegrees = 90.0f;
		Specs.Add(MoonlightShadow);

		FSacrificeSpec ShadowMark;
		ShadowMark.Key = TEXT("ShadowMark");
		ShadowMark.DisplayName = TEXT("\u6697\u5f71\u5370\u8bb0");
		ShadowMark.Description = TEXT("\u732e\u796d\u88ab\u52a8\u7b26\u6587\u3002\u51b2\u523a\u6b21\u6570 +1\uff1b\u51b2\u523a\u8def\u5f84\u4e0a\u78b0\u5230\u7684\u654c\u4eba\u83b7\u5f97\u6697\u5f71\u5370\u8bb0\uff0c\u88ab\u73a9\u5bb6\u653b\u51fb\u540e\u5f15\u7206\u5e76\u9020\u6210\u8303\u56f4\u4f24\u5bb3\u3002");
		ShadowMark.Summary = TEXT("\u51b2\u523a\u6b21\u6570 +1\uff1b\u51b2\u523a\u78b0\u5230\u7684\u654c\u4eba\u88ab\u653b\u51fb\u540e\u5f15\u7206\u5e76\u9020\u6210\u8303\u56f4\u4f24\u5bb3\u3002");
		ShadowMark.TargetAssetName = TEXT("DA_Rune512_Sacrifice_ShadowMark");
		ShadowMark.FlowTargetName = TEXT("FA_Rune512_Sacrifice_ShadowMark");
		ShadowMark.IconAssetName = TEXT("T_Rune512_Sacrifice_ShadowMark");
		ShadowMark.Config.PassiveType = ESacrificeRunePassiveType::ShadowMark;
		ShadowMark.Config.DashChargeBonus = 1.0f;
		ShadowMark.Config.DashMarkRadius = 95.0f;
		ShadowMark.Config.MarkDuration = 8.0f;
		ShadowMark.Config.MarkExplosionDamage = 35.0f;
		ShadowMark.Config.MarkExplosionRadius = 260.0f;
		Specs.Add(ShadowMark);

		FSacrificeSpec GiantSwing;
		GiantSwing.Key = TEXT("GiantSwing");
		GiantSwing.DisplayName = TEXT("\u5de8\u529b\u6325\u821e");
		GiantSwing.Description = TEXT("\u732e\u796d\u88ab\u52a8\u7b26\u6587\u3002\u73a9\u5bb6\u6240\u6709\u653b\u51fb\u90fd\u4f1a\u51fb\u9000\u654c\u4eba\uff1b\u654c\u4eba\u88ab\u51fb\u9000\u8fc7\u7a0b\u4e2d\u78b0\u5230\u5176\u4ed6\u5355\u4f4d\u65f6\uff0c\u5bf9\u88ab\u78b0\u5355\u4f4d\u9020\u6210\u4e00\u6b21\u78b0\u649e\u4f24\u5bb3\u3002");
		GiantSwing.Summary = TEXT("\u6240\u6709\u653b\u51fb\u51fb\u9000\u654c\u4eba\uff1b\u88ab\u51fb\u9000\u7684\u654c\u4eba\u649e\u5230\u5176\u4ed6\u5355\u4f4d\u65f6\u9020\u6210\u78b0\u649e\u4f24\u5bb3\u3002");
		GiantSwing.TargetAssetName = TEXT("DA_Rune512_Sacrifice_GiantSwing");
		GiantSwing.FlowTargetName = TEXT("FA_Rune512_Sacrifice_GiantSwing");
		GiantSwing.IconAssetName = TEXT("T_Rune512_Sacrifice_GiantSwing");
		GiantSwing.Config.PassiveType = ESacrificeRunePassiveType::GiantSwing;
		GiantSwing.Config.KnockbackDistance = 520.0f;
		GiantSwing.Config.KnockbackCollisionDamage = 22.0f;
		GiantSwing.Config.KnockbackCollisionRadius = 120.0f;
		GiantSwing.Config.KnockbackCollisionDuration = 0.35f;
		GiantSwing.Config.KnockbackCollisionTickInterval = 0.05f;
		Specs.Add(GiantSwing);

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

	enum class EMoonlightFlowProfile : uint8
	{
		None,
		Base,
		ForwardAttack,
		ForwardBurn,
		ForwardPoison,
		ForwardSplit,
		ForwardShield,
		ForwardPierce,
		ForwardReduceDamage,
		ReversedAttack,
		ReversedBurn,
		ReversedPoison,
		ReversedSplit,
		ReversedShield,
		ReversedPierce,
		ReversedReduceDamage
	};

	EMoonlightFlowProfile ResolveMoonlightFlowProfile(const FString& FlowName)
	{
		if (FlowName.Equals(TEXT("FA_Rune512_Moonlight_Base"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::Base;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Attack"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardAttack;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Burn"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardBurn;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Poison"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardPoison;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Split"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardSplit;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Shield"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardShield;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_Pierce"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardPierce;
		}
		if (FlowName.Contains(TEXT("Moonlight_Forward_ReduceDamage"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ForwardReduceDamage;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Attack"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedAttack;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Burn"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedBurn;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Poison"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedPoison;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Split"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedSplit;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Shield"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedShield;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_Pierce"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedPierce;
		}
		if (FlowName.Contains(TEXT("Moonlight_Reversed_ReduceDamage"), ESearchCase::IgnoreCase))
		{
			return EMoonlightFlowProfile::ReversedReduceDamage;
		}
		return EMoonlightFlowProfile::None;
	}

	void ConfigureMoonlightSlashWaveFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile Profile = ResolveMoonlightFlowProfile(FlowName);
		if (Profile == EMoonlightFlowProfile::None)
		{
			return;
		}
		if (Profile == EMoonlightFlowProfile::ReversedBurn || Profile == EMoonlightFlowProfile::ReversedPoison)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s` uses Rune Ground Path; slash-wave projectile configuration is bypassed."), *FlowName));
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure Moonlight slash-wave node in `%s`."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure Moonlight flow `%s`: asset not loaded."), *FlowName));
			return;
		}

		const TCHAR* CleanVfxPolicy = TEXT("projectile inline Niagara cleared; projectile uses BP/default visuals; hit/status uses independent atomic VFX nodes");

		int32 UpdatedNodeCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(Pair.Value);
			if (!SlashNode)
			{
				continue;
			}

			SlashNode->Modify();
			SlashNode->LaunchNiagaraSystem = nullptr;
			SlashNode->LaunchNiagaraEffectName = NAME_None;
			SlashNode->bAttachLaunchNiagaraToSource = false;
			SlashNode->LaunchNiagaraOffset = FVector::ZeroVector;
			SlashNode->LaunchNiagaraRotationOffset = FRotator::ZeroRotator;
			SlashNode->LaunchNiagaraScale = FVector(1.f, 1.f, 1.f);
			SlashNode->bDestroyLaunchNiagaraWithFlow = false;
			SlashNode->ProjectileVisualNiagaraSystem = nullptr;
			SlashNode->ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
			SlashNode->bHideDefaultProjectileVisuals = false;
			SlashNode->bScaleVisualWithCollisionExtent = true;
			SlashNode->ProjectileCount = 1;
			SlashNode->ProjectileConeAngleDegrees = 0.f;
			SlashNode->bSpawnProjectilesSequentially = false;
			SlashNode->SequentialProjectileSpawnInterval = 0.12f;
			SlashNode->bAddComboStacksToProjectileCount = false;
			SlashNode->ProjectilesPerComboStack = 1;
			SlashNode->MaxBonusProjectiles = 0;
			SlashNode->bDestroyOnWorldStaticHit = false;
			SlashNode->bForcePureDamage = false;
			SlashNode->BonusArmorDamageMultiplier = 0.f;
			SlashNode->bAddSourceArmorToDamage = false;
			SlashNode->SourceArmorToDamageMultiplier = 1.f;
			SlashNode->bConsumeSourceArmorOnSpawn = false;
			SlashNode->SourceArmorConsumeMultiplier = 1.f;
			SlashNode->AdditionalHitEffect = nullptr;
			SlashNode->AdditionalHitSetByCallerTag = FGameplayTag();
			SlashNode->AdditionalHitSetByCallerValue = 0.f;
			SlashNode->bSplitOnFirstHit = false;
			SlashNode->MaxSplitGenerations = 1;
			SlashNode->SplitProjectileCount = 3;
			SlashNode->SplitConeAngleDegrees = 45.f;
			SlashNode->SplitDamageMultiplier = 0.5f;
			SlashNode->SplitSpeedMultiplier = 2.f;
			SlashNode->SplitMaxDistanceMultiplier = 0.6f;
			SlashNode->SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);
			SlashNode->bBounceSplitChildrenOnEnemyHit = false;
			SlashNode->SplitChildMaxEnemyBounces = 0;
			SlashNode->HitNiagaraSystem = nullptr;
			SlashNode->HitNiagaraScale = FVector(0.75f, 0.75f, 0.75f);
			SlashNode->ExpireNiagaraSystem = nullptr;
			SlashNode->ExpireNiagaraScale = FVector(0.5f, 0.5f, 0.5f);

			switch (Profile)
			{
			case EMoonlightFlowProfile::Base:
				SlashNode->Damage = 30.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Base");
				SlashNode->Speed = 1400.f;
				SlashNode->MaxDistance = 800.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->DamageApplicationInterval = 0.25f;
				SlashNode->CollisionBoxExtent = FVector(30.f, 60.f, 35.f);
				SlashNode->VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(0.85f, 0.85f, 0.85f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardAttack:
				SlashNode->Damage = 45.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Attack");
				SlashNode->Speed = 1400.f;
				SlashNode->MaxDistance = 800.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->DamageApplicationInterval = 0.25f;
				SlashNode->CollisionBoxExtent = FVector(60.f, 120.f, 55.f);
				SlashNode->VisualScaleMultiplier = FVector(1.35f, 1.35f, 1.2f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.35f, 1.35f, 1.2f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardBurn:
				SlashNode->Damage = 35.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Burn");
				SlashNode->Speed = 1100.f;
				SlashNode->MaxDistance = 800.f;
				SlashNode->MaxHitCount = 3;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->DamageApplicationInterval = 0.25f;
				SlashNode->CollisionBoxExtent = FVector(50.f, 105.f, 50.f);
				SlashNode->VisualScaleMultiplier = FVector(1.25f, 1.25f, 1.1f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.2f, 1.2f, 1.1f);
				SlashNode->HitNiagaraScale = FVector(0.3f, 0.3f, 0.3f);
				SlashNode->ExpireNiagaraScale = FVector(0.3f, 0.3f, 0.3f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardPoison:
				SlashNode->Damage = 25.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Poison");
				SlashNode->Speed = 900.f;
				SlashNode->MaxDistance = 800.f;
				SlashNode->MaxHitCount = 3;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->DamageApplicationInterval = 0.25f;
				SlashNode->CollisionBoxExtent = FVector(45.f, 95.f, 45.f);
				SlashNode->VisualScaleMultiplier = FVector(1.15f, 1.15f, 1.f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
				SlashNode->HitNiagaraScale = FVector(0.65f, 0.65f, 0.65f);
				SlashNode->ExpireNiagaraScale = FVector(0.5f, 0.5f, 0.5f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardSplit:
				SlashNode->Damage = 22.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Split");
				SlashNode->Speed = 1400.f;
				SlashNode->MaxDistance = 750.f;
				SlashNode->MaxHitCount = 1;
				SlashNode->ProjectileCount = 5;
				SlashNode->ProjectileConeAngleDegrees = 55.f;
				SlashNode->CollisionBoxExtent = FVector(26.f, 56.f, 32.f);
				SlashNode->VisualScaleMultiplier = FVector(0.85f, 0.85f, 0.85f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(0.75f, 0.75f, 0.75f);
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardShield:
				SlashNode->Damage = 20.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Shield");
				SlashNode->Speed = 1000.f;
				SlashNode->MaxDistance = 650.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->bAddSourceArmorToDamage = true;
				SlashNode->SourceArmorToDamageMultiplier = 1.f;
				SlashNode->bConsumeSourceArmorOnSpawn = true;
				SlashNode->SourceArmorConsumeMultiplier = 1.f;
				SlashNode->CollisionBoxExtent = FVector(70.f, 130.f, 60.f);
				SlashNode->VisualScaleMultiplier = FVector(1.4f, 1.4f, 1.2f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.2f, 1.2f, 1.1f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardPierce:
				SlashNode->Damage = 30.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_Pierce");
				SlashNode->Speed = 1550.f;
				SlashNode->MaxDistance = 900.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->bDestroyOnWorldStaticHit = true;
				SlashNode->bForcePureDamage = true;
				SlashNode->BonusArmorDamageMultiplier = 0.75f;
				SlashNode->CollisionBoxExtent = FVector(36.f, 80.f, 35.f);
				SlashNode->VisualScaleMultiplier = FVector(1.05f, 1.05f, 1.f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.05f, 1.05f, 1.f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ForwardReduceDamage:
				SlashNode->Damage = 24.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Forward_ReduceDamage");
				SlashNode->Speed = 850.f;
				SlashNode->MaxDistance = 650.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->CollisionBoxExtent = FVector(75.f, 135.f, 70.f);
				SlashNode->VisualScaleMultiplier = FVector(1.35f, 1.35f, 1.25f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
				SlashNode->bAddComboStacksToProjectileCount = true;
				SlashNode->ProjectilesPerComboStack = 1;
				SlashNode->MaxBonusProjectiles = 2;
				SlashNode->ProjectileConeAngleDegrees = 0.f;
				SlashNode->bSpawnProjectilesSequentially = true;
				SlashNode->SequentialProjectileSpawnInterval = 0.12f;
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedAttack:
				SlashNode->Damage = 30.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Attack");
				SlashNode->Speed = 180.f;
				SlashNode->MaxDistance = 200.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->DamageApplicationsPerTarget = 3;
				SlashNode->DamageApplicationInterval = 0.3f;
				SlashNode->CollisionBoxExtent = FVector(140.f, 260.f, 90.f);
				SlashNode->VisualScaleMultiplier = FVector(2.1f, 2.1f, 1.5f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(2.2f, 2.2f, 1.6f);
				SlashNode->SpawnOffset = FVector(110.f, 0.f, 55.f);
				break;
			case EMoonlightFlowProfile::ReversedBurn:
				SlashNode->Damage = 20.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Burn");
				SlashNode->Speed = 320.f;
				SlashNode->MaxDistance = 240.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->ProjectileCount = 3;
				SlashNode->ProjectileConeAngleDegrees = 50.f;
				SlashNode->CollisionBoxExtent = FVector(95.f, 145.f, 70.f);
				SlashNode->VisualScaleMultiplier = FVector(1.6f, 1.6f, 1.25f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.4f, 1.4f, 1.1f);
				SlashNode->HitNiagaraScale = FVector(0.3f, 0.3f, 0.3f);
				SlashNode->ExpireNiagaraScale = FVector(0.3f, 0.3f, 0.3f);
				SlashNode->SpawnOffset = FVector(95.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedPoison:
				SlashNode->Damage = 12.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Poison");
				SlashNode->Speed = 280.f;
				SlashNode->MaxDistance = 220.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->ProjectileCount = 3;
				SlashNode->ProjectileConeAngleDegrees = 45.f;
				SlashNode->DamageApplicationsPerTarget = 3;
				SlashNode->DamageApplicationInterval = 0.2f;
				SlashNode->CollisionBoxExtent = FVector(85.f, 130.f, 65.f);
				SlashNode->VisualScaleMultiplier = FVector(1.45f, 1.45f, 1.15f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.1f, 1.1f, 1.f);
				SlashNode->HitNiagaraScale = FVector(0.65f, 0.65f, 0.65f);
				SlashNode->SpawnOffset = FVector(95.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedSplit:
				SlashNode->Damage = 28.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Split");
				SlashNode->Speed = 900.f;
				SlashNode->MaxDistance = 700.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->bSplitOnFirstHit = true;
				SlashNode->bDestroyOnWorldStaticHit = true;
				SlashNode->MaxSplitGenerations = 1;
				SlashNode->SplitProjectileCount = 4;
				SlashNode->SplitConeAngleDegrees = 100.f;
				SlashNode->bRandomizeSplitDirections = true;
				SlashNode->SplitRandomYawJitterDegrees = 22.f;
				SlashNode->SplitRandomPitchDegrees = 0.f;
				SlashNode->SplitDamageMultiplier = 0.5f;
				SlashNode->SplitSpeedMultiplier = 1.6f;
				SlashNode->SplitMaxDistanceMultiplier = 1.25f;
				SlashNode->SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);
				SlashNode->bBounceSplitChildrenOnEnemyHit = true;
				SlashNode->SplitChildMaxEnemyBounces = 1;
				SlashNode->CollisionBoxExtent = FVector(45.f, 95.f, 45.f);
				SlashNode->VisualScaleMultiplier = FVector(1.1f, 1.1f, 1.f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(0.9f, 0.9f, 0.9f);
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedShield:
				SlashNode->Damage = 10.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Shield");
				SlashNode->Speed = 220.f;
				SlashNode->MaxDistance = 160.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->CollisionBoxExtent = FVector(130.f, 190.f, 85.f);
				SlashNode->VisualScaleMultiplier = FVector(1.8f, 1.8f, 1.35f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.2f, 1.2f, 1.2f);
				SlashNode->SpawnOffset = FVector(75.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedPierce:
				SlashNode->Damage = 35.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_Pierce");
				SlashNode->Speed = 900.f;
				SlashNode->MaxDistance = 600.f;
				SlashNode->MaxHitCount = 2;
				SlashNode->BonusArmorDamageMultiplier = 0.75f;
				SlashNode->CollisionBoxExtent = FVector(45.f, 110.f, 45.f);
				SlashNode->VisualScaleMultiplier = FVector(1.25f, 1.25f, 1.1f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.1f, 1.1f, 1.f);
				SlashNode->SpawnOffset = FVector(80.f, 0.f, 45.f);
				break;
			case EMoonlightFlowProfile::ReversedReduceDamage:
				SlashNode->Damage = 8.f;
				SlashNode->DamageLogType = TEXT("Rune_Moonlight_Reversed_ReduceDamage");
				SlashNode->Speed = 180.f;
				SlashNode->MaxDistance = 120.f;
				SlashNode->MaxHitCount = 0;
				SlashNode->DamageApplicationsPerTarget = 1;
				SlashNode->CollisionBoxExtent = FVector(150.f, 210.f, 95.f);
				SlashNode->VisualScaleMultiplier = FVector(1.9f, 1.9f, 1.35f);
				SlashNode->ProjectileVisualNiagaraScale = FVector(1.25f, 1.25f, 1.2f);
				SlashNode->SpawnOffset = FVector(70.f, 0.f, 45.f);
				break;
			default:
				break;
			}

			SlashNode->LaunchNiagaraSystem = nullptr;
			SlashNode->LaunchNiagaraEffectName = NAME_None;
			SlashNode->ProjectileVisualNiagaraSystem = nullptr;
			SlashNode->ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);
			SlashNode->bHideDefaultProjectileVisuals = false;
			SlashNode->HitNiagaraSystem = nullptr;
			SlashNode->HitNiagaraScale = FVector(1.f, 1.f, 1.f);
			SlashNode->ExpireNiagaraSystem = nullptr;
			SlashNode->ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

			const bool bIsPoisonLinkProfile =
				Profile == EMoonlightFlowProfile::ForwardPoison
				|| Profile == EMoonlightFlowProfile::ReversedPoison;
			const bool bIsBurnLinkProfile =
				Profile == EMoonlightFlowProfile::ForwardBurn
				|| Profile == EMoonlightFlowProfile::ReversedBurn;
			if (bIsPoisonLinkProfile)
			{
				SlashNode->HitGameplayEventTag = RequestTag(MoonlightPoisonHitEventTag, ReportLines);
				SlashNode->ExpireGameplayEventTag = RequestTag(MoonlightPoisonExpireEventTag, ReportLines);
				SlashNode->HitNiagaraSystem = nullptr;
				SlashNode->ExpireNiagaraSystem = nullptr;
				SlashNode->AdditionalHitEffect = nullptr;
				SlashNode->AdditionalHitSetByCallerTag = FGameplayTag();
				SlashNode->AdditionalHitSetByCallerValue = 0.f;
			}
			else if (bIsBurnLinkProfile)
			{
				SlashNode->HitGameplayEventTag = RequestTag(MoonlightBurnHitEventTag, ReportLines);
				SlashNode->ExpireGameplayEventTag = FGameplayTag();
				SlashNode->HitNiagaraSystem = nullptr;
				SlashNode->ExpireNiagaraSystem = nullptr;
			}
			else
			{
				SlashNode->HitGameplayEventTag = FGameplayTag();
				SlashNode->ExpireGameplayEventTag = FGameplayTag();
			}

			++UpdatedNodeCount;
		}

		if (UpdatedNodeCount > 0)
		{
			FlowAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(FlowAsset->GetPackage());
			const bool bIsPoisonLinkProfile =
				Profile == EMoonlightFlowProfile::ForwardPoison
				|| Profile == EMoonlightFlowProfile::ReversedPoison;
			const bool bIsBurnLinkProfile =
				Profile == EMoonlightFlowProfile::ForwardBurn
				|| Profile == EMoonlightFlowProfile::ReversedBurn;
			if (bIsPoisonLinkProfile)
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Configured `%s`: Moonlight slash-wave nodes=%d, %s, HitEvent=`%s`."),
					*FlowName,
					UpdatedNodeCount,
					CleanVfxPolicy,
					*MoonlightPoisonHitEventTag));
			}
			else if (bIsBurnLinkProfile)
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Configured `%s`: Moonlight slash-wave nodes=%d, %s, HitEvent=`%s`."),
					*FlowName,
					UpdatedNodeCount,
					CleanVfxPolicy,
					*MoonlightBurnHitEventTag));
			}
			else
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Configured `%s`: Moonlight slash-wave nodes=%d, %s."),
					*FlowName,
					UpdatedNodeCount,
					CleanVfxPolicy));
				if (Profile == EMoonlightFlowProfile::Base)
				{
					ReportLines.Add(TEXT("- Configured `FA_Rune512_Moonlight_Base`: Combo1=1 projectile, Combo2=2 projectiles, Combo3+=3 projectiles."));
				}
			}
		}
		else
		{
			ReportLines.Add(FString::Printf(TEXT("- No Spawn Slash Wave Projectile node found in `%s`."), *FlowName));
		}
	}

	UEdGraphPin* GetFirstInputPin(UFlowNode* Node)
	{
		UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
		return GraphNode ? GraphNode->GetInputPin(0) : nullptr;
	}

	UEdGraphPin* GetFirstOutputPin(UFlowNode* Node)
	{
		UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
		return GraphNode ? GraphNode->GetOutputPin(0) : nullptr;
	}

	void LinkFlowNodes(UFlowNode* FromNode, UFlowNode* ToNode)
	{
		UEdGraphPin* FromPin = GetFirstOutputPin(FromNode);
		UEdGraphPin* ToPin = GetFirstInputPin(ToNode);
		if (!FromPin || !ToPin)
		{
			return;
		}

		FromPin->BreakAllPinLinks();
		ToPin->BreakAllPinLinks();
		FromPin->MakeLinkTo(ToPin);
	}

	UEdGraphPin* FindGraphPinByName(UFlowNode* Node, const FName PinName, const EEdGraphPinDirection Direction)
	{
		UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
		if (!GraphNode)
		{
			return nullptr;
		}

		for (UEdGraphPin* Pin : GraphNode->Pins)
		{
			if (Pin && Pin->PinName == PinName && Pin->Direction == Direction)
			{
				return Pin;
			}
		}

		return nullptr;
	}

	void RefreshGraphNodePins(UFlowNode* Node)
	{
		if (UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr)
		{
			GraphNode->ReconstructNode();
		}
	}

	bool LinkDataPins(UFlowNode* FromNode, const FName FromPinName, UFlowNode* ToNode, const FName ToPinName)
	{
		UEdGraphPin* FromPin = FindGraphPinByName(FromNode, FromPinName, EGPD_Output);
		UEdGraphPin* ToPin = FindGraphPinByName(ToNode, ToPinName, EGPD_Input);
		if (!FromPin || !ToPin)
		{
			return false;
		}

		ToPin->BreakAllPinLinks();
		FromPin->MakeLinkTo(ToPin);
		return true;
	}

	UFlowNode* FindPreviousFlowNode(UFlowNode* Node)
	{
		UEdGraphPin* InputPin = GetFirstInputPin(Node);
		if (!InputPin)
		{
			return nullptr;
		}

		for (UEdGraphPin* LinkedPin : InputPin->LinkedTo)
		{
			if (!LinkedPin)
			{
				continue;
			}

			if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(LinkedPin->GetOwningNode()))
			{
				return Cast<UFlowNode>(GraphNode->GetFlowNodeBase());
			}
		}

		return nullptr;
	}

	UFlowNode* CreateFlowNodeAfter(UFlowGraph* FlowGraph, UFlowNode* FromNode, UClass* NodeClass, const FVector2D& NodeLocation)
	{
		UEdGraphPin* FromPin = GetFirstOutputPin(FromNode);
		if (!FlowGraph || !FromPin || !NodeClass)
		{
			return nullptr;
		}

		FromPin->BreakAllPinLinks();
		UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			FlowGraph,
			FromPin,
			NodeClass,
			NodeLocation,
			false);
		return NewGraphNode ? Cast<UFlowNode>(NewGraphNode->GetFlowNodeBase()) : nullptr;
	}

	template <typename TPredicate>
	UFlowNode* FindFirstNode(UFlowAsset* FlowAsset, TPredicate Predicate)
	{
		if (!FlowAsset)
		{
			return nullptr;
		}

		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (Pair.Value && Predicate(Pair.Value))
			{
				return Pair.Value;
			}
		}
		return nullptr;
	}

	UBFNode_ApplyEffect* FindBurnApplyEffectNode(UFlowAsset* FlowAsset)
	{
		return Cast<UBFNode_ApplyEffect>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node)
			{
				UBFNode_ApplyEffect* ApplyNode = Cast<UBFNode_ApplyEffect>(Node);
				return ApplyNode && ApplyNode->Effect == UGE_RuneBurn::StaticClass();
			}));
	}

	UBFNode_PlayNiagara* FindNiagaraNodeByEffectName(UFlowAsset* FlowAsset, const FName EffectName)
	{
		return Cast<UBFNode_PlayNiagara>(FindFirstNode(
			FlowAsset,
			[EffectName](UFlowNode* Node)
			{
				const UBFNode_PlayNiagara* PlayNode = Cast<UBFNode_PlayNiagara>(Node);
				return PlayNode && PlayNode->EffectName == EffectName;
			}));
	}

	UBFNode_PlayFlipbookVFX* FindFlipbookNodeByEffectName(UFlowAsset* FlowAsset, const FName EffectName)
	{
		return Cast<UBFNode_PlayFlipbookVFX>(FindFirstNode(
			FlowAsset,
			[EffectName](UFlowNode* Node)
			{
				const UBFNode_PlayFlipbookVFX* PlayNode = Cast<UBFNode_PlayFlipbookVFX>(Node);
				return PlayNode && PlayNode->EffectName == EffectName;
			}));
	}

	int32 ClearNiagaraVfxNodes(UFlowAsset* FlowAsset)
	{
		if (!FlowAsset)
		{
			return 0;
		}

		int32 ClearedCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (UBFNode_PlayNiagara* NiagaraNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				NiagaraNode->Modify();
				NiagaraNode->NiagaraSystem = nullptr;
				NiagaraNode->EffectName = NAME_None;
				NiagaraNode->Lifetime = 0.f;
				NiagaraNode->bDestroyWithFlow = false;
				++ClearedCount;
			}
		}
		return ClearedCount;
	}

	int32 ClearFlipbookVfxNodes(UFlowAsset* FlowAsset)
	{
		if (!FlowAsset)
		{
			return 0;
		}

		int32 ClearedCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (UBFNode_PlayFlipbookVFX* FlipbookNode = Cast<UBFNode_PlayFlipbookVFX>(Pair.Value))
			{
				FlipbookNode->Modify();
				FlipbookNode->Texture = nullptr;
				FlipbookNode->Material = nullptr;
				FlipbookNode->PlaneMesh = nullptr;
				FlipbookNode->EffectName = NAME_None;
				FlipbookNode->bDestroyWithFlow = false;
				++ClearedCount;
			}
		}
		return ClearedCount;
	}

	FString MakeEffectProfileNameForFlow(const FString& FlowName)
	{
		FString ProfileName = FlowName;
		ProfileName.RemoveFromStart(TEXT("FA_"));
		return TEXT("EP_") + ProfileName;
	}

	FString GetProfileEffectTagName(EMoonlightFlowProfile Profile)
	{
		switch (Profile)
		{
		case EMoonlightFlowProfile::Base:
		case EMoonlightFlowProfile::ForwardAttack:
		case EMoonlightFlowProfile::ReversedAttack:
			return TEXT("Card.Effect.Moonlight");
		case EMoonlightFlowProfile::ForwardBurn:
		case EMoonlightFlowProfile::ReversedBurn:
			return TEXT("Card.Effect.Burn");
		case EMoonlightFlowProfile::ForwardPoison:
		case EMoonlightFlowProfile::ReversedPoison:
			return TEXT("Card.Effect.Poison");
		case EMoonlightFlowProfile::ForwardSplit:
		case EMoonlightFlowProfile::ReversedSplit:
			return TEXT("Card.Effect.SplashSplit");
		case EMoonlightFlowProfile::ForwardShield:
		case EMoonlightFlowProfile::ReversedShield:
			return TEXT("Card.Effect.Shield");
		case EMoonlightFlowProfile::ForwardPierce:
		case EMoonlightFlowProfile::ReversedPierce:
			return TEXT("Card.Effect.Pierce");
		case EMoonlightFlowProfile::ForwardReduceDamage:
		case EMoonlightFlowProfile::ReversedReduceDamage:
			return TEXT("Card.Effect.Defense.ReduceDamage");
		default:
			return FString();
		}
	}

	URuneCardEffectProfileDA* EnsureEffectProfileAsset(
		const FString& ProfileName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (ProfileName.IsEmpty())
		{
			return nullptr;
		}

		const FString TargetPath = GeneratedProfileRoot + TEXT("/") + ProfileName;
		if (URuneCardEffectProfileDA* Existing = LoadAssetByPackagePath<URuneCardEffectProfileDA>(TargetPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found existing EffectProfile `%s`."), *TargetPath));
			return Existing;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would create EffectProfile `%s`."), *TargetPath));
			return nullptr;
		}

		UPackage* Package = CreatePackage(*TargetPath);
		if (!Package)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create EffectProfile package `%s`."), *TargetPath));
			return nullptr;
		}

		const FString AssetName = FPackageName::GetLongPackageAssetName(TargetPath);
		URuneCardEffectProfileDA* Profile = NewObject<URuneCardEffectProfileDA>(
			Package,
			URuneCardEffectProfileDA::StaticClass(),
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Profile)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create EffectProfile asset `%s`."), *TargetPath));
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Profile);
		Profile->MarkPackageDirty();
		DirtyPackages.AddUnique(Profile->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Created EffectProfile `%s`."), *TargetPath));
		return Profile;
	}

	void CopySlashWaveNodeToProfile(
		const UBFNode_SpawnSlashWaveProjectile* SlashNode,
		URuneCardEffectProfileDA* Profile)
	{
		if (!SlashNode || !Profile)
		{
			return;
		}

		Profile->DamageMode = ERuneCardProfileDamageMode::Fixed;
		Profile->DamageValue = SlashNode->Damage;
		Profile->DamageLogType = SlashNode->DamageLogType;

		FRuneCardProfileProjectileConfig& Projectile = Profile->Projectile;
		Projectile.ProjectileClass = SlashNode->ProjectileClass;
		Projectile.DamageEffect = SlashNode->DamageEffect;
		Projectile.SourceSelector = SlashNode->SourceSelector;
		Projectile.Speed = SlashNode->Speed;
		Projectile.MaxDistance = SlashNode->MaxDistance;
		Projectile.MaxHitCount = SlashNode->MaxHitCount;
		Projectile.DamageApplicationsPerTarget = SlashNode->DamageApplicationsPerTarget;
		Projectile.DamageApplicationInterval = SlashNode->DamageApplicationInterval;
		Projectile.CollisionBoxExtent = SlashNode->CollisionBoxExtent;
		Projectile.bScaleVisualWithCollisionExtent = SlashNode->bScaleVisualWithCollisionExtent;
		Projectile.VisualScaleMultiplier = SlashNode->VisualScaleMultiplier;
		Projectile.ProjectileVisualNiagaraSystem = SlashNode->ProjectileVisualNiagaraSystem;
		Projectile.ProjectileVisualNiagaraScale = SlashNode->ProjectileVisualNiagaraScale;
		Projectile.bHideDefaultProjectileVisuals = SlashNode->bHideDefaultProjectileVisuals;
		Projectile.HitNiagaraSystem = SlashNode->HitNiagaraSystem;
		Projectile.HitNiagaraScale = SlashNode->HitNiagaraScale;
		Projectile.ExpireNiagaraSystem = SlashNode->ExpireNiagaraSystem;
		Projectile.ExpireNiagaraScale = SlashNode->ExpireNiagaraScale;
		Projectile.HitGameplayEventTag = SlashNode->HitGameplayEventTag;
		Projectile.ExpireGameplayEventTag = SlashNode->ExpireGameplayEventTag;
		Projectile.ProjectileCount = SlashNode->ProjectileCount;
		Projectile.bAddComboStacksToProjectileCount = SlashNode->bAddComboStacksToProjectileCount;
		Projectile.ProjectilesPerComboStack = SlashNode->ProjectilesPerComboStack;
		Projectile.MaxBonusProjectiles = SlashNode->MaxBonusProjectiles;
		Projectile.ProjectileConeAngleDegrees = SlashNode->ProjectileConeAngleDegrees;
		Projectile.bSpawnProjectilesSequentially = SlashNode->bSpawnProjectilesSequentially;
		Projectile.SequentialProjectileSpawnInterval = SlashNode->SequentialProjectileSpawnInterval;
		Projectile.bDestroyOnWorldStaticHit = SlashNode->bDestroyOnWorldStaticHit;
		Projectile.bForcePureDamage = SlashNode->bForcePureDamage;
		Projectile.BonusArmorDamageMultiplier = SlashNode->BonusArmorDamageMultiplier;
		Projectile.bAddSourceArmorToDamage = SlashNode->bAddSourceArmorToDamage;
		Projectile.SourceArmorToDamageMultiplier = SlashNode->SourceArmorToDamageMultiplier;
		Projectile.bConsumeSourceArmorOnSpawn = SlashNode->bConsumeSourceArmorOnSpawn;
		Projectile.SourceArmorConsumeMultiplier = SlashNode->SourceArmorConsumeMultiplier;
		Projectile.AdditionalHitEffect = SlashNode->AdditionalHitEffect;
		Projectile.AdditionalHitSetByCallerTag = SlashNode->AdditionalHitSetByCallerTag;
		Projectile.AdditionalHitSetByCallerValue = SlashNode->AdditionalHitSetByCallerValue;
		Projectile.bSplitOnFirstHit = SlashNode->bSplitOnFirstHit;
		Projectile.MaxSplitGenerations = SlashNode->MaxSplitGenerations;
		Projectile.SplitProjectileCount = SlashNode->SplitProjectileCount;
		Projectile.SplitConeAngleDegrees = SlashNode->SplitConeAngleDegrees;
		Projectile.bRandomizeSplitDirections = SlashNode->bRandomizeSplitDirections;
		Projectile.SplitRandomYawJitterDegrees = SlashNode->SplitRandomYawJitterDegrees;
		Projectile.SplitRandomPitchDegrees = SlashNode->SplitRandomPitchDegrees;
		Projectile.SplitDamageMultiplier = SlashNode->SplitDamageMultiplier;
		Projectile.SplitSpeedMultiplier = SlashNode->SplitSpeedMultiplier;
		Projectile.SplitMaxDistanceMultiplier = SlashNode->SplitMaxDistanceMultiplier;
		Projectile.SplitCollisionBoxExtentMultiplier = SlashNode->SplitCollisionBoxExtentMultiplier;
		Projectile.bBounceSplitChildrenOnEnemyHit = SlashNode->bBounceSplitChildrenOnEnemyHit;
		Projectile.SplitChildMaxEnemyBounces = SlashNode->SplitChildMaxEnemyBounces;
		Projectile.SpawnOffset = SlashNode->SpawnOffset;
		Projectile.LaunchNiagaraSystem = SlashNode->LaunchNiagaraSystem;
		Projectile.LaunchNiagaraEffectName = SlashNode->LaunchNiagaraEffectName;
		Projectile.bAttachLaunchNiagaraToSource = SlashNode->bAttachLaunchNiagaraToSource;
		Projectile.LaunchNiagaraOffset = SlashNode->LaunchNiagaraOffset;
		Projectile.LaunchNiagaraRotationOffset = SlashNode->LaunchNiagaraRotationOffset;
		Projectile.LaunchNiagaraScale = SlashNode->LaunchNiagaraScale;
		Projectile.bDestroyLaunchNiagaraWithFlow = SlashNode->bDestroyLaunchNiagaraWithFlow;
	}

	void CopyGroundPathNodeToProfile(
		const UBFNode_SpawnRuneGroundPathEffect* GroundPathNode,
		URuneCardEffectProfileDA* Profile)
	{
		if (!GroundPathNode || !Profile)
		{
			return;
		}

		FRuneCardProfileAreaConfig& Area = Profile->Area;
		Area.Effect = GroundPathNode->Effect;
		Area.TargetPolicy = GroundPathNode->TargetPolicy;
		Area.Shape = GroundPathNode->Shape;
		Area.FacingMode = GroundPathNode->FacingMode;
		Area.bCenterOnPathLength = GroundPathNode->bCenterOnPathLength;
		Area.RotationYawOffset = GroundPathNode->RotationYawOffset;
		Area.Duration = GroundPathNode->Duration;
		Area.TickInterval = GroundPathNode->TickInterval;
		Area.Length = GroundPathNode->Length;
		Area.Width = GroundPathNode->Width;
		Area.Height = GroundPathNode->Height;
		Area.DecalProjectionDepth = GroundPathNode->DecalProjectionDepth;
		Area.DecalPlaneRotationDegrees = GroundPathNode->DecalPlaneRotationDegrees;
		Area.SpawnOffset = GroundPathNode->SpawnOffset;
		Area.Source = GroundPathNode->Source;
		Area.DecalMaterial = GroundPathNode->DecalMaterial;
		Area.NiagaraSystem = GroundPathNode->NiagaraSystem;
		Area.NiagaraScale = GroundPathNode->NiagaraScale;
		Area.NiagaraInstanceCount = GroundPathNode->NiagaraInstanceCount;
		Area.SetByCallerTag1 = GroundPathNode->SetByCallerTag1;
		Area.SetByCallerValue1 = GroundPathNode->SetByCallerValue1.Value;
		Area.SetByCallerTag2 = GroundPathNode->SetByCallerTag2;
		Area.SetByCallerValue2 = GroundPathNode->SetByCallerValue2.Value;
		Area.ApplicationCount = GroundPathNode->ApplicationCount;
		Area.bApplyOncePerTarget = GroundPathNode->bApplyOncePerTarget;
	}

	URuneCardEffectProfileDA* ConfigureMoonlightEffectProfile(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile FlowProfile = ResolveMoonlightFlowProfile(FlowName);
		if (FlowProfile == EMoonlightFlowProfile::None)
		{
			return nullptr;
		}

		const FString ProfileName = MakeEffectProfileNameForFlow(FlowName);
		URuneCardEffectProfileDA* Profile = EnsureEffectProfileAsset(ProfileName, bDryRun, ReportLines, DirtyPackages);
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would update EffectProfile `%s` from `%s`."), *ProfileName, *FlowName));
			return Profile;
		}
		if (!Profile)
		{
			return nullptr;
		}

		Profile->Modify();
		Profile->DebugName = FName(*ProfileName);
		Profile->DebugColor = FLinearColor(0.55f, 0.72f, 1.0f, 1.0f);
		const FString EffectTagName = GetProfileEffectTagName(FlowProfile);
		if (!EffectTagName.IsEmpty())
		{
			Profile->EffectIdTag = RequestTag(EffectTagName, ReportLines);
		}

		bool bCopied = false;
		if (FlowProfile == EMoonlightFlowProfile::ReversedBurn || FlowProfile == EMoonlightFlowProfile::ReversedPoison)
		{
			UBFNode_SpawnRuneGroundPathEffect* GroundPathNode = Cast<UBFNode_SpawnRuneGroundPathEffect>(FindFirstNode(
				FlowAsset,
				[](UFlowNode* Node) { return Cast<UBFNode_SpawnRuneGroundPathEffect>(Node) != nullptr; }));
			if (GroundPathNode)
			{
				CopyGroundPathNodeToProfile(GroundPathNode, Profile);
				bCopied = true;
			}
		}
		else
		{
			UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
				FlowAsset,
				[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
			if (SlashNode)
			{
				CopySlashWaveNodeToProfile(SlashNode, Profile);
				bCopied = true;
			}
		}

		const bool bIsBurnProfile = FlowProfile == EMoonlightFlowProfile::ForwardBurn || FlowProfile == EMoonlightFlowProfile::ReversedBurn;
		if (bIsBurnProfile)
		{
			const FGameplayTag BurnDamageTag = RequestTag(TEXT("Data.Damage.Burn"), ReportLines);
			Profile->Effect.GameplayEffectClass = UGE_RuneBurn::StaticClass();
			Profile->Effect.EffectDataAsset = nullptr;
			Profile->Effect.ApplicationCount = 1;
			Profile->Effect.bRemoveEffectOnCleanup = false;
			Profile->Effect.bOverrideDuration = true;
			Profile->Effect.Duration = FlowProfile == EMoonlightFlowProfile::ReversedBurn ? Profile->Area.Duration : 4.0f;
			Profile->Effect.bOverridePeriod = true;
			Profile->Effect.Period = 1.0f;
			Profile->Effect.SetByCallerValues.Reset();
			if (BurnDamageTag.IsValid())
			{
				FRuneCardProfileSetByCaller BurnDamage;
				BurnDamage.Tag = BurnDamageTag;
				BurnDamage.Value = FlowProfile == EMoonlightFlowProfile::ReversedBurn ? Profile->Area.SetByCallerValue1 : 6.0f;
				BurnDamage.bUseCombatCardEffectMultiplier = false;
				Profile->Effect.SetByCallerValues.Add(BurnDamage);
			}

			Profile->VFX.NiagaraSystem = LoadAssetByPackagePath<UNiagaraSystem>(BurnNiagaraPath);
			Profile->VFX.EffectName = FName(TEXT("Rune.Burn.ProfileNiagara"));
			Profile->VFX.AttachSocketName = FName(TEXT("spine_03"));
			Profile->VFX.AttachSocketFallbackNames = { FName(TEXT("spine_02")), FName(TEXT("pelvis")), FName(TEXT("root")) };
			Profile->VFX.AttachTarget = EBFTargetSelector::LastDamageTarget;
			Profile->VFX.bAttachToTarget = true;
			Profile->VFX.LocationOffset = FVector(0.0f, 0.0f, 6.0f);
			Profile->VFX.RotationOffset = FRotator::ZeroRotator;
			Profile->VFX.Scale = FVector(0.28f);
			Profile->VFX.Lifetime = Profile->Effect.Duration;
			Profile->VFX.bDestroyWithFlow = false;
		}

		Profile->MarkPackageDirty();
		DirtyPackages.AddUnique(Profile->GetPackage());
		ReportLines.Add(FString::Printf(
			TEXT("- Updated EffectProfile `%s` from `%s` (%s)."),
			*ProfileName,
			*FlowName,
			bCopied ? TEXT("copied node parameters") : TEXT("no matching legacy node found")));
		return Profile;
	}

	void ConfigureMoonlightProjectileProfileNode(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		URuneCardEffectProfileDA* Profile,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile FlowProfile = ResolveMoonlightFlowProfile(FlowName);
		if (FlowProfile == EMoonlightFlowProfile::None
			|| FlowProfile == EMoonlightFlowProfile::ReversedBurn
			|| FlowProfile == EMoonlightFlowProfile::ReversedPoison)
		{
			return;
		}
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would connect `%s` to Spawn Rune Projectile Profile."), *FlowName));
			return;
		}
		if (!FlowAsset || !Profile)
		{
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		if (!FlowGraph)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot profile-migrate `%s`: missing FlowGraph."), *FlowName));
			return;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
		if (!SlashNode)
		{
			return;
		}

		UFlowNode* PreviousNode = FindPreviousFlowNode(SlashNode);
		if (!PreviousNode)
		{
			PreviousNode = FlowAsset->GetDefaultEntryNode();
		}
		if (!PreviousNode)
		{
			return;
		}

		TArray<UEdGraphPin*> OldNextPins;
		if (UEdGraphPin* SlashOutput = GetFirstOutputPin(SlashNode))
		{
			OldNextPins = SlashOutput->LinkedTo;
		}

		UBFNode_SpawnRuneProjectileProfile* ProfileNode = Cast<UBFNode_SpawnRuneProjectileProfile>(FindFirstNode(
			FlowAsset,
			[Profile](UFlowNode* Node)
			{
				const UBFNode_SpawnRuneProjectileProfile* Candidate = Cast<UBFNode_SpawnRuneProjectileProfile>(Node);
				return Candidate && Candidate->Profile == Profile;
			}));

		if (!ProfileNode)
		{
			const UFlowGraphNode* SlashGraphNode = Cast<UFlowGraphNode>(SlashNode->GetGraphNode());
			const FVector2D NodeLocation(
				SlashGraphNode ? static_cast<float>(SlashGraphNode->NodePosX) : 320.f,
				SlashGraphNode ? static_cast<float>(SlashGraphNode->NodePosY + 160) : 160.f);
			ProfileNode = Cast<UBFNode_SpawnRuneProjectileProfile>(CreateFlowNodeAfter(
				FlowGraph,
				PreviousNode,
				UBFNode_SpawnRuneProjectileProfile::StaticClass(),
				NodeLocation));
		}

		if (!ProfileNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Spawn Rune Projectile Profile node for `%s`."), *FlowName));
			return;
		}

		ProfileNode->Modify();
		ProfileNode->Profile = Profile;
		RefreshGraphNodePins(ProfileNode);

		if (UEdGraphPin* SlashInput = GetFirstInputPin(SlashNode))
		{
			SlashInput->BreakAllPinLinks();
		}
		if (UEdGraphPin* SlashOutput = GetFirstOutputPin(SlashNode))
		{
			SlashOutput->BreakAllPinLinks();
		}
		LinkFlowNodes(PreviousNode, ProfileNode);
		if (UEdGraphPin* ProfileOutput = GetFirstOutputPin(ProfileNode); ProfileOutput && OldNextPins.Num() > 0)
		{
			ProfileOutput->BreakAllPinLinks();
			for (UEdGraphPin* NextPin : OldNextPins)
			{
				if (NextPin)
				{
					NextPin->BreakAllPinLinks();
					ProfileOutput->MakeLinkTo(NextPin);
				}
			}
		}

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();
		ReportLines.Add(FString::Printf(TEXT("- `%s` now executes Spawn Rune Projectile Profile `%s`; legacy slash-wave node is retained but disconnected."), *FlowName, *GetNameSafe(Profile)));
	}

	void ConfigureMoonlightAreaProfileNode(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		URuneCardEffectProfileDA* Profile,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile FlowProfile = ResolveMoonlightFlowProfile(FlowName);
		if (FlowProfile != EMoonlightFlowProfile::ReversedBurn
			&& FlowProfile != EMoonlightFlowProfile::ReversedPoison)
		{
			return;
		}
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would connect `%s` to Spawn Rune Area Profile."), *FlowName));
			return;
		}
		if (!FlowAsset || !Profile)
		{
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		UBFNode_CalcRuneGroundPathTransform* CalcNode = Cast<UBFNode_CalcRuneGroundPathTransform>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_CalcRuneGroundPathTransform>(Node) != nullptr; }));
		UBFNode_SpawnRuneGroundPathEffect* GroundPathNode = Cast<UBFNode_SpawnRuneGroundPathEffect>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnRuneGroundPathEffect>(Node) != nullptr; }));
		if (!FlowGraph || !CalcNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot profile-migrate `%s`: missing FlowGraph or Calc Rune Ground Path Transform."), *FlowName));
			return;
		}

		UBFNode_SpawnRuneAreaProfile* ProfileNode = Cast<UBFNode_SpawnRuneAreaProfile>(FindFirstNode(
			FlowAsset,
			[Profile](UFlowNode* Node)
			{
				const UBFNode_SpawnRuneAreaProfile* Candidate = Cast<UBFNode_SpawnRuneAreaProfile>(Node);
				return Candidate && Candidate->Profile == Profile;
			}));

		if (!ProfileNode)
		{
			const UFlowGraphNode* GroundGraphNode = GroundPathNode ? Cast<UFlowGraphNode>(GroundPathNode->GetGraphNode()) : nullptr;
			const FVector2D NodeLocation(
				GroundGraphNode ? static_cast<float>(GroundGraphNode->NodePosX) : 560.f,
				GroundGraphNode ? static_cast<float>(GroundGraphNode->NodePosY + 160) : 160.f);
			ProfileNode = Cast<UBFNode_SpawnRuneAreaProfile>(CreateFlowNodeAfter(
				FlowGraph,
				CalcNode,
				UBFNode_SpawnRuneAreaProfile::StaticClass(),
				NodeLocation));
		}

		if (!ProfileNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Spawn Rune Area Profile node for `%s`."), *FlowName));
			return;
		}

		ProfileNode->Modify();
		ProfileNode->Profile = Profile;
		RefreshGraphNodePins(ProfileNode);
		LinkFlowNodes(CalcNode, ProfileNode);
		const bool bLocationLinked = LinkDataPins(
			CalcNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnLocation),
			ProfileNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnLocationOverride));
		const bool bRotationLinked = LinkDataPins(
			CalcNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnRotation),
			ProfileNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneAreaProfile, SpawnRotationOverride));
		if (GroundPathNode)
		{
			if (UEdGraphPin* GroundInput = GetFirstInputPin(GroundPathNode))
			{
				GroundInput->BreakAllPinLinks();
			}
		}

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();
		ReportLines.Add(FString::Printf(
			TEXT("- `%s` now executes Spawn Rune Area Profile `%s`; LocationPin=%d RotationPin=%d, legacy ground-path node retained but disconnected."),
			*FlowName,
			*GetNameSafe(Profile),
			bLocationLinked ? 1 : 0,
			bRotationLinked ? 1 : 0));
	}

	template <typename TPredicate>
	int32 RemoveFlowNodesWhere(UFlowAsset* FlowAsset, UFlowGraph* FlowGraph, TPredicate Predicate)
	{
		if (!FlowAsset || !FlowGraph)
		{
			return 0;
		}

		TArray<TPair<FGuid, UFlowNode*>> NodesToRemove;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (Pair.Value && Predicate(Pair.Value))
			{
				NodesToRemove.Emplace(Pair.Key, Pair.Value);
			}
		}

		for (const TPair<FGuid, UFlowNode*>& Pair : NodesToRemove)
		{
			if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pair.Value->GetGraphNode()))
			{
				FlowGraph->GetSchema()->BreakNodeLinks(*GraphNode);
				GraphNode->DestroyNode();
			}

			FlowAsset->UnregisterNode(Pair.Key);
		}

		return NodesToRemove.Num();
	}

	void ConfigureNiagaraNode(
		UBFNode_PlayNiagara* NiagaraNode,
		UNiagaraSystem* NiagaraSystem,
		const FName EffectName,
		const EBFTargetSelector Target,
		const FName SocketName,
		const TArray<FName>& SocketFallbackNames,
		const bool bAttachToTarget,
		const FVector& LocationOffset,
		const FRotator& RotationOffset,
		const FVector& Scale,
		const float Lifetime,
		const bool bDestroyWithFlow)
	{
		if (!NiagaraNode)
		{
			return;
		}

		NiagaraNode->Modify();
		NiagaraNode->NiagaraSystem = NiagaraSystem;
		NiagaraNode->EffectName = EffectName;
		NiagaraNode->AttachTarget = Target;
		NiagaraNode->AttachSocketName = SocketName;
		NiagaraNode->AttachSocketFallbackNames = SocketFallbackNames;
		NiagaraNode->bAttachToTarget = bAttachToTarget;
		NiagaraNode->LocationOffset = LocationOffset;
		NiagaraNode->RotationOffset = RotationOffset;
		NiagaraNode->Scale = Scale;
		NiagaraNode->Lifetime = Lifetime;
		NiagaraNode->bDestroyWithFlow = bDestroyWithFlow;
	}

	void ConfigureFlipbookNode(
		UBFNode_PlayFlipbookVFX* FlipbookNode,
		UTexture2D* Texture,
		UMaterialInterface* Material,
		const FName EffectName,
		const EBFTargetSelector Target,
		const FName SocketName,
		const TArray<FName>& SocketFallbackNames,
		const FVector& Offset,
		const float Size,
		const float Duration,
		const float Lifetime,
		const bool bLoop,
		const bool bDestroyWithFlow,
		const bool bProjectToVisibleSurface)
	{
		if (!FlipbookNode)
		{
			return;
		}

		FlipbookNode->Modify();
		FlipbookNode->Texture = Texture;
		FlipbookNode->Material = Material;
		FlipbookNode->PlaneMesh = nullptr;
		FlipbookNode->Rows = 4;
		FlipbookNode->Columns = 4;
		FlipbookNode->Duration = Duration;
		FlipbookNode->Lifetime = Lifetime;
		FlipbookNode->bLoop = bLoop;
		FlipbookNode->Size = Size;
		FlipbookNode->Target = Target;
		FlipbookNode->Socket = SocketName;
		FlipbookNode->SocketFallbackNames = SocketFallbackNames;
		FlipbookNode->Offset = Offset;
		FlipbookNode->bProjectToVisibleSurface = bProjectToVisibleSurface;
		FlipbookNode->SurfaceOffset = 8.f;
		FlipbookNode->SurfaceFallbackRadiusScale = 0.45f;
		FlipbookNode->SurfaceTraceExtraDistance = 140.f;
		FlipbookNode->bFaceCamera = true;
		FlipbookNode->bDestroyWithFlow = bDestroyWithFlow;
		FlipbookNode->EmissiveColor = FLinearColor(1.25f, 0.78f, 0.36f, 1.f);
		FlipbookNode->AlphaScale = 1.f;
		FlipbookNode->EffectName = EffectName;
	}

	void ConfigureBurnApplyEffectNode(
		UBFNode_ApplyEffect* ApplyNode,
		float DamagePerTick,
		TArray<FString>& ReportLines)
	{
		if (!ApplyNode)
		{
			return;
		}

		const FGameplayTag BurnDamageTag = RequestTag(TEXT("Data.Damage.Burn"), ReportLines);
		ApplyNode->Modify();
		ApplyNode->Effect = UGE_RuneBurn::StaticClass();
		ApplyNode->Level = FFlowDataPinInputProperty_Float(1.f);
		ApplyNode->Target = EBFTargetSelector::LastDamageTarget;
		ApplyNode->ApplicationCount = 1;
		ApplyNode->bRemoveEffectOnCleanup = false;
		ApplyNode->SetByCallerTag1 = BurnDamageTag;
		ApplyNode->SetByCallerValue1 = FFlowDataPinInputProperty_Float(DamagePerTick);
		ApplyNode->SetByCallerTag2 = FGameplayTag();
		ApplyNode->SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.f);
		ApplyNode->SetByCallerTag3 = FGameplayTag();
		ApplyNode->SetByCallerValue3 = FFlowDataPinInputProperty_Float(0.f);
	}

	void ConfigureMoonlightReversedGroundPathFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile Profile = ResolveMoonlightFlowProfile(FlowName);
		if (Profile != EMoonlightFlowProfile::ReversedBurn
			&& Profile != EMoonlightFlowProfile::ReversedPoison)
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure `%s` as Moonlight reversed ground path."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure ground path `%s`: asset not loaded."), *FlowName));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		if (!FlowGraph)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing FlowGraph."), *FlowName));
			return;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
		UFlowNode* PreviousNode = SlashNode ? FindPreviousFlowNode(SlashNode) : nullptr;
		UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
		UFlowNode* ChainStartNode = PreviousNode ? PreviousNode : EntryNode;

		UBFNode_SpawnRuneGroundPathEffect* GroundPathNode = Cast<UBFNode_SpawnRuneGroundPathEffect>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnRuneGroundPathEffect>(Node) != nullptr; }));
		UBFNode_CalcRuneGroundPathTransform* CalcTransformNode = Cast<UBFNode_CalcRuneGroundPathTransform>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_CalcRuneGroundPathTransform>(Node) != nullptr; }));

		UFlowGraphNode* SlashGraphNode = SlashNode ? Cast<UFlowGraphNode>(SlashNode->GetGraphNode()) : nullptr;
		const FVector2D NodeLocation(
			SlashGraphNode ? static_cast<float>(SlashGraphNode->NodePosX) : 320.f,
			SlashGraphNode ? static_cast<float>(SlashGraphNode->NodePosY) : 0.f);

		const FVector2D CalcNodeLocation(NodeLocation.X - 260.0f, NodeLocation.Y);

		if (!CalcTransformNode)
		{
			if (!ChainStartNode)
			{
				ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: no entry node found for rebuilt ground path chain."), *FlowName));
				return;
			}

			CalcTransformNode = Cast<UBFNode_CalcRuneGroundPathTransform>(CreateFlowNodeAfter(
				FlowGraph,
				ChainStartNode,
				UBFNode_CalcRuneGroundPathTransform::StaticClass(),
				CalcNodeLocation));
		}
		else if (ChainStartNode && ChainStartNode != CalcTransformNode)
		{
			LinkFlowNodes(ChainStartNode, CalcTransformNode);
		}

		if (!GroundPathNode)
		{
			if (!CalcTransformNode)
			{
				ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: no calc transform node found."), *FlowName));
				return;
			}

			GroundPathNode = Cast<UBFNode_SpawnRuneGroundPathEffect>(CreateFlowNodeAfter(
				FlowGraph,
				CalcTransformNode,
				UBFNode_SpawnRuneGroundPathEffect::StaticClass(),
				NodeLocation));
		}
		else if (CalcTransformNode && static_cast<UFlowNode*>(CalcTransformNode) != static_cast<UFlowNode*>(GroundPathNode))
		{
			LinkFlowNodes(CalcTransformNode, GroundPathNode);
		}

		if (!GroundPathNode || !CalcTransformNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Rune Ground Path node for `%s`."), *FlowName));
			return;
		}

		RefreshGraphNodePins(CalcTransformNode);
		RefreshGraphNodePins(GroundPathNode);
		if (ChainStartNode && ChainStartNode != CalcTransformNode)
		{
			LinkFlowNodes(ChainStartNode, CalcTransformNode);
		}
		if (static_cast<UFlowNode*>(CalcTransformNode) != static_cast<UFlowNode*>(GroundPathNode))
		{
			LinkFlowNodes(CalcTransformNode, GroundPathNode);
		}

		if (SlashNode)
		{
			if (UEdGraphPin* SlashInput = GetFirstInputPin(SlashNode))
			{
				SlashInput->BreakAllPinLinks();
			}
		}
		if (UEdGraphPin* GroundOut = GetFirstOutputPin(GroundPathNode))
		{
			GroundOut->BreakAllPinLinks();
		}

		const bool bIsBurn = Profile == EMoonlightFlowProfile::ReversedBurn;
		UNiagaraSystem* NiagaraSystem = LoadAssetByPackagePath<UNiagaraSystem>(bIsBurn ? BurnNiagaraPath : PoisonHitNiagaraPath);
		UMaterialInterface* DecalMaterial = LoadAssetByPackagePath<UMaterialInterface>(bIsBurn ? BurnGroundPathDecalMaterialPath : PoisonGroundPathDecalMaterialPath);
		TSubclassOf<UGameplayEffect> PoisonEffect = LoadBlueprintClassByPackagePath<UGameplayEffect>(PoisonEffectPath);
		const FGameplayTag BurnDamageTag = RequestTag(TEXT("Data.Damage.Burn"), ReportLines);

		CalcTransformNode->Modify();
		CalcTransformNode->Source = EBFTargetSelector::BuffOwner;
		CalcTransformNode->FacingMode = ERuneGroundPathFacingMode::ToLastDamageTarget;
		CalcTransformNode->Length = FFlowDataPinInputProperty_Float(bIsBurn ? 520.0f : 560.0f);
		CalcTransformNode->SpawnOffset = FVector(45.0f, 0.0f, 6.0f);
		CalcTransformNode->bCenterOnPathLength = true;
		CalcTransformNode->RotationYawOffset = 0.0f;
		CalcTransformNode->bProjectToGround = true;
		CalcTransformNode->GroundTraceUp = 240.0f;
		CalcTransformNode->GroundTraceDown = 900.0f;

		GroundPathNode->Modify();
		GroundPathNode->Effect = bIsBurn ? TSubclassOf<UGameplayEffect>(UGE_RuneBurn::StaticClass()) : PoisonEffect;
		GroundPathNode->TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;
		GroundPathNode->Shape = bIsBurn ? ERuneGroundPathShape::Fan : ERuneGroundPathShape::Rectangle;
		GroundPathNode->FacingMode = ERuneGroundPathFacingMode::ToLastDamageTarget;
		GroundPathNode->bCenterOnPathLength = true;
		GroundPathNode->RotationYawOffset = 0.0f;
		GroundPathNode->Duration = bIsBurn ? 4.0f : 4.5f;
		GroundPathNode->TickInterval = bIsBurn ? 0.5f : 1.0f;
		GroundPathNode->Length = bIsBurn ? 520.0f : 560.0f;
		GroundPathNode->Width = bIsBurn ? 230.0f : 210.0f;
		GroundPathNode->Height = 120.0f;
		GroundPathNode->DecalProjectionDepth = bIsBurn ? 18.0f : 18.0f;
		GroundPathNode->DecalPlaneRotationDegrees = 0.0f;
		GroundPathNode->SpawnOffset = FVector(45.0f, 0.0f, 6.0f);
		GroundPathNode->Source = EBFTargetSelector::BuffOwner;
		GroundPathNode->DecalMaterial = DecalMaterial;
		GroundPathNode->NiagaraSystem = NiagaraSystem;
		GroundPathNode->NiagaraScale = bIsBurn ? FVector(0.38f, 0.38f, 0.16f) : FVector(0.30f, 0.30f, 0.16f);
		GroundPathNode->NiagaraInstanceCount = bIsBurn ? 7 : 1;
		GroundPathNode->ApplicationCount = 1;
		GroundPathNode->SetByCallerTag1 = bIsBurn ? BurnDamageTag : FGameplayTag();
		GroundPathNode->SetByCallerValue1 = FFlowDataPinInputProperty_Float(bIsBurn ? 6.0f : 0.0f);
		GroundPathNode->SetByCallerTag2 = FGameplayTag();
		GroundPathNode->SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.0f);
		GroundPathNode->bApplyOncePerTarget = bIsBurn;

		const bool bLocationPinLinked = LinkDataPins(
			CalcTransformNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnLocation),
			GroundPathNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneGroundPathEffect, SpawnLocationOverride));
		const bool bRotationPinLinked = LinkDataPins(
			CalcTransformNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_CalcRuneGroundPathTransform, SpawnRotation),
			GroundPathNode,
			GET_MEMBER_NAME_CHECKED(UBFNode_SpawnRuneGroundPathEffect, SpawnRotationOverride));

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: Start -> Calc Rune Ground Path Transform -> Spawn Rune Ground Path Effect, Facing=ToLastDamageTarget, Policy=EnemiesOnly, Shape=%s, Effect=%s, Length=%.0f, Width=%.0f, Duration=%.1f, Tick=%.1f, Damage=%.1f, DecalPlaneRot=%.0f, LocationPin=%d, RotationPin=%d; legacy slash-wave disconnected."),
			*FlowName,
			bIsBurn ? TEXT("Fan") : TEXT("Rectangle"),
			*GetNameSafe(GroundPathNode->Effect),
			GroundPathNode->Length,
			GroundPathNode->Width,
			GroundPathNode->Duration,
			GroundPathNode->TickInterval,
			GroundPathNode->SetByCallerValue1.Value,
			GroundPathNode->DecalPlaneRotationDegrees,
			bLocationPinLinked ? 1 : 0,
			bRotationPinLinked ? 1 : 0));
	}

	void ConfigureMoonlightBurnHitVfxFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile Profile = ResolveMoonlightFlowProfile(FlowName);
		if (Profile != EMoonlightFlowProfile::ForwardBurn)
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure Moonlight burn hit VFX nodes in `%s`."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure Moonlight burn VFX `%s`: asset not loaded."), *FlowName));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		if (!FlowGraph)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing FlowGraph."), *FlowName));
			return;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
		if (!SlashNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing Spawn Slash Wave Projectile node."), *FlowName));
			return;
		}

		const FGameplayTag HitEventTag = RequestTag(MoonlightBurnHitEventTag, ReportLines);
		UNiagaraSystem* BurnNiagara = LoadAssetByPackagePath<UNiagaraSystem>(BurnNiagaraPath);
		if (!BurnNiagara)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing burn Niagara dependency for `%s`: `%s`."),
				*FlowName,
				*BurnNiagaraPath));
		}

		UFlowGraphNode* SlashGraphNode = Cast<UFlowGraphNode>(SlashNode->GetGraphNode());
		const int32 BaseX = SlashGraphNode ? SlashGraphNode->NodePosX : 320;
		const int32 BaseY = SlashGraphNode ? SlashGraphNode->NodePosY : 0;

		UBFNode_WaitGameplayEvent* WaitNode = Cast<UBFNode_WaitGameplayEvent>(FindFirstNode(
			FlowAsset,
			[HitEventTag](UFlowNode* Node)
			{
				UBFNode_WaitGameplayEvent* Wait = Cast<UBFNode_WaitGameplayEvent>(Node);
				return Wait && Wait->EventTag == HitEventTag;
			}));
		if (!WaitNode)
		{
			WaitNode = Cast<UBFNode_WaitGameplayEvent>(CreateFlowNodeAfter(
				FlowGraph,
				SlashNode,
				UBFNode_WaitGameplayEvent::StaticClass(),
				FVector2D(BaseX + 320.f, BaseY)));
		}

		const FName BurnNiagaraEffectName = TEXT("Rune.Moonlight.BurnHitNiagara");
		UBFNode_PlayNiagara* HitVfxNode = FindNiagaraNodeByEffectName(FlowAsset, BurnNiagaraEffectName);
		if (!HitVfxNode)
		{
			HitVfxNode = Cast<UBFNode_PlayNiagara>(CreateFlowNodeAfter(
				FlowGraph,
				WaitNode,
				UBFNode_PlayNiagara::StaticClass(),
				FVector2D(BaseX + 640.f, BaseY)));
		}

		if (!WaitNode || !HitVfxNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create burn hit Niagara nodes for `%s`."), *FlowName));
			return;
		}

		UBFNode_ApplyEffect* BurnEffectNode = FindBurnApplyEffectNode(FlowAsset);
		if (!BurnEffectNode)
		{
			BurnEffectNode = Cast<UBFNode_ApplyEffect>(CreateFlowNodeAfter(
				FlowGraph,
				HitVfxNode,
				UBFNode_ApplyEffect::StaticClass(),
				FVector2D(BaseX + 960.f, BaseY)));
		}

		if (!BurnEffectNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create burn hit VFX/DOT nodes for `%s`."), *FlowName));
			return;
		}

		LinkFlowNodes(SlashNode, WaitNode);
		LinkFlowNodes(WaitNode, HitVfxNode);
		LinkFlowNodes(HitVfxNode, BurnEffectNode);

		SlashNode->Modify();
		SlashNode->HitGameplayEventTag = HitEventTag;
		SlashNode->HitNiagaraSystem = nullptr;
		SlashNode->ExpireNiagaraSystem = nullptr;

		WaitNode->Modify();
		WaitNode->EventTag = HitEventTag;
		WaitNode->Target = EBFTargetSelector::BuffOwner;

		ConfigureNiagaraNode(
			HitVfxNode,
			BurnNiagara,
			BurnNiagaraEffectName,
			EBFTargetSelector::LastDamageTarget,
			TEXT("spine_03"),
			{ TEXT("spine_03"), TEXT("spine_02"), TEXT("spine_01"), TEXT("spine"), TEXT("pelvis"), TEXT("body"), TEXT("root") },
			true,
			FVector(0.f, 0.f, 6.f),
			FRotator::ZeroRotator,
			FVector(0.28f, 0.28f, 0.28f),
			3.2f,
			false);

		ConfigureBurnApplyEffectNode(BurnEffectNode, 6.f, ReportLines);
		const int32 ClearedFlipbookCount = ClearFlipbookVfxNodes(FlowAsset);

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara `%s` -> persistent UGE_RuneBurn DOT; cleared Flipbook nodes=%d."),
			*FlowName,
			*MoonlightBurnHitEventTag,
			*BurnNiagaraPath,
			ClearedFlipbookCount));
	}

	void ConfigureMoonlightPoisonAtomicFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile Profile = ResolveMoonlightFlowProfile(FlowName);
		if (Profile != EMoonlightFlowProfile::ForwardPoison)
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure atomic Moonlight poison LinkFlow nodes in `%s`."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure Moonlight poison atomic flow `%s`: asset not loaded."), *FlowName));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		if (!FlowGraph)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing FlowGraph."), *FlowName));
			return;
		}

		UBFNode_SpawnSlashWaveProjectile* SlashNode = Cast<UBFNode_SpawnSlashWaveProjectile>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnSlashWaveProjectile>(Node) != nullptr; }));
		if (!SlashNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing Spawn Slash Wave Projectile node."), *FlowName));
			return;
		}

		const FGameplayTag HitEventTag = RequestTag(MoonlightPoisonHitEventTag, ReportLines);
		const FGameplayTag DamageSetByCallerTag = RequestTag(TEXT("Data.Damage"), ReportLines);
		TSubclassOf<UGameplayEffect> PoisonEffect = LoadBlueprintClassByPackagePath<UGameplayEffect>(PoisonEffectPath);
		TSubclassOf<UGameplayEffect> PoisonSplashEffect = LoadBlueprintClassByPackagePath<UGameplayEffect>(PoisonSplashEffectPath);
		if (!PoisonSplashEffect)
		{
			PoisonSplashEffect = PoisonEffect;
		}

		UNiagaraSystem* PoisonHitNiagara = LoadAssetByPackagePath<UNiagaraSystem>(PoisonHitNiagaraPath);
		UNiagaraSystem* PoisonSpreadNiagara = LoadAssetByPackagePath<UNiagaraSystem>(PoisonSpreadNiagaraPath);
		if (!PoisonEffect)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing poison GE class `%s`; ApplyEffect node still created but Effect is empty."), *PoisonEffectPath));
		}
		if (!PoisonHitNiagara || !PoisonSpreadNiagara)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing poison Niagara dependencies for `%s`: Hit=%s Spread=%s."),
				*FlowName,
				*PoisonHitNiagaraPath,
				*PoisonSpreadNiagaraPath));
		}

		UFlowGraphNode* SlashGraphNode = Cast<UFlowGraphNode>(SlashNode->GetGraphNode());
		const int32 BaseX = SlashGraphNode ? SlashGraphNode->NodePosX : 320;
		const int32 BaseY = SlashGraphNode ? SlashGraphNode->NodePosY : 0;

		UBFNode_WaitGameplayEvent* WaitNode = Cast<UBFNode_WaitGameplayEvent>(FindFirstNode(
			FlowAsset,
			[HitEventTag](UFlowNode* Node)
			{
				UBFNode_WaitGameplayEvent* Wait = Cast<UBFNode_WaitGameplayEvent>(Node);
				return Wait && Wait->EventTag == HitEventTag;
			}));
		if (!WaitNode)
		{
			WaitNode = Cast<UBFNode_WaitGameplayEvent>(CreateFlowNodeAfter(
				FlowGraph,
				SlashNode,
				UBFNode_WaitGameplayEvent::StaticClass(),
				FVector2D(BaseX + 320.f, BaseY)));
		}

		const FName PoisonHitEffectName = TEXT("Rune.Moonlight.PoisonHitNiagara");
		UBFNode_PlayNiagara* HitVfxNode = FindNiagaraNodeByEffectName(FlowAsset, PoisonHitEffectName);
		if (!HitVfxNode)
		{
			HitVfxNode = Cast<UBFNode_PlayNiagara>(CreateFlowNodeAfter(
				FlowGraph,
				WaitNode,
				UBFNode_PlayNiagara::StaticClass(),
				FVector2D(BaseX + 640.f, BaseY)));
		}

		UBFNode_ApplyEffect* PrimaryPoisonNode = Cast<UBFNode_ApplyEffect>(FindFirstNode(
			FlowAsset,
			[PoisonEffect](UFlowNode* Node)
			{
				UBFNode_ApplyEffect* ApplyNode = Cast<UBFNode_ApplyEffect>(Node);
				return ApplyNode && PoisonEffect && ApplyNode->Effect == PoisonEffect;
			}));
		if (!PrimaryPoisonNode)
		{
			PrimaryPoisonNode = Cast<UBFNode_ApplyEffect>(CreateFlowNodeAfter(
				FlowGraph,
				HitVfxNode,
				UBFNode_ApplyEffect::StaticClass(),
				FVector2D(BaseX + 960.f, BaseY)));
		}

		const FName PoisonSpreadEffectName = TEXT("Rune.Moonlight.PoisonSpreadNiagara");
		UBFNode_PlayNiagara* SpreadVfxNode = FindNiagaraNodeByEffectName(FlowAsset, PoisonSpreadEffectName);
		if (!SpreadVfxNode)
		{
			SpreadVfxNode = Cast<UBFNode_PlayNiagara>(CreateFlowNodeAfter(
				FlowGraph,
				PrimaryPoisonNode,
				UBFNode_PlayNiagara::StaticClass(),
				FVector2D(BaseX + 1280.f, BaseY)));
		}

		UBFNode_ApplyGEInRadius* RadiusPoisonNode = Cast<UBFNode_ApplyGEInRadius>(FindFirstNode(
			FlowAsset,
			[PoisonSplashEffect](UFlowNode* Node)
			{
				UBFNode_ApplyGEInRadius* RadiusNode = Cast<UBFNode_ApplyGEInRadius>(Node);
				return RadiusNode && PoisonSplashEffect && RadiusNode->Effect == PoisonSplashEffect;
			}));
		if (!RadiusPoisonNode)
		{
			RadiusPoisonNode = Cast<UBFNode_ApplyGEInRadius>(CreateFlowNodeAfter(
				FlowGraph,
				SpreadVfxNode,
				UBFNode_ApplyGEInRadius::StaticClass(),
				FVector2D(BaseX + 1600.f, BaseY)));
		}

		if (!WaitNode || !HitVfxNode || !PrimaryPoisonNode || !SpreadVfxNode || !RadiusPoisonNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create all atomic poison nodes for `%s`."), *FlowName));
			return;
		}

		LinkFlowNodes(SlashNode, WaitNode);
		LinkFlowNodes(WaitNode, HitVfxNode);
		LinkFlowNodes(HitVfxNode, PrimaryPoisonNode);
		LinkFlowNodes(PrimaryPoisonNode, SpreadVfxNode);
		LinkFlowNodes(SpreadVfxNode, RadiusPoisonNode);

		WaitNode->Modify();
		WaitNode->EventTag = HitEventTag;
		WaitNode->Target = EBFTargetSelector::BuffOwner;

		ConfigureNiagaraNode(
			HitVfxNode,
			PoisonHitNiagara,
			PoisonHitEffectName,
			EBFTargetSelector::LastDamageTarget,
			TEXT("spine_02"),
			{ TEXT("spine_03"), TEXT("spine_02"), TEXT("spine_01"), TEXT("pelvis"), TEXT("root") },
			true,
			FVector(0.f, 0.f, 8.f),
			FRotator::ZeroRotator,
			FVector(0.32f, 0.32f, 0.32f),
			1.2f,
			false);

		PrimaryPoisonNode->Modify();
		PrimaryPoisonNode->Effect = PoisonEffect;
		PrimaryPoisonNode->Target = EBFTargetSelector::LastDamageTarget;
		PrimaryPoisonNode->ApplicationCount = 3;
		PrimaryPoisonNode->bRemoveEffectOnCleanup = false;

		ConfigureNiagaraNode(
			SpreadVfxNode,
			PoisonSpreadNiagara,
			PoisonSpreadEffectName,
			EBFTargetSelector::LastDamageTarget,
			NAME_None,
			{},
			false,
			FVector(0.f, 0.f, 18.f),
			FRotator::ZeroRotator,
			FVector(0.45f, 0.45f, 0.45f),
			1.4f,
			false);

		RadiusPoisonNode->Modify();
		RadiusPoisonNode->Effect = PoisonSplashEffect;
		RadiusPoisonNode->Radius = FFlowDataPinInputProperty_Float(300.f);
		RadiusPoisonNode->LocationSource = EBFTargetSelector::LastDamageTarget;
		RadiusPoisonNode->bUseKillLocation = false;
		RadiusPoisonNode->LocationOffset = FVector::ZeroVector;
		RadiusPoisonNode->bEnemyOnly = true;
		RadiusPoisonNode->bExcludeSelf = true;
		RadiusPoisonNode->bExcludeLocationSourceActor = true;
		RadiusPoisonNode->MaxTargets = 3;
		RadiusPoisonNode->ApplicationCount = 1;
		RadiusPoisonNode->SetByCallerTag1 = DamageSetByCallerTag;
		RadiusPoisonNode->SetByCallerValue1 = FFlowDataPinInputProperty_Float(5.f);
		const int32 ClearedFlipbookCount = ClearFlipbookVfxNodes(FlowAsset);

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> Play Niagara hit -> Apply GE_Poison x3 -> Play Niagara spread -> ApplyGEInRadius radius=300 max=3; cleared Flipbook nodes=%d."),
			*FlowName,
			*MoonlightPoisonHitEventTag,
			ClearedFlipbookCount));
	}

	void ConfigureStandaloneNiagaraVfxFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const bool bIsBurnFlow = FlowName.Equals(TEXT("FA_Rune512_Burn_Base"), ESearchCase::IgnoreCase);
		const bool bIsPoisonFlow = FlowName.Equals(TEXT("FA_Rune512_Poison_Base"), ESearchCase::IgnoreCase);
		if (!bIsBurnFlow && !bIsPoisonFlow)
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure standalone Play Niagara VFX node in `%s`."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure standalone Niagara VFX `%s`: asset not loaded."), *FlowName));
			return;
		}

		const FString NiagaraPath = bIsBurnFlow ? BurnNiagaraPath : PoisonHitNiagaraPath;
		UNiagaraSystem* NiagaraSystem = LoadAssetByPackagePath<UNiagaraSystem>(NiagaraPath);
		if (!NiagaraSystem)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing standalone Niagara asset for `%s`: `%s`."),
				*FlowName,
				*NiagaraPath));
			return;
		}

		const FName EffectName = bIsBurnFlow ? TEXT("Rune.Burn.ApplyNiagara") : TEXT("Rune.Poison.ApplyNiagara");
		const int32 ClearedFlipbookCount = ClearFlipbookVfxNodes(FlowAsset);
		UBFNode_PlayNiagara* ExistingVfxNode = FindNiagaraNodeByEffectName(FlowAsset, EffectName);
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (UBFNode_PlayNiagara* PlayNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				if (PlayNode->EffectName != EffectName)
				{
					PlayNode->Modify();
					PlayNode->NiagaraSystem = nullptr;
					PlayNode->EffectName = NAME_None;
					PlayNode->Lifetime = 0.f;
					PlayNode->bDestroyWithFlow = false;
				}
			}
		}

		UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
		UFlowGraphNode* EntryGraphNode = EntryNode ? Cast<UFlowGraphNode>(EntryNode->GetGraphNode()) : nullptr;
		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		UEdGraphPin* EntryOutputPin = EntryGraphNode ? EntryGraphNode->GetOutputPin(0) : nullptr;
		if (!EntryNode || !EntryGraphNode || !FlowGraph || !EntryOutputPin)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing FlowGraph entry node or output pin."), *FlowName));
			return;
		}

		UBFNode_PlayNiagara* VfxNode = ExistingVfxNode;
		bool bInsertedNode = false;
		if (!VfxNode)
		{
			const FVector2D NodeLocation(
				static_cast<float>(EntryGraphNode->NodePosX + 320),
				static_cast<float>(EntryGraphNode->NodePosY));
			UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
				FlowGraph,
				EntryOutputPin,
				UBFNode_PlayNiagara::StaticClass(),
				NodeLocation,
				false);
			VfxNode = NewGraphNode ? Cast<UBFNode_PlayNiagara>(NewGraphNode->GetFlowNodeBase()) : nullptr;
			bInsertedNode = VfxNode != nullptr;
		}

		if (!VfxNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Play Niagara node for `%s`."), *FlowName));
			return;
		}
		LinkFlowNodes(EntryNode, VfxNode);

		ConfigureNiagaraNode(
			VfxNode,
			NiagaraSystem,
			EffectName,
			EBFTargetSelector::LastDamageTarget,
			bIsBurnFlow ? FName(TEXT("spine_03")) : FName(TEXT("spine_02")),
			{ TEXT("spine_03"), TEXT("spine_02"), TEXT("spine_01"), TEXT("spine"), TEXT("pelvis"), TEXT("body"), TEXT("root") },
			true,
			bIsBurnFlow ? FVector(0.f, 0.f, 6.f) : FVector(0.f, 0.f, 8.f),
			FRotator::ZeroRotator,
			bIsBurnFlow ? FVector(0.28f, 0.28f, 0.28f) : FVector(0.32f, 0.32f, 0.32f),
			bIsBurnFlow ? 3.2f : 1.2f,
			false);

		UBFNode_ApplyEffect* BurnEffectNode = nullptr;
		if (bIsBurnFlow)
		{
			BurnEffectNode = FindBurnApplyEffectNode(FlowAsset);
			if (!BurnEffectNode)
			{
				const FVector2D NodeLocation(
					static_cast<float>(EntryGraphNode->NodePosX + 640),
					static_cast<float>(EntryGraphNode->NodePosY));
				BurnEffectNode = Cast<UBFNode_ApplyEffect>(CreateFlowNodeAfter(
					FlowGraph,
					VfxNode,
					UBFNode_ApplyEffect::StaticClass(),
					NodeLocation));
			}

			if (BurnEffectNode)
			{
				ConfigureBurnApplyEffectNode(BurnEffectNode, 8.f, ReportLines);
				LinkFlowNodes(VfxNode, BurnEffectNode);
			}
			else
			{
				ReportLines.Add(FString::Printf(TEXT("- Failed to create persistent burn DOT node for `%s`."), *FlowName));
			}
		}

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		if (FlowGraph)
		{
			FlowGraph->Modify();
			FlowGraph->NotifyGraphChanged();
		}

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: Play Niagara node `%s` -> `%s`, Socket=%s, Offset=%s, Scale=%s, lifetime=%.1f, burnDOT=%d, cleared Flipbook nodes=%d (%s)."),
			*FlowName,
			*EffectName.ToString(),
			*NiagaraPath,
			*VfxNode->AttachSocketName.ToString(),
			*VfxNode->LocationOffset.ToString(),
			*VfxNode->Scale.ToString(),
			VfxNode->Lifetime,
			BurnEffectNode ? 1 : 0,
			ClearedFlipbookCount,
			bInsertedNode ? TEXT("inserted after Start") : TEXT("updated existing node")));
	}

	void StripApplyAttributeInlineVfx(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const bool bIsBurnFlow = FlowName.Equals(TEXT("FA_Rune512_Burn_Base"), ESearchCase::IgnoreCase);
		const bool bIsPoisonFlow = FlowName.Equals(TEXT("FA_Rune512_Poison_Base"), ESearchCase::IgnoreCase);
		if (!bIsBurnFlow && !bIsPoisonFlow)
		{
			return;
		}

		if (bDryRun || !FlowAsset)
		{
			return;
		}

		int32 ClearedNodeCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			UBFNode_ApplyAttributeModifier* ApplyNode = Cast<UBFNode_ApplyAttributeModifier>(Pair.Value);
			if (!ApplyNode)
			{
				continue;
			}

			ApplyNode->Modify();
			++ClearedNodeCount;
		}

		if (ClearedNodeCount > 0)
		{
			FlowAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(FlowAsset->GetPackage());
			ReportLines.Add(FString::Printf(
				TEXT("- Verified `%s`: Apply Attribute Modifier has no inline VFX fields; checked nodes=%d."),
				*FlowName,
				ClearedNodeCount));
		}
	}

	void ConfigureCombatCardAttributeMultiplierFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const bool bShouldUseCombatCardMultiplier =
			FlowName.Equals(TEXT("FA_Rune512_Attack_Base"), ESearchCase::IgnoreCase);
		if (!bShouldUseCombatCardMultiplier)
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Would configure `%s`: Apply Attribute Modifier uses Combat Card Effect Multiplier."),
				*FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Cannot configure `%s`: Flow asset was not loaded."),
				*FlowName));
			return;
		}

		int32 UpdatedNodeCount = 0;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			UBFNode_ApplyAttributeModifier* ApplyNode = Cast<UBFNode_ApplyAttributeModifier>(Pair.Value);
			if (!ApplyNode)
			{
				continue;
			}

			ApplyNode->Modify();
			ApplyNode->bUseCombatCardEffectMultiplier = true;
			++UpdatedNodeCount;
		}

		if (UpdatedNodeCount > 0)
		{
			FlowAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(FlowAsset->GetPackage());
			ReportLines.Add(FString::Printf(
				TEXT("- Configured `%s`: Apply Attribute Modifier nodes use Combat Card Effect Multiplier; nodes=%d."),
				*FlowName,
				UpdatedNodeCount));
		}
	}

	void ConfigureSplitBaseFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (FlowName != TEXT("FA_Rune512_Split_Base"))
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(TEXT("- Would configure `FA_Rune512_Split_Base`: Start -> Spawn Ranged Projectiles, YawOffsets=-8/+8, no slash-wave node."));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(TEXT("- Cannot configure `FA_Rune512_Split_Base`: Flow asset was not loaded."));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
		if (!FlowGraph || !EntryNode)
		{
			ReportLines.Add(TEXT("- Cannot configure `FA_Rune512_Split_Base`: missing FlowGraph or Entry node."));
			return;
		}

		UBFNode_SpawnRangedProjectiles* ExistingSpawnNode = Cast<UBFNode_SpawnRangedProjectiles>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnRangedProjectiles>(Node) != nullptr; }));

		const int32 RemovedLegacyNodes = RemoveFlowNodesWhere(
			FlowAsset,
			FlowGraph,
			[EntryNode, ExistingSpawnNode](UFlowNode* Node)
			{
				return Node != EntryNode && Node != ExistingSpawnNode;
			});

		UBFNode_SpawnRangedProjectiles* SpawnNode = Cast<UBFNode_SpawnRangedProjectiles>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_SpawnRangedProjectiles>(Node) != nullptr; }));

		if (!SpawnNode)
		{
			SpawnNode = Cast<UBFNode_SpawnRangedProjectiles>(CreateFlowNodeAfter(
				FlowGraph,
				EntryNode,
				UBFNode_SpawnRangedProjectiles::StaticClass(),
				FVector2D(320.f, 0.f)));
		}
		else
		{
			LinkFlowNodes(EntryNode, SpawnNode);
		}

		if (!SpawnNode)
		{
			ReportLines.Add(TEXT("- Failed to create Spawn Ranged Projectiles node for `FA_Rune512_Split_Base`."));
			return;
		}

		SpawnNode->Modify();
		SpawnNode->SourceSelector = EBFTargetSelector::BuffOwner;
		SpawnNode->BulletClass = LoadBlueprintClassByPackagePath<AMusketBullet>(MusketBulletBlueprintPath);
		if (!SpawnNode->BulletClass)
		{
			SpawnNode->BulletClass = AMusketBullet::StaticClass();
		}
		SpawnNode->DamageEffectClass = UGE_MusketBullet_Damage::StaticClass();
		SpawnNode->YawOffsets = { -8.f, 8.f };
		SpawnNode->bUseCombatCardAttackDamage = true;
		SpawnNode->Damage = FFlowDataPinInputProperty_Float(0.f);
		SpawnNode->bShareAttackInstanceGuid = true;
		RefreshGraphNodePins(SpawnNode);
		LinkFlowNodes(EntryNode, SpawnNode);

		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		ReportLines.Add(FString::Printf(
			TEXT("- Configured `FA_Rune512_Split_Base`: Start -> Spawn Ranged Projectiles, YawOffsets=-8/+8, shared AttackInstanceGuid, removed %d legacy nodes, no moonblade."),
			RemovedLegacyNodes));
	}

	void ConfigureSplashBaseFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (FlowName != TEXT("FA_Rune512_Splash_Base"))
		{
			return;
		}

		if (bDryRun)
		{
			ReportLines.Add(TEXT("- Would configure `FA_Rune512_Splash_Base`: OnDamageDealt -> MathFloat(*0.2) -> ApplyGEInRadius radius=300."));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(TEXT("- Cannot configure `FA_Rune512_Splash_Base`: Flow asset was not loaded."));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
		if (!FlowGraph || !EntryNode)
		{
			ReportLines.Add(TEXT("- Cannot configure `FA_Rune512_Splash_Base`: missing FlowGraph or Entry node."));
			return;
		}

		UBFNode_OnDamageDealt* OnDamageNode = Cast<UBFNode_OnDamageDealt>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_OnDamageDealt>(Node) != nullptr; }));
		if (!OnDamageNode)
		{
			OnDamageNode = Cast<UBFNode_OnDamageDealt>(CreateFlowNodeAfter(
				FlowGraph,
				EntryNode,
				UBFNode_OnDamageDealt::StaticClass(),
				FVector2D(320.f, 0.f)));
		}
		else
		{
			LinkFlowNodes(EntryNode, OnDamageNode);
		}

		UBFNode_MathFloat* MathNode = Cast<UBFNode_MathFloat>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_MathFloat>(Node) != nullptr; }));
		if (!MathNode)
		{
			MathNode = Cast<UBFNode_MathFloat>(CreateFlowNodeAfter(
				FlowGraph,
				OnDamageNode,
				UBFNode_MathFloat::StaticClass(),
				FVector2D(640.f, 0.f)));
		}
		else
		{
			LinkFlowNodes(OnDamageNode, MathNode);
		}

		UBFNode_ApplyGEInRadius* RadiusNode = Cast<UBFNode_ApplyGEInRadius>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node) { return Cast<UBFNode_ApplyGEInRadius>(Node) != nullptr; }));
		if (!RadiusNode)
		{
			RadiusNode = Cast<UBFNode_ApplyGEInRadius>(CreateFlowNodeAfter(
				FlowGraph,
				MathNode,
				UBFNode_ApplyGEInRadius::StaticClass(),
				FVector2D(960.f, 0.f)));
		}
		else
		{
			LinkFlowNodes(MathNode, RadiusNode);
		}

		if (!OnDamageNode || !MathNode || !RadiusNode)
		{
			ReportLines.Add(TEXT("- Failed to configure `FA_Rune512_Splash_Base`: missing generated nodes."));
			return;
		}

		const FGameplayTag DamageTag = RequestTag(TEXT("Attribute.ActDamage"), ReportLines);
		OnDamageNode->Modify();
		OnDamageNode->bOncePerSwing = false;

		MathNode->Modify();
		MathNode->A = FFlowDataPinInputProperty_Float(0.f);
		MathNode->Operator = EBFMathOp::Multiply;
		MathNode->B = FFlowDataPinInputProperty_Float(0.2f);

		RadiusNode->Modify();
		RadiusNode->Effect = LoadBlueprintClassByPackagePath<UGameplayEffect>(DamageBasicSetByCallerPath);
		if (!RadiusNode->Effect)
		{
			RadiusNode->Effect = UGE_MusketBullet_Damage::StaticClass();
		}
		RadiusNode->Radius = FFlowDataPinInputProperty_Float(300.f);
		RadiusNode->LocationSource = EBFTargetSelector::LastDamageTarget;
		RadiusNode->bUseKillLocation = false;
		RadiusNode->LocationOffset = FVector::ZeroVector;
		RadiusNode->bEnemyOnly = true;
		RadiusNode->bExcludeSelf = true;
		RadiusNode->bExcludeLocationSourceActor = true;
		RadiusNode->MaxTargets = 0;
		RadiusNode->ApplicationCount = 1;
		RadiusNode->SetByCallerTag1 = DamageTag;
		RadiusNode->SetByCallerValue1 = FFlowDataPinInputProperty_Float(0.f);
		RadiusNode->SetByCallerTag2 = FGameplayTag();
		RadiusNode->SetByCallerValue2 = FFlowDataPinInputProperty_Float(0.f);

		RefreshGraphNodePins(OnDamageNode);
		RefreshGraphNodePins(MathNode);
		RefreshGraphNodePins(RadiusNode);
		LinkFlowNodes(EntryNode, OnDamageNode);
		LinkFlowNodes(OnDamageNode, MathNode);
		LinkFlowNodes(MathNode, RadiusNode);
		LinkDataPins(OnDamageNode, TEXT("LastDamageOutput"), MathNode, TEXT("A"));
		LinkDataPins(MathNode, TEXT("Result"), RadiusNode, TEXT("SetByCallerValue1"));

		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		ReportLines.Add(TEXT("- Configured `FA_Rune512_Splash_Base`: OnDamageDealt -> MathFloat(*0.2) -> ApplyGEInRadius radius=300, exclude main target."));
	}

	void ConfigureProductionWeaponComboAssets(
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Production weapon combo config"));

		UGameplayAbilityComboGraph* ComboGraph = LoadAssetByPackagePath<UGameplayAbilityComboGraph>(THSwordComboGraph);
		if (!ComboGraph)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing combo graph `%s`; weapon DA was not changed."), *THSwordComboGraph));
			return;
		}

		const TArray<FString> WeaponPaths = {
			ProductionTHSwordWeapon,
			ProductionRedSwordWeapon
		};

		for (const FString& WeaponPath : WeaponPaths)
		{
			UObject* Weapon = LoadAssetByPackagePath<UObject>(WeaponPath);
			if (!Weapon)
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing weapon DA `%s`."), *WeaponPath));
				continue;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> GameplayAbilityComboGraph `%s`."),
				bDryRun ? TEXT("Would assign") : TEXT("Assigned"),
				*WeaponPath,
				*THSwordComboGraph));

			if (bDryRun)
			{
				continue;
			}

			Weapon->Modify();
			if (FObjectPropertyBase* ComboGraphProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("GameplayAbilityComboGraph")))
			{
				ComboGraphProperty->SetObjectPropertyValue_InContainer(Weapon, ComboGraph);
			}
			else
			{
				ReportLines.Add(FString::Printf(TEXT("- `%s` has no GameplayAbilityComboGraph property."), *WeaponPath));
			}

			if (FObjectPropertyBase* ComboConfigProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("WeaponComboConfig")))
			{
				ComboConfigProperty->SetObjectPropertyValue_InContainer(Weapon, nullptr);
			}
			Weapon->MarkPackageDirty();
			DirtyPackages.AddUnique(Weapon->GetPackage());
		}

		ReportLines.Add(TEXT("## Production weapon combat card pool config"));
		URuneDataAsset* SplashCard = LoadAssetByPackagePath<URuneDataAsset>(GeneratedRoot + TEXT("/DA_Rune512_Splash"));
		URuneDataAsset* SplitCard = LoadAssetByPackagePath<URuneDataAsset>(GeneratedRoot + TEXT("/DA_Rune512_Split"));
		if (!SplashCard || !SplitCard)
		{
			ReportLines.Add(TEXT("- Missing generated Splash/Split DA; weapon combat deck replacement skipped."));
			return;
		}

		auto ReplaceCardInObjectArray = [](UObject* Owner, const FName PropertyName, URuneDataAsset* FromCard, URuneDataAsset* ToCard)
		{
			if (!Owner || !FromCard || !ToCard)
			{
				return 0;
			}

			FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Owner->GetClass(), PropertyName);
			if (!ArrayProperty)
			{
				return 0;
			}

			FObjectPropertyBase* ObjectInner = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
			if (!ObjectInner)
			{
				return 0;
			}

			void* ArrayValuePtr = ArrayProperty->ContainerPtrToValuePtr<void>(Owner);
			FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayValuePtr);
			int32 ReplacedCount = 0;
			for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
			{
				void* ItemPtr = ArrayHelper.GetRawPtr(Index);
				if (ObjectInner->GetObjectPropertyValue(ItemPtr) == FromCard)
				{
					ObjectInner->SetObjectPropertyValue(ItemPtr, ToCard);
					++ReplacedCount;
				}
			}
			return ReplacedCount;
		};

		auto ReplaceWeaponCard = [&](const FString& WeaponPath, URuneDataAsset* FromCard, URuneDataAsset* ToCard, const TCHAR* Label)
		{
			UObject* WeaponDef = LoadAssetByPackagePath<UObject>(WeaponPath);
			if (!WeaponDef)
			{
				ReportLines.Add(FString::Printf(TEXT("- Missing weapon DA `%s` for %s card pool replacement."), *WeaponPath, Label));
				return;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s`: replace %s -> `%s` in InitialCombatDeck/InitialRunes."),
				bDryRun ? TEXT("Would update") : TEXT("Updated"),
				*WeaponPath,
				Label,
				*GetNameSafe(ToCard)));

			if (bDryRun)
			{
				return;
			}

			WeaponDef->Modify();
			const int32 CombatDeckReplaced = ReplaceCardInObjectArray(WeaponDef, TEXT("InitialCombatDeck"), FromCard, ToCard);
			const int32 RuneListReplaced = ReplaceCardInObjectArray(WeaponDef, TEXT("InitialRunes"), FromCard, ToCard);
			if (CombatDeckReplaced + RuneListReplaced > 0)
			{
				ReportLines.Add(FString::Printf(
					TEXT("- `%s` replaced cards: InitialCombatDeck=%d, InitialRunes=%d."),
					*WeaponPath,
					CombatDeckReplaced,
					RuneListReplaced));
				WeaponDef->MarkPackageDirty();
				DirtyPackages.AddUnique(WeaponDef->GetPackage());
			}
		};

		ReplaceWeaponCard(ProductionTHSwordWeapon, SplitCard, SplashCard, TEXT("melee Split"));
		ReplaceWeaponCard(ProductionRedSwordWeapon, SplitCard, SplashCard, TEXT("melee Split"));
		ReplaceWeaponCard(ProductionHarquebusWeapon, SplashCard, SplitCard, TEXT("ranged Splash"));
	}

	UFlowAsset* EnsureFlowAssetAtPath(
		const FString& TemplatePath,
		const FString& TargetPath,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		UObject* FlowObject = DuplicateAssetIfMissing(TemplatePath, TargetPath, bDryRun, ReportLines, DirtyPackages);
		return Cast<UFlowAsset>(FlowObject);
	}

	void ConfigureSacrificePassiveFlow(
		UFlowAsset* FlowAsset,
		const FSacrificeSpec& Spec,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would configure `%s`: Start -> Grant Sacrifice Passive(%s)."), *Spec.FlowTargetName, *Spec.Key));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: Flow asset was not loaded."), *Spec.FlowTargetName));
			return;
		}

		UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
		UFlowNode* EntryNode = FlowAsset->GetDefaultEntryNode();
		if (!FlowGraph || !EntryNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure `%s`: missing FlowGraph or Entry node."), *Spec.FlowTargetName));
			return;
		}

		UBFNode_GrantSacrificePassive* ExistingGrantNode = Cast<UBFNode_GrantSacrificePassive>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node)
			{
				return Cast<UBFNode_GrantSacrificePassive>(Node) != nullptr;
			}));

		const int32 RemovedNodes = RemoveFlowNodesWhere(
			FlowAsset,
			FlowGraph,
			[EntryNode, ExistingGrantNode](UFlowNode* Node)
			{
				return Node != EntryNode && Node != ExistingGrantNode;
			});

		UBFNode_GrantSacrificePassive* GrantNode = Cast<UBFNode_GrantSacrificePassive>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node)
			{
				return Cast<UBFNode_GrantSacrificePassive>(Node) != nullptr;
			}));
		if (!GrantNode)
		{
			GrantNode = Cast<UBFNode_GrantSacrificePassive>(CreateFlowNodeAfter(
				FlowGraph,
				EntryNode,
				UBFNode_GrantSacrificePassive::StaticClass(),
				FVector2D(320.f, 0.f)));
		}
		else
		{
			LinkFlowNodes(EntryNode, GrantNode);
		}

		if (!GrantNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Grant Sacrifice Passive node for `%s`."), *Spec.FlowTargetName));
			return;
		}

		GrantNode->Modify();
		GrantNode->Config = Spec.Config;
		if (GrantNode->Config.PassiveType == ESacrificeRunePassiveType::ShadowMark)
		{
			GrantNode->Config.ShadowMarkTag = RequestTag(TEXT("Buff.Status.ShadowMark"), ReportLines);
		}
		RefreshGraphNodePins(GrantNode);
		LinkFlowNodes(EntryNode, GrantNode);

		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: hidden passive grant node `%s`, removed %d template nodes."),
			*Spec.FlowTargetName,
			*Spec.Key,
			RemovedNodes));
	}

	void ApplySacrificeSpec(
		const FSacrificeSpec& Spec,
		int32 RuneId,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const FString TargetPath = SacrificeGeneratedRoot + TEXT("/") + Spec.TargetAssetName;
		const FString FlowPath = SacrificeGeneratedFlowRoot + TEXT("/") + Spec.FlowTargetName;
		ReportLines.Add(FString::Printf(TEXT("## Sacrifice passive `%s`"), *Spec.TargetAssetName));

		RequestTag(FString(TEXT("Rune.Sacrifice.")) + Spec.Key, ReportLines);
		if (Spec.Config.PassiveType == ESacrificeRunePassiveType::ShadowMark)
		{
			RequestTag(TEXT("Buff.Status.ShadowMark"), ReportLines);
		}

		UFlowAsset* FlowAsset = EnsureFlowAssetAtPath(AttackTemplateFlow, FlowPath, bDryRun, ReportLines, DirtyPackages);
		ConfigureSacrificePassiveFlow(FlowAsset, Spec, bDryRun, ReportLines, DirtyPackages);

		URuneDataAsset* RuneDA = Cast<URuneDataAsset>(DuplicateAssetIfMissing(
			AttackTemplateDA,
			TargetPath,
			bDryRun,
			ReportLines,
			DirtyPackages));
		if (bDryRun)
		{
			ReportLines.Add(TEXT("- Dry run: no sacrifice DA fields were changed."));
			return;
		}
		if (!RuneDA)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot update sacrifice DA `%s`: asset not loaded or created."), *Spec.TargetAssetName));
			return;
		}

		UTexture2D* Icon = LoadAssetByPackagePath<UTexture2D>(IconRoot + TEXT("/") + Spec.IconAssetName);
		if (!Icon)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing sacrifice icon `%s/%s`."), *IconRoot, *Spec.IconAssetName));
		}

		FRuneInstance& RuneInfo = RuneDA->RuneInfo;
		RuneInfo.RuneConfig.RuneName = FName(*Spec.DisplayName);
		RuneInfo.RuneConfig.RuneDescription = FText::FromString(Spec.Description);
		RuneInfo.RuneConfig.HUDSummaryText = FText::FromString(Spec.Summary.IsEmpty() ? Spec.Description : Spec.Summary);
		RuneInfo.RuneConfig.RuneIcon = Icon;
		RuneInfo.RuneConfig.RuneID = RuneId;
		RuneInfo.RuneConfig.RuneType = ERuneType::Buff;
		RuneInfo.RuneConfig.TriggerType = ERuneTriggerType::Passive;
		RuneInfo.RuneConfig.GenericEffects.Reset();
		RuneInfo.Shape.Cells.Reset();
		RuneInfo.Flow.FlowAsset = FlowAsset;
		RuneInfo.CombatCard = FCombatCardConfig();

		RuneDA->MarkPackageDirty();
		DirtyPackages.AddUnique(RuneDA->GetPackage());
		ReportLines.Add(TEXT("- Updated sacrifice RuneInfo: empty Shape, passive TriggerType, non-combat-card."));
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
		ConfigureMoonlightSlashWaveFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureStandaloneNiagaraVfxFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		StripApplyAttributeInlineVfx(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureCombatCardAttributeMultiplierFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureSplitBaseFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureSplashBaseFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		URuneCardEffectProfileDA* BaseProfile = ConfigureMoonlightEffectProfile(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureMoonlightProjectileProfileNode(BaseFlow, Spec.BaseFlowTargetName, BaseProfile, bDryRun, ReportLines, DirtyPackages);
		ConfigureMoonlightAreaProfileNode(BaseFlow, Spec.BaseFlowTargetName, BaseProfile, bDryRun, ReportLines, DirtyPackages);

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
		CombatCard.TriggerTiming = Spec.TriggerTiming;
		CombatCard.BaseFlow = BaseFlow;
		CombatCard.LinkRecipes.Reset();
		CombatCard.DefaultLinkOrientation = Spec.DefaultLinkOrientation;
		CombatCard.DisplayName = FText::FromString(Spec.DisplayName);
		CombatCard.HUDReasonText = FText::FromString(Spec.Description);
		CombatCard.bUseComboEffectScaling = false;
		CombatCard.ComboScalarPerIndex = 0.f;
		CombatCard.MaxComboScalar = 0.f;
		if (Spec.CardIdTag == TEXT("Card.ID.AttackUp"))
		{
			CombatCard.bUseComboEffectScaling = true;
			CombatCard.ComboScalarPerIndex = 0.25f;
			CombatCard.MaxComboScalar = 0.5f;
			ReportLines.Add(TEXT("- Configured Combo Scaling: bUse=true, ComboScalarPerIndex=0.25, MaxComboScalar=0.5."));
		}
		for (const FLinkRecipeSpec& LinkSpec : Spec.LinkRecipes)
		{
			const FString TemplateFlow = LinkSpec.Direction == ECombatCardLinkOrientation::Forward
				? MoonlightForwardTemplateFlow
				: MoonlightReversedTemplateFlow;
			const FString FlowName = TEXT("FA_Rune512_Moonlight_") + LinkSpec.Suffix;
			UFlowAsset* LinkFlow = EnsureFlowAsset(TemplateFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightSlashWaveFlow(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightReversedGroundPathFlow(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightBurnHitVfxFlow(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightPoisonAtomicFlow(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			URuneCardEffectProfileDA* LinkProfile = ConfigureMoonlightEffectProfile(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightProjectileProfileNode(LinkFlow, FlowName, LinkProfile, bDryRun, ReportLines, DirtyPackages);
			ConfigureMoonlightAreaProfileNode(LinkFlow, FlowName, LinkProfile, bDryRun, ReportLines, DirtyPackages);

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
	ReportLines.Add(FString::Printf(TEXT("- Generated EffectProfile root: `%s`"), *GeneratedProfileRoot));
	ReportLines.Add(FString::Printf(TEXT("- Generated sacrifice root: `%s`"), *SacrificeGeneratedRoot));
	ReportLines.Add(TEXT(""));

	if (bImportIcons)
	{
		ImportIcons(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
		ImportVfxTextures(bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	ConfigurePoisonGameplayEffectAssets(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));

	int32 RuneId = 51201;
	for (const FCardSpec& Spec : MakeCardSpecs())
	{
		ApplyCardSpec(Spec, RuneId++, bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	for (const FSacrificeSpec& Spec : MakeSacrificeSpecs())
	{
		ApplySacrificeSpec(Spec, RuneId++, bDryRun, ReportLines, DirtyPackages);
		ReportLines.Add(TEXT(""));
	}

	ConfigureProductionWeaponComboAssets(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));

	ReportLines.Add(TEXT("## Generic Rune status card integration"));
	ReportLines.Add(TEXT("- Bleed, Rend, Wound, Knockback, Fear, Freeze, Stun, and Curse are generated as Normal combat cards."));
	ReportLines.Add(TEXT("- These cards reuse Playtest_GA/RuneBaseEffect FA templates. Tune the underlying Generic Rune GA/GE when the status rule itself needs to change."));
	ReportLines.Add(TEXT("- Bloodvine remains design-only; no dedicated 512 card is generated in this pass."));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## FA VFX todos"));
	ReportLines.Add(TEXT("- Card VFX is configured in each BaseFlow/LinkFlow, not on CombatCard data."));
	ReportLines.Add(TEXT("- Moonlight generated slash-wave nodes clear inline Niagara fields and use BP/default projectile visuals; hit/status visuals should be independent atomic VFX nodes."));
	ReportLines.Add(TEXT("- Moonlight base and forward LinkFlows use combo projectiles as sequential same-path shots: Cone=0, Sequential=true, Interval=0.12, MaxBonus=2."));
	ReportLines.Add(TEXT("- Moonlight projectile/area tuning is mirrored into EffectProfile assets; new profile nodes execute the copied profile while legacy nodes are retained for parameter comparison."));
	ReportLines.Add(TEXT("- Moonlight forward poison LinkFlow is configured as atomic nodes: projectile event -> Wait Gameplay Event -> Play Niagara -> ApplyEffect -> Play Niagara -> ApplyGEInRadius."));
	ReportLines.Add(TEXT("- Moonlight forward burn LinkFlow is configured as atomic nodes: projectile event -> Wait Gameplay Event -> Play Niagara attached to enemy socket -> persistent UGE_RuneBurn."));
	ReportLines.Add(TEXT("- Moonlight reversed poison/burn LinkFlows use Spawn Rune Ground Path Effect and disconnect legacy slash-wave execution."));
	ReportLines.Add(TEXT("- Burn/Poison base VFX use compact Play Niagara nodes; stale Flipbook nodes are cleared when found."));
	ReportLines.Add(TEXT("- Projectile inline Niagara fields remain empty. Link/status VFX must live in independent FA visual nodes."));
	ReportLines.Add(TEXT("- Projectile visuals stay on projectile-spawn nodes; hit/status visuals should use separate visual nodes."));
	ReportLines.Add(TEXT("- Splash/Split are one weapon-adaptive card family: both variants share Card.ID.SplashSplit and Card.Effect.SplashSplit; melee uses Splash BaseFlow, ranged uses Split BaseFlow; Moonlight Split only exists as reversed LinkFlow."));
	ReportLines.Add(TEXT("- Any Flow copied from a template must be opened once and checked against the 512 design doc before gameplay signoff."));

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TEXT("512RuneCardBatchReport.md"), ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("512 rune card batch finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
