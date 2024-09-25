// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Editors/SComboGraphAnimationAssetBrowser.h"

#include "AnimPreviewInstance.h"
#include "ContentBrowserModule.h"
#include "FileHelpers.h"
#include "ComboGraphEditorLog.h"
#include "IContentBrowserSingleton.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "Animation/AnimMontage.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editors/IComboGraphBlueprintEditor.h"
#include "Settings/SkeletalMeshEditorSettings.h"
#include "Slate/SceneViewport.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "ACEAnimationAssetBrowser"

const FString SComboGraphAnimationAssetBrowser::SettingsIniSection = TEXT("SequenceBrowser");
const int32 SComboGraphAnimationAssetBrowser::MaxAssetsHistory = 10;

//////////////////////////////////////////////////////////////////////////
// FComboGraphAnimationAssetViewportClient

FComboGraphAnimationAssetViewportClient::FComboGraphAnimationAssetViewportClient(FPreviewScene& InPreviewScene)
	: FEditorViewportClient(nullptr, &InPreviewScene)
{
	// ReSharper disable once CppVirtualFunctionCallInsideCtor
	SetViewMode(VMI_Lit);

	// Always composite editor objects after post processing in the editor
	EngineShowFlags.SetCompositeEditorPrimitives(true);
	EngineShowFlags.DisableAdvancedFeatures();

	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = true;
	DrawHelper.GridColorAxis = FColor(70, 70, 70);
	DrawHelper.GridColorMajor = FColor(40, 40, 40);
	DrawHelper.GridColorMinor = FColor(20, 20, 20);
	DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;
	bDrawAxes = false;
}

FSceneInterface* FComboGraphAnimationAssetViewportClient::GetScene() const
{
	return PreviewScene->GetScene();
}

FLinearColor FComboGraphAnimationAssetViewportClient::GetBackgroundColor() const
{
	return FLinearColor(0.8f, 0.85f, 0.85f);
}

//////////////////////////////////////////////////////////////////////////
// SComboGraphAnimationAssetBrowser

SComboGraphAnimationAssetBrowser::~SComboGraphAnimationAssetBrowser()
{
	if (PreviewComponent)
	{
		for (int32 ComponentIdx = PreviewComponent->GetAttachChildren().Num() - 1; ComponentIdx >= 0; --ComponentIdx)
		{
			USceneComponent* Component = PreviewComponent->GetAttachChildren()[ComponentIdx];
			if (Component)
			{
				CleanupPreviewSceneComponent(Component);
			}
		}
		check(PreviewComponent->GetAttachChildren().Num() == 0);
	}

	if (ViewportClient.IsValid())
	{
		ViewportClient->Viewport = NULL;
	}
}

void SComboGraphAnimationAssetBrowser::Construct(const FArguments& InArgs, const TSharedRef<IPersonaToolkit>& InPersonaToolkit, const TSharedRef<IComboGraphBlueprintEditor>& InBlueprintEditor)
{
	PersonaToolkitPtr = InPersonaToolkit;
	BlueprintEditorPtr = InBlueprintEditor;

	OnOpenNewAsset = InArgs._OnOpenNewAsset;
	bShowHistory = InArgs._ShowHistory;

	Commands = MakeShareable(new FUICommandList());
	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateSP(this, &SComboGraphAnimationAssetBrowser::FindInContentBrowser),
			FCanExecuteAction::CreateSP(this, &SComboGraphAnimationAssetBrowser::CanFindInContentBrowser)
		)
	);

	CurrentAssetHistoryIndex = INDEX_NONE;
	bTriedToCacheOriginalAsset = false;

	bIsActiveTimerRegistered = false;

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	CreateAssetTooltipResources();

	// Configure filter for asset picker
	Filter.bRecursiveClasses = true;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	Filter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
#else
	Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
	Filter.ClassNames.Add(UAnimMontage::StaticClass()->GetFName());
