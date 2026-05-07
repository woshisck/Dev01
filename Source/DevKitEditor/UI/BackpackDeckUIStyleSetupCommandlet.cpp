#include "UI/BackpackDeckUIStyleSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/RichTextBlockDecorator.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "AutomatedAssetImportData.h"
#include "Engine/Texture2D.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "CommonTextBlock.h"
#include "Styling/SlateBrush.h"
#include "UObject/UnrealType.h"
#include "UI/BackpackGridWidget.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "UI/CombatDeckBarWidget.h"
#include "UI/CombatDeckCardSlotWidget.h"
#include "UI/CombatDeckEditCardSlotWidget.h"
#include "UI/CombatDeckEditWidget.h"
#include "UI/InputActionRichTextDecorator.h"
#include "UI/PendingGridWidget.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/RuneSlotWidget.h"
#include "UI/YogCommonRichTextBlock.h"
#include "UI/YogCommonUITextBlock.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

namespace BackpackDeckUIStyleSetup
{
	//const FString ReportFileName = TEXT("BackpackDeckUIStyleSetupReport.md");
	const FString BackpackScreenPath = TEXT("/Game/UI/Playtest_UI/WBP_BackpackScreen");
	const FString BackpackGridPath = TEXT("/Game/UI/Playtest_UI/BackpackGrid/WBP_BackpackGrid");
	const FString PendingGridPath = TEXT("/Game/UI/Playtest_UI/BackpackGrid/WBP_PendingGrid");
	const FString RuneSlotPath = TEXT("/Game/UI/Playtest_UI/Runes/WBP_RuneSlot");
	const FString RuneInfoCardClassPath = TEXT("/Game/UI/Playtest_UI/Runes/WBP_RuneInfoCard.WBP_RuneInfoCard_C");
	const FString CombatDeckBarPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_CombatDeckBar");
	const FString CombatDeckCardSlotPath = TEXT("/Game/UI/Playtest_UI/HUD/WBP_CombatDeckCardSlot");
	const FString CombatDeckEditPath = TEXT("/Game/UI/Playtest_UI/CombatInfo/WBP_CombatDeckEditWidget");
	const FString CombatDeckEditCardSlotPath = TEXT("/Game/UI/Playtest_UI/CombatInfo/WBP_CombatDeckEditCardSlot");
	const FString BackpackStylePath = TEXT("/Game/Docs/UI/RunCard/GlobalSet/DA_BackpackStyle");
	const FString BackpackInspectTextureDir = TEXT("/Game/Docs/UI/RunCard/BackpackInspect");
	const FString SourceArtBackpackInspectDir = TEXT("SourceArt/UI/BackpackInspect");
	const FString MainPanelFrameTexturePath = TEXT("/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_MainPanelFrame");
	const FString CellFrameTexturePath = TEXT("/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_CellFrame");
	const FString TarotCardFrameTexturePath = TEXT("/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_TarotCardFrame");
	const FString WeaponIconTexturePath = TEXT("/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_WeaponIcon_TrickBlade");
	const FString InputActionDecoratorPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator");
	const FString InputActionDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator.BP_InputActionDecorator_C");
	const FString KeywordDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_KeywordDecorator.BP_KeywordDecorator_C");
	const FString InfoPopupTextStyleClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InfoPopupTextStyle.BP_InfoPopupTextStyle_C");
	const FString InteractInputActionObjectPath = TEXT("/Game/Code/Core/Input/Actions/IA_Interact.IA_Interact");
	const FString EscInputActionObjectPath = TEXT("/Game/Code/Core/Input/Actions/IA_Esc.IA_Esc");
	const FString MouseClickInputActionObjectPath = TEXT("/Game/Code/Core/Input/Actions/IA_MouseClick.IA_MouseClick");

