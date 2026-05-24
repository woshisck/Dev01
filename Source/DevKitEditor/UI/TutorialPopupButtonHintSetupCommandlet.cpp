#include "UI/TutorialPopupButtonHintSetupCommandlet.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "FileHelpers.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UI/YogCommonRichTextBlock.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace TutorialPopupButtonHintSetup
{
	const FString WidgetPackagePath = TEXT("/Game/UI/Playtest_UI/Tutorial/WBP_TutorialPopup");
	const FString ReportFileName = TEXT("TutorialPopupButtonHintSetupReport.md");
	const FString InputActionDecoratorClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InputActionDecorator.BP_InputActionDecorator_C");
	const FString InfoPopupTextStyleClassPath = TEXT("/Game/Docs/UI/Tutorial/BP_InfoPopupTextStyle.BP_InfoPopupTextStyle_C");

	const FLinearColor HintColor(0.92f, 0.94f, 0.98f, 1.0f);

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
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

	template <typename TWidget>
	TWidget* FindOrCreateWidget(UWidgetTree* WidgetTree, const FName WidgetName, const bool bVariable)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		if (UWidget* ExistingWidget = WidgetTree->FindWidget(WidgetName))
		{
			if (TWidget* TypedWidget = Cast<TWidget>(ExistingWidget))
			{
				TypedWidget->bIsVariable = bVariable;
				return TypedWidget;
			}

			const FString OldName = FString::Printf(
				TEXT("TutorialPopupOld_%s_%s"),
				*WidgetName.ToString(),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			WidgetTree->RemoveWidget(ExistingWidget);
			ExistingWidget->Rename(*OldName, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}

		TWidget* NewWidget = WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), WidgetName);
		if (NewWidget)
		{
			NewWidget->bIsVariable = bVariable;
		}
		return NewWidget;
	}

	void DetachFromParent(UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		if (UPanelWidget* Parent = Widget->GetParent())
		{
			Parent->RemoveChild(Widget);
		}
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

	void ConfigureInputHint(UYogCommonRichTextBlock* HintText)
	{
		if (!HintText)
		{
			return;
		}

		HintText->SetText(FText::FromString(TEXT("<input action=\"Interact\"/>")));
		HintText->SetVisibility(ESlateVisibility::HitTestInvisible);
		HintText->SetAutoWrapText(false);
		HintText->SetJustification(ETextJustify::Center);
		HintText->SetClipping(EWidgetClipping::ClipToBounds);
		HintText->FontStyleClass = LoadClass<UCommonTextStyle>(nullptr, *InfoPopupTextStyleClassPath);
		HintText->OverrideFontSize = 18;
		HintText->OverrideColor = HintColor;

		TArray<UClass*> DecoratorClasses;
		if (UClass* InputActionDecoratorClass = LoadClass<URichTextBlockDecorator>(nullptr, *InputActionDecoratorClassPath))
		{
			DecoratorClasses.Add(InputActionDecoratorClass);
		}
		SetClassArrayProperty(HintText, TEXT("DecoratorClasses"), DecoratorClasses);
	}

	UTextBlock* ConfigurePageArrowLabel(UTextBlock* Label, const FString& Text)
	{
		if (!Label)
		{
			return nullptr;
		}

		Label->Modify();
		Label->SetText(FText::FromString(Text));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(HintColor));
		Label->SetFont(FSlateFontInfo(Label->GetFont().FontObject, 24));
		return Label;
	}

	void ConfigurePageNavInputHint(UYogCommonRichTextBlock* HintText, const FString& ActionToken)
	{
		ConfigureInputHint(HintText);
		if (HintText)
		{
			HintText->SetText(FText::FromString(FString::Printf(TEXT("<input action=\"%s\"/>"), *ActionToken)));
			HintText->OverrideFontSize = 16;
		}
	}

	UButton* ConfigurePageArrowButton(UButton* Button, UWidget* Content)
	{
		if (!Button)
		{
			return nullptr;
		}

		Button->Modify();
		Button->SetVisibility(ESlateVisibility::Visible);
		if (Content)
		{
			Button->SetContent(Content);
		}
		return Button;
	}

	bool UpdateTutorialPopupButton(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			ReportLines.Add(TEXT("- Missing widget blueprint or widget tree."));
			return false;
		}

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UButton* BtnConfirm = Cast<UButton>(WidgetTree->FindWidget(TEXT("BtnConfirm")));
		if (!BtnConfirm)
		{
			ReportLines.Add(TEXT("- Missing required Button `BtnConfirm`."));
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetTree->Modify();
		BtnConfirm->Modify();

		UHorizontalBox* ContentRow = FindOrCreateWidget<UHorizontalBox>(WidgetTree, TEXT("BtnConfirmContentRow"), false);
		USizeBox* HintBox = FindOrCreateWidget<USizeBox>(WidgetTree, TEXT("BtnConfirmInputHintBox"), false);
		UYogCommonRichTextBlock* InputHint = FindOrCreateWidget<UYogCommonRichTextBlock>(WidgetTree, TEXT("BtnConfirmInputHint"), true);
		UTextBlock* Label = FindOrCreateWidget<UTextBlock>(WidgetTree, TEXT("BtnConfirmLabel"), true);

		if (!ContentRow || !HintBox || !InputHint || !Label)
		{
			ReportLines.Add(TEXT("- Failed to create button content widgets."));
			return false;
		}

		DetachFromParent(ContentRow);
		DetachFromParent(HintBox);
		DetachFromParent(InputHint);
		DetachFromParent(Label);

		HintBox->Modify();
		HintBox->SetWidthOverride(54.0f);
		HintBox->SetHeightOverride(30.0f);
		HintBox->SetContent(InputHint);
		ConfigureInputHint(InputHint);

		ContentRow->Modify();
		ContentRow->ClearChildren();
		if (UHorizontalBoxSlot* HintSlot = ContentRow->AddChildToHorizontalBox(HintBox))
		{
			HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			HintSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			HintSlot->SetHorizontalAlignment(HAlign_Center);
			HintSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* LabelSlot = ContentRow->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			LabelSlot->SetPadding(FMargin(0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		BtnConfirm->SetContent(ContentRow);

		WidgetBlueprint->MarkPackageDirty();
		ReportLines.Add(TEXT("- Added `BtnConfirmInputHint` before `BtnConfirmLabel` in `BtnConfirm`."));
		return true;
	}

	bool UpdatePageNavigation(UWidgetBlueprint* WidgetBlueprint, TArray<FString>& ReportLines)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			ReportLines.Add(TEXT("- Missing widget blueprint or widget tree for page navigation."));
			return false;
		}

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UTextBlock* PageHint = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("PageHint")));
		if (!PageHint)
		{
			ReportLines.Add(TEXT("- Missing required TextBlock `PageHint`."));
			return false;
		}

		UPanelWidget* OriginalParent = PageHint->GetParent();
		const int32 OriginalIndex = OriginalParent ? OriginalParent->GetChildIndex(PageHint) : INDEX_NONE;

		WidgetBlueprint->Modify();
		WidgetTree->Modify();
		PageHint->Modify();

		UHorizontalBox* PageNavigationRow = FindOrCreateWidget<UHorizontalBox>(WidgetTree, TEXT("PageNavigationRow"), false);
		UButton* BtnPreviousPage = FindOrCreateWidget<UButton>(WidgetTree, TEXT("BtnPreviousPage"), true);
		UButton* BtnNextPage = FindOrCreateWidget<UButton>(WidgetTree, TEXT("BtnNextPage"), true);
		UTextBlock* BtnPreviousPageLabel = FindOrCreateWidget<UTextBlock>(WidgetTree, TEXT("BtnPreviousPageLabel"), true);
		UTextBlock* BtnNextPageLabel = FindOrCreateWidget<UTextBlock>(WidgetTree, TEXT("BtnNextPageLabel"), true);
		UYogCommonRichTextBlock* PagePreviousInputHint =
			FindOrCreateWidget<UYogCommonRichTextBlock>(WidgetTree, TEXT("PagePreviousInputHint"), true);
		UYogCommonRichTextBlock* PageNextInputHint =
			FindOrCreateWidget<UYogCommonRichTextBlock>(WidgetTree, TEXT("PageNextInputHint"), true);

		if (!PageNavigationRow || !BtnPreviousPage || !BtnNextPage || !BtnPreviousPageLabel || !BtnNextPageLabel ||
			!PagePreviousInputHint || !PageNextInputHint)
		{
			ReportLines.Add(TEXT("- Failed to create page navigation widgets."));
			return false;
		}

		DetachFromParent(PageNavigationRow);
		DetachFromParent(BtnPreviousPage);
		DetachFromParent(BtnNextPage);
		DetachFromParent(BtnPreviousPageLabel);
		DetachFromParent(BtnNextPageLabel);
		DetachFromParent(PagePreviousInputHint);
		DetachFromParent(PageNextInputHint);
		DetachFromParent(PageHint);

		ConfigurePageArrowButton(BtnPreviousPage, ConfigurePageArrowLabel(BtnPreviousPageLabel, TEXT("<")));
		ConfigurePageArrowButton(BtnNextPage, ConfigurePageArrowLabel(BtnNextPageLabel, TEXT(">")));
		ConfigurePageNavInputHint(PagePreviousInputHint, TEXT("SwitchCombatItemPrevious"));
		ConfigurePageNavInputHint(PageNextInputHint, TEXT("SwitchCombatItemNext"));

		PageHint->SetJustification(ETextJustify::Center);

		PageNavigationRow->Modify();
		PageNavigationRow->ClearChildren();
		if (UHorizontalBoxSlot* PrevButtonSlot = PageNavigationRow->AddChildToHorizontalBox(BtnPreviousPage))
		{
			PrevButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PrevButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			PrevButtonSlot->SetHorizontalAlignment(HAlign_Center);
			PrevButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* PrevHintSlot = PageNavigationRow->AddChildToHorizontalBox(PagePreviousInputHint))
		{
			PrevHintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PrevHintSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			PrevHintSlot->SetHorizontalAlignment(HAlign_Center);
			PrevHintSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* PageSlot = PageNavigationRow->AddChildToHorizontalBox(PageHint))
		{
			PageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PageSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			PageSlot->SetHorizontalAlignment(HAlign_Center);
			PageSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* NextHintSlot = PageNavigationRow->AddChildToHorizontalBox(PageNextInputHint))
		{
			NextHintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			NextHintSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			NextHintSlot->SetHorizontalAlignment(HAlign_Center);
			NextHintSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* NextButtonSlot = PageNavigationRow->AddChildToHorizontalBox(BtnNextPage))
		{
			NextButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			NextButtonSlot->SetPadding(FMargin(0.0f));
			NextButtonSlot->SetHorizontalAlignment(HAlign_Center);
			NextButtonSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (OriginalParent)
		{
			if (OriginalIndex != INDEX_NONE)
			{
				OriginalParent->InsertChildAt(OriginalIndex, PageNavigationRow);
			}
			else
			{
				OriginalParent->AddChild(PageNavigationRow);
			}
		}

		WidgetBlueprint->MarkPackageDirty();
		ReportLines.Add(TEXT("- Replaced `PageHint` with `PageNavigationRow` containing previous/next arrow buttons and gamepad DPad hints."));
		return true;
	}
}