#endif

	FAssetPickerConfig Config;
	Config.Filter = Filter;
	Config.InitialAssetViewType = EAssetViewType::Column;
	Config.bAddFilterUI = true;
	Config.bShowPathInColumnView = true;
	Config.bSortByPathInColumnView = true;

	// Customize no results text
	Config.AssetShowWarningText = TAttribute<FText>(this, &SComboGraphAnimationAssetBrowser::GetAssetShowWarningText);

	// Configure response to click and double-click
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SComboGraphAnimationAssetBrowser::OnRequestOpenAsset, false);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SComboGraphAnimationAssetBrowser::OnGetAssetContextMenu);
	Config.OnAssetTagWantsToBeDisplayed = FOnShouldDisplayAssetTag::CreateSP(this, &SComboGraphAnimationAssetBrowser::CanShowColumnForAssetRegistryTag);
	Config.SyncToAssetsDelegates.Add(&SyncToAssetsDelegate);
	Config.OnShouldFilterAsset = FOnShouldFilterAsset::CreateSP(this, &SComboGraphAnimationAssetBrowser::HandleFilterAsset);
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.SetFilterDelegates.Add(&SetFilterDelegate);
	Config.RefreshAssetViewDelegates.Add(&RefreshAssetPickerAssetViewDelegate);
	Config.bFocusSearchBoxWhenOpened = false;
	Config.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;

	Config.SaveSettingsName = SettingsIniSection;

	Config.OnIsAssetValidForCustomToolTip = FOnIsAssetValidForCustomToolTip::CreateLambda([](const FAssetData& AssetData) { return AssetData.IsAssetLoaded(); });
	Config.OnGetCustomAssetToolTip = FOnGetCustomAssetToolTip::CreateSP(this, &SComboGraphAnimationAssetBrowser::CreateCustomAssetToolTip);
	Config.OnVisualizeAssetToolTip = FOnVisualizeAssetToolTip::CreateSP(this, &SComboGraphAnimationAssetBrowser::OnVisualizeAssetToolTip);
	Config.OnAssetToolTipClosing = FOnAssetToolTipClosing::CreateSP( this, &SComboGraphAnimationAssetBrowser::OnAssetToolTipClosing );

	// hide all asset registry columns by default (we only really want the name and path)
	TArray<UObject::FAssetRegistryTag> AssetRegistryTags;
	UAnimSequence::StaticClass()->GetDefaultObject()->GetAssetRegistryTags(AssetRegistryTags);
	for(UObject::FAssetRegistryTag& AssetRegistryTag : AssetRegistryTags)
	{
		Config.HiddenColumnNames.Add(AssetRegistryTag.Name.ToString());
	}

	// Also hide the type column by default (but allow users to enable it, so don't use bShowTypeInColumnView)
	Config.HiddenColumnNames.Add(TEXT("Class"));

	static const FName DefaultForegroundName("DefaultForeground");

	const TSharedRef<SMenuAnchor> BackMenuAnchorPtr = SNew(SMenuAnchor)
		.Placement(MenuPlacement_BelowAnchor)
		.OnGetMenuContent(this, &SComboGraphAnimationAssetBrowser::CreateHistoryMenu, true)
		[
			SNew(SButton)
			.OnClicked(this, &SComboGraphAnimationAssetBrowser::OnGoBackInHistory)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
#else
			.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
#endif
			.ButtonStyle(FAppStyle::Get(), "FlatButton")
			.ContentPadding(FMargin(1, 0))
			.IsEnabled(this, &SComboGraphAnimationAssetBrowser::CanStepBackwardInHistory)
			.ToolTipText(LOCTEXT("Backward_Tooltip", "Step backward in the asset history. Right click to see full history."))
			[
				SNew(STextBlock)
				// .TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
				.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
				// .Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
				.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
				.Text(FText::FromString(FString(TEXT("\xf060"))) /*fa-arrow-left*/)
			]
		];

	const TSharedRef<SMenuAnchor> FwdMenuAnchorPtr = SNew(SMenuAnchor)
		.Placement(MenuPlacement_BelowAnchor)
		.OnGetMenuContent(this, &SComboGraphAnimationAssetBrowser::CreateHistoryMenu, false)
		[
			SNew(SButton)
			.OnClicked(this, &SComboGraphAnimationAssetBrowser::OnGoForwardInHistory)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
#else
			.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
#endif
			.ButtonStyle(FAppStyle::Get(), "FlatButton")
			.ContentPadding(FMargin(1, 0))
			.IsEnabled(this, &SComboGraphAnimationAssetBrowser::CanStepForwardInHistory)
			.ToolTipText(LOCTEXT("Forward_Tooltip", "Step forward in the asset history. Right click to see full history."))
			[
				SNew(STextBlock)
				// .TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
				.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
				// .Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
				.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
				.Text(FText::FromString(FString(TEXT("\xf061"))) /*fa-arrow-right*/)
			]
		];

	const TSharedRef<SButton> RefreshAnchorPtr = SNew(SButton)
		.OnClicked(this, &SComboGraphAnimationAssetBrowser::OnRefreshRequested)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