	const FLinearColor SilverText(0.86f, 0.88f, 0.90f, 1.0f);
	const FLinearColor MutedSilver(0.58f, 0.62f, 0.66f, 1.0f);
	const FLinearColor BrightSilver(0.92f, 0.94f, 0.98f, 1.0f);
	const FLinearColor DarkPanel(0.015f, 0.017f, 0.020f, 0.88f);
	const FLinearColor DeepPanel(0.030f, 0.033f, 0.040f, 0.93f);
	const FLinearColor CardFill(0.050f, 0.055f, 0.065f, 0.94f);
	const FLinearColor CardFillSoft(0.075f, 0.080f, 0.092f, 0.92f);
	const FLinearColor SilverLine(0.70f, 0.74f, 0.78f, 0.72f);
	const FLinearColor AccentGold(0.84f, 0.86f, 0.92f, 1.0f);
	const FLinearColor BloodAccent(0.42f, 0.035f, 0.045f, 0.90f);

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	FString ToClassPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath) + TEXT("_C");
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	UWidgetBlueprint* LoadWidgetBlueprint(const FString& PackagePath)
	{
		if (UWidgetBlueprint* Existing = FindObject<UWidgetBlueprint>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	UObject* LoadObjectByPackagePath(const FString& PackagePath, UClass* ObjectClass)
	{
		return StaticLoadObject(ObjectClass, nullptr, *ToObjectPath(PackagePath));
	}

	UTexture2D* LoadTextureByPackagePath(const FString& PackagePath)
	{
		return Cast<UTexture2D>(LoadObjectByPackagePath(PackagePath, UTexture2D::StaticClass()));
	}

	UTexture2D* ImportTextureIfNeeded(const FString& SourceFileName, const FString& PackagePath, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UTexture2D* ExistingTexture = LoadTextureByPackagePath(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found texture `%s`."), *PackagePath));
			return ExistingTexture;
		}

		const FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), SourceArtBackpackInspectDir / SourceFileName);
		if (!FPaths::FileExists(SourcePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing source texture `%s`."), *SourcePath));
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TArray<FString> ImportFiles;
		ImportFiles.Add(SourcePath);
		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->GroupName = TEXT("BackpackInspectUI");
		ImportData->Filenames = ImportFiles;
		ImportData->DestinationPath = BackpackInspectTextureDir;
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;
		TArray<UObject*> ImportedObjects = AssetTools.ImportAssetsAutomated(ImportData);

		UTexture2D* ImportedTexture = LoadTextureByPackagePath(PackagePath);
		if (!ImportedTexture)
		{
			for (UObject* ImportedObject : ImportedObjects)
			{
				if (UTexture2D* Candidate = Cast<UTexture2D>(ImportedObject))
				{
					ImportedTexture = Candidate;
					break;
				}
			}
		}

		ReportLines.Add(FString::Printf(TEXT("- Imported `%s` -> `%s`: %s."),
			*SourcePath,
			*PackagePath,
			ImportedTexture ? TEXT("ok") : TEXT("failed")));
		if (ImportedTexture)
		{
			ImportedTexture->MarkPackageDirty();
			DirtyPackages.AddUnique(ImportedTexture->GetPackage());
		}
		return ImportedTexture;
	}

	void ImportBackpackInspectTextures(TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ImportTextureIfNeeded(TEXT("T_BackpackInspect_MainPanelFrame.png"), MainPanelFrameTexturePath, ReportLines, DirtyPackages);
		ImportTextureIfNeeded(TEXT("T_BackpackInspect_CellFrame.png"), CellFrameTexturePath, ReportLines, DirtyPackages);
		ImportTextureIfNeeded(TEXT("T_BackpackInspect_TarotCardFrame.png"), TarotCardFrameTexturePath, ReportLines, DirtyPackages);
		ImportTextureIfNeeded(TEXT("T_BackpackInspect_WeaponIcon_TrickBlade.png"), WeaponIconTexturePath, ReportLines, DirtyPackages);
	}

	void EnsureInputActionDecoratorMappings(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
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
			ReportLines.Add(FString::Printf(TEXT("- Missing input action decorator `%s`; CommonUI icon mappings were not updated."), *InputActionDecoratorPath));
			return;
		}

		auto EnsureActionMapping = [DecoratorCDO](const FName ActionName, const FString& ActionObjectPath) -> bool
		{
			const TSoftObjectPtr<UInputAction> DesiredAction{ FSoftObjectPath(ActionObjectPath) };
			const TSoftObjectPtr<UInputAction>* ExistingAction = DecoratorCDO->ActionMap.Find(ActionName);
			if (ExistingAction && ExistingAction->ToSoftObjectPath() == DesiredAction.ToSoftObjectPath())
			{
				return false;
			}

			DecoratorCDO->ActionMap.Add(ActionName, DesiredAction);
			return true;
		};

		bool bChanged = false;
		if (!bDryRun)
		{
			DecoratorCDO->Modify();
			bChanged |= EnsureActionMapping(TEXT("Interact"), InteractInputActionObjectPath);
			bChanged |= EnsureActionMapping(TEXT("Esc"), EscInputActionObjectPath);
			bChanged |= EnsureActionMapping(TEXT("MouseClick"), MouseClickInputActionObjectPath);
			if (DecoratorCDO->AutoResolvePath != TEXT("/Game/Code/Core/Input/Actions/"))
			{
				DecoratorCDO->AutoResolvePath = TEXT("/Game/Code/Core/Input/Actions/");
				bChanged = true;
			}

			if (bChanged)
			{
				if (DecoratorBlueprint)
				{
					DecoratorBlueprint->Modify();
					DecoratorBlueprint->MarkPackageDirty();
					DirtyPackages.AddUnique(DecoratorBlueprint->GetPackage());
				}
				else
				{
					DecoratorCDO->MarkPackageDirty();
					DirtyPackages.AddUnique(DecoratorCDO->GetOutermost());
				}
			}
		}

		ReportLines.Add(FString::Printf(TEXT("- BP_InputActionDecorator ActionMap includes Interact, Esc, and MouseClick mappings%s."),
			bChanged ? TEXT(" (updated)") : TEXT("")));
	}

	template <typename TWidget>
	TWidget* ConstructNamedWidget(UWidgetTree* WidgetTree, const FName WidgetName, bool bVariable = true)
	{
		TWidget* Widget = WidgetTree ? WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName) : nullptr;
		if (Widget)
		{
			Widget->bIsVariable = bVariable;
		}
		return Widget;
	}

	UWidget* ConstructWidgetFromClassPath(UWidgetTree* WidgetTree, const FString& ClassPath, const FName WidgetName, UClass* FallbackClass, TArray<FString>& ReportLines)
	{
		UClass* WidgetClass = LoadClass<UWidget>(nullptr, *ClassPath);
		if (!WidgetClass)
		{
			WidgetClass = FallbackClass;
			ReportLines.Add(FString::Printf(TEXT("- Missing widget class `%s`; using fallback for `%s`."), *ClassPath, *WidgetName.ToString()));
		}

		if (!WidgetTree || !WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
		{
			ReportLines.Add(FString::Printf(TEXT("- Failed to construct `%s`."), *WidgetName.ToString()));
			return nullptr;
		}

		UWidget* Widget = WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
		if (Widget)
		{
			Widget->bIsVariable = true;
		}
		return Widget;
	}

	UWidgetBlueprint* CreateWidgetBlueprint(const FString& PackagePath, UClass* ParentClass, bool bDryRun, TArray<FString>& ReportLines, bool& bCreated)
	{
		bCreated = false;
		if (UWidgetBlueprint* Existing = LoadWidgetBlueprint(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UObject* NewAsset = AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UWidgetBlueprint::StaticClass(),
			Factory);

		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(NewAsset);
		if (WidgetBlueprint)
		{
			bCreated = true;
			FAssetRegistryModule::AssetCreated(WidgetBlueprint);
		}

		return WidgetBlueprint;
	}

	void ResetWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		TArray<UWidget*> ExistingWidgets;
		TArray<UObject*> ExistingObjects;
		GetObjectsWithOuter(WidgetBlueprint->WidgetTree, ExistingObjects, true);
		for (UObject* ExistingObject : ExistingObjects)
		{
			if (UWidget* ExistingWidget = Cast<UWidget>(ExistingObject))
			{
				ExistingWidgets.AddUnique(ExistingWidget);
			}
		}

		if (WidgetBlueprint->WidgetTree->RootWidget)
		{
			WidgetBlueprint->WidgetTree->RemoveWidget(WidgetBlueprint->WidgetTree->RootWidget);
			WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		}

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (!ExistingWidget)
			{
				continue;
			}

			const FString OldName = FString::Printf(
				TEXT("BDUI_Old_%s_%s"),
				*ExistingWidget->GetName(),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			WidgetBlueprint->WidgetTree->RemoveWidget(ExistingWidget);
			ExistingWidget->Rename(*OldName, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
	}

	void ConfigureText(UTextBlock* TextBlock, const FString& Text, const FLinearColor& Color, int32 Size, ETextJustify::Type Justification = ETextJustify::Center)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetClipping(EWidgetClipping::ClipToBounds);
		TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = Size;
		TextBlock->SetFont(FontInfo);
	}

	void SetClassArrayProperty(UObject* Object, const FName PropertyName, const TArray<UClass*>& Values)
	{
		if (!Object)
		{
			return;
		}

		if (FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName))
		{
			if (FClassProperty* ClassProperty = CastField<FClassProperty>(ArrayProperty->Inner))
			{
				Object->Modify();
				FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
				Helper.EmptyValues();

				for (UClass* Value : Values)
				{
					if (!Value)
					{
						continue;
					}

					const int32 Index = Helper.AddValue();
					ClassProperty->SetPropertyValue(Helper.GetRawPtr(Index), Value);
				}
			}
		}
	}

	void ConfigureComboRichText(UYogCommonRichTextBlock* RichTextBlock, const FString& Text, const FLinearColor& Color, int32 Size)
	{
		if (!RichTextBlock)
		{
			return;
		}

		RichTextBlock->SetText(FText::FromString(Text));
		RichTextBlock->SetAutoWrapText(true);
		RichTextBlock->SetJustification(ETextJustify::Left);
		RichTextBlock->SetClipping(EWidgetClipping::ClipToBounds);
		RichTextBlock->FontStyleClass = LoadClass<UCommonTextStyle>(nullptr, *InfoPopupTextStyleClassPath);
		RichTextBlock->OverrideFontSize = Size;
		RichTextBlock->OverrideColor = Color;

		TArray<UClass*> DecoratorClasses;
		if (UClass* InputActionDecoratorClass = LoadClass<URichTextBlockDecorator>(nullptr, *InputActionDecoratorClassPath))
		{
			DecoratorClasses.Add(InputActionDecoratorClass);
		}

		if (UClass* KeywordDecoratorClass = LoadClass<URichTextBlockDecorator>(nullptr, *KeywordDecoratorClassPath))
		{
			DecoratorClasses.Add(KeywordDecoratorClass);
		}
		SetClassArrayProperty(RichTextBlock, TEXT("DecoratorClasses"), DecoratorClasses);
	}

	void ConfigureBorder(UBorder* Border, const FLinearColor& Color, const FMargin& Padding = FMargin())
	{
		if (!Border)
		{
			return;
		}

		Border->SetBrushColor(Color);
		Border->SetPadding(Padding);
	}

	FSlateBrush MakeColorBrush(const FLinearColor& Color, const FVector2D& ImageSize = FVector2D(32.0f, 32.0f))
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = ImageSize;
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	}

	void ConfigureButton(UButton* Button, const FLinearColor& NormalColor, const FLinearColor& HoverColor, const FLinearColor& PressedColor)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal = MakeColorBrush(NormalColor);
		Style.Hovered = MakeColorBrush(HoverColor);
		Style.Pressed = MakeColorBrush(PressedColor);
		Style.Disabled = MakeColorBrush(FLinearColor(0.02f, 0.02f, 0.022f, 0.35f));
		Button->SetStyle(Style);
		Button->SetColorAndOpacity(FLinearColor::White);
		Button->SetBackgroundColor(FLinearColor::White);
	}

	void AddOverlayChild(UOverlay* Overlay, UWidget* Child, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& Padding = FMargin())
	{
		if (Overlay && Child)
		{
			if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Child))
			{
				Slot->SetHorizontalAlignment(HAlign);
				Slot->SetVerticalAlignment(VAlign);
				Slot->SetPadding(Padding);
			}
		}
	}

	void AddVerticalChild(UVerticalBox* Box, UWidget* Child, EHorizontalAlignment HAlign = HAlign_Fill, const FMargin& Padding = FMargin(), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
	{
		if (Box && Child)
		{
			if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child))
			{
				Slot->SetHorizontalAlignment(HAlign);
				Slot->SetPadding(Padding);
				Slot->SetSize(FSlateChildSize(SizeRule));
			}
		}
	}

	void AddHorizontalChild(UHorizontalBox* Box, UWidget* Child, EVerticalAlignment VAlign = VAlign_Fill, const FMargin& Padding = FMargin(), ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic)
	{
		if (Box && Child)
		{
			if (UHorizontalBoxSlot* Slot = Box->AddChildToHorizontalBox(Child))
			{
				Slot->SetVerticalAlignment(VAlign);
				Slot->SetPadding(Padding);
				Slot->SetSize(FSlateChildSize(SizeRule));
			}
		}
	}

	void ConfigureCanvasSlot(UCanvasPanelSlot* Slot, const FAnchors& Anchors, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, int32 ZOrder)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetAnchors(Anchors);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAlignment(Alignment);
		Slot->SetAutoSize(false);
		Slot->SetZOrder(ZOrder);
	}

	USizeBox* WrapSize(UWidgetTree* WidgetTree, UWidget* Child, const FName Name, float Width, float Height)
	{
		USizeBox* SizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, Name, false);
		if (SizeBox)
		{
			SizeBox->SetWidthOverride(Width);
			SizeBox->SetHeightOverride(Height);
			SizeBox->AddChild(Child);
		}
		return SizeBox;
	}

	void SetClassProperty(UObject* Object, const FName PropertyName, UClass* Value)
	{
		if (!Object || !Value)
		{
			return;
		}

		if (FClassProperty* ClassProperty = FindFProperty<FClassProperty>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			ClassProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	void SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
	{
		if (!Object || !Value)
		{
			return;
		}

		if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
		}
	}

	void SetLinearColorProperty(UObject* Object, const FName PropertyName, const FLinearColor& Value)
	{
		if (!Object)
		{
			return;
		}

		if (FStructProperty* StructProperty = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName))
		{
			if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
			{
				Object->Modify();
				StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(Object), &Value);
			}
		}
	}

	void SetFloatProperty(UObject* Object, const FName PropertyName, float Value)
	{
		if (!Object)
		{
			return;
		}

		if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			FloatProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	void SetBoolProperty(UObject* Object, const FName PropertyName, bool Value)
	{
		if (!Object)
		{
			return;
		}

		if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
		{
			Object->Modify();
			BoolProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	UTextBlock* MakeLabel(UWidgetTree* WidgetTree, const FName Name, const FString& Text, int32 Size, const FLinearColor& Color = SilverText, ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* Label = ConstructNamedWidget<UYogCommonUITextBlock>(WidgetTree, Name);
		ConfigureText(Label, Text, Color, Size, Justification);
		return Label;
	}

	UYogCommonRichTextBlock* MakeComboRichLabel(UWidgetTree* WidgetTree, const FName Name, const FString& Text, int32 Size, const FLinearColor& Color = SilverText)
	{
		UYogCommonRichTextBlock* Label = ConstructNamedWidget<UYogCommonRichTextBlock>(WidgetTree, Name);
		ConfigureComboRichText(Label, Text, Color, Size);
		return Label;
	}

	UOverlay* MakeFramedPanel(UWidgetTree* WidgetTree, const FName Name, UWidget*& OutContentRoot, const FLinearColor& FillColor = DeepPanel, const FMargin& Padding = FMargin(14.0f))
	{
		UOverlay* Panel = ConstructNamedWidget<UOverlay>(WidgetTree, Name, false);
		UBorder* Back = ConstructNamedWidget<UBorder>(WidgetTree, FName(*(Name.ToString() + TEXT("_Back"))), false);
		UBorder* Edge = ConstructNamedWidget<UBorder>(WidgetTree, FName(*(Name.ToString() + TEXT("_Edge"))), false);
		UBorder* TopLine = ConstructNamedWidget<UBorder>(WidgetTree, FName(*(Name.ToString() + TEXT("_TopLine"))), false);
		USizeBox* TopLineSize = ConstructNamedWidget<USizeBox>(WidgetTree, FName(*(Name.ToString() + TEXT("_TopLineSize"))), false);
		UBorder* ContentBorder = ConstructNamedWidget<UBorder>(WidgetTree, FName(*(Name.ToString() + TEXT("_Content"))), false);

		if (!Panel || !Back || !Edge || !TopLine || !TopLineSize || !ContentBorder)
		{
			OutContentRoot = nullptr;
			return Panel;
		}

		ConfigureBorder(Back, FillColor, FMargin());
		ConfigureBorder(Edge, FLinearColor(0.58f, 0.62f, 0.68f, 0.18f), FMargin());
		ConfigureBorder(TopLine, SilverLine, FMargin());
		ConfigureBorder(ContentBorder, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), Padding);
		TopLineSize->SetHeightOverride(2.0f);
		TopLineSize->AddChild(TopLine);

		AddOverlayChild(Panel, Back, HAlign_Fill, VAlign_Fill);
		AddOverlayChild(Panel, Edge, HAlign_Fill, VAlign_Fill, FMargin(1.0f));
		AddOverlayChild(Panel, TopLineSize, HAlign_Fill, VAlign_Top, FMargin(10.0f, 8.0f, 10.0f, 0.0f));
		AddOverlayChild(Panel, ContentBorder, HAlign_Fill, VAlign_Fill);

		OutContentRoot = ContentBorder;
		return Panel;
	}

	void BuildRuneSlotTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UOverlay* Root = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("Root"));
		UImage* CellBG = ConstructNamedWidget<UImage>(WidgetTree, TEXT("CellBG"));
		UImage* ActiveZoneOverlay = ConstructNamedWidget<UImage>(WidgetTree, TEXT("ActiveZoneOverlay"));
		UImage* CellIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("CellIcon"));
		UImage* SelectionBorder = ConstructNamedWidget<UImage>(WidgetTree, TEXT("SelectionBorder"));
		UImage* DisabledOverlay = ConstructNamedWidget<UImage>(WidgetTree, TEXT("DisabledOverlay"));
		if (!RootSize || !Root || !CellBG || !ActiveZoneOverlay || !CellIcon || !SelectionBorder || !DisabledOverlay)
		{
			return;
		}

		RootSize->SetWidthOverride(68.0f);
		RootSize->SetHeightOverride(68.0f);
		RootSize->AddChild(Root);
		WidgetTree->RootWidget = RootSize;

		CellBG->SetColorAndOpacity(CardFillSoft);
		ActiveZoneOverlay->SetColorAndOpacity(FLinearColor(0.82f, 0.88f, 1.0f, 0.18f));
		ActiveZoneOverlay->SetVisibility(ESlateVisibility::Collapsed);
		CellIcon->SetColorAndOpacity(FLinearColor::White);
		SelectionBorder->SetColorAndOpacity(FLinearColor(0.90f, 0.92f, 0.96f, 0.28f));
		SelectionBorder->SetVisibility(ESlateVisibility::Collapsed);
		DisabledOverlay->SetVisibility(ESlateVisibility::Collapsed);

		AddOverlayChild(Root, CellBG, HAlign_Fill, VAlign_Fill);
		AddOverlayChild(Root, ActiveZoneOverlay, HAlign_Fill, VAlign_Fill, FMargin(3.0f));
		AddOverlayChild(Root, CellIcon, HAlign_Fill, VAlign_Fill, FMargin(10.0f));
		AddOverlayChild(Root, SelectionBorder, HAlign_Fill, VAlign_Fill, FMargin(2.0f));
		AddOverlayChild(Root, DisabledOverlay, HAlign_Fill, VAlign_Fill);

		ReportLines.Add(TEXT("- Rune slot rebuilt as a compact silver card cell."));
	}

	void BuildBackpackGridTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UWidget* PanelContent = nullptr;
		UOverlay* RootPanel = MakeFramedPanel(WidgetTree, TEXT("BackpackGridPanel"), PanelContent, DeepPanel, FMargin(14.0f));
		UVerticalBox* Stack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("GridStack"), false);
		USizeBox* GridSizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("GridSizeBox"));
		UUniformGridPanel* BackpackGrid = ConstructNamedWidget<UUniformGridPanel>(WidgetTree, TEXT("BackpackGrid"));
		if (!RootPanel || !PanelContent || !Stack || !GridSizeBox || !BackpackGrid)
		{
			return;
		}

		WidgetBlueprint->WidgetTree->RootWidget = RootPanel;
		Cast<UBorder>(PanelContent)->SetContent(Stack);

		GridSizeBox->AddChild(BackpackGrid);
		AddVerticalChild(Stack, GridSizeBox, HAlign_Center);

		ReportLines.Add(TEXT("- Backpack grid wrapper rebuilt without legacy heat phase controls."));
	}

	void BuildPendingGridTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UWidget* PanelContent = nullptr;
		UOverlay* RootPanel = MakeFramedPanel(WidgetTree, TEXT("PendingGridPanel"), PanelContent, FLinearColor(0.025f, 0.028f, 0.034f, 0.82f), FMargin(10.0f));
		USizeBox* PendingGridSizeBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PendingGridSizeBox"));
		UUniformGridPanel* PendingRuneGrid = ConstructNamedWidget<UUniformGridPanel>(WidgetTree, TEXT("PendingRuneGrid"));
		if (!RootPanel || !PanelContent || !PendingGridSizeBox || !PendingRuneGrid)
		{
			return;
		}

		WidgetTree->RootWidget = RootPanel;
		PendingGridSizeBox->AddChild(PendingRuneGrid);
		Cast<UBorder>(PanelContent)->SetContent(PendingGridSizeBox);
		ReportLines.Add(TEXT("- Pending grid wrapper rebuilt to match the silver card grid style."));
	}

	void BuildCombatDeckCardSlotTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UOverlay* Root = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("Root"));
		UBorder* CardFrame = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("CardFrame"));
		UVerticalBox* CardStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("CardStack"), false);
		USizeBox* IconBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("IconBox"), false);
		UImage* CardIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("CardIcon"));
		UTextBlock* StateText = MakeLabel(WidgetTree, TEXT("StateText"), TEXT("NEXT"), 12, AccentGold);
		UTextBlock* CardNameText = MakeLabel(WidgetTree, TEXT("CardNameText"), TEXT("Card"), 13, BrightSilver);
		UTextBlock* ActionText = MakeLabel(WidgetTree, TEXT("ActionText"), TEXT("Light"), 11, MutedSilver);
		UTextBlock* TypeText = MakeLabel(WidgetTree, TEXT("TypeText"), TEXT("Attack"), 10, SilverLine);
		UBorder* TopLine = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("TopSilverLine"), false);
		USizeBox* TopLineSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("TopSilverLineSize"), false);
		if (!RootSize || !Root || !CardFrame || !CardStack || !IconBox || !CardIcon || !StateText || !CardNameText || !ActionText || !TypeText || !TopLine || !TopLineSize)
		{
			return;
		}

		RootSize->SetWidthOverride(72.0f);
		RootSize->SetHeightOverride(86.0f);
		RootSize->AddChild(Root);
		WidgetTree->RootWidget = RootSize;

		ConfigureBorder(CardFrame, CardFill, FMargin(5.0f, 6.0f, 5.0f, 5.0f));
		AddOverlayChild(Root, CardFrame, HAlign_Fill, VAlign_Fill, FMargin(2.0f));
		CardFrame->SetContent(CardStack);

		ConfigureBorder(TopLine, SilverLine, FMargin());
		TopLineSize->SetHeightOverride(1.5f);
		TopLineSize->AddChild(TopLine);
		AddOverlayChild(Root, TopLineSize, HAlign_Fill, VAlign_Top, FMargin(10.0f, 6.0f, 10.0f, 0.0f));

		AddVerticalChild(CardStack, StateText, HAlign_Center, FMargin(0.0f, 0.0f, 0.0f, 1.0f));
		IconBox->SetWidthOverride(36.0f);
		IconBox->SetHeightOverride(28.0f);
		CardIcon->SetColorAndOpacity(FLinearColor::White);
		IconBox->AddChild(CardIcon);
		AddVerticalChild(CardStack, IconBox, HAlign_Center, FMargin(0.0f, 0.0f, 0.0f, 2.0f));
		AddVerticalChild(CardStack, CardNameText, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 1.0f));
		AddVerticalChild(CardStack, ActionText, HAlign_Fill);
		AddVerticalChild(CardStack, TypeText, HAlign_Fill);

		ReportLines.Add(TEXT("- Combat deck HUD card slot rebuilt in the tutorial-card silver style."));
	}

	void BuildCombatDeckBarTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UOverlay* Root = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("Root"));
		UBorder* Back = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("ConveyorBack"), false);
		UVerticalBox* Stack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("ConveyorStack"), false);
		UHorizontalBox* HeaderRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("HeaderRow"), false);
		UTextBlock* TitleText = MakeLabel(WidgetTree, TEXT("TitleText"), TEXT("1D DECK"), 16, BrightSilver, ETextJustify::Left);
		UTextBlock* StatusText = MakeLabel(WidgetTree, TEXT("StatusText"), TEXT("Deck: 0"), 14, MutedSilver, ETextJustify::Right);
		UHorizontalBox* CardRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("CardRow"));
		UOverlay* ShufflePanel = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("ShufflePanel"));
		UBorder* ShuffleBack = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("ShuffleBack"), false);
		UProgressBar* ShuffleProgressBar = ConstructNamedWidget<UProgressBar>(WidgetTree, TEXT("ShuffleProgressBar"));
		UTextBlock* ShuffleText = MakeLabel(WidgetTree, TEXT("ShuffleText"), TEXT("Reloading"), 13, SilverText);
		UTextBlock* ConsumedToastText = MakeLabel(WidgetTree, TEXT("ConsumedToastText"), TEXT("Consumed"), 14, AccentGold);
		UTextBlock* RewardToastText = MakeLabel(WidgetTree, TEXT("RewardToastText"), TEXT("Added"), 14, BrightSilver);
		if (!RootSize || !Root || !Back || !Stack || !HeaderRow || !TitleText || !StatusText || !CardRow || !ShufflePanel || !ShuffleBack || !ShuffleProgressBar || !ShuffleText || !ConsumedToastText || !RewardToastText)
		{
			return;
		}

		RootSize->SetWidthOverride(900.0f);
		RootSize->SetHeightOverride(112.0f);
		RootSize->AddChild(Root);
		WidgetTree->RootWidget = RootSize;

		ConfigureBorder(Back, FLinearColor(0.012f, 0.014f, 0.018f, 0.78f), FMargin(16.0f, 8.0f));
		Back->SetContent(Stack);
		AddOverlayChild(Root, Back, HAlign_Fill, VAlign_Fill);

		AddHorizontalChild(HeaderRow, TitleText, VAlign_Center, FMargin(), ESlateSizeRule::Fill);
		AddHorizontalChild(HeaderRow, StatusText, VAlign_Center, FMargin(), ESlateSizeRule::Fill);
		AddVerticalChild(Stack, HeaderRow, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		AddVerticalChild(Stack, CardRow, HAlign_Left);

		const FString SlotClassPath = ToClassPath(CombatDeckCardSlotPath);
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FName SlotName(*FString::Printf(TEXT("CardSlot_%d"), Index));
			UWidget* SlotWidget = ConstructWidgetFromClassPath(WidgetTree, SlotClassPath, SlotName, UCombatDeckCardSlotWidget::StaticClass(), ReportLines);
			AddHorizontalChild(CardRow, SlotWidget, VAlign_Center, FMargin(2.0f, 0.0f));
		}

		ConfigureBorder(ShuffleBack, FLinearColor(0.02f, 0.025f, 0.032f, 0.94f), FMargin(10.0f, 6.0f));
		AddOverlayChild(ShufflePanel, ShuffleBack, HAlign_Fill, VAlign_Fill);
		AddOverlayChild(ShufflePanel, ShuffleProgressBar, HAlign_Fill, VAlign_Bottom, FMargin(12.0f, 0.0f, 12.0f, 8.0f));
		AddOverlayChild(ShufflePanel, ShuffleText, HAlign_Center, VAlign_Center);
		ShufflePanel->SetVisibility(ESlateVisibility::Collapsed);
		AddOverlayChild(Root, ShufflePanel, HAlign_Fill, VAlign_Bottom, FMargin(16.0f, 0.0f, 16.0f, 8.0f));

		ConsumedToastText->SetRenderOpacity(0.0f);
		ConsumedToastText->SetVisibility(ESlateVisibility::Collapsed);
		RewardToastText->SetRenderOpacity(0.0f);
		RewardToastText->SetVisibility(ESlateVisibility::Collapsed);
		AddOverlayChild(Root, ConsumedToastText, HAlign_Left, VAlign_Top, FMargin(24.0f, 18.0f, 0.0f, 0.0f));
		AddOverlayChild(Root, RewardToastText, HAlign_Right, VAlign_Top, FMargin(0.0f, 18.0f, 24.0f, 0.0f));

		ReportLines.Add(TEXT("- Combat deck HUD conveyor rebuilt with 8 silver card slots and reload overlay."));
	}

	void BuildCombatDeckEditCardSlotTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		USizeBox* RootSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("RootSize"), false);
		UOverlay* Root = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("Root"));
		UImage* CardBG = ConstructNamedWidget<UImage>(WidgetTree, TEXT("CardBG"));
		UBorder* CardBack = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("CardBack"), false);
		UVerticalBox* CardStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("CardStack"), false);
		USizeBox* IconBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("IconBox"), false);
		UImage* CardIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("CardIcon"));
		UTextBlock* CardNameText = MakeLabel(WidgetTree, TEXT("CardNameText"), TEXT("Card"), 15, BrightSilver, ETextJustify::Center);
		USizeBox* CardNameBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("CardNameBox"), false);
		UTextBlock* TypeText = MakeLabel(WidgetTree, TEXT("TypeText"), TEXT("Attack"), 12, MutedSilver, ETextJustify::Center);
		UButton* ReverseButton = ConstructNamedWidget<UButton>(WidgetTree, TEXT("ReverseButton"));
		UTextBlock* ReverseText = MakeLabel(WidgetTree, TEXT("ReverseButtonText"), TEXT(""), 1, BrightSilver);
		UBorder* LinkHintOverlay = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("LinkHintOverlay"));
		UOverlay* LinkGemPanel = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("LinkGemPanel"), false);
		UImage* LinkGemGlow = ConstructNamedWidget<UImage>(WidgetTree, TEXT("LinkGemGlow"));
		UImage* LinkGemCore = ConstructNamedWidget<UImage>(WidgetTree, TEXT("LinkGemCore"));
		UBorder* SelectedMark = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("SelectedMark"));
		UBorder* TopLine = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("TopSilverLine"), false);
		USizeBox* TopLineSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("TopSilverLineSize"), false);
		if (!RootSize || !Root || !CardBG || !CardBack || !CardStack || !IconBox || !CardIcon || !CardNameText || !CardNameBox || !TypeText || !ReverseButton || !ReverseText || !LinkHintOverlay || !LinkGemPanel || !LinkGemGlow || !LinkGemCore || !SelectedMark || !TopLine || !TopLineSize)
		{
			return;
		}

		RootSize->SetWidthOverride(124.0f);
		RootSize->SetHeightOverride(300.0f);
		RootSize->AddChild(Root);
		WidgetTree->RootWidget = RootSize;

		if (UTexture2D* TarotFrame = LoadTextureByPackagePath(TarotCardFrameTexturePath))
		{
			CardBG->SetBrushFromTexture(TarotFrame, false);
		}
		CardBG->SetColorAndOpacity(FLinearColor(0.84f, 0.88f, 0.92f, 1.0f));
		AddOverlayChild(Root, CardBG, HAlign_Fill, VAlign_Fill);

		ConfigureBorder(CardBack, FLinearColor(0.010f, 0.012f, 0.016f, 0.34f), FMargin(10.0f, 20.0f, 10.0f, 18.0f));
		CardBack->SetContent(CardStack);
		AddOverlayChild(Root, CardBack, HAlign_Fill, VAlign_Fill, FMargin(4.0f));

		ConfigureBorder(LinkHintOverlay, FLinearColor(0.62f, 0.78f, 0.90f, 0.13f), FMargin());
		LinkHintOverlay->SetVisibility(ESlateVisibility::Collapsed);
		AddOverlayChild(Root, LinkHintOverlay, HAlign_Fill, VAlign_Fill, FMargin(5.0f));

		ConfigureBorder(SelectedMark, FLinearColor(0.88f, 0.90f, 0.94f, 0.22f), FMargin());
		SelectedMark->SetVisibility(ESlateVisibility::Collapsed);
		AddOverlayChild(Root, SelectedMark, HAlign_Fill, VAlign_Fill);

		LinkGemGlow->SetBrush(MakeColorBrush(FLinearColor(0.48f, 0.82f, 1.00f, 0.34f), FVector2D(58.0f, 20.0f)));
		LinkGemCore->SetBrush(MakeColorBrush(FLinearColor(0.72f, 0.88f, 0.96f, 0.76f), FVector2D(18.0f, 18.0f)));
		FWidgetTransform GemCoreTransform;
		GemCoreTransform.Angle = 45.0f;
		LinkGemCore->SetRenderTransform(GemCoreTransform);
		LinkGemCore->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		LinkGemPanel->SetVisibility(ESlateVisibility::Collapsed);
		AddOverlayChild(LinkGemPanel, LinkGemGlow, HAlign_Fill, VAlign_Center, FMargin(4.0f, 8.0f));
		AddOverlayChild(LinkGemPanel, LinkGemCore, HAlign_Center, VAlign_Center);
		ConfigureButton(ReverseButton, FLinearColor(1.0f, 1.0f, 1.0f, 0.0f), FLinearColor(0.80f, 0.88f, 0.96f, 0.08f), FLinearColor(0.95f, 0.92f, 0.76f, 0.14f));
		ReverseButton->SetContent(ReverseText);
		AddOverlayChild(LinkGemPanel, ReverseButton, HAlign_Fill, VAlign_Fill);
		AddOverlayChild(Root, WrapSize(WidgetTree, LinkGemPanel, TEXT("LinkGemPanelSize"), 66.0f, 34.0f), HAlign_Center, VAlign_Bottom, FMargin(0.0f, 0.0f, 0.0f, 20.0f));

		ConfigureBorder(TopLine, SilverLine, FMargin());
		TopLineSize->SetHeightOverride(2.0f);
		TopLineSize->AddChild(TopLine);
		AddOverlayChild(Root, TopLineSize, HAlign_Fill, VAlign_Top, FMargin(18.0f, 15.0f, 18.0f, 0.0f));

		IconBox->SetWidthOverride(60.0f);
		IconBox->SetHeightOverride(60.0f);
		IconBox->AddChild(CardIcon);
		AddVerticalChild(CardStack, IconBox, HAlign_Center, FMargin(0.0f, 24.0f, 0.0f, 24.0f));

		CardNameText->SetAutoWrapText(true);
		CardNameText->SetWrapTextAt(96.0f);
		CardNameText->SetMinDesiredWidth(96.0f);
		CardNameBox->SetWidthOverride(98.0f);
		CardNameBox->SetHeightOverride(54.0f);
		CardNameBox->AddChild(CardNameText);
		AddVerticalChild(CardStack, CardNameBox, HAlign_Center, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		TypeText->SetAutoWrapText(false);
		AddVerticalChild(CardStack, TypeText, HAlign_Fill);

		ReportLines.Add(TEXT("- Combat deck edit card slot rebuilt larger with wrapped title text and a bottom glass-gem Link orientation indicator."));
	}

	void BuildCombatDeckEditWidgetTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UWidget* PanelContent = nullptr;
		UOverlay* RootPanel = MakeFramedPanel(WidgetTree, TEXT("DeckEditPanel"), PanelContent, FLinearColor(0.018f, 0.020f, 0.026f, 0.88f), FMargin(16.0f));
		UScrollBox* CardListPanel = ConstructNamedWidget<UScrollBox>(WidgetTree, TEXT("CardListPanel"));
		UHorizontalBox* CardListBox = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("CardListBox"));
		if (!RootPanel || !PanelContent || !CardListPanel || !CardListBox)
		{
			return;
		}

		WidgetTree->RootWidget = RootPanel;
		Cast<UBorder>(PanelContent)->SetContent(CardListPanel);

		CardListPanel->SetOrientation(EOrientation::Orient_Horizontal);
		CardListPanel->SetScrollBarVisibility(ESlateVisibility::Visible);
		CardListPanel->SetAlwaysShowScrollbar(true);
		CardListPanel->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
		CardListPanel->AddChild(CardListBox);

		ReportLines.Add(TEXT("- Combat deck edit widget rebuilt as a horizontal 1D card preview list with a visible scrollbar."));
	}

	void BuildBackpackScreenTree(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		ResetWidgetTree(WidgetBlueprint);
		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;

		UCanvasPanel* Root = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("Root"));
		UBorder* BackgroundDim = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("BackgroundDim"), false);
		USizeBox* PanelSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("PanelSize"), false);
		UOverlay* MainPanel = ConstructNamedWidget<UOverlay>(WidgetTree, TEXT("MainPanel"), false);
		UBorder* MainBack = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("MainBack"), false);
		UVerticalBox* MainStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("MainStack"), false);
		UHorizontalBox* Columns = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("Columns"), false);
		UVerticalBox* LeftColumn = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("LeftColumn"), false);
		UVerticalBox* CenterColumn = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("CenterColumn"), false);
		UVerticalBox* RightColumn = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("RightColumn"), false);
		UTextBlock* LeftTitle = MakeLabel(WidgetTree, TEXT("MainBackpackTitle"), TEXT("持有武器"), 20, BrightSilver, ETextJustify::Left);
		UTextBlock* CenterTitle = MakeLabel(WidgetTree, TEXT("DeckOrderTitle"), TEXT("卡组预览"), 20, BrightSilver, ETextJustify::Left);
		UTextBlock* RightTitle = MakeLabel(WidgetTree, TEXT("CardDetailTitle"), TEXT("CARD DETAIL"), 20, BrightSilver, ETextJustify::Left);
		UBorder* WeaponInfoPanel = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("WeaponInfoPanel"), false);
		UVerticalBox* WeaponInfoStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("WeaponInfoStack"), false);
		UHorizontalBox* WeaponInfoRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("WeaponInfoRow"), false);
		USizeBox* WeaponIconBox = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("WeaponIconBox"), false);
		UImage* WeaponIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("WeaponIcon"));
		UVerticalBox* WeaponTextStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("WeaponTextStack"), false);
		UTextBlock* WeaponNameText = MakeLabel(WidgetTree, TEXT("WeaponNameText"), TEXT("未装备武器"), 17, BrightSilver, ETextJustify::Left);
		UTextBlock* WeaponDescText = MakeLabel(WidgetTree, TEXT("WeaponDescText"), TEXT("拾取武器后显示武器说明。"), 12, MutedSilver, ETextJustify::Left);
		UWidget* CombatDeckEditWidget = ConstructWidgetFromClassPath(WidgetTree, ToClassPath(CombatDeckEditPath), TEXT("CombatDeckEditWidget"), UCombatDeckEditWidget::StaticClass(), ReportLines);
		UWidget* RuneInfoCard = ConstructWidgetFromClassPath(WidgetTree, RuneInfoCardClassPath, TEXT("RuneInfoCard"), URuneInfoCardWidget::StaticClass(), ReportLines);
		UBorder* OperationHintWidget = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("OperationHintWidget"));
		UYogCommonRichTextBlock* OperationHintText = MakeComboRichLabel(WidgetTree, TEXT("OperationHintText"), TEXT("<input action=\"MouseClick\"/> 拖拽卡牌调整顺序  R 反转 Link"), 12, BrightSilver);
		UBorder* BottomActionBar = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("BottomActionBar"), false);
		USizeBox* ButtonRowSize = ConstructNamedWidget<USizeBox>(WidgetTree, TEXT("ButtonRowSize"), false);
		UHorizontalBox* ButtonRow = ConstructNamedWidget<UHorizontalBox>(WidgetTree, TEXT("ButtonRow"), false);
		UButton* ConfirmButton = ConstructNamedWidget<UButton>(WidgetTree, TEXT("ConfirmButton"));
		UButton* CloseButton = ConstructNamedWidget<UButton>(WidgetTree, TEXT("CloseButton"));
		UButton* EndPreviewButton = ConstructNamedWidget<UButton>(WidgetTree, TEXT("EndPreviewButton"));
		UYogCommonRichTextBlock* ConfirmText = MakeComboRichLabel(WidgetTree, TEXT("ConfirmText"), TEXT("<input action=\"MouseClick\"/> 选择"), 14, BrightSilver);
		UYogCommonRichTextBlock* CloseText = MakeComboRichLabel(WidgetTree, TEXT("CloseText"), TEXT("<input action=\"Esc\"/> 取消"), 14, BrightSilver);
		UYogCommonRichTextBlock* EndPreviewText = MakeComboRichLabel(WidgetTree, TEXT("EndPreviewText"), TEXT("<input action=\"Esc\"/> 结束预览"), 14, BrightSilver);
		UBorder* ComboHintWidget = ConstructNamedWidget<UBorder>(WidgetTree, TEXT("ComboHintWidget"), false);
		UVerticalBox* ComboHintStack = ConstructNamedWidget<UVerticalBox>(WidgetTree, TEXT("ComboHintStack"), false);
		UTextBlock* ComboHintTitle = MakeLabel(WidgetTree, TEXT("ComboHintTitle"), TEXT("出招表"), 15, BrightSilver, ETextJustify::Left);
		UYogCommonRichTextBlock* ComboHintText = MakeComboRichLabel(
			WidgetTree,
			TEXT("ComboHintText"),
			TEXT("连段 01   <input action=\"LightAttack\"/> -> <input action=\"LightAttack\"/> -> <input action=\"HeavyAttack\"/>"),
			13,
			SilverText);
		UImage* GrabbedRuneIcon = ConstructNamedWidget<UImage>(WidgetTree, TEXT("GrabbedRuneIcon"));
		UCanvasPanel* ShapePreviewCanvas = ConstructNamedWidget<UCanvasPanel>(WidgetTree, TEXT("ShapePreviewCanvas"));
		if (!Root || !BackgroundDim || !PanelSize || !MainPanel || !MainBack || !MainStack || !Columns || !LeftColumn || !CenterColumn || !RightColumn || !LeftTitle || !CenterTitle || !RightTitle || !WeaponInfoPanel || !WeaponInfoStack || !WeaponInfoRow || !WeaponIconBox || !WeaponIcon || !WeaponTextStack || !WeaponNameText || !WeaponDescText || !CombatDeckEditWidget || !RuneInfoCard || !OperationHintWidget || !OperationHintText || !BottomActionBar || !ButtonRowSize || !ButtonRow || !ConfirmButton || !CloseButton || !EndPreviewButton || !ConfirmText || !CloseText || !EndPreviewText || !ComboHintWidget || !ComboHintStack || !ComboHintTitle || !ComboHintText || !GrabbedRuneIcon || !ShapePreviewCanvas)
		{
			return;
		}

		WidgetTree->RootWidget = Root;
		ConfigureBorder(BackgroundDim, FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
		ConfigureCanvasSlot(Root->AddChildToCanvas(BackgroundDim), FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, 0);

		PanelSize->SetWidthOverride(1440.0f);
		PanelSize->SetHeightOverride(810.0f);
		PanelSize->AddChild(MainPanel);
		ConfigureCanvasSlot(Root->AddChildToCanvas(PanelSize), FAnchors(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(1440.0f, 810.0f), FVector2D(0.5f, 0.5f), 1);

		ConfigureBorder(MainBack, FLinearColor(0.008f, 0.009f, 0.012f, 0.58f), FMargin(28.0f));
		MainBack->SetContent(MainStack);
		AddOverlayChild(MainPanel, MainBack, HAlign_Fill, VAlign_Fill);

		AddVerticalChild(LeftColumn, LeftTitle, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		ConfigureBorder(WeaponInfoPanel, FLinearColor(0.030f, 0.033f, 0.040f, 0.94f), FMargin(12.0f, 10.0f));
		WeaponInfoPanel->SetContent(WeaponInfoStack);
		WeaponIconBox->SetWidthOverride(82.0f);
		WeaponIconBox->SetHeightOverride(82.0f);
		if (UTexture2D* WeaponIconTexture = LoadTextureByPackagePath(WeaponIconTexturePath))
		{
			WeaponIcon->SetBrushFromTexture(WeaponIconTexture, false);
		}
		WeaponIconBox->AddChild(WeaponIcon);
		AddHorizontalChild(WeaponInfoRow, WeaponIconBox, VAlign_Top, FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		WeaponDescText->SetAutoWrapText(true);
		AddVerticalChild(WeaponTextStack, WeaponNameText, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		AddVerticalChild(WeaponTextStack, WeaponDescText, HAlign_Fill);
		AddHorizontalChild(WeaponInfoRow, WeaponTextStack, VAlign_Fill, FMargin(), ESlateSizeRule::Fill);
		AddVerticalChild(WeaponInfoStack, WeaponInfoRow, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 10.0f));

		ComboHintTitle->SetText(FText::FromString(TEXT("出招表")));
		ConfigureComboRichText(
			ComboHintText,
			TEXT("连段 01   <input action=\"LightAttack\"/> -> <input action=\"LightAttack\"/> -> <input action=\"HeavyAttack\"/>\n连段 02   <input action=\"LightAttack\"/> -> <input action=\"HeavyAttack\"/>"),
			SilverText,
			13);
		ConfigureBorder(ComboHintWidget, FLinearColor(0.038f, 0.050f, 0.075f, 0.90f), FMargin(10.0f, 8.0f));
		ComboHintWidget->SetContent(ComboHintStack);
		AddVerticalChild(ComboHintStack, ComboHintTitle, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		AddVerticalChild(ComboHintStack, ComboHintText, HAlign_Fill);
		AddVerticalChild(WeaponInfoStack, ComboHintWidget, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 10.0f));

		ConfigureBorder(OperationHintWidget, FLinearColor(0.055f, 0.058f, 0.065f, 0.92f), FMargin(12.0f, 4.0f));
		OperationHintText->SetAutoWrapText(false);
		OperationHintWidget->SetContent(OperationHintText);

		ConfigureButton(ConfirmButton, FLinearColor(0.12f, 0.13f, 0.15f, 0.88f), FLinearColor(0.34f, 0.36f, 0.40f, 0.94f), FLinearColor(0.52f, 0.54f, 0.58f, 1.0f));
		ConfirmText->SetAutoWrapText(false);
		ConfirmButton->SetContent(ConfirmText);
		ConfigureButton(CloseButton, FLinearColor(0.10f, 0.11f, 0.13f, 0.85f), FLinearColor(0.32f, 0.34f, 0.38f, 0.92f), FLinearColor(0.48f, 0.50f, 0.55f, 1.0f));
		CloseText->SetAutoWrapText(false);
		CloseButton->SetContent(CloseText);
		ConfigureButton(EndPreviewButton, FLinearColor(0.10f, 0.11f, 0.13f, 0.85f), FLinearColor(0.32f, 0.34f, 0.38f, 0.92f), FLinearColor(0.48f, 0.50f, 0.55f, 1.0f));
		EndPreviewText->SetAutoWrapText(false);
		EndPreviewButton->SetContent(EndPreviewText);
		AddHorizontalChild(ButtonRow, ConfirmButton, VAlign_Fill, FMargin(0.0f, 0.0f, 8.0f, 0.0f), ESlateSizeRule::Fill);
		AddHorizontalChild(ButtonRow, CloseButton, VAlign_Fill, FMargin(0.0f, 0.0f, 8.0f, 0.0f), ESlateSizeRule::Fill);
		AddHorizontalChild(ButtonRow, EndPreviewButton, VAlign_Fill, FMargin(), ESlateSizeRule::Fill);
		ConfigureBorder(BottomActionBar, FLinearColor(0.018f, 0.020f, 0.026f, 0.72f), FMargin(8.0f, 6.0f));
		ButtonRowSize->SetWidthOverride(430.0f);
		ButtonRowSize->SetHeightOverride(38.0f);
		ButtonRowSize->AddChild(ButtonRow);
		BottomActionBar->SetContent(ButtonRowSize);
		AddVerticalChild(LeftColumn, WeaponInfoPanel, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 14.0f), ESlateSizeRule::Fill);

		AddVerticalChild(CenterColumn, CenterTitle, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		AddVerticalChild(CenterColumn, WrapSize(WidgetTree, CombatDeckEditWidget, TEXT("DeckPreviewSize"), 668.0f, 548.0f), HAlign_Fill);
		AddVerticalChild(CenterColumn, WrapSize(WidgetTree, OperationHintWidget, TEXT("DeckOperationHintSize"), 668.0f, 64.0f), HAlign_Fill, FMargin(0.0f, 8.0f, 0.0f, 0.0f));

		AddVerticalChild(RightColumn, RightTitle, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		AddVerticalChild(RightColumn, WrapSize(WidgetTree, RuneInfoCard, TEXT("RuneInfoCardSize"), 340.0f, 622.0f), HAlign_Fill);

		AddHorizontalChild(Columns, WrapSize(WidgetTree, LeftColumn, TEXT("LeftColumnSize"), 340.0f, 656.0f), VAlign_Fill, FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		AddHorizontalChild(Columns, CenterColumn, VAlign_Fill, FMargin(), ESlateSizeRule::Fill);
		AddHorizontalChild(Columns, WrapSize(WidgetTree, RightColumn, TEXT("RightColumnSize"), 340.0f, 656.0f), VAlign_Fill, FMargin(18.0f, 0.0f, 0.0f, 0.0f));
		AddVerticalChild(MainStack, Columns, HAlign_Fill, FMargin(0.0f, 0.0f, 0.0f, 18.0f), ESlateSizeRule::Fill);
		AddVerticalChild(MainStack, BottomActionBar, HAlign_Center);

		GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
		ShapePreviewCanvas->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureCanvasSlot(Root->AddChildToCanvas(GrabbedRuneIcon), FAnchors(0.0f, 0.0f), FVector2D::ZeroVector, FVector2D(72.0f, 72.0f), FVector2D(0.5f, 0.5f), 20);
		ConfigureCanvasSlot(Root->AddChildToCanvas(ShapePreviewCanvas), FAnchors(0.0f, 0.0f), FVector2D::ZeroVector, FVector2D(400.0f, 400.0f), FVector2D::ZeroVector, 21);

		ReportLines.Add(TEXT("- Backpack screen rebuilt as a 16:9 inspect panel using live widgets only: 25% weapon inspection, 50% tarot deck conveyor, 25% card details; no full-screen art overlay."));
		ReportLines.Add(TEXT("- Weapon combo list now uses YogCommonRichTextBlock with BP_InputActionDecorator/BP_KeywordDecorator for CommonUI adaptive input icons."));
		ReportLines.Add(TEXT("- Deck operation hints now live in a 64px reserved row under the deck preview, separated from the global bottom confirm/cancel action bar."));
	}

	UBackpackStyleDataAsset* LoadOrCreateBackpackStyle(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UBackpackStyleDataAsset* ExistingStyle = Cast<UBackpackStyleDataAsset>(LoadObjectByPackagePath(BackpackStylePath, UBackpackStyleDataAsset::StaticClass())))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *BackpackStylePath));
			if (!bDryRun)
			{
				ExistingStyle->Modify();
				ExistingStyle->MainPanelFrameTexture = LoadTextureByPackagePath(MainPanelFrameTexturePath);
				ExistingStyle->DeckCardFrameTexture = LoadTextureByPackagePath(TarotCardFrameTexturePath);
				ExistingStyle->DefaultWeaponIconTexture = LoadTextureByPackagePath(WeaponIconTexturePath);
				ExistingStyle->PanelBackgroundTint = FLinearColor(0.025f, 0.024f, 0.028f, 0.94f);
				ExistingStyle->MoonlitIronTint = FLinearColor(0.78f, 0.84f, 0.88f, 1.0f);
				ExistingStyle->OxbloodAccentTint = FLinearColor(0.34f, 0.035f, 0.045f, 1.0f);
				ExistingStyle->DeckCardWidth = 124.0f;
				ExistingStyle->DeckCardHeight = 300.0f;
				ExistingStyle->DeckCardSpacing = 14.0f;
				UTexture2D* CellFrameTexture = LoadTextureByPackagePath(CellFrameTexturePath);
				ExistingStyle->CellEmptyTexture = CellFrameTexture;
				ExistingStyle->CellActiveTexture = CellFrameTexture;
				ExistingStyle->CellOccupiedActiveTexture = CellFrameTexture;
				ExistingStyle->CellOccupiedInactiveTexture = CellFrameTexture;
				ExistingStyle->CellSelectedTexture = CellFrameTexture;
				ExistingStyle->CellHoverTexture = CellFrameTexture;
				ExistingStyle->CellGrabbedSourceTexture = CellFrameTexture;
				ExistingStyle->EmptyColor = FLinearColor(0.09f, 0.09f, 0.105f, 0.82f);
				ExistingStyle->EmptyActiveColor = FLinearColor(0.46f, 0.54f, 0.62f, 0.92f);
				ExistingStyle->OccupiedActiveColor = FLinearColor(0.74f, 0.80f, 0.84f, 1.0f);
				ExistingStyle->OccupiedInactiveColor = FLinearColor(0.20f, 0.18f, 0.19f, 0.96f);
				ExistingStyle->SelectedColor = FLinearColor(0.84f, 0.88f, 0.92f, 1.0f);
				ExistingStyle->HoverColor = FLinearColor(0.62f, 0.70f, 0.74f, 1.0f);
				ExistingStyle->GrabbedSourceColor = FLinearColor(0.34f, 0.035f, 0.045f, 1.0f);
				ExistingStyle->PendingHasRuneColor = FLinearColor(0.18f, 0.045f, 0.055f, 0.96f);
				ExistingStyle->PendingEmptyColor = FLinearColor(0.08f, 0.08f, 0.095f, 0.82f);
				ExistingStyle->ActiveZoneOverlayOpacity = 0.22f;
				ExistingStyle->InactiveZoneOpacity = 0.48f;
				ExistingStyle->ZoneGlowOpacity = 0.18f;
				ExistingStyle->CellSize = 68.0f;
				ExistingStyle->CellPadding = 3.0f;
				ExistingStyle->CellCornerRadius = 4.0f;
				ExistingStyle->IconPadding = 8.0f;
				ExistingStyle->MarkPackageDirty();
				DirtyPackages.AddUnique(ExistingStyle->GetPackage());
			}
			return ExistingStyle;
		}

		ReportLines.Add(FString::Printf(TEXT("- Missing `%s`."), *BackpackStylePath));
		return nullptr;
	}

	void AssignDefaults(UWidgetBlueprint* WidgetBlueprint, UObject* StyleDA, UClass* RuneSlotClass, UClass* EditCardSlotClass, TArray<UPackage*>& DirtyPackages)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->GeneratedClass)
		{
			return;
		}

		UObject* CDO = WidgetBlueprint->GeneratedClass->GetDefaultObject();
		SetObjectProperty(CDO, TEXT("StyleDA"), StyleDA);
		SetClassProperty(CDO, TEXT("RuneSlotClass"), RuneSlotClass);
		SetClassProperty(CDO, TEXT("CardSlotClass"), EditCardSlotClass);
		SetObjectProperty(CDO, TEXT("DefaultCardFrameTexture"), LoadTextureByPackagePath(TarotCardFrameTexturePath));
		SetLinearColorProperty(CDO, TEXT("DefaultCardFrameTint"), FLinearColor(0.84f, 0.88f, 0.92f, 1.0f));
		SetBoolProperty(CDO, TEXT("bUseFixedTarotCardSize"), true);
		SetFloatProperty(CDO, TEXT("FallbackDeckCardWidth"), 124.0f);
		SetFloatProperty(CDO, TEXT("FallbackDeckCardHeight"), 300.0f);
		SetFloatProperty(CDO, TEXT("FallbackDeckCardSpacing"), 14.0f);
		SetLinearColorProperty(CDO, TEXT("NextCardFrameColor"), FLinearColor(0.78f, 0.82f, 0.90f, 0.96f));
		SetLinearColorProperty(CDO, TEXT("NormalCardFrameColor"), FLinearColor(0.050f, 0.055f, 0.065f, 0.94f));
		SetLinearColorProperty(CDO, TEXT("EmptyCardFrameColor"), FLinearColor(0.020f, 0.022f, 0.026f, 0.46f));
		WidgetBlueprint->MarkPackageDirty();
		DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
	}
}

