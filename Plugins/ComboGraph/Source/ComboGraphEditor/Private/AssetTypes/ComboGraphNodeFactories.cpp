// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "AssetTypes/ComboGraphNodeFactories.h"

#include "BlueprintEditorSettings.h"
#include "ClassViewerFilter.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/ComboGraphNodeBlueprints.h"
#include "Graph/ComboGraphNodeMontage.h"
#include "Graph/ComboGraphNodeSequence.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "UComboGraphNodeMontageFactory"

class FComboGraphBlueprintParentFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, const TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};

/** Dialog to configure creation properties */
class SComboGraphNodeMontageBlueprintCreateDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SComboGraphNodeMontageBlueprintCreateDialog) {}

	SLATE_ARGUMENT(TSet<const UClass*>, AllowedClasses)
	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(TWeakObjectPtr<UClass>, DefaultClass)

	SLATE_END_ARGS()



	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs)
	{
		bOkClicked = false;
		DefaultClass = InArgs._DefaultClass;
		AllowedClasses = InArgs._AllowedClasses;
		ParentClass = InArgs._DefaultClass;
		Title = InArgs._Title;

		ChildSlot
		[
			SNew(SBorder)
			.Visibility(EVisibility::Visible)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
#else
			.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
#endif
			[
				SNew(SBox)
				.Visibility(EVisibility::Visible)
				.WidthOverride(500.0f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.FillHeight(1)
					[
						SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
#else
						.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
#endif

						.Content()
						[
							SAssignNew(ParentClassContainer, SVerticalBox)
						]
					]

					// Ok/Cancel buttons
					+SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(8)
					[
						SNew(SUniformGridPanel)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
						.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
						.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
#else
						.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
						.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
						.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
#endif
						+SUniformGridPanel::Slot(0,0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
#else
							.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
#endif
							.OnClicked(this, &SComboGraphNodeMontageBlueprintCreateDialog::OkClicked)
							.Text(LOCTEXT("CreateComboGraphNodeMontageBlueprintCreate", "Create"))
						]
						+SUniformGridPanel::Slot(1,0)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
							.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
#else
							.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
#endif
							.OnClicked(this, &SComboGraphNodeMontageBlueprintCreateDialog::CancelClicked)
							.Text(LOCTEXT("CreateComboGraphNodeMontageBlueprintCancel", "Cancel"))
						]
					]
				]
			]
		];

		MakeParentClassPicker();
	}

	bool ConfigureProperties(const TWeakObjectPtr<UBlueprintFactory> InComboGraphNodeMontageBlueprintFactory)
	{
		ComboGraphBlueprintFactory = InComboGraphNodeMontageBlueprintFactory;

		const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(Title)
		.ClientSize(FVector2D(400, 400))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			AsShared()
		];

		PickerWindow = Window;

		GEditor->EditorAddModalWindow(Window);
		ComboGraphBlueprintFactory.Reset();

		return bOkClicked;
	}

private:
	TSet<const UClass*> AllowedClasses;
	FText Title;

	/** Creates the combo menu for the parent class */
	void MakeParentClassPicker()
	{
		// Load the classviewer module to display a class picker
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		// Fill in options
		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;

		const TSharedRef<FComboGraphBlueprintParentFilter> Filter = MakeShareable(new FComboGraphBlueprintParentFilter());

		#if ENGINE_MAJOR_VERSION == 5
		Options.ClassFilters.Add(Filter);
		#else
		Options.ClassFilter = Filter;
		#endif

		// All child child classes of UComboGraphNodeMontage are valid.
		Filter->AllowedChildrenOfClasses.Append(AllowedClasses);

		ParentClassContainer->ClearChildren();
		ParentClassContainer->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ParentClass", "Parent Class:"))
			.ShadowOffset(FVector2D(1.0f, 1.0f))
		];

		ParentClassContainer->AddSlot()
		[
			ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SComboGraphNodeMontageBlueprintCreateDialog::OnClassPicked))
		];
	}

	/** Handler for when a parent class is selected */
	void OnClassPicked(UClass* ChosenClass)
	{
		ParentClass = ChosenClass;
	}

	/** Handler for when ok is clicked */
	FReply OkClicked()
	{
		if (ComboGraphBlueprintFactory.IsValid())
		{
			ComboGraphBlueprintFactory->ParentClass = ParentClass.Get();
		}

		CloseDialog(true);

		return FReply::Handled();
	}

	void CloseDialog(bool bWasPicked = false)
	{
		bOkClicked = bWasPicked;
		if ( PickerWindow.IsValid() )
		{
			PickerWindow.Pin()->RequestDestroyWindow();
		}
	}

	/** Handler for when cancel is clicked */
	FReply CancelClicked()
	{
		CloseDialog();
		return FReply::Handled();
	}

	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			CloseDialog();
			return FReply::Handled();
		}
		return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