#else
		.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
#endif
		.ButtonStyle(FAppStyle::Get(), "FlatButton")
		.ContentPadding(FMargin(1, 0))
		.ToolTipText(LOCTEXT("Refresh_Tooltip", "Refresh forward in the asset history. Right click to see full history."))
		[
			SNew(STextBlock)
			// .TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
			.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
			// .Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
			.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
			.Text(FText::FromString(FString(TEXT("\xf061"))) /*fa-arrow-right*/)
		];


	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Visibility(this, &SComboGraphAnimationAssetBrowser::GetHistoryVisibility)
			.Padding(FMargin(3))
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
#else
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
#endif
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.OnMouseButtonDown(this, &SComboGraphAnimationAssetBrowser::OnMouseDownHistory, TWeakPtr<SMenuAnchor>(BackMenuAnchorPtr))
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.BorderImage(FAppStyle::GetBrush("NoBorder"))
#else
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
#endif
						[
							BackMenuAnchorPtr
						]
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
						.OnMouseButtonDown(this, &SComboGraphAnimationAssetBrowser::OnMouseDownHistory, TWeakPtr<SMenuAnchor>(FwdMenuAnchorPtr))
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.BorderImage(FAppStyle::GetBrush("NoBorder"))
#else
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
#endif
						[
							FwdMenuAnchorPtr
						]
					]


					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.BorderImage(FAppStyle::GetBrush("NoBorder"))
#else
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
#endif
						[
							RefreshAnchorPtr
						]
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
#else
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
#endif
			[
				ContentBrowserModule.Get().CreateAssetPicker(Config)
			]
		]
	];

	// Create the ignore set for asset registry tags
	// Making Skeleton to be private, and now GET_MEMBER_NAME_CHECKED doesn't work
	AssetRegistryTagsToIgnore.Add(TEXT("Skeleton"));

	// UE5: Making public use of SequenceLength to be deprecated, and now GET_MEMBER_NAME_CHECKED doesn't work
	AssetRegistryTagsToIgnore.Add(TEXT("SequenceLength"));
	// AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, SequenceLength));
	AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, RateScale));
}

void SComboGraphAnimationAssetBrowser::OnRequestOpenAsset(const FAssetData& AssetData, const bool bFromHistory)
{
	if (UObject* RawAsset = AssetData.GetAsset())
	{
		if (UAnimationAsset* AnimationAsset = Cast<UAnimationAsset>(RawAsset))
		{
			if (!bFromHistory)
			{
				AddAssetToHistory(AssetData);
			}

			OnOpenNewAsset.ExecuteIfBound(AnimationAsset);
		}
	}
}

TSharedPtr<SWidget> SComboGraphAnimationAssetBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/ true, Commands);

	MenuBuilder.BeginSection("AnimationSequenceOptions", LOCTEXT("OptionsHeading", "Options") );
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SaveSelectedAssets", "Save"),
		LOCTEXT("SaveSelectedAssets_ToolTip", "Save the selected assets"),
		// FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.SaveIcon16x"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Level.SaveIcon16x"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SComboGraphAnimationAssetBrowser::SaveSelectedAssets, SelectedAssets),
			FCanExecuteAction::CreateSP(this, &SComboGraphAnimationAssetBrowser::CanSaveSelectedAssets, SelectedAssets)
		)
	);

	MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SComboGraphAnimationAssetBrowser::SelectAsset(UAnimationAsset* AnimAsset)
{
	const FAssetData AssetData(AnimAsset);

	if (AssetData.IsValid())
	{
		const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();

		if (!CurrentSelection.Contains(AssetData))
		{
			TArray<FAssetData> AssetsToSelect;
			AssetsToSelect.Add(AssetData);

			SyncToAssetsDelegate.Execute(AssetsToSelect);
		}
	}
}

