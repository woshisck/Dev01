#include "UI/GamepadInputSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/YogPlayerControllerBase.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UObject/Package.h"

namespace
{
	const FString ActionRootPath = TEXT("/Game/Code/Core/Input/Actions");
	const FString MappingContextPath = TEXT("/Game/Code/Core/Input/IMC_YogPlayerBase");
	const FString ControllerClassPath = TEXT("/Game/Code/Core/Controller/B_YogPlayerControllerBase.B_YogPlayerControllerBase_C");
	const FString InputActionDecoratorPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator");
	const FString InputActionDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator.BP_InputActionDecorator_C");
	const FString ReportFileName = TEXT("GamepadInputSetupReport.md");

	FString ToObjectPath(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		return PackagePath + TEXT(".") + AssetName;
	}

	UObject* LoadObjectByPackagePath(const FString& PackagePath, UClass* ExpectedClass)
	{
		return StaticLoadObject(ExpectedClass, nullptr, *ToObjectPath(PackagePath));
	}

	UInputAction* LoadOrCreateInputAction(const FString& AssetName, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		const FString PackagePath = ActionRootPath / AssetName;
		if (UInputAction* Existing = Cast<UInputAction>(LoadObjectByPackagePath(PackagePath, UInputAction::StaticClass())))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found input action `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s input action `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		UInputAction* NewAction = NewObject<UInputAction>(
			Package,
			UInputAction::StaticClass(),
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!NewAction)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(NewAction);
		NewAction->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		return NewAction;
	}