UBackpackDeckUIStyleSetupCommandlet::UBackpackDeckUIStyleSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UBackpackDeckUIStyleSetupCommandlet::Main(const FString& Params)
{
	using namespace BackpackDeckUIStyleSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;
	const bool bBackpackGridOnly = Params.Contains(TEXT("BackpackGridOnly"), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Backpack Deck UI Style Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- BackpackGridOnly: %s"), bBackpackGridOnly ? TEXT("true") : TEXT("false")));
	ReportLines.Add(TEXT("- Style target: bright silver gothic card UI, aligned with 512 tutorial illustrations."));
	ReportLines.Add(TEXT(""));

	if (!bDryRun && !bBackpackGridOnly)
	{
		ImportBackpackInspectTextures(ReportLines, DirtyPackages);
	}
	if (!bBackpackGridOnly)
	{
		EnsureInputActionDecoratorMappings(bDryRun, ReportLines, DirtyPackages);
	}

	UBackpackStyleDataAsset* BackpackStyle = bBackpackGridOnly
		? Cast<UBackpackStyleDataAsset>(LoadObjectByPackagePath(BackpackStylePath, UBackpackStyleDataAsset::StaticClass()))
		: LoadOrCreateBackpackStyle(bDryRun, ReportLines, DirtyPackages);
	if (bBackpackGridOnly)
	{
		ReportLines.Add(FString::Printf(TEXT("- %s `%s` without rewriting style settings."), BackpackStyle ? TEXT("Found") : TEXT("Missing"), *BackpackStylePath));
	}

	struct FWidgetBuildSpec
	{
		FString PackagePath;
		UClass* ParentClass;
		void (*BuildTree)(UWidgetBlueprint*, TArray<FString>&);
	};

	TArray<FWidgetBuildSpec> Specs;
	if (bBackpackGridOnly)
	{
		Specs.Add({ BackpackGridPath, UBackpackGridWidget::StaticClass(), &BuildBackpackGridTree });
	}
	else
	{
		Specs = {
			{ RuneSlotPath, URuneSlotWidget::StaticClass(), &BuildRuneSlotTree },
			{ BackpackGridPath, UBackpackGridWidget::StaticClass(), &BuildBackpackGridTree },
			{ PendingGridPath, UPendingGridWidget::StaticClass(), &BuildPendingGridTree },
			{ CombatDeckCardSlotPath, UCombatDeckCardSlotWidget::StaticClass(), &BuildCombatDeckCardSlotTree },
			{ CombatDeckBarPath, UCombatDeckBarWidget::StaticClass(), &BuildCombatDeckBarTree },
			{ CombatDeckEditCardSlotPath, UCombatDeckEditCardSlotWidget::StaticClass(), &BuildCombatDeckEditCardSlotTree },
			{ CombatDeckEditPath, UCombatDeckEditWidget::StaticClass(), &BuildCombatDeckEditWidgetTree },
			{ BackpackScreenPath, UBackpackScreenWidget::StaticClass(), &BuildBackpackScreenTree },
		};
	}

	for (const FWidgetBuildSpec& Spec : Specs)
	{
		bool bCreated = false;
		UWidgetBlueprint* WidgetBlueprint = CreateWidgetBlueprint(Spec.PackagePath, Spec.ParentClass, bDryRun, ReportLines, bCreated);
		if (!WidgetBlueprint)
		{
			continue;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s` layout."), bDryRun ? TEXT("Would rebuild") : TEXT("Rebuilt"), *Spec.PackagePath));
		if (!bDryRun)
		{
			Spec.BuildTree(WidgetBlueprint, ReportLines);
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		}
	}

	if (!bDryRun)
	{
		UClass* RuneSlotClass = LoadClass<UUserWidget>(nullptr, *ToClassPath(RuneSlotPath));
		UClass* EditCardSlotClass = LoadClass<UUserWidget>(nullptr, *ToClassPath(CombatDeckEditCardSlotPath));
		for (const FWidgetBuildSpec& Spec : Specs)
		{
			if (UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(Spec.PackagePath))
			{
				AssignDefaults(WidgetBlueprint, BackpackStyle, RuneSlotClass, EditCardSlotClass, DirtyPackages);
				FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			}
		}

		if (DirtyPackages.Num() > 0)
		{
			UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
		}
	}

	// const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), BackpackDeckUIStyleSetup::ReportFileName);
	// FFileHelper::SaveStringToFile(
	// 	FString::Join(ReportLines, LINE_TERMINATOR),
	// 	*ReportPath,
	// 	FFileHelper::EEncodingOptions::ForceUTF8);

	// UE_LOG(LogTemp, Display, TEXT("Backpack deck UI style setup finished. Report: %s"), *ReportPath);
	return 0;
}