void SComboGraphAnimationAssetBrowser::AddToHistory(UAnimationAsset* AnimAsset)
{
	if (AnimAsset)
	{
		const FAssetData AssetData(AnimAsset);
		AddAssetToHistory(AssetData);
	}
}

void SComboGraphAnimationAssetBrowser::FilterBySkeletonNotify(const FName& InNotifyName)
{
	// noop
}

void SComboGraphAnimationAssetBrowser::FindInContentBrowser()
{
	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
}

bool SComboGraphAnimationAssetBrowser::CanFindInContentBrowser() const
{
	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SComboGraphAnimationAssetBrowser::SaveSelectedAssets(const TArray<FAssetData> ObjectsToSave) const
{
	TArray<UPackage*> PackagesToSave;
	GetSelectedPackages(ObjectsToSave, PackagesToSave);

	constexpr bool bCheckDirty = false;
	constexpr bool bPromptToSave = false;
	const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
}

bool SComboGraphAnimationAssetBrowser::CanSaveSelectedAssets(const TArray<FAssetData> ObjectsToSave) const
{
	TArray<UPackage*> Packages;
	GetSelectedPackages(ObjectsToSave, Packages);
	// Don't offer save option if none of the packages are loaded
	return Packages.Num() > 0;
}

void SComboGraphAnimationAssetBrowser::HandlePreviewMeshChanged(USkeletalMesh* InPreviewMesh)
{
	CG_EDITOR_LOG(Verbose, TEXT("SComboGraphAnimationAssetBrowser HandlePreviewMeshChanged ... %s"), *GetNameSafe(InPreviewMesh))
}

void SComboGraphAnimationAssetBrowser::RefreshAssetView()
{
	RefreshAssetPickerAssetViewDelegate.Execute(true);
}

void SComboGraphAnimationAssetBrowser::GetSelectedPackages(const TArray<FAssetData>& Assets, TArray<UPackage*>& OutPackages) const
{
	for (int32 AssetIdx = 0; AssetIdx < Assets.Num(); ++AssetIdx)
	{
		UPackage* Package = FindPackage(nullptr, *Assets[AssetIdx].PackageName.ToString());

		if ( Package )
		{
			OutPackages.Add(Package);
		}
	}
}

void SComboGraphAnimationAssetBrowser::CreateAssetTooltipResources()
{
	SAssignNew(ViewportWidget, SViewport)
		.EnableGammaCorrection(false)
		.ViewportSize(FVector2D(128, 128));

	ViewportClient = MakeShareable(new FComboGraphAnimationAssetViewportClient(PreviewScene));
	SceneViewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), ViewportWidget));
	PreviewComponent = NewObject<UDebugSkelMeshComponent>();

	// Client options
	ViewportClient->ViewportType = LVT_Perspective;
	ViewportClient->bSetListenerPosition = false;
	// Default view until we need to show the viewport
	ViewportClient->SetViewLocation(EditorViewportDefs::DefaultPerspectiveViewLocation);
	ViewportClient->SetViewRotation(EditorViewportDefs::DefaultPerspectiveViewRotation);

	ViewportClient->Viewport = SceneViewport.Get();
	ViewportClient->SetRealtime(true);
	ViewportClient->SetViewMode(VMI_Lit);
	ViewportClient->ToggleOrbitCamera(true);
	ViewportClient->VisibilityDelegate.BindSP(this, &SComboGraphAnimationAssetBrowser::IsToolTipPreviewVisible);

	// Add the scene viewport
	ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());

	// Setup the preview component to ensure an animation will update when requested
	PreviewComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	PreviewScene.AddComponent(PreviewComponent, FTransform::Identity);

	const USkeletalMeshEditorSettings* Options = GetDefault<USkeletalMeshEditorSettings>();

	PreviewScene.SetLightDirection(Options->AnimPreviewLightingDirection);
	PreviewScene.SetLightColor(Options->AnimPreviewDirectionalColor);
	PreviewScene.SetLightBrightness(Options->AnimPreviewLightBrightness);
}