	bool HasMapping(const UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		if (!Context || !Action)
		{
			return false;
		}

		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key == Key)
			{
				return true;
			}
		}
		return false;
	}

	void EnsureMappings(
		UInputMappingContext* Context,
		UInputAction* Action,
		const FString& ActionName,
		const TArray<FKey>& DesiredKeys,
		const TArray<FKey>& RemovedKeys,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		if (!Context || !Action)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing action `%s`; mapping skipped."), *ActionName));
			return;
		}

		bool bChanged = false;
		if (!bDryRun)
		{
			Context->Modify();
		}

		for (const FKey& Key : RemovedKeys)
		{
			if (HasMapping(Context, Action, Key))
			{
				if (!bDryRun)
				{
					Context->UnmapKey(Action, Key);
				}
				bChanged = true;
			}
		}

		for (const FKey& Key : DesiredKeys)
		{
			if (!HasMapping(Context, Action, Key))
			{
				if (!bDryRun)
				{
					Context->MapKey(Action, Key);
				}
				bChanged = true;
			}
		}

		if (bChanged && !bDryRun)
		{
			Context->MarkPackageDirty();
			DirtyPackages.AddUnique(Context->GetPackage());
		}

		TArray<FString> KeyNames;
		for (const FKey& Key : DesiredKeys)
		{
			KeyNames.Add(Key.ToString());
		}
		ReportLines.Add(FString::Printf(
			TEXT("- `%s` maps to %s%s."),
			*ActionName,
			*FString::Join(KeyNames, TEXT(", ")),
			bChanged ? TEXT(" (updated)") : TEXT("")));
	}

	void AssignControllerDefaults(
		UInputMappingContext* MappingContext,
		UInputAction* IA_Interact,
		UInputAction* IA_Dash,
		UInputAction* IA_Esc,
		UInputAction* IA_OpenBackpack,
		UInputAction* IA_Reload,
		UInputAction* IA_UseCombatItem,
		UInputAction* IA_SwitchCombatItemNext,
		UInputAction* IA_SwitchCombatItemPrevious,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
	{
		UClass* ControllerClass = LoadClass<AYogPlayerControllerBase>(nullptr, *ControllerClassPath);
		AYogPlayerControllerBase* ControllerCDO = ControllerClass ? Cast<AYogPlayerControllerBase>(ControllerClass->GetDefaultObject()) : nullptr;
		if (!ControllerCDO)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing controller class `%s`; defaults were not assigned."), *ControllerClassPath));
			return;
		}

		bool bChanged = false;
		auto AssignContext = [&](TObjectPtr<UInputMappingContext>& Target, UInputMappingContext* Value, const TCHAR* Label)
		{
			if (Value && Target != Value)
			{
				if (!bDryRun)
				{
					Target = Value;
				}
				bChanged = true;
				ReportLines.Add(FString::Printf(TEXT("- Controller default `%s` -> `%s`."), Label, *Value->GetPathName()));
			}
		};
		auto AssignAction = [&](TObjectPtr<UInputAction>& Target, UInputAction* Value, const TCHAR* Label)
		{
			if (Value && Target != Value)
			{
				if (!bDryRun)
				{
					Target = Value;
				}
				bChanged = true;
				ReportLines.Add(FString::Printf(TEXT("- Controller default `%s` -> `%s`."), Label, *Value->GetPathName()));
			}
		};

		if (!bDryRun)
		{
			ControllerCDO->Modify();
		}
		AssignContext(ControllerCDO->DefaultMappingContext, MappingContext, TEXT("DefaultMappingContext"));
		AssignAction(ControllerCDO->Input_Interact, IA_Interact, TEXT("Input_Interact"));
		AssignAction(ControllerCDO->Input_Dash, IA_Dash, TEXT("Input_Dash"));
		AssignAction(ControllerCDO->Input_PauseAction, IA_Esc, TEXT("Input_PauseAction"));
		AssignAction(ControllerCDO->Input_OpenBackpack, IA_OpenBackpack, TEXT("Input_OpenBackpack"));
		AssignAction(ControllerCDO->Input_Reload, IA_Reload, TEXT("Input_Reload"));
		AssignAction(ControllerCDO->Input_UseCombatItem, IA_UseCombatItem, TEXT("Input_UseCombatItem"));
		AssignAction(ControllerCDO->Input_SwitchCombatItem, IA_SwitchCombatItemNext, TEXT("Input_SwitchCombatItem"));
		AssignAction(ControllerCDO->Input_SwitchCombatItemPrevious, IA_SwitchCombatItemPrevious, TEXT("Input_SwitchCombatItemPrevious"));

		if (bChanged && !bDryRun)
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(ControllerClass->ClassGeneratedBy))
			{
				Blueprint->MarkPackageDirty();
				DirtyPackages.AddUnique(Blueprint->GetPackage());
			}
			else
			{
				ControllerCDO->MarkPackageDirty();
				DirtyPackages.AddUnique(ControllerCDO->GetPackage());
			}
		}
		if (!bChanged)
		{
			ReportLines.Add(TEXT("- Controller input defaults already match target actions."));
		}
	}

	void EnsureInputActionDecoratorMappings(
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages,
		const TMap<FName, FString>& DesiredMappings)
	{
		UBlueprint* DecoratorBlueprint = Cast<UBlueprint>(LoadObjectByPackagePath(InputActionDecoratorPath, UBlueprint::StaticClass()));
		UClass* DecoratorClass = DecoratorBlueprint ? DecoratorBlueprint->GeneratedClass.Get() : nullptr;
		if (!DecoratorClass)
		{
			DecoratorClass = LoadClass<UInputActionRichTextDecorator>(nullptr, *InputActionDecoratorClassPath);
		}
		UInputActionRichTextDecorator* DecoratorCDO = DecoratorClass ? Cast<UInputActionRichTextDecorator>(DecoratorClass->GetDefaultObject()) : nullptr;
		if (!DecoratorCDO)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing input action decorator `%s`; icon mappings were not updated."), *InputActionDecoratorPath));
			return;
		}

		bool bChanged = false;
		if (!bDryRun)
		{
			DecoratorCDO->Modify();
			for (const TPair<FName, FString>& Pair : DesiredMappings)
			{
				const TSoftObjectPtr<UInputAction> DesiredAction{ FSoftObjectPath(Pair.Value) };
				const TSoftObjectPtr<UInputAction>* ExistingAction = DecoratorCDO->ActionMap.Find(Pair.Key);
				if (!ExistingAction || ExistingAction->ToSoftObjectPath() != DesiredAction.ToSoftObjectPath())
				{
					DecoratorCDO->ActionMap.Add(Pair.Key, DesiredAction);
					bChanged = true;
				}
			}
			if (DecoratorCDO->AutoResolvePath != TEXT("/Game/Code/Core/Input/Actions/"))
			{
				DecoratorCDO->AutoResolvePath = TEXT("/Game/Code/Core/Input/Actions/");
				bChanged = true;
			}
		}

		if (bChanged && !bDryRun)
		{
			if (DecoratorBlueprint)
			{
				DecoratorBlueprint->MarkPackageDirty();
				DirtyPackages.AddUnique(DecoratorBlueprint->GetPackage());
			}
			else
			{
				DecoratorCDO->MarkPackageDirty();
				DirtyPackages.AddUnique(DecoratorCDO->GetPackage());
			}
		}

		ReportLines.Add(FString::Printf(
			TEXT("- BP_InputActionDecorator has %d required action mappings%s."),
			DesiredMappings.Num(),
			bChanged ? TEXT(" (updated)") : TEXT("")));
	}
}