private:
	/** The factory for which we are setting up properties */
	TWeakObjectPtr<UBlueprintFactory> ComboGraphBlueprintFactory;

	/** A pointer to the window that is asking the user to select a parent class */
	TWeakPtr<SWindow> PickerWindow;

	/** The container for the Parent Class picker */
	TSharedPtr<SVerticalBox> ParentClassContainer;

	/** The selected class */
	TWeakObjectPtr<UClass> ParentClass;

	/** The selected class */
	TWeakObjectPtr<UClass> DefaultClass;

	/** True if Ok was clicked */
	bool bOkClicked = false;
};

// ------- FComboGraphNodeFactoryUtils -------------

UObject* FComboGraphNodeFactoryUtils::FactoryCreateNew(UClass* Class, UClass* ParentClass, UClass* BlueprintClassType, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	check(Class->IsChildOf(BlueprintClassType));

	if ((ParentClass == nullptr) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ClassName"), (ParentClass != nullptr) ? FText::FromString(ParentClass->GetName()) : LOCTEXT("Null", "(null)"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotCreateBlueprintFromClass", "Cannot create a blueprint based on the class '{0}'."), Args));
		return nullptr;
	}

	UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BPTYPE_Normal, BlueprintClassType, UBlueprintGeneratedClass::StaticClass(), CallingContext);
	if (NewBP)
	{
		UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
		if (Settings && Settings->bSpawnDefaultBlueprintNodes)
		{
			int32 NodePositionY = 0;
			UEdGraph* EventGraph = NewBP->UbergraphPages[0];
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, EventGraph, FName(TEXT("OnInitialized")), UComboGraphNodeAnimBase::StaticClass(), NodePositionY);
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, EventGraph, FName(TEXT("OnActivated")), UComboGraphNodeAnimBase::StaticClass(), NodePositionY);
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, EventGraph, FName(TEXT("OnDeactivated")), UComboGraphNodeAnimBase::StaticClass(), NodePositionY);
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, EventGraph, FName(TEXT("OnMontagePlay")), UComboGraphNodeAnimBase::StaticClass(), NodePositionY);
			FKismetEditorUtilities::AddDefaultEventNode(NewBP, EventGraph, FName(TEXT("OnEventReceived")), UComboGraphNodeAnimBase::StaticClass(), NodePositionY);
		}
	}

	return NewBP;
}

// ------- UComboGraphNodeMontageFactory -------------

UComboGraphNodeMontageFactory::UComboGraphNodeMontageFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UComboGraphNodeMontageBlueprint::StaticClass();
	ParentClass = UComboGraphNodeMontage::StaticClass();
}

bool UComboGraphNodeMontageFactory::ConfigureProperties()
{

	const TSharedRef<SComboGraphNodeMontageBlueprintCreateDialog> Dialog = SNew(SComboGraphNodeMontageBlueprintCreateDialog)
		.AllowedClasses({ UComboGraphNodeMontage::StaticClass() })
		.DefaultClass(UComboGraphNodeMontage::StaticClass())
		.Title(LOCTEXT("CreateComboGraphNodeMontageBlueprintOptions", "Create Combo Graph Node Blueprint (Montage)"));

	return Dialog->ConfigureProperties(this);
}

UObject* UComboGraphNodeMontageFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return FComboGraphNodeFactoryUtils::FactoryCreateNew(Class, ParentClass, SupportedClass, InParent, Name, Flags, Context, Warn, CallingContext);
}

UObject* UComboGraphNodeMontageFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

// ------- UComboGraphNodeSequenceFactory -------------

UComboGraphNodeSequenceFactory::UComboGraphNodeSequenceFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UComboGraphNodeSequenceBlueprint::StaticClass();
	ParentClass = UComboGraphNodeSequence::StaticClass();
}

bool UComboGraphNodeSequenceFactory::ConfigureProperties()
{
	const TSharedRef<SComboGraphNodeMontageBlueprintCreateDialog> Dialog = SNew(SComboGraphNodeMontageBlueprintCreateDialog)
		.AllowedClasses({ UComboGraphNodeSequence::StaticClass() })
		.DefaultClass(UComboGraphNodeSequence::StaticClass())
		.Title(LOCTEXT("CreateComboGraphNodeSequenceBlueprintOptions", "Create Combo Graph Node Blueprint (Sequence)"));

	return Dialog->ConfigureProperties(this);
}

UObject* UComboGraphNodeSequenceFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return FComboGraphNodeFactoryUtils::FactoryCreateNew(Class, ParentClass, SupportedClass, InParent, Name, Flags, Context, Warn, CallingContext);
}

UObject* UComboGraphNodeSequenceFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

#undef LOCTEXT_NAMESPACE