UTutorialPopupButtonHintSetupCommandlet::UTutorialPopupButtonHintSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UTutorialPopupButtonHintSetupCommandlet::Main(const FString& Params)
{
	using namespace TutorialPopupButtonHintSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Tutorial Popup Button Hint Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Target: `%s`"), *WidgetPackagePath));
	ReportLines.Add(TEXT(""));

	UWidgetBlueprint* WidgetBlueprint = LoadWidgetBlueprint(WidgetPackagePath);
	if (!WidgetBlueprint)
	{
		ReportLines.Add(TEXT("- Missing target WBP."));
	}
	else if (bDryRun)
	{
		ReportLines.Add(TEXT("- Would add `BtnConfirmInputHint` before `BtnConfirmLabel`."));
		ReportLines.Add(TEXT("- Would wrap `PageHint` with previous/next navigation controls."));
	}
	else
	{
		const bool bUpdatedButton = UpdateTutorialPopupButton(WidgetBlueprint, ReportLines);
		const bool bUpdatedNavigation = UpdatePageNavigation(WidgetBlueprint, ReportLines);
		if (bUpdatedButton || bUpdatedNavigation)
		{
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		}
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TutorialPopupButtonHintSetup::ReportFileName, ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Tutorial popup button hint setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return WidgetBlueprint ? 0 : 1;
}
