#include "DevKitEditor/Rune/RuneCardBatchGeneratorCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AbilitySystem/Execution/GEExec_PoisonDamage.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_ApplyGEInRadius.h"
#include "BuffFlow/Nodes/BFNode_PlayFlipbookVFX.h"
#include "BuffFlow/Nodes/BFNode_PlayNiagara.h"
#include "BuffFlow/Nodes/BFNode_SpawnSlashWaveProjectile.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "Data/GameplayAbilityComboGraph.h"
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
#include "NiagaraSystem.h"
#include "Nodes/FlowNode.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace Rune512Batch
{
	const FString GeneratedRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated");
	const FString GeneratedFlowRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/Flow");
	const FString IconRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons");
	const FString VfxTextureRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures");
	const FString VfxMaterialRoot = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials");
	const FString FlipbookMaterialPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Materials/M_Rune512_FlipbookSprite");
	const FString PoisonHitTexturePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures/T_Rune512_VFX_Poison_Hit");
	const FString PoisonSpreadTexturePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures/T_Rune512_VFX_Poison_Spread");
	const FString BurnHitTexturePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures/T_Rune512_VFX_Burn_Hit");
	const FString MoonlightPoisonHitEventTag = TEXT("Action.Rune.MoonlightPoisonHit");
	const FString MoonlightPoisonExpireEventTag = TEXT("Action.Rune.MoonlightPoisonExpired");
	const FString PoisonEffectPath = TEXT("/Game/Code/GAS/Abilities/Shared/GE_Poison");
	const FString PoisonSplashEffectPath = TEXT("/Game/Docs/BuffDocs/Playtest_GA/DeathPoison/GE_PoisonSplash");
	const FString AttackTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_AttackUp_01");
	const FString MoonlightTemplateDA = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/DA_Rune_MoonBlade_01");
	const FString MoonlightBaseTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Base");
	const FString MoonlightForwardTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Forward");
	const FString MoonlightReversedTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Moonlight_Backward");
	const FString AttackTemplateFlow = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/GenericRune/FA_Rune_AttackUp_01");
	const FString ProductionTHSwordWeapon = TEXT("/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword");
	const FString ProductionRedSwordWeapon = TEXT("/Game/Code/Weapon/GreatSword/DA_WPN_RedSword");
	const FString THSwordComboGraph = TEXT("/Game/Docs/Combat/TwoHandedSword/CG_THSword_Test");

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

			if (LoadAssetByPackagePath<UTexture2D>(TexturePackagePath))
			{
				ReportLines.Add(FString::Printf(TEXT("- Found VFX texture `%s`."), *TexturePackagePath));
				continue;
			}

			ReportLines.Add(FString::Printf(
				TEXT("- %s `%s` -> `%s`."),
				bDryRun ? TEXT("Would import") : TEXT("Imported"),
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
			{ TEXT("补毒液路径碰撞体与 3 秒爆发 Execution；命中表现使用小尺寸 Flipbook VFX。") }));

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
			TEXT("Forward/Reversed 配方 Flow 已按模板复制，复杂节点连接与 Flipbook VFX 参数需要按配置文档检查。")
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

		const TCHAR* CleanVfxPolicy = TEXT("legacy Niagara cleared; projectile uses BP/default visuals; hit/status uses atomic Flipbook VFX nodes");

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
				SlashNode->ProjectileConeAngleDegrees = 15.f;
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
				SlashNode->MaxSplitGenerations = 1;
				SlashNode->SplitProjectileCount = 5;
				SlashNode->SplitConeAngleDegrees = 70.f;
				SlashNode->SplitDamageMultiplier = 0.5f;
				SlashNode->SplitSpeedMultiplier = 1.6f;
				SlashNode->SplitMaxDistanceMultiplier = 0.55f;
				SlashNode->SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);
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
			if (bIsPoisonLinkProfile)
			{
				ReportLines.Add(FString::Printf(
					TEXT("- Configured `%s`: Moonlight slash-wave nodes=%d, %s, HitEvent=`%s`."),
					*FlowName,
					UpdatedNodeCount,
					CleanVfxPolicy,
					*MoonlightPoisonHitEventTag));
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

	void ConfigureMoonlightPoisonAtomicFlow(
		UFlowAsset* FlowAsset,
		const FString& FlowName,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		const EMoonlightFlowProfile Profile = ResolveMoonlightFlowProfile(FlowName);
		if (Profile != EMoonlightFlowProfile::ForwardPoison
			&& Profile != EMoonlightFlowProfile::ReversedPoison)
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

		UTexture2D* PoisonHitTexture = LoadAssetByPackagePath<UTexture2D>(PoisonHitTexturePath);
		UTexture2D* PoisonSpreadTexture = LoadAssetByPackagePath<UTexture2D>(PoisonSpreadTexturePath);
		UMaterialInterface* FlipbookMaterial = LoadAssetByPackagePath<UMaterialInterface>(FlipbookMaterialPath);
		if (!PoisonEffect)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing poison GE class `%s`; ApplyEffect node still created but Effect is empty."), *PoisonEffectPath));
		}
		if (!PoisonHitTexture || !PoisonSpreadTexture || !FlipbookMaterial)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing poison flipbook dependencies for `%s`: HitTexture=%s SpreadTexture=%s Material=%s."),
				*FlowName,
				*GetNameSafe(PoisonHitTexture),
				*GetNameSafe(PoisonSpreadTexture),
				*GetNameSafe(FlipbookMaterial)));
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

		UBFNode_PlayFlipbookVFX* HitVfxNode = Cast<UBFNode_PlayFlipbookVFX>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node)
			{
				UBFNode_PlayFlipbookVFX* PlayNode = Cast<UBFNode_PlayFlipbookVFX>(Node);
				return PlayNode && PlayNode->EffectName == FName(TEXT("Rune.Moonlight.PoisonHitVFX"));
			}));
		if (!HitVfxNode)
		{
			HitVfxNode = Cast<UBFNode_PlayFlipbookVFX>(CreateFlowNodeAfter(
				FlowGraph,
				WaitNode,
				UBFNode_PlayFlipbookVFX::StaticClass(),
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

		UBFNode_PlayFlipbookVFX* SpreadVfxNode = Cast<UBFNode_PlayFlipbookVFX>(FindFirstNode(
			FlowAsset,
			[](UFlowNode* Node)
			{
				UBFNode_PlayFlipbookVFX* PlayNode = Cast<UBFNode_PlayFlipbookVFX>(Node);
				return PlayNode && PlayNode->EffectName == FName(TEXT("Rune.Moonlight.PoisonSpreadVFX"));
			}));
		if (!SpreadVfxNode)
		{
			SpreadVfxNode = Cast<UBFNode_PlayFlipbookVFX>(CreateFlowNodeAfter(
				FlowGraph,
				PrimaryPoisonNode,
				UBFNode_PlayFlipbookVFX::StaticClass(),
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

		HitVfxNode->Modify();
		HitVfxNode->Texture = PoisonHitTexture;
		HitVfxNode->Material = FlipbookMaterial;
		HitVfxNode->Rows = 4;
		HitVfxNode->Columns = 4;
		HitVfxNode->Duration = 0.38f;
		HitVfxNode->Size = 72.f;
		HitVfxNode->Target = EBFTargetSelector::LastDamageTarget;
		HitVfxNode->Socket = TEXT("spine_02");
		HitVfxNode->SocketFallbackNames = { TEXT("spine_03"), TEXT("spine_01"), TEXT("pelvis"), TEXT("root") };
		HitVfxNode->Offset = FVector(0.f, 0.f, 8.f);
		HitVfxNode->bFaceCamera = true;
		HitVfxNode->bDestroyWithFlow = false;
		HitVfxNode->EmissiveColor = FLinearColor(0.55f, 1.0f, 0.35f, 1.f);
		HitVfxNode->AlphaScale = 1.f;
		HitVfxNode->EffectName = TEXT("Rune.Moonlight.PoisonHitVFX");

		PrimaryPoisonNode->Modify();
		PrimaryPoisonNode->Effect = PoisonEffect;
		PrimaryPoisonNode->Target = EBFTargetSelector::LastDamageTarget;
		PrimaryPoisonNode->ApplicationCount = 3;

		SpreadVfxNode->Modify();
		SpreadVfxNode->Texture = PoisonSpreadTexture;
		SpreadVfxNode->Material = FlipbookMaterial;
		SpreadVfxNode->Rows = 4;
		SpreadVfxNode->Columns = 4;
		SpreadVfxNode->Duration = 0.45f;
		SpreadVfxNode->Size = 180.f;
		SpreadVfxNode->Target = EBFTargetSelector::LastDamageTarget;
		SpreadVfxNode->Socket = NAME_None;
		SpreadVfxNode->SocketFallbackNames.Reset();
		SpreadVfxNode->Offset = FVector(0.f, 0.f, 18.f);
		SpreadVfxNode->bFaceCamera = true;
		SpreadVfxNode->bDestroyWithFlow = false;
		SpreadVfxNode->EmissiveColor = FLinearColor(0.55f, 1.0f, 0.35f, 1.f);
		SpreadVfxNode->AlphaScale = 0.85f;
		SpreadVfxNode->EffectName = TEXT("Rune.Moonlight.PoisonSpreadVFX");

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

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		FlowGraph->Modify();
		FlowGraph->NotifyGraphChanged();

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: SpawnSlashWave -> Wait `%s` -> PlayFlipbook hit -> Apply GE_Poison x3 -> PlayFlipbook spread -> ApplyGEInRadius radius=300 max=3."),
			*FlowName,
			*MoonlightPoisonHitEventTag));
	}

	void ConfigureStandaloneFlipbookVfxFlow(
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
			ReportLines.Add(FString::Printf(TEXT("- Would configure standalone Play Flipbook VFX node in `%s`."), *FlowName));
			return;
		}

		if (!FlowAsset)
		{
			ReportLines.Add(FString::Printf(TEXT("- Cannot configure standalone Flipbook VFX `%s`: asset not loaded."), *FlowName));
			return;
		}

		const FString TexturePath = bIsBurnFlow ? BurnHitTexturePath : PoisonHitTexturePath;
		UTexture2D* Texture = LoadAssetByPackagePath<UTexture2D>(TexturePath);
		UMaterialInterface* FlipbookMaterial = LoadAssetByPackagePath<UMaterialInterface>(FlipbookMaterialPath);
		if (!Texture || !FlipbookMaterial)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Missing standalone Flipbook VFX asset for `%s`: Texture=`%s` Loaded=%d Material=`%s` Loaded=%d."),
				*FlowName,
				*TexturePath,
				Texture ? 1 : 0,
				*FlipbookMaterialPath,
				FlipbookMaterial ? 1 : 0));
			return;
		}

		const FName EffectName = bIsBurnFlow ? TEXT("Rune.Burn.ApplyFlipbookVFX") : TEXT("Rune.Poison.ApplyFlipbookVFX");
		int32 ClearedLegacyNiagaraCount = 0;
		UBFNode_PlayFlipbookVFX* ExistingVfxNode = nullptr;
		for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
		{
			if (UBFNode_PlayNiagara* PlayNode = Cast<UBFNode_PlayNiagara>(Pair.Value))
			{
				PlayNode->Modify();
				PlayNode->NiagaraSystem = nullptr;
				PlayNode->EffectName = NAME_None;
				PlayNode->AttachSocketName = NAME_None;
				PlayNode->AttachSocketFallbackNames.Reset();
				PlayNode->AttachTarget = EBFTargetSelector::BuffOwner;
				PlayNode->bAttachToTarget = false;
				PlayNode->LocationOffset = FVector::ZeroVector;
				PlayNode->RotationOffset = FRotator::ZeroRotator;
				PlayNode->Scale = FVector(1.f, 1.f, 1.f);
				PlayNode->bDestroyWithFlow = false;
				++ClearedLegacyNiagaraCount;
			}
			if (UBFNode_PlayFlipbookVFX* FlipbookNode = Cast<UBFNode_PlayFlipbookVFX>(Pair.Value))
			{
				if (FlipbookNode->EffectName == EffectName)
				{
					ExistingVfxNode = FlipbookNode;
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

		UBFNode_PlayFlipbookVFX* VfxNode = ExistingVfxNode;
		const FConnectedPin EntryConnection = EntryNode->GetConnection(TEXT("Out"));
		const bool bEntryAlreadyTargetsVfx = VfxNode && EntryConnection.NodeGuid == VfxNode->GetGuid();
		bool bInsertedNode = false;
		if (!bEntryAlreadyTargetsVfx)
		{
			const FVector2D NodeLocation(
				static_cast<float>(EntryGraphNode->NodePosX + 320),
				static_cast<float>(EntryGraphNode->NodePosY));
			UFlowGraphNode* NewGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
				FlowGraph,
				EntryOutputPin,
				UBFNode_PlayFlipbookVFX::StaticClass(),
				NodeLocation,
				false);
			VfxNode = NewGraphNode ? Cast<UBFNode_PlayFlipbookVFX>(NewGraphNode->GetFlowNodeBase()) : nullptr;
			bInsertedNode = VfxNode != nullptr;
		}

		if (!VfxNode)
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to create Play Flipbook VFX node for `%s`."), *FlowName));
			return;
		}

		VfxNode->Modify();
		VfxNode->Texture = Texture;
		VfxNode->Material = FlipbookMaterial;
		VfxNode->PlaneMesh = nullptr;
		VfxNode->Rows = 4;
		VfxNode->Columns = 4;
		VfxNode->Duration = bIsBurnFlow ? 0.42f : 0.38f;
		VfxNode->Size = bIsBurnFlow ? 78.f : 72.f;
		VfxNode->Target = EBFTargetSelector::LastDamageTarget;
		VfxNode->Socket = bIsBurnFlow ? FName(TEXT("spine_03")) : FName(TEXT("spine_02"));
		VfxNode->SocketFallbackNames.Reset();
		VfxNode->SocketFallbackNames.Add(TEXT("spine_03"));
		VfxNode->SocketFallbackNames.Add(TEXT("spine_02"));
		VfxNode->SocketFallbackNames.Add(TEXT("spine_01"));
		VfxNode->SocketFallbackNames.Add(TEXT("spine"));
		VfxNode->SocketFallbackNames.Add(TEXT("pelvis"));
		VfxNode->SocketFallbackNames.Add(TEXT("body"));
		VfxNode->SocketFallbackNames.Add(TEXT("root"));
		VfxNode->Offset = bIsBurnFlow ? FVector(0.f, 0.f, 6.f) : FVector(0.f, 0.f, 8.f);
		VfxNode->bFaceCamera = true;
		VfxNode->bDestroyWithFlow = false;
		VfxNode->EmissiveColor = bIsBurnFlow
			? FLinearColor(1.f, 0.36f, 0.1f, 1.f)
			: FLinearColor(0.45f, 1.f, 0.25f, 1.f);
		VfxNode->AlphaScale = 0.95f;
		VfxNode->EffectName = EffectName;

		FlowAsset->HarvestNodeConnections();
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(FlowAsset->GetPackage());
		if (FlowGraph)
		{
			FlowGraph->Modify();
			FlowGraph->NotifyGraphChanged();
		}

		ReportLines.Add(FString::Printf(
			TEXT("- Configured `%s`: Play Flipbook VFX node `%s` -> `%s`, Socket=%s, Offset=%s, Size=%.1f, cleared legacy Niagara nodes=%d (%s)."),
			*FlowName,
			*EffectName.ToString(),
			*TexturePath,
			*VfxNode->Socket.ToString(),
			*VfxNode->Offset.ToString(),
			VfxNode->Size,
			ClearedLegacyNiagaraCount,
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
		ConfigureStandaloneFlipbookVfxFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		StripApplyAttributeInlineVfx(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);
		ConfigureCombatCardAttributeMultiplierFlow(BaseFlow, Spec.BaseFlowTargetName, bDryRun, ReportLines, DirtyPackages);

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
		CombatCard.TriggerTiming = ECombatCardTriggerTiming::OnHit;
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
			ConfigureMoonlightPoisonAtomicFlow(LinkFlow, FlowName, bDryRun, ReportLines, DirtyPackages);

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

	ConfigureProductionWeaponComboAssets(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));

	ReportLines.Add(TEXT("## Skipped design-only items"));
	ReportLines.Add(TEXT("- Bleed(X), Rend(X), Bloodvine(X): documented only; no assets generated in this pass."));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## FA VFX todos"));
	ReportLines.Add(TEXT("- Card VFX is configured in each BaseFlow/LinkFlow, not on CombatCard data."));
	ReportLines.Add(TEXT("- Moonlight generated slash-wave nodes clear legacy Niagara fields and use BP/default projectile visuals; hit/status visuals should be atomic Flipbook nodes."));
	ReportLines.Add(TEXT("- Moonlight poison LinkFlow is configured as atomic nodes: projectile event -> Wait Gameplay Event -> Play Flipbook VFX -> ApplyEffect -> ApplyGEInRadius."));
	ReportLines.Add(TEXT("- Burn/Poison base VFX use small Play Flipbook VFX nodes; legacy Play Niagara nodes are cleared when found."));
	ReportLines.Add(TEXT("- Projectile visuals stay on projectile-spawn nodes; hit/status visuals should use separate visual nodes."));
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
