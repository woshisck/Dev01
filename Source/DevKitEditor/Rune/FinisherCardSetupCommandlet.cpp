#include "DevKitEditor/Rune/FinisherCardSetupCommandlet.h"

#include "AbilitySystem/Abilities/GA_ApplyFinisherMark.h"
#include "AbilitySystem/Abilities/GA_FinisherCharge.h"
#include "AbilitySystem/Abilities/GA_Player_FinisherAttack.h"
#include "Asset/FlowAssetFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"
#include "BuffFlow/Nodes/BFNode_DoDamage.h"
#include "BuffFlow/Nodes/BFNode_FinishBuff.h"
#include "BuffFlow/Nodes/BFNode_GetRuneTuningValue.h"
#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "BuffFlow/Nodes/YogFlowNodes.h"
#include "BuffFlow/YogRuneFlowAsset.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Data/RuneDataAsset.h"
#include "FileHelpers.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagsManager.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace FinisherCardSetup
{
	const FString ReportFileName = TEXT("FinisherCardSetupReport.md");

	const FString BGA_FinisherChargePath = TEXT("/Game/Code/GAS/Abilities/Finisher/BGA_FinisherCharge");
	const FString BGA_PlayerFinisherAttackPath = TEXT("/Game/Code/GAS/Abilities/Finisher/BGA_Player_FinisherAttack");
	const FString BGA_ApplyMarkFinisherPath = TEXT("/Game/Code/GAS/Abilities/Finisher/BGA_ApplyMark_Finisher");
	const FString LegacyBGA_ApplyFinisherMarkPath = TEXT("/Game/Code/GAS/Abilities/Finisher/BGA_ApplyFinisherMark");
	const FString GE_FinisherChargePath = TEXT("/Game/Code/GAS/Abilities/Finisher/GE_FinisherCharge");
	const FString GE_FinisherDamagePath = TEXT("/Game/Code/GAS/Abilities/Finisher/GE_FinisherDamage");
	const FString GE_MarkFinisherPath = TEXT("/Game/Code/GAS/Abilities/Finisher/GE_Mark_Finisher");
	const FString LegacyGE_FinisherMarkPath = TEXT("/Game/Code/GAS/Abilities/Finisher/GE_FinisherMark");
	const FString FinisherMontagePath = TEXT("/Game/Code/GAS/Abilities/Finisher/AM_Player_FinisherAttack");

	const FString SourceFinisherMontagePath = TEXT("/YogAnimSource/ElianAnim/TwoHandedSword/AM_Sword_FinisherAtk_01");
	const FString DamageTemplatePath = TEXT("/Game/Code/GAS/GameplayEffects/GE_Damage_Basic_SetByCaller");

	const FString RunePath = TEXT("/Game/YogRuneEditor/Runes/DA_Rune_Finisher");
	const FString BaseFlowPath = TEXT("/Game/YogRuneEditor/Flows/FA_FinisherCard_BaseEffect");
	const FString ChargeHitFlowPath = TEXT("/Game/YogRuneEditor/Flows/FA_FinisherCard_ChargeHit");
	const FString DetonateFlowPath = TEXT("/Game/YogRuneEditor/Flows/FA_FinisherCard_Detonate");
	const FString LegacyChargeHitFlowPath = TEXT("/Game/YogRuneEditor/Flows/FA_Finisher_ChargeHit");
	const FString LegacyDetonateFlowPath = TEXT("/Game/YogRuneEditor/Flows/FA_Finisher_Detonate");
	const FString AbilitySetPath = TEXT("/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial");

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString PackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &PackageFile);
	}

	template <typename T>
	T* LoadAsset(const FString& PackagePath)
	{
		if (PackagePath.IsEmpty())
		{
			return nullptr;
		}
		if (T* Found = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Found;
		}
		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	template <typename T>
	TSubclassOf<T> LoadBlueprintClass(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		UClass* Class = Cast<UClass>(StaticLoadObject(
			UClass::StaticClass(),
			nullptr,
			*(PackagePath + TEXT(".") + AssetName + TEXT("_C"))));
		return Class && Class->IsChildOf(T::StaticClass()) ? TSubclassOf<T>(Class) : nullptr;
	}

	FGameplayTag RequestTag(const TCHAR* TagName, TArray<FString>& ReportLines)
	{
		const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(TagName), false);
		if (!Tag.IsValid())
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing GameplayTag `%s`."), TagName));
		}
		return Tag;
	}

	void SetTagContainerProperty(UObject* Object, FName PropertyName, TArray<FGameplayTag> Tags, TArray<FString>& ReportLines)
	{
		if (!Object)
		{
			return;
		}

		FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
		if (!Property || Property->Struct != FGameplayTagContainer::StaticStruct())
		{
			ReportLines.Add(FString::Printf(
				TEXT("- `%s` does not expose `%s` as an FGameplayTagContainer."),
				*GetNameSafe(Object),
				*PropertyName.ToString()));
			return;
		}

		FGameplayTagContainer* Container = Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Object);
		Container->Reset();
		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid())
			{
				Container->AddTag(Tag);
			}
		}
	}

	void SetAbilityTriggersProperty(UObject* Object, const FGameplayTag& TriggerTag, TArray<FString>& ReportLines)
	{
		if (!Object)
		{
			return;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), TEXT("AbilityTriggers"));
		FStructProperty* TriggerStructProperty = ArrayProperty ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !TriggerStructProperty)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s` does not expose `AbilityTriggers` as a struct array."), *GetNameSafe(Object)));
			return;
		}

		FStructProperty* TriggerTagProperty = CastField<FStructProperty>(TriggerStructProperty->Struct->FindPropertyByName(TEXT("TriggerTag")));
		FByteProperty* TriggerSourceProperty = CastField<FByteProperty>(TriggerStructProperty->Struct->FindPropertyByName(TEXT("TriggerSource")));
		if (!TriggerTagProperty || TriggerTagProperty->Struct != FGameplayTag::StaticStruct() || !TriggerSourceProperty)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s.AbilityTriggers` is missing `TriggerTag` or `TriggerSource`."), *GetNameSafe(Object)));
			return;
		}

		FScriptArrayHelper TriggerArray(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		TriggerArray.EmptyValues();
		if (!TriggerTag.IsValid())
		{
			return;
		}

		const int32 NewIndex = TriggerArray.AddValue();
		uint8* NewEntryData = TriggerArray.GetRawPtr(NewIndex);
		FGameplayTag* StoredTag = TriggerTagProperty->ContainerPtrToValuePtr<FGameplayTag>(NewEntryData);
		*StoredTag = TriggerTag;
		TriggerSourceProperty->SetPropertyValue_InContainer(NewEntryData, static_cast<uint8>(EGameplayAbilityTriggerSource::GameplayEvent));
	}

	bool AddClassToClassArrayProperty(UObject* Object, FName PropertyName, UClass* ClassToAdd, TArray<FString>& ReportLines)
	{
		if (!Object || !ClassToAdd)
		{
			return false;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		FClassProperty* ClassProperty = ArrayProperty ? CastField<FClassProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !ClassProperty)
		{
			return false;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			if (ClassProperty->GetPropertyValue(ArrayHelper.GetRawPtr(Index)) == ClassToAdd)
			{
				return false;
			}
		}

		const int32 NewIndex = ArrayHelper.AddValue();
		ClassProperty->SetPropertyValue(ArrayHelper.GetRawPtr(NewIndex), ClassToAdd);
		ReportLines.Add(FString::Printf(
			TEXT("- Added `%s` to `%s.%s`."),
			*GetNameSafe(ClassToAdd),
			*GetNameSafe(Object),
			*PropertyName.ToString()));
		return true;
	}

	bool RemoveNullClassEntriesFromClassArrayProperty(UObject* Object, FName PropertyName, TArray<FString>& ReportLines)
	{
		if (!Object)
		{
			return false;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		FClassProperty* ClassProperty = ArrayProperty ? CastField<FClassProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !ClassProperty)
		{
			return false;
		}

		bool bRemovedAny = false;
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = ArrayHelper.Num() - 1; Index >= 0; --Index)
		{
			if (!ClassProperty->GetPropertyValue(ArrayHelper.GetRawPtr(Index)))
			{
				ArrayHelper.RemoveValues(Index);
				bRemovedAny = true;
			}
		}

		if (bRemovedAny)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Removed null legacy entries from `%s.%s`."),
				*GetNameSafe(Object),
				*PropertyName.ToString()));
		}
		return bRemovedAny;
	}

	bool AddAbilityClassToGrantedStructArray(UObject* AbilitySet, UClass* AbilityClass, TArray<FString>& ReportLines)
	{
		if (!AbilitySet || !AbilityClass)
		{
			return false;
		}

		FArrayProperty* GrantedAbilitiesProperty = FindFProperty<FArrayProperty>(AbilitySet->GetClass(), TEXT("GrantedGameplayAbilities"));
		FStructProperty* EntryProperty = GrantedAbilitiesProperty
			? CastField<FStructProperty>(GrantedAbilitiesProperty->Inner)
			: nullptr;
		if (!GrantedAbilitiesProperty || !EntryProperty)
		{
			return false;
		}

		FClassProperty* AbilityProperty = CastField<FClassProperty>(EntryProperty->Struct->FindPropertyByName(TEXT("Ability")));
		FIntProperty* AbilityLevelProperty = CastField<FIntProperty>(EntryProperty->Struct->FindPropertyByName(TEXT("AbilityLevel")));
		if (!AbilityProperty || !AbilityLevelProperty)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- `%s.GrantedGameplayAbilities` is missing `Ability` or `AbilityLevel`."),
				*GetNameSafe(AbilitySet)));
			return false;
		}

		FScriptArrayHelper GrantedAbilities(GrantedAbilitiesProperty, GrantedAbilitiesProperty->ContainerPtrToValuePtr<void>(AbilitySet));
		for (int32 Index = 0; Index < GrantedAbilities.Num(); ++Index)
		{
			uint8* EntryData = GrantedAbilities.GetRawPtr(Index);
			if (AbilityProperty->GetPropertyValue_InContainer(EntryData) == AbilityClass)
			{
				return false;
			}
		}

		const int32 NewIndex = GrantedAbilities.AddValue();
		uint8* NewEntryData = GrantedAbilities.GetRawPtr(NewIndex);
		AbilityProperty->SetPropertyValue_InContainer(NewEntryData, AbilityClass);
		AbilityLevelProperty->SetPropertyValue_InContainer(NewEntryData, 1);
		ReportLines.Add(FString::Printf(TEXT("- Added `%s` to base ability set."), *GetNameSafe(AbilityClass)));
		return true;
	}

	bool AddAbilityClassToAbilitySet(UObject* AbilitySet, UClass* AbilityClass, TArray<FString>& ReportLines)
	{
		if (AddAbilityClassToGrantedStructArray(AbilitySet, AbilityClass, ReportLines))
		{
			return true;
		}

		if (AddClassToClassArrayProperty(AbilitySet, TEXT("AbilityMap"), AbilityClass, ReportLines))
		{
			return true;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- `%s` already has `%s`, or no supported ability list was found."),
			*GetNameSafe(AbilitySet),
			*GetNameSafe(AbilityClass)));
		return false;
	}

	UObject* RenameAssetIfNeeded(
		const FString& SourcePath,
		const FString& TargetPath,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UObject* ExistingTarget = LoadAsset<UObject>(TargetPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *TargetPath));
			return ExistingTarget;
		}

		UObject* SourceAsset = LoadAsset<UObject>(SourcePath);
		if (!SourceAsset)
		{
			return nullptr;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s` to `%s`."), bDryRun ? TEXT("Would rename") : TEXT("Renamed"), *SourcePath, *TargetPath));
		if (bDryRun)
		{
			return SourceAsset;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TArray<FAssetRenameData> RenameData;
		RenameData.Emplace(SourceAsset, FPackageName::GetLongPackagePath(TargetPath), FPackageName::GetLongPackageAssetName(TargetPath));
		if (!AssetTools.RenameAssets(RenameData))
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to rename `%s` to `%s`."), *SourcePath, *TargetPath));
			return nullptr;
		}

		SourceAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(SourceAsset->GetPackage());
		return SourceAsset;
	}

	UObject* DuplicateAssetIfMissing(
		const FString& TemplatePath,
		const FString& TargetPath,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UObject* Existing = LoadAsset<UObject>(TargetPath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *TargetPath));
			return Existing;
		}

		UObject* Template = LoadAsset<UObject>(TemplatePath);
		if (!Template)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing template `%s`; cannot create `%s`."), *TemplatePath, *TargetPath));
			return nullptr;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s` from `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *TargetPath, *TemplatePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UObject* NewAsset = AssetTools.DuplicateAsset(
			FPackageName::GetLongPackageAssetName(TargetPath),
			FPackageName::GetLongPackagePath(TargetPath),
			Template);
		if (NewAsset)
		{
			NewAsset->MarkPackageDirty();
			DirtyPackages.AddUnique(NewAsset->GetPackage());
		}
		return NewAsset;
	}

	UBlueprint* CreateBlueprintAsset(
		const FString& PackagePath,
		UClass* ParentClass,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UBlueprint* Existing = LoadAsset<UBlueprint>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s Blueprint `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UBlueprint::StaticClass(),
			Factory));
		if (Blueprint)
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			FAssetRegistryModule::AssetCreated(Blueprint);
			Blueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(Blueprint->GetPackage());
		}
		return Blueprint;
	}

	UFlowAsset* CreateFlowAsset(
		const FString& PackagePath,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (UFlowAsset* Existing = LoadAsset<UFlowAsset>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s Flow `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UFlowAssetFactory* Factory = NewObject<UFlowAssetFactory>();
		Factory->AssetClass = UYogRuneFlowAsset::StaticClass();
		UFlowAsset* Flow = Cast<UFlowAsset>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UYogRuneFlowAsset::StaticClass(),
			Factory));
		if (Flow)
		{
			FAssetRegistryModule::AssetCreated(Flow);
			Flow->MarkPackageDirty();
			DirtyPackages.AddUnique(Flow->GetPackage());
		}
		return Flow;
	}

	URuneDataAsset* CreateRuneAsset(
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (URuneDataAsset* Existing = LoadAsset<URuneDataAsset>(RunePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *RunePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s Rune `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *RunePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
		Factory->DataAssetClass = URuneDataAsset::StaticClass();
		URuneDataAsset* Rune = Cast<URuneDataAsset>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(RunePath),
			FPackageName::GetLongPackagePath(RunePath),
			URuneDataAsset::StaticClass(),
			Factory));
		if (Rune)
		{
			FAssetRegistryModule::AssetCreated(Rune);
			Rune->MarkPackageDirty();
			DirtyPackages.AddUnique(Rune->GetPackage());
		}
		return Rune;
	}

	UFlowGraphNode* GetGraphNode(UFlowNode* Node)
	{
		return Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
	}

	UEdGraphPin* FindGraphPin(UFlowNode* Node, FName PinName, EEdGraphPinDirection Direction)
	{
		if (UFlowGraphNode* GraphNode = GetGraphNode(Node))
		{
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin && Pin->PinName == PinName && Pin->Direction == Direction)
				{
					return Pin;
				}
			}
		}
		return nullptr;
	}

	void RefreshPins(UFlowNode* Node)
	{
		if (UFlowGraphNode* GraphNode = GetGraphNode(Node))
		{
			GraphNode->ReconstructNode();
		}
	}

	bool LinkExec(UFlowNode* FromNode, FName FromPinName, UFlowNode* ToNode, FName ToPinName = TEXT("In"))
	{
		UEdGraphPin* FromPin = FindGraphPin(FromNode, FromPinName, EGPD_Output);
		UEdGraphPin* ToPin = FindGraphPin(ToNode, ToPinName, EGPD_Input);
		if (!FromPin || !ToPin)
		{
			return false;
		}
		ToPin->BreakAllPinLinks();
		FromPin->MakeLinkTo(ToPin);
		return true;
	}

	bool LinkData(UFlowNode* FromNode, FName FromPinName, UFlowNode* ToNode, FName ToPinName)
	{
		UEdGraphPin* FromPin = FindGraphPin(FromNode, FromPinName, EGPD_Output);
		UEdGraphPin* ToPin = FindGraphPin(ToNode, ToPinName, EGPD_Input);
		if (!FromPin || !ToPin)
		{
			return false;
		}
		ToPin->BreakAllPinLinks();
		FromPin->MakeLinkTo(ToPin);
		return true;
	}

	UFlowNode* CreateNode(UFlowGraph* FlowGraph, UFlowNode* FromNode, UClass* NodeClass, FVector2D Location)
	{
		UEdGraphPin* FromPin = FromNode ? FindGraphPin(FromNode, TEXT("Out"), EGPD_Output) : nullptr;
		UFlowGraphNode* GraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, FromPin, NodeClass, Location, false);
		return GraphNode ? Cast<UFlowNode>(GraphNode->GetFlowNodeBase()) : nullptr;
	}

	void ClearNonEntryNodes(UFlowAsset* Flow)
	{
		UFlowGraph* FlowGraph = Flow ? Cast<UFlowGraph>(Flow->GetGraph()) : nullptr;
		UFlowNode* EntryNode = Flow ? Flow->GetDefaultEntryNode() : nullptr;
		if (!Flow || !FlowGraph || !EntryNode)
		{
			return;
		}

		TArray<TPair<FGuid, UFlowNode*>> NodesToRemove;
		for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
		{
			if (Pair.Value && Pair.Value != EntryNode)
			{
				NodesToRemove.Add(Pair);
			}
		}

		for (const TPair<FGuid, UFlowNode*>& Pair : NodesToRemove)
		{
			if (UFlowGraphNode* GraphNode = GetGraphNode(Pair.Value))
			{
				FlowGraph->GetSchema()->BreakNodeLinks(*GraphNode);
				GraphNode->DestroyNode();
			}
			Flow->UnregisterNode(Pair.Key);
		}
	}

	void FinalizeFlow(UFlowAsset* Flow, TArray<UPackage*>& DirtyPackages)
	{
		if (!Flow)
		{
			return;
		}
		Flow->HarvestNodeConnections();
		Flow->PostEditChange();
		Flow->MarkPackageDirty();
		DirtyPackages.AddUnique(Flow->GetPackage());
		if (UFlowGraph* Graph = Cast<UFlowGraph>(Flow->GetGraph()))
		{
			Graph->Modify();
			Graph->NotifyGraphChanged();
		}
	}

	void ConfigureGrantedTagGE(
		const FString& Path,
		float Duration,
		int32 StackLimit,
		const TCHAR* GrantedTagName,
		EGameplayEffectStackingDurationPolicy DurationRefreshPolicy,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		UBlueprint* BP = CreateBlueprintAsset(Path, UGameplayEffect::StaticClass(), bDryRun, ReportLines, DirtyPackages);
		if (bDryRun || !BP || !BP->GeneratedClass)
		{
			return;
		}

		UGameplayEffect* GE = Cast<UGameplayEffect>(BP->GeneratedClass->GetDefaultObject());
		if (!GE)
		{
			ReportLines.Add(FString::Printf(TEXT("- `%s` CDO is not UGameplayEffect."), *Path));
			return;
		}

		GE->Modify();
		GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		GE->Period = FScalableFloat(0.f);
		GE->StackingType = EGameplayEffectStackingType::AggregateByTarget;
		GE->StackLimitCount = FMath::Max(1, StackLimit);
		GE->StackDurationRefreshPolicy = DurationRefreshPolicy;
		GE->StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
		GE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
		GE->Modifiers.Reset();
		GE->Executions.Reset();

		const FGameplayTag GrantedTag = RequestTag(GrantedTagName, ReportLines);
		if (GrantedTag.IsValid())
		{
			FInheritedTagContainer GrantedTagChanges;
			GrantedTagChanges.Added.AddTag(GrantedTag);
			UTargetTagsGameplayEffectComponent& TargetTagsComponent = GE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
			TargetTagsComponent.SetAndApplyTargetTagChanges(GrantedTagChanges);
		}

		GE->PostEditChange();
		BP->MarkPackageDirty();
		DirtyPackages.AddUnique(BP->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Configured `%s`: duration=%.1fs stackLimit=%d grantedTag=%s."), *Path, Duration, StackLimit, GrantedTagName));
	}

	void ConfigureGameplayEffectAssets(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## GameplayEffect assets"));
		RenameAssetIfNeeded(LegacyGE_FinisherMarkPath, GE_MarkFinisherPath, bDryRun, ReportLines, DirtyPackages);

		ConfigureGrantedTagGE(
			GE_FinisherChargePath,
			8.f,
			5,
			TEXT("Buff.Status.FinisherCharge"),
			EGameplayEffectStackingDurationPolicy::NeverRefresh,
			bDryRun,
			ReportLines,
			DirtyPackages);

		ConfigureGrantedTagGE(
			GE_MarkFinisherPath,
			12.f,
			1,
			TEXT("Buff.Status.Mark.Finisher"),
			EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication,
			bDryRun,
			ReportLines,
			DirtyPackages);

		UObject* DamageGE = DuplicateAssetIfMissing(DamageTemplatePath, GE_FinisherDamagePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && DamageGE)
		{
			DamageGE->MarkPackageDirty();
			DirtyPackages.AddUnique(DamageGE->GetPackage());
			ReportLines.Add(TEXT("- `GE_FinisherDamage` duplicated from project SetByCaller damage template; FA uses `Data.Damage`."));
		}
	}

	void ConfigureAbilityBlueprints(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## GameplayAbility Blueprints"));
		RenameAssetIfNeeded(LegacyBGA_ApplyFinisherMarkPath, BGA_ApplyMarkFinisherPath, bDryRun, ReportLines, DirtyPackages);

		UBlueprint* ChargeBP = CreateBlueprintAsset(BGA_FinisherChargePath, UGA_FinisherCharge::StaticClass(), bDryRun, ReportLines, DirtyPackages);
		UBlueprint* AttackBP = CreateBlueprintAsset(BGA_PlayerFinisherAttackPath, UGA_Player_FinisherAttack::StaticClass(), bDryRun, ReportLines, DirtyPackages);
		UBlueprint* MarkBP = CreateBlueprintAsset(BGA_ApplyMarkFinisherPath, UGA_ApplyFinisherMark::StaticClass(), bDryRun, ReportLines, DirtyPackages);

		if (bDryRun)
		{
			return;
		}

		const FGameplayTag TagCharge = RequestTag(TEXT("PlayerState.AbilityCast.FinisherCharge"), ReportLines);
		const FGameplayTag TagFinisher = RequestTag(TEXT("PlayerState.AbilityCast.Finisher"), ReportLines);
		const FGameplayTag TagFinisherExecuting = RequestTag(TEXT("Buff.Status.FinisherExecuting"), ReportLines);
		const FGameplayTag TagFinisherWindowOpen = RequestTag(TEXT("Buff.Status.FinisherWindowOpen"), ReportLines);
		const FGameplayTag TagDead = RequestTag(TEXT("Buff.Status.Dead"), ReportLines);
		const FGameplayTag TagLight = RequestTag(TEXT("PlayerState.AbilityCast.LightAtk"), ReportLines);
		const FGameplayTag TagHeavy = RequestTag(TEXT("PlayerState.AbilityCast.HeavyAtk"), ReportLines);
		const FGameplayTag TagDash = RequestTag(TEXT("PlayerState.AbilityCast.Dash"), ReportLines);
		const FGameplayTag TagApplyFinisherMark = RequestTag(TEXT("Action.Mark.Apply.Finisher"), ReportLines);

		TSubclassOf<UGameplayEffect> ChargeGEClass = LoadBlueprintClass<UGameplayEffect>(GE_FinisherChargePath);
		TSubclassOf<UGameplayEffect> MarkGEClass = LoadBlueprintClass<UGameplayEffect>(GE_MarkFinisherPath);

		DuplicateAssetIfMissing(SourceFinisherMontagePath, FinisherMontagePath, bDryRun, ReportLines, DirtyPackages);
		UAnimMontage* FinisherMontage = LoadAsset<UAnimMontage>(FinisherMontagePath);

		if (ChargeBP && ChargeBP->GeneratedClass)
		{
			UGA_FinisherCharge* CDO = Cast<UGA_FinisherCharge>(ChargeBP->GeneratedClass->GetDefaultObject());
			if (CDO)
			{
				CDO->Modify();
				SetTagContainerProperty(CDO, TEXT("AbilityTags"), { TagCharge }, ReportLines);
				SetTagContainerProperty(CDO, TEXT("ActivationBlockedTags"), { TagCharge }, ReportLines);
				CDO->MaxCharges = 5;
				CDO->FinisherChargeGEClass = ChargeGEClass;
				ChargeBP->MarkPackageDirty();
				DirtyPackages.AddUnique(ChargeBP->GetPackage());
				ReportLines.Add(TEXT("- Configured `BGA_FinisherCharge`."));
			}
		}

		if (AttackBP && AttackBP->GeneratedClass)
		{
			UGA_Player_FinisherAttack* CDO = Cast<UGA_Player_FinisherAttack>(AttackBP->GeneratedClass->GetDefaultObject());
			if (CDO)
			{
				CDO->Modify();
				SetTagContainerProperty(CDO, TEXT("AbilityTags"), { TagFinisher }, ReportLines);
				SetTagContainerProperty(CDO, TEXT("ActivationOwnedTags"), { TagFinisherExecuting }, ReportLines);
				SetTagContainerProperty(CDO, TEXT("ActivationRequiredTags"), { TagFinisherWindowOpen }, ReportLines);
				SetTagContainerProperty(CDO, TEXT("ActivationBlockedTags"), { TagDead, TagFinisherExecuting }, ReportLines);
				SetTagContainerProperty(CDO, TEXT("CancelAbilitiesWithTag"), { TagLight, TagHeavy, TagDash }, ReportLines);
				CDO->FinisherMontage = FinisherMontage;
				CDO->ConfirmedDamageMultiplier = 2.f;
				AttackBP->MarkPackageDirty();
				DirtyPackages.AddUnique(AttackBP->GetPackage());
				ReportLines.Add(TEXT("- Configured `BGA_Player_FinisherAttack`; CancelAbilitiesWithTag excludes FinisherCharge."));
			}
		}

		if (MarkBP && MarkBP->GeneratedClass)
		{
			UGA_ApplyFinisherMark* CDO = Cast<UGA_ApplyFinisherMark>(MarkBP->GeneratedClass->GetDefaultObject());
			if (CDO)
			{
				CDO->Modify();
				SetAbilityTriggersProperty(CDO, TagApplyFinisherMark, ReportLines);
				CDO->FinisherMarkGEClass = MarkGEClass;
				MarkBP->MarkPackageDirty();
				DirtyPackages.AddUnique(MarkBP->GetPackage());
				ReportLines.Add(TEXT("- Configured `BGA_ApplyMark_Finisher`: trigger Action.Mark.Apply.Finisher -> `GE_Mark_Finisher`."));
			}
		}
	}

	void ConfigureBaseFlow(UFlowAsset* Flow, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun || !Flow)
		{
			ReportLines.Add(TEXT("- Would configure `FA_FinisherCard_BaseEffect`: Apply GE_FinisherCharge x5 -> Activate event -> FinishBuff."));
			return;
		}

		ClearNonEntryNodes(Flow);
		UFlowGraph* Graph = Cast<UFlowGraph>(Flow->GetGraph());
		UFlowNode* Entry = Flow->GetDefaultEntryNode();
		if (!Graph || !Entry)
		{
			ReportLines.Add(TEXT("- Cannot configure base flow: missing graph or entry node."));
			return;
		}

		UBFNode_ApplyEffect* Apply = Cast<UBFNode_ApplyEffect>(CreateNode(Graph, Entry, UYogFlowNode_EffectApplyState::StaticClass(), FVector2D(320.f, 0.f)));
		UBFNode_SendGameplayEvent* Activate = Cast<UBFNode_SendGameplayEvent>(CreateNode(Graph, Apply, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), FVector2D(660.f, 0.f)));
		UBFNode_FinishBuff* Finish = Cast<UBFNode_FinishBuff>(CreateNode(Graph, Activate, UYogFlowNode_LifecycleFinishBuff::StaticClass(), FVector2D(1000.f, 0.f)));
		if (!Apply || !Activate || !Finish)
		{
			ReportLines.Add(TEXT("- Failed to create base flow nodes."));
			return;
		}

		Apply->Effect = LoadBlueprintClass<UGameplayEffect>(GE_FinisherChargePath);
		Apply->Target = EBFTargetSelector::BuffOwner;
		Apply->ApplicationCount = 5;
		Apply->bRemoveEffectOnCleanup = false;
		Activate->EventTag = RequestTag(TEXT("Action.FinisherCharge.Activate"), ReportLines);
		Activate->Target = EBFTargetSelector::BuffOwner;
		Activate->PayloadTarget = EBFTargetSelector::BuffOwner;
		Activate->Instigator = EBFTargetSelector::BuffOwner;

		RefreshPins(Apply);
		RefreshPins(Activate);
		RefreshPins(Finish);
		LinkExec(Entry, TEXT("Out"), Apply);
		LinkExec(Apply, TEXT("Out"), Activate);
		LinkExec(Activate, TEXT("Out"), Finish);
		FinalizeFlow(Flow, DirtyPackages);
		ReportLines.Add(TEXT("- Configured `FA_FinisherCard_BaseEffect`."));
	}

	void ConfigureChargeHitFlow(UFlowAsset* Flow, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun || !Flow)
		{
			ReportLines.Add(TEXT("- Would configure `FA_FinisherCard_ChargeHit`: Wait ChargeConsumed -> tuning KnockbackDistance -> Knockback -> Action.Mark.Apply.Finisher."));
			return;
		}

		ClearNonEntryNodes(Flow);
		UFlowGraph* Graph = Cast<UFlowGraph>(Flow->GetGraph());
		UFlowNode* Entry = Flow->GetDefaultEntryNode();
		if (!Graph || !Entry)
		{
			ReportLines.Add(TEXT("- Cannot configure charge hit flow: missing graph or entry node."));
			return;
		}

		UBFNode_WaitGameplayEvent* Wait = Cast<UBFNode_WaitGameplayEvent>(CreateNode(Graph, Entry, UYogFlowNode_TriggerGameplayEvent::StaticClass(), FVector2D(320.f, 0.f)));
		UBFNode_GetRuneTuningValue* Tuning = Cast<UBFNode_GetRuneTuningValue>(CreateNode(Graph, Wait, UYogFlowNode_RuneTuningValue::StaticClass(), FVector2D(660.f, 0.f)));
		UBFNode_SendGameplayEvent* Knockback = Cast<UBFNode_SendGameplayEvent>(CreateNode(Graph, Tuning, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), FVector2D(1000.f, 0.f)));
		UBFNode_SendGameplayEvent* Mark = Cast<UBFNode_SendGameplayEvent>(CreateNode(Graph, Knockback, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), FVector2D(1340.f, 0.f)));
		if (!Wait || !Tuning || !Knockback || !Mark)
		{
			ReportLines.Add(TEXT("- Failed to create charge hit nodes."));
			return;
		}

		Wait->EventTag = RequestTag(TEXT("Action.FinisherCharge.ChargeConsumed"), ReportLines);
		Wait->Target = EBFTargetSelector::BuffOwner;
		Tuning->Key = TEXT("KnockbackDistance");
		Tuning->DefaultValue = 400.f;
		Knockback->EventTag = RequestTag(TEXT("Action.Knockback"), ReportLines);
		Knockback->Target = EBFTargetSelector::LastDamageTarget;
		Knockback->PayloadTarget = EBFTargetSelector::LastDamageTarget;
		Knockback->Instigator = EBFTargetSelector::DamageCauser;
		Mark->EventTag = RequestTag(TEXT("Action.Mark.Apply.Finisher"), ReportLines);
		Mark->Target = EBFTargetSelector::LastDamageTarget;
		Mark->PayloadTarget = EBFTargetSelector::LastDamageTarget;
		Mark->Instigator = EBFTargetSelector::DamageCauser;

		RefreshPins(Wait);
		RefreshPins(Tuning);
		RefreshPins(Knockback);
		RefreshPins(Mark);
		LinkExec(Entry, TEXT("Out"), Wait);
		LinkExec(Wait, TEXT("Out"), Tuning);
		LinkExec(Tuning, TEXT("Found"), Knockback);
		LinkExec(Tuning, TEXT("NotFound"), Knockback);
		LinkExec(Knockback, TEXT("Out"), Mark);
		LinkData(Tuning, TEXT("Value"), Knockback, TEXT("Magnitude"));
		FinalizeFlow(Flow, DirtyPackages);
		ReportLines.Add(TEXT("- Configured `FA_FinisherCard_ChargeHit`."));
	}

	void ConfigureDetonateFlow(UFlowAsset* Flow, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun || !Flow)
		{
			ReportLines.Add(TEXT("- Would configure `FA_FinisherCard_Detonate`: Wait Action.Mark.Detonate.Finisher -> damage math -> DoDamage -> Slash -> conditional Knockback."));
			return;
		}

		ClearNonEntryNodes(Flow);
		UFlowGraph* Graph = Cast<UFlowGraph>(Flow->GetGraph());
		UFlowNode* Entry = Flow->GetDefaultEntryNode();
		if (!Graph || !Entry)
		{
			ReportLines.Add(TEXT("- Cannot configure detonate flow: missing graph or entry node."));
			return;
		}

		UBFNode_WaitGameplayEvent* Wait = Cast<UBFNode_WaitGameplayEvent>(CreateNode(Graph, Entry, UYogFlowNode_TriggerGameplayEvent::StaticClass(), FVector2D(320.f, 0.f)));
		UBFNode_GetRuneTuningValue* Tuning = Cast<UBFNode_GetRuneTuningValue>(CreateNode(Graph, Wait, UYogFlowNode_RuneTuningValue::StaticClass(), FVector2D(660.f, 0.f)));
		UBFNode_MathFloat* Math = Cast<UBFNode_MathFloat>(CreateNode(Graph, Tuning, UYogFlowNode_MathFloat::StaticClass(), FVector2D(1000.f, 0.f)));
		UBFNode_DoDamage* Damage = Cast<UBFNode_DoDamage>(CreateNode(Graph, Math, UYogFlowNode_EffectDamage::StaticClass(), FVector2D(1340.f, 0.f)));
		UBFNode_SendGameplayEvent* Slash = Cast<UBFNode_SendGameplayEvent>(CreateNode(Graph, Damage, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), FVector2D(1680.f, 0.f)));
		UBFNode_CompareFloat* Compare = Cast<UBFNode_CompareFloat>(CreateNode(Graph, Slash, UYogFlowNode_ConditionCompareFloat::StaticClass(), FVector2D(2020.f, 0.f)));
		UBFNode_SendGameplayEvent* Knockback = Cast<UBFNode_SendGameplayEvent>(CreateNode(Graph, Compare, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), FVector2D(2360.f, -100.f)));
		if (!Wait || !Tuning || !Math || !Damage || !Slash || !Compare || !Knockback)
		{
			ReportLines.Add(TEXT("- Failed to create detonate nodes."));
			return;
		}

		Wait->EventTag = RequestTag(TEXT("Action.Mark.Detonate.Finisher"), ReportLines);
		Wait->Target = EBFTargetSelector::BuffOwner;
		Tuning->Key = TEXT("DetonationDamage");
		Tuning->DefaultValue = 80.f;
		Math->Operator = EBFMathOp::Multiply;
		Damage->DamageEffect = LoadBlueprintClass<UGameplayEffect>(GE_FinisherDamagePath);
		Damage->TargetSelector = EBFTargetSelector::LastDamageTarget;
		Damage->DamageMultiplier.Value = 1.f;
		Slash->EventTag = RequestTag(TEXT("Action.Slash"), ReportLines);
		Slash->Target = EBFTargetSelector::LastDamageTarget;
		Slash->PayloadTarget = EBFTargetSelector::LastDamageTarget;
		Slash->Instigator = EBFTargetSelector::DamageCauser;
		Compare->Operator = EBFCompareOp::GreaterOrEqual;
		Compare->B = FFlowDataPinInputProperty_Float(2.f);
		Knockback->EventTag = RequestTag(TEXT("Action.Knockback"), ReportLines);
		Knockback->Target = EBFTargetSelector::LastDamageTarget;
		Knockback->PayloadTarget = EBFTargetSelector::LastDamageTarget;
		Knockback->Instigator = EBFTargetSelector::DamageCauser;

		RefreshPins(Wait);
		RefreshPins(Tuning);
		RefreshPins(Math);
		RefreshPins(Damage);
		RefreshPins(Slash);
		RefreshPins(Compare);
		RefreshPins(Knockback);
		LinkExec(Entry, TEXT("Out"), Wait);
		LinkExec(Wait, TEXT("Out"), Tuning);
		LinkExec(Tuning, TEXT("Found"), Math);
		LinkExec(Tuning, TEXT("NotFound"), Math);
		LinkExec(Math, TEXT("Out"), Damage);
		LinkExec(Damage, TEXT("Out"), Slash);
		LinkExec(Slash, TEXT("Out"), Compare);
		LinkExec(Compare, TEXT("True"), Knockback);
		LinkData(Tuning, TEXT("Value"), Math, TEXT("A"));
		LinkData(Wait, TEXT("EventMagnitude"), Math, TEXT("B"));
		LinkData(Math, TEXT("Result"), Damage, TEXT("FlatDamage"));
		LinkData(Wait, TEXT("EventMagnitude"), Compare, TEXT("A"));
		FinalizeFlow(Flow, DirtyPackages);
		ReportLines.Add(TEXT("- Configured `FA_FinisherCard_Detonate`."));
	}

	void ConfigureRuneAndFlows(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Rune and Flow assets"));
		RenameAssetIfNeeded(LegacyChargeHitFlowPath, ChargeHitFlowPath, bDryRun, ReportLines, DirtyPackages);
		RenameAssetIfNeeded(LegacyDetonateFlowPath, DetonateFlowPath, bDryRun, ReportLines, DirtyPackages);

		UFlowAsset* BaseFlow = CreateFlowAsset(BaseFlowPath, bDryRun, ReportLines, DirtyPackages);
		UFlowAsset* ChargeHitFlow = CreateFlowAsset(ChargeHitFlowPath, bDryRun, ReportLines, DirtyPackages);
		UFlowAsset* DetonateFlow = CreateFlowAsset(DetonateFlowPath, bDryRun, ReportLines, DirtyPackages);

		ConfigureBaseFlow(BaseFlow, bDryRun, ReportLines, DirtyPackages);
		ConfigureChargeHitFlow(ChargeHitFlow, bDryRun, ReportLines, DirtyPackages);
		ConfigureDetonateFlow(DetonateFlow, bDryRun, ReportLines, DirtyPackages);

		URuneDataAsset* Rune = CreateRuneAsset(bDryRun, ReportLines, DirtyPackages);
		if (bDryRun || !Rune)
		{
			return;
		}

		Rune->Modify();
		FRuneInstance& RuneInfo = Rune->RuneInfo;
		RuneInfo.RuneConfig.RuneName = TEXT("Finisher");
		RuneInfo.RuneConfig.RuneDescription = FText::FromString(TEXT("Finisher card: opens a charge window, marks targets, then detonates marked enemies."));
		RuneInfo.RuneConfig.HUDSummaryText = FText::FromString(TEXT("Charge hits mark enemies; the finisher detonates marks."));
		RuneInfo.RuneConfig.RuneType = ERuneType::Buff;
		RuneInfo.RuneConfig.TriggerType = ERuneTriggerType::OnAttackHit;
		RuneInfo.RuneConfig.TuningScalars.Reset();

		FRuneTuningScalar DetonationDamage;
		DetonationDamage.Key = TEXT("DetonationDamage");
		DetonationDamage.DisplayName = FText::FromString(TEXT("Detonation Damage"));
		DetonationDamage.Category = TEXT("Finisher");
		DetonationDamage.ValueSource = ERuneTuningValueSource::Literal;
		DetonationDamage.Value = 80.f;
		DetonationDamage.MinValue = 0.f;
		DetonationDamage.UnitText = FText::FromString(TEXT("damage"));
		DetonationDamage.Description = FText::FromString(TEXT("Base damage per detonated finisher mark."));
		RuneInfo.RuneConfig.TuningScalars.Add(DetonationDamage);

		FRuneTuningScalar KnockbackDistance;
		KnockbackDistance.Key = TEXT("KnockbackDistance");
		KnockbackDistance.DisplayName = FText::FromString(TEXT("Knockback Distance"));
		KnockbackDistance.Category = TEXT("Finisher");
		KnockbackDistance.ValueSource = ERuneTuningValueSource::Literal;
		KnockbackDistance.Value = 400.f;
		KnockbackDistance.MinValue = 0.f;
		KnockbackDistance.UnitText = FText::FromString(TEXT("cm"));
		KnockbackDistance.Description = FText::FromString(TEXT("Knockback distance sent with charge hit events."));
		RuneInfo.RuneConfig.TuningScalars.Add(KnockbackDistance);

		RuneInfo.Flow.FlowAsset = BaseFlow;
		FCombatCardConfig& CombatCard = RuneInfo.CombatCard;
		CombatCard.bIsCombatCard = true;
		CombatCard.CardType = ECombatCardType::Finisher;
		CombatCard.RequiredAction = ECardRequiredAction::Any;
		CombatCard.TriggerTiming = ECombatCardTriggerTiming::OnHit;
		CombatCard.BaseFlow = BaseFlow;
		CombatCard.DisplayName = FText::FromString(TEXT("Finisher"));
		CombatCard.HUDSummaryText = FText::FromString(TEXT("Opens a finisher charge window."));
		CombatCard.HUDReasonText = FText::FromString(TEXT("Charge, mark, detonate."));

		Rune->MarkPackageDirty();
		DirtyPackages.AddUnique(Rune->GetPackage());
		ReportLines.Add(TEXT("- Configured `DA_Rune_Finisher` tuning rows and CombatCard BaseFlow."));
	}

	void ConfigureAbilitySet(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(TEXT("## Ability set"));
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would add finisher abilities to `%s`."), *AbilitySetPath));
			return;
		}

		UObject* AbilitySet = LoadAsset<UObject>(AbilitySetPath);
		if (!AbilitySet)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing `%s`; ability pre-grant skipped."), *AbilitySetPath));
			return;
		}

		TArray<TSubclassOf<UYogGameplayAbility>> AbilityClasses;
		AbilityClasses.Add(LoadBlueprintClass<UYogGameplayAbility>(BGA_FinisherChargePath));
		AbilityClasses.Add(LoadBlueprintClass<UYogGameplayAbility>(BGA_PlayerFinisherAttackPath));
		AbilityClasses.Add(LoadBlueprintClass<UYogGameplayAbility>(BGA_ApplyMarkFinisherPath));

		AbilitySet->Modify();
		RemoveNullClassEntriesFromClassArrayProperty(AbilitySet, TEXT("AbilityMap"), ReportLines);
		for (const TSubclassOf<UYogGameplayAbility>& AbilityClass : AbilityClasses)
		{
			AddAbilityClassToAbilitySet(AbilitySet, AbilityClass.Get(), ReportLines);
		}

		AbilitySet->MarkPackageDirty();
		DirtyPackages.AddUnique(AbilitySet->GetPackage());
	}
}

UFinisherCardSetupCommandlet::UFinisherCardSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UFinisherCardSetupCommandlet::Main(const FString& Params)
{
	using namespace FinisherCardSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Finisher Card Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(TEXT(""));

	ConfigureGameplayEffectAssets(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));
	ConfigureAbilityBlueprints(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));
	ConfigureRuneAndFlows(bDryRun, ReportLines, DirtyPackages);
	ReportLines.Add(TEXT(""));
	ConfigureAbilitySet(bDryRun, ReportLines, DirtyPackages);

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath);
	UE_LOG(LogTemp, Display, TEXT("Finisher card setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