void SComboGraphAnimationAssetBrowser::AddAssetToHistory(const FAssetData& AssetData)
{
	CacheOriginalAnimAssetHistory();

	if (CurrentAssetHistoryIndex == AssetHistory.Num() - 1)
	{
		// History added to the end
		if (AssetHistory.Num() == MaxAssetsHistory)
		{
			// If max history entries has been reached
			// remove the oldest history
			AssetHistory.RemoveAt(0);
		}
	}
	else
	{
		// Clear out any history that is in front of the current location in the history list
		AssetHistory.RemoveAt(CurrentAssetHistoryIndex + 1, AssetHistory.Num() - (CurrentAssetHistoryIndex + 1), true);
	}

	AssetHistory.Add(AssetData);
	CurrentAssetHistoryIndex = AssetHistory.Num() - 1;
}

void SComboGraphAnimationAssetBrowser::CacheOriginalAnimAssetHistory()
{
	// If we have nothing in the AssetHistory see if we can store
	// anything for where we currently are as we can't do this on construction

	if (!bTriedToCacheOriginalAsset)
	{
		bTriedToCacheOriginalAsset = true;

		if (AssetHistory.Num() == 0)
		{
			if (const UObject* PreviewAsset = PersonaToolkitPtr.Pin()->GetPreviewScene()->GetPreviewAnimationAsset())
			{
				const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(PreviewAsset));
#else
				const FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName(*PreviewAsset->GetPathName()));
#endif
				AssetHistory.Add(AssetData);
				CurrentAssetHistoryIndex = AssetHistory.Num() - 1;
			}
		}
	}
}

bool SComboGraphAnimationAssetBrowser::IsToolTipPreviewVisible()
{
	bool bVisible = false;

	if (ViewportWidget.IsValid())
	{
		bVisible = ViewportWidget->GetVisibility() == EVisibility::Visible;
	}

	return bVisible;
}

bool SComboGraphAnimationAssetBrowser::CanShowColumnForAssetRegistryTag(FName AssetType, const FName TagName) const
{
	return !AssetRegistryTagsToIgnore.Contains(TagName);
}

bool SComboGraphAnimationAssetBrowser::HandleFilterAsset(const FAssetData& InAssetData) const
{
	if (InAssetData.GetClass()->IsChildOf(UAnimSequenceBase::StaticClass()))
	{
		USkeleton* PreviewSkeleton = BlueprintEditorPtr.Pin()->GetSkeleton();
		if (PreviewSkeleton)
		{
			const FString SkeletonString = FAssetData(PreviewSkeleton).GetExportTextName();
			#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 26
				return (InAssetData.TagsAndValues.FindRef(TEXT("Skeleton")) != SkeletonString);
			#else
				return (InAssetData.TagsAndValues.FindTag(TEXT("Skeleton")) != SkeletonString);
			#endif
		}

		return false;
	}

	return true;
}

TSharedRef<SToolTip> SComboGraphAnimationAssetBrowser::CreateCustomAssetToolTip(FAssetData& AssetData)
{
	// Make a list of tags to show
	TArray<UObject::FAssetRegistryTag> Tags;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const UClass* AssetClass = FindObject<UClass>(AssetData.AssetClassPath);
#else
	UClass* AssetClass = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());