UGamepadInputSetupCommandlet::UGamepadInputSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGamepadInputSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Gamepad Input Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(TEXT("- Layout: A=Interact/Accept, B=Dash/Back, X=Light/Secondary, Y=Heavy/Details, Menu=Pause, View=Backpack."));
	ReportLines.Add(TEXT(""));

	UInputMappingContext* MappingContext = Cast<UInputMappingContext>(LoadObjectByPackagePath(MappingContextPath, UInputMappingContext::StaticClass()));
	if (!MappingContext)
	{
		ReportLines.Add(FString::Printf(TEXT("- Missing input mapping context `%s`; setup aborted."), *MappingContextPath));
		const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
		FFileHelper::SaveStringToFile(FString::Join(ReportLines, LINE_TERMINATOR), *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8);
		return 1;
	}

	UInputAction* IA_Interact = Cast<UInputAction>(LoadObjectByPackagePath(ActionRootPath / TEXT("IA_Interact"), UInputAction::StaticClass()));
	UInputAction* IA_Dash = Cast<UInputAction>(LoadObjectByPackagePath(ActionRootPath / TEXT("IA_Dash"), UInputAction::StaticClass()));
	UInputAction* IA_Esc = Cast<UInputAction>(LoadObjectByPackagePath(ActionRootPath / TEXT("IA_Esc"), UInputAction::StaticClass()));
	UInputAction* IA_OpenBackpack = Cast<UInputAction>(LoadObjectByPackagePath(ActionRootPath / TEXT("IA_OpenBackback"), UInputAction::StaticClass()));
	UInputAction* IA_Reload = Cast<UInputAction>(LoadObjectByPackagePath(ActionRootPath / TEXT("IA_Reload"), UInputAction::StaticClass()));
	UInputAction* IA_UseCombatItem = LoadOrCreateInputAction(TEXT("IA_UseCombatItem"), bDryRun, ReportLines, DirtyPackages);
	UInputAction* IA_SwitchCombatItemNext = LoadOrCreateInputAction(TEXT("IA_SwitchCombatItemNext"), bDryRun, ReportLines, DirtyPackages);
	UInputAction* IA_SwitchCombatItemPrevious = LoadOrCreateInputAction(TEXT("IA_SwitchCombatItemPrevious"), bDryRun, ReportLines, DirtyPackages);

	EnsureMappings(MappingContext, IA_Interact, TEXT("IA_Interact"), { EKeys::E, EKeys::Gamepad_FaceButton_Bottom }, { EKeys::Gamepad_FaceButton_Right }, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_Dash, TEXT("IA_Dash"), { EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Right }, { EKeys::Gamepad_FaceButton_Bottom }, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_Esc, TEXT("IA_Esc"), { EKeys::Escape, EKeys::Gamepad_Special_Right }, {}, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_OpenBackpack, TEXT("IA_OpenBackback"), { EKeys::Tab, EKeys::Gamepad_Special_Left }, {}, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_Reload, TEXT("IA_Reload"), { EKeys::R, EKeys::Gamepad_RightShoulder }, { EKeys::Gamepad_RightThumbstick }, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_UseCombatItem, TEXT("IA_UseCombatItem"), { EKeys::F, EKeys::Gamepad_LeftShoulder }, {}, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_SwitchCombatItemNext, TEXT("IA_SwitchCombatItemNext"), { EKeys::Q, EKeys::Gamepad_DPad_Right }, {}, bDryRun, ReportLines, DirtyPackages);
	EnsureMappings(MappingContext, IA_SwitchCombatItemPrevious, TEXT("IA_SwitchCombatItemPrevious"), { EKeys::Z, EKeys::Gamepad_DPad_Left }, {}, bDryRun, ReportLines, DirtyPackages);

	AssignControllerDefaults(
		MappingContext,
		IA_Interact,
		IA_Dash,
		IA_Esc,
		IA_OpenBackpack,
		IA_Reload,
		IA_UseCombatItem,
		IA_SwitchCombatItemNext,
		IA_SwitchCombatItemPrevious,
		bDryRun,
		ReportLines,
		DirtyPackages);

	const TMap<FName, FString> DecoratorMappings = {
		{ TEXT("Accept"), ToObjectPath(ActionRootPath / TEXT("IA_Interact")) },
		{ TEXT("Back"), ToObjectPath(ActionRootPath / TEXT("IA_Dash")) },
		{ TEXT("Interact"), ToObjectPath(ActionRootPath / TEXT("IA_Interact")) },
		{ TEXT("Dash"), ToObjectPath(ActionRootPath / TEXT("IA_Dash")) },
		{ TEXT("Pause"), ToObjectPath(ActionRootPath / TEXT("IA_Esc")) },
		{ TEXT("Esc"), ToObjectPath(ActionRootPath / TEXT("IA_Esc")) },
		{ TEXT("OpenBackpack"), ToObjectPath(ActionRootPath / TEXT("IA_OpenBackback")) },
		{ TEXT("Backpack"), ToObjectPath(ActionRootPath / TEXT("IA_OpenBackback")) },
		{ TEXT("Reload"), ToObjectPath(ActionRootPath / TEXT("IA_Reload")) },
		{ TEXT("UseCombatItem"), ToObjectPath(ActionRootPath / TEXT("IA_UseCombatItem")) },
		{ TEXT("SwitchCombatItemNext"), ToObjectPath(ActionRootPath / TEXT("IA_SwitchCombatItemNext")) },
		{ TEXT("SwitchCombatItemPrevious"), ToObjectPath(ActionRootPath / TEXT("IA_SwitchCombatItemPrevious")) },
		{ TEXT("LightAttack"), ToObjectPath(ActionRootPath / TEXT("IA_LightAttack")) },
		{ TEXT("HeavyAttack"), ToObjectPath(ActionRootPath / TEXT("IA_HeavyAttack")) },
	};
	EnsureInputActionDecoratorMappings(bDryRun, ReportLines, DirtyPackages, DecoratorMappings);

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), ReportFileName);
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("Gamepad input setup finished. Report: %s"), *ReportPath);
	return 0;
}