#endif
	check(AssetClass);
	AssetClass->GetDefaultObject()->GetAssetRegistryTags(Tags);

	TArray<FName> TagsToShow;
	for (UObject::FAssetRegistryTag& TagEntry : Tags)
	{
		if (TagEntry.Name != FName(TEXT("Skeleton")) && TagEntry.Type != UObject::FAssetRegistryTag::TT_Hidden)
		{
			TagsToShow.Add(TagEntry.Name);
		}
	}

	// Add asset registry tags to a text list; except skeleton as that is implied in Persona
	const TSharedRef<SVerticalBox> DescriptionBox = SNew(SVerticalBox);

	#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 26
	for(TPair<FName, FString> TagPair : AssetData.TagsAndValues)
	#else
	for (TPair<FName, FAssetTagValueRef> TagPair : AssetData.TagsAndValues)
	#endif
	{
		if (TagsToShow.Contains(TagPair.Key))
		{
			// Check for DisplayName metadata
			FText DisplayName;
			if (const FProperty* Field = FindFProperty<FProperty>(AssetClass, TagPair.Key))
			{
				DisplayName = Field->GetDisplayNameText();
			}
			else
			{
				DisplayName = FText::FromName(TagPair.Key);
			}


			#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 26
			FText TagValue = FText::FromString(TagPair.Value);
			#else
			FText TagValue = TagPair.Value.AsText();
			#endif

			DescriptionBox->AddSlot()
			.AutoHeight()
			.Padding(0,0,5,0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("AssetTagKey", "{0}: "), DisplayName))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(TagValue)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
		}
	}

	DescriptionBox->AddSlot()
		.AutoHeight()
		.Padding(0,0,5,0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetBrowser_FolderPathLabel", "Folder :"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromName(AssetData.PackagePath))
				.ColorAndOpacity(FSlateColor::UseForeground())
				.WrapTextAt(300.f)
			]
		];

	TSharedPtr<SHorizontalBox> ContentBox = nullptr;
	TSharedRef<SToolTip> ToolTipWidget = SNew(SToolTip)
	.TextMargin(1)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
#else
	.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
#endif
	[
		SNew(SBorder)
		.Padding(6)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
#else
		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
#endif
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,0,0,4)
			[
				SNew(SBorder)
				.Padding(6)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#endif
				[
					SNew(SBox)
					.HAlign(HAlign_Left)
					[
						SNew(STextBlock)
						.Text(FText::FromName(AssetData.AssetName))
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
						.Font(FAppStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
#else
						.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
#endif
					]
				]
			]

			+ SVerticalBox::Slot()
			[
				SAssignNew(ContentBox, SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBorder)
					.Padding(6)
					.Visibility(AssetClass->IsChildOf<UAnimationAsset>() ? EVisibility::Visible : EVisibility::Collapsed)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
					.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#endif
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("NoPreviewMesh", "No Preview Mesh"))
						]

						+ SOverlay::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							ViewportWidget.ToSharedRef()
						]
					]
				]
			]
		]
	];

	// add an extra section to the tooltip for it.
	ContentBox->AddSlot()
	.Padding(AssetClass->IsChildOf<UAnimationAsset>() ? 4 : 0, 0, 0, 0)
	[
		SNew(SBorder)
		.Padding(6)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#else
		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
#endif
		[
			DescriptionBox
		]
	];

	return ToolTipWidget;
}

bool SComboGraphAnimationAssetBrowser::OnVisualizeAssetToolTip(const TSharedPtr<SWidget>& TooltipContent, FAssetData& AssetData)
{
	// Resolve the asset
	USkeletalMesh* MeshToUse = nullptr;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	const UClass* AssetClass = FindObject<UClass>(AssetData.AssetClassPath);
#else
	const UClass* AssetClass = FindObject<UClass>(ANY_PACKAGE, *AssetData.AssetClass.ToString());
#endif
	if (AssetClass->IsChildOf(UAnimationAsset::StaticClass()) && AssetData.IsAssetLoaded() && AssetData.GetAsset())
	{
		// Set up the viewport to show the asset. Catching the visualize allows us to use
		// one viewport between all of the assets in the sequence browser.
		UAnimationAsset* Asset = StaticCast<UAnimationAsset*>(AssetData.GetAsset());
		USkeleton* Skeleton = Asset->GetSkeleton();
		if (Skeleton)
		{
			MeshToUse = Skeleton->GetAssetPreviewMesh(Asset);
		}

		if (MeshToUse)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			if (PreviewComponent->GetSkeletalMeshAsset() != MeshToUse)
			{
				PreviewComponent->SetSkeletalMesh(MeshToUse);
			}
#else
			if (PreviewComponent->SkeletalMesh != MeshToUse)
			{
				PreviewComponent->SetSkeletalMesh(MeshToUse);
			}
#endif

			PreviewComponent->EnablePreview(true, Asset);
			PreviewComponent->PreviewInstance->PlayAnim(true);

			const FBoxSphereBounds MeshImportedBounds = MeshToUse->GetImportedBounds();
			const float HalfFov = FMath::DegreesToRadians(ViewportClient->ViewFOV) / 2.0f;
			const float TargetDist = MeshImportedBounds.SphereRadius / FMath::Tan(HalfFov);

			ViewportClient->SetViewRotation(FRotator(0.0f, -45.0f, 0.0f));
			ViewportClient->SetViewLocationForOrbiting(FVector(0.0f, 0.0f, MeshImportedBounds.BoxExtent.Z / 2.0f), TargetDist);

			ViewportWidget->SetVisibility(EVisibility::Visible);

			// Update the preview as long as the tooltip is visible
			if (!bIsActiveTimerRegistered)
			{
				bIsActiveTimerRegistered = true;
				RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SComboGraphAnimationAssetBrowser::UpdateTooltipPreview));
			}
		}
		else
		{
			ViewportWidget->SetVisibility(EVisibility::Hidden);
		}
	}

	// We return false here as we aren't visualizing the tooltip - just detecting when it is about to be shown.
	// We still want slate to draw it.
	return false;
}

void SComboGraphAnimationAssetBrowser::OnAssetToolTipClosing()
{
	// Noop
}

bool SComboGraphAnimationAssetBrowser::CanStepBackwardInHistory() const
{
	int32 HistoryIdx = CurrentAssetHistoryIndex - 1;
	while (HistoryIdx >= 0)
	{
		if (AssetHistory[HistoryIdx].IsValid())
		{
			return true;
		}

		--HistoryIdx;
	}
	return false;
}

bool SComboGraphAnimationAssetBrowser::CanStepForwardInHistory() const
{
	int32 HistoryIdx = CurrentAssetHistoryIndex + 1;
	while (HistoryIdx < AssetHistory.Num())
	{
		if (AssetHistory[HistoryIdx].IsValid())
		{
			return true;
		}

		++HistoryIdx;
	}
	return false;
}

FReply SComboGraphAnimationAssetBrowser::OnMouseDownHistory(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, const TWeakPtr<SMenuAnchor> InMenuAnchor)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		InMenuAnchor.Pin()->SetIsOpen(true);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> SComboGraphAnimationAssetBrowser::CreateHistoryMenu(const bool bInBackHistory) const
{
	FMenuBuilder MenuBuilder(true, nullptr);
	if (bInBackHistory)
	{
		int32 HistoryIdx = CurrentAssetHistoryIndex - 1;
		while (HistoryIdx >= 0)
		{
			const FAssetData& AssetData = AssetHistory[HistoryIdx];

			if (AssetData.IsValid())
			{
				const FText DisplayName = FText::FromName(AssetData.AssetName);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				const FText Tooltip = FText::FromString(AssetData.GetObjectPathString());
#else
				const FText Tooltip = FText::FromString(AssetData.ObjectPath.ToString());
#endif

				MenuBuilder.AddMenuEntry(
					DisplayName,
					Tooltip,
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(const_cast<SComboGraphAnimationAssetBrowser*>(this), &SComboGraphAnimationAssetBrowser::GoToHistoryIndex, HistoryIdx)
					),
					NAME_None,
					EUserInterfaceActionType::Button
				);
			}

			--HistoryIdx;
		}
	}
	else
	{
		int32 HistoryIdx = CurrentAssetHistoryIndex + 1;
		while (HistoryIdx < AssetHistory.Num())
		{
			const FAssetData& AssetData = AssetHistory[HistoryIdx];

			if (AssetData.IsValid())
			{
				const FText DisplayName = FText::FromName(AssetData.AssetName);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				const FText Tooltip = FText::FromString(AssetData.GetObjectPathString());
#else
				const FText Tooltip = FText::FromString(AssetData.ObjectPath.ToString());
#endif

				MenuBuilder.AddMenuEntry(
					DisplayName,
					Tooltip,
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateRaw(const_cast<SComboGraphAnimationAssetBrowser*>(this), &SComboGraphAnimationAssetBrowser::GoToHistoryIndex, HistoryIdx)
					),
					NAME_None,
					EUserInterfaceActionType::Button
				);
			}

			++HistoryIdx;
		}
	}

	return MenuBuilder.MakeWidget();
}

void SComboGraphAnimationAssetBrowser::GoToHistoryIndex(const int32 InHistoryIdx)
{
	if (AssetHistory[InHistoryIdx].IsValid())
	{
		CurrentAssetHistoryIndex = InHistoryIdx;
		OnRequestOpenAsset(AssetHistory[InHistoryIdx], /**bFromHistory=*/true);
	}
}

FReply SComboGraphAnimationAssetBrowser::OnGoForwardInHistory()
{
	while (CurrentAssetHistoryIndex < AssetHistory.Num() - 1)
	{
		++CurrentAssetHistoryIndex;

		if (AssetHistory[CurrentAssetHistoryIndex].IsValid())
		{
			GoToHistoryIndex(CurrentAssetHistoryIndex);
			break;
		}
	}

	return FReply::Handled();
}

FReply SComboGraphAnimationAssetBrowser::OnGoBackInHistory()
{
	while (CurrentAssetHistoryIndex > 0)
	{
		--CurrentAssetHistoryIndex;

		if (AssetHistory[CurrentAssetHistoryIndex].IsValid())
		{
			GoToHistoryIndex(CurrentAssetHistoryIndex);
			break;
		}
	}

	return FReply::Handled();
}

FReply SComboGraphAnimationAssetBrowser::OnRefreshRequested()
{
	RefreshAssetPickerAssetViewDelegate.Execute(true);
	return FReply::Handled();
}

EVisibility SComboGraphAnimationAssetBrowser::GetHistoryVisibility() const
{
	return bShowHistory ? EVisibility::Visible : EVisibility::Collapsed;
}

void SComboGraphAnimationAssetBrowser::CleanupPreviewSceneComponent(USceneComponent* Component)
{
	if (Component)
	{
		for (int32 ComponentIdx = Component->GetAttachChildren().Num() - 1; ComponentIdx >= 0; --ComponentIdx)
		{
			USceneComponent* ChildComponent = Component->GetAttachChildren()[ComponentIdx];
			CleanupPreviewSceneComponent(ChildComponent);
		}
		check(Component->GetAttachChildren().Num() == 0);
		Component->DestroyComponent();
	}
}

FText SComboGraphAnimationAssetBrowser::GetAssetShowWarningText() const
{
	const FText NothingToShowText = LOCTEXT("NothingToShowCheckFilter", "No results, check your filters and / or preview mesh.");

	FText MeshText, SkeletonText;
	USkeletalMesh* PreviewMesh = BlueprintEditorPtr.Pin()->GetPreviewMesh();
	if (PreviewMesh)
	{
		MeshText = FText::Format(LOCTEXT("NothingToShowCheckFilterSkeleton", "Current Preview Mesh: {0}"), FText::FromString(PreviewMesh->GetName()));
	}

	USkeleton* PreviewSkeleton = BlueprintEditorPtr.Pin()->GetSkeleton();
	if (PreviewSkeleton)
	{
		SkeletonText = FText::Format(LOCTEXT("NothingToShowCheckFilterSkeleton", "Current Skeleton: {0} (from Preview Mesh)"), FText::FromString(PreviewSkeleton->GetName()));
	}


	return FText::Format(LOCTEXT("NothingToShowPattern", "{0}\n\n{1}"), NothingToShowText, SkeletonText);
}

EActiveTimerReturnType SComboGraphAnimationAssetBrowser::UpdateTooltipPreview(double InCurrentTime, const float InDeltaTime)
{
	if (PreviewComponent && IsToolTipPreviewVisible() && ViewportWidget->IsParentValid())
	{
		// Tick the world to update preview viewport for tooltips
		PreviewComponent->GetScene()->GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}
	else
	{
		bIsActiveTimerRegistered = false;
		return EActiveTimerReturnType::Stop;
	}

	return EActiveTimerReturnType::Continue;
}

#undef LOCTEXT_NAMESPACE
