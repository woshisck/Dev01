#include "SStylizedEmissiveLibraryWidget.h"

#include "ActorFactories/ActorFactoryStylizedEmissivePreset.h"
#include "Actors/StylizedEmissiveLight.h"
#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetData.h"
#include "CelesLightEditorLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "ContentBrowserModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "EditorViewportClient.h"
#include "Engine/StaticMesh.h"
#include "FileHelpers.h"
#include "IDetailsView.h"
#include "IContentBrowserSingleton.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SEditorViewport.h"
#include "Styling/AppStyle.h"
#include "StylizedEmissiveModelLibrary.h"
#include "StylizedEmissivePresetEditProxy.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "StylizedEmissiveLibraryWidget"

namespace
{
	FText GetEntryDisplayName(const FStylizedEmissiveModelEntry& Entry)
	{
		return Entry.DisplayName.IsEmpty() ? FText::FromString(Entry.ModelId.ToString()) : Entry.DisplayName;
	}

	FText GetLightingOutputText(const EStylizedEmissiveLightingOutput Output)
	{
		switch (Output)
		{
		case EStylizedEmissiveLightingOutput::LumenGI:
			return LOCTEXT("OutputLumen", "Lumen GI");
		case EStylizedEmissiveLightingOutput::Hybrid:
			return LOCTEXT("OutputHybrid", "风格化 + Lumen");
		default:
			return LOCTEXT("OutputStylized", "风格化材质光");
		}
	}
}

class SStylizedEmissivePreviewViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SStylizedEmissivePreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
		PreviewScene->SetFloorVisibility(true);

		PreviewMeshComponent = TStrongObjectPtr<UStaticMeshComponent>(NewObject<UStaticMeshComponent>(GetTransientPackage()));
		PreviewMeshComponent->SetMobility(EComponentMobility::Movable);
		PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewMeshComponent->SetCastShadow(false);
		PreviewScene->AddComponent(PreviewMeshComponent.Get(), FTransform::Identity);

		SEditorViewport::Construct(SEditorViewport::FArguments());
	}

	void SetPreset(const FStylizedEmissiveModelEntry* Entry)
	{
		if (!PreviewMeshComponent.IsValid())
		{
			return;
		}

		UStaticMesh* Mesh = Entry && Entry->bUseMesh ? Entry->Mesh.Get() : nullptr;
		PreviewMeshComponent->SetStaticMesh(Mesh);
		PreviewMeshComponent->SetRelativeTransform(Entry ? Entry->RelativeTransform : FTransform::Identity);
		PreviewMaterial.Reset();

		if (Mesh && Entry && Entry->Material)
		{
			UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(Entry->Material, GetTransientPackage());
			PreviewMaterial = TStrongObjectPtr<UMaterialInstanceDynamic>(MaterialInstance);
			if (MaterialInstance)
			{
				MaterialInstance->SetVectorParameterValue(TEXT("Emissive Color"), Entry->LightColor);
				MaterialInstance->SetScalarParameterValue(TEXT("Emissive Intensity"), FMath::Max(0.0f, Entry->EmissiveIntensity));
				PreviewMeshComponent->SetMaterial(0, MaterialInstance);
			}
		}
		else
		{
			PreviewMeshComponent->SetMaterial(0, nullptr);
		}

		PreviewMeshComponent->UpdateBounds();
		PreviewMeshComponent->MarkRenderStateDirty();
		if (Mesh)
		{
			FocusMesh(Mesh, Entry ? Entry->RelativeTransform : FTransform::Identity);
		}
		Invalidate();
	}

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
	{
		ViewportClient = MakeShared<FEditorViewportClient>(nullptr, PreviewScene.Get(), SharedThis(this));
		ViewportClient->ViewportType = LVT_Perspective;
		ViewportClient->SetViewMode(VMI_Lit);
		ViewportClient->SetRealtime(true);
		ViewportClient->EngineShowFlags.SetSelectionOutline(false);
		ViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
		return ViewportClient.ToSharedRef();
	}

private:
	void FocusMesh(const UStaticMesh* Mesh, const FTransform& RelativeTransform)
	{
		if (!ViewportClient || !Mesh)
		{
			return;
		}

		const FBoxSphereBounds Bounds = Mesh->GetBounds().TransformBy(RelativeTransform);
		const float Radius = FMath::Max(25.0f, Bounds.SphereRadius);
		const FVector Target = Bounds.Origin;
		const FVector ViewLocation = Target + FVector(-Radius * 2.4f, Radius * 1.6f, Radius * 1.1f);
		ViewportClient->SetLookAtLocation(Target);
		ViewportClient->SetViewLocation(ViewLocation);
		ViewportClient->SetViewRotation((Target - ViewLocation).Rotation());
		ViewportClient->Invalidate();
	}

	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FEditorViewportClient> ViewportClient;
	TStrongObjectPtr<UStaticMeshComponent> PreviewMeshComponent;
	TStrongObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
};

class SStylizedEmissivePresetDragCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStylizedEmissivePresetDragCard) {}
		SLATE_ARGUMENT(TSharedPtr<FStylizedEmissivePresetListItem>, Item)
		SLATE_ARGUMENT(TWeakPtr<SStylizedEmissiveLibraryWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
			.Padding(FMargin(8.0f, 6.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(Item.IsValid() ? Item->DisplayName : FText::GetEmpty())
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(Item.IsValid()
						? FText::FromString(Item->bUsesMesh ? Item->MeshName : TEXT("无 Mesh · 数据光源"))
						: FText::GetEmpty())
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Item.IsValid())
		{
			if (const TSharedPtr<SStylizedEmissiveLibraryWidget> Owner = OwnerWidget.Pin())
			{
				Owner->SelectPresetByIndex(Item->EntryIndex);
			}
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (Item.IsValid())
		{
			if (const TSharedPtr<SStylizedEmissiveLibraryWidget> Owner = OwnerWidget.Pin())
			{
				return Owner->BeginPresetDrag(Item->EntryIndex);
			}
		}
		return FReply::Unhandled();
	}

private:
	TSharedPtr<FStylizedEmissivePresetListItem> Item;
	TWeakPtr<SStylizedEmissiveLibraryWidget> OwnerWidget;
};

void SStylizedEmissiveLibraryWidget::Construct(const FArguments& InArgs)
{
	Library = UCelesLightEditorLibrary::GetOrCreateStylizedEmissiveModelLibrary();
	EditProxy = NewObject<UStylizedEmissivePresetEditProxy>(GetTransientPackage());
	EditProxyRoot = TStrongObjectPtr<UObject>(EditProxy);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	DetailsView->SetObject(EditProxy);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SStylizedEmissiveLibraryWidget::HandleFinishedChangingProperties);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildHeader()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.27f)
			[
				BuildPresetPanel()
			]
			+ SSplitter::Slot()
			.Value(0.40f)
			[
				BuildPreviewPanel()
			]
			+ SSplitter::Slot()
			.Value(0.33f)
			[
				BuildSettingsPanel()
			]
		]
	];

	RefreshPresetItems();
}

TSharedRef<SWidget> SStylizedEmissiveLibraryWidget::BuildHeader()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ToolTitle", "风格化自发光库"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ToolDescription", "在这里配置模型、自发光材质和灯光参数。保存后，从左侧拖动预设到关卡视口即可直接生成可用 Actor；无 Mesh 预设只提供风格化材质光照数据。"))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

TSharedRef<SWidget> SStylizedEmissiveLibraryWidget::BuildPresetPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PresetLibraryTitle", "预设库（拖动到场景）"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(LOCTEXT("SearchPresetHint", "搜索预设、模型"))
				.OnTextChanged(this, &SStylizedEmissiveLibraryWidget::HandleSearchChanged)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(PresetListView, SListView<FPresetItemPtr>)
				.ListItemsSource(&PresetItems)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SStylizedEmissiveLibraryWidget::GeneratePresetRow)
				.OnSelectionChanged(this, &SStylizedEmissiveLibraryWidget::HandlePresetSelectionChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton).Text(LOCTEXT("NewPreset", "新建")).OnClicked(this, &SStylizedEmissiveLibraryWidget::CreatePreset)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SButton).Text(LOCTEXT("DuplicatePreset", "复制")).IsEnabled(this, &SStylizedEmissiveLibraryWidget::HasSelectedPreset).OnClicked(this, &SStylizedEmissiveLibraryWidget::DuplicatePreset)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("DeletePreset", "删除")).IsEnabled(this, &SStylizedEmissiveLibraryWidget::HasSelectedPreset).OnClicked(this, &SStylizedEmissiveLibraryWidget::DeletePreset)
				]
			]
		];
}

TSharedRef<SWidget> SStylizedEmissiveLibraryWidget::BuildPreviewPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(this, &SStylizedEmissiveLibraryWidget::GetSelectedPresetTitle)
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(this, &SStylizedEmissiveLibraryWidget::GetSelectedPresetSummary)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(PreviewViewport, SStylizedEmissivePreviewViewport)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.Visibility(this, &SStylizedEmissiveLibraryWidget::GetDataOnlyMessageVisibility)
					.Padding(12.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DataOnlyPreview", "无 Mesh 数据预设\n运行时不显示物体，只输出风格化光照数据"))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("PlacePreset", "放置到当前视口"))
					.ToolTipText(LOCTEXT("PlacePresetTooltip", "在当前编辑器视口前方生成一次；也可以直接从左侧拖动到场景中的准确位置。"))
					.IsEnabled(this, &SStylizedEmissiveLibraryWidget::HasSelectedPreset)
					.OnClicked(this, &SStylizedEmissiveLibraryWidget::PlaceSelectedPreset)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(this, &SStylizedEmissiveLibraryWidget::GetStatusText)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
}

TSharedRef<SWidget> SStylizedEmissiveLibraryWidget::BuildSettingsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PresetSettingsTitle", "预设配置"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 6.0f)
			[
				DetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveLibrary", "保存预设库"))
					.IsEnabled_Lambda([this]() { return Library.IsValid(); })
					.OnClicked(this, &SStylizedEmissiveLibraryWidget::SaveLibrary)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ShowLibraryAsset", "在内容浏览器定位"))
					.IsEnabled_Lambda([this]() { return Library.IsValid(); })
					.OnClicked(this, &SStylizedEmissiveLibraryWidget::ShowLibraryInContentBrowser)
				]
			]
		];
}

TSharedRef<ITableRow> SStylizedEmissiveLibraryWidget::GeneratePresetRow(FPresetItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FPresetItemPtr>, OwnerTable)
		.Padding(2.0f)
		[
			SNew(SStylizedEmissivePresetDragCard)
			.Item(Item)
			.OwnerWidget(SharedThis(this))
		];
}

void SStylizedEmissiveLibraryWidget::RefreshPresetItems(const FName PresetToSelect)
{
	PresetItems.Reset();
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject)
	{
		return;
	}

	for (int32 Index = 0; Index < LibraryObject->Models.Num(); ++Index)
	{
		const FStylizedEmissiveModelEntry& Entry = LibraryObject->Models[Index];
		const FString Searchable = FString::Printf(TEXT("%s %s %s"), *Entry.ModelId.ToString(), *GetEntryDisplayName(Entry).ToString(), Entry.Mesh ? *Entry.Mesh->GetName() : TEXT(""));
		if (!SearchFilter.IsEmpty() && !Searchable.Contains(SearchFilter, ESearchCase::IgnoreCase))
		{
			continue;
		}

		FPresetItemPtr Item = MakeShared<FStylizedEmissivePresetListItem>();
		Item->EntryIndex = Index;
		Item->PresetId = Entry.ModelId;
		Item->DisplayName = GetEntryDisplayName(Entry);
		Item->bUsesMesh = Entry.bUseMesh && Entry.Mesh != nullptr;
		Item->MeshName = Entry.Mesh ? Entry.Mesh->GetName() : FString();
		PresetItems.Add(Item);
	}

	if (PresetListView)
	{
		PresetListView->RequestListRefresh();
		const FName TargetPreset = PresetToSelect.IsNone() && HasSelectedPreset()
			? LibraryObject->Models[SelectedEntryIndex].ModelId
			: PresetToSelect;
		if (!TargetPreset.IsNone())
		{
			if (const FPresetItemPtr* Item = PresetItems.FindByPredicate([TargetPreset](const FPresetItemPtr& Candidate)
			{
				return Candidate.IsValid() && Candidate->PresetId == TargetPreset;
			}))
			{
				PresetListView->SetSelection(*Item);
				PresetListView->RequestScrollIntoView(*Item);
			}
		}
		else if (!PresetItems.IsEmpty())
		{
			PresetListView->SetSelection(PresetItems[0]);
		}
	}
}

void SStylizedEmissiveLibraryWidget::HandlePresetSelectionChanged(FPresetItemPtr Item, ESelectInfo::Type SelectInfo)
{
	SelectedEntryIndex = Item.IsValid() ? Item->EntryIndex : INDEX_NONE;
	LoadSelectedEntryIntoProxy();
	RefreshPreview();
}

void SStylizedEmissiveLibraryWidget::SelectPresetByIndex(const int32 EntryIndex)
{
	SelectedEntryIndex = EntryIndex;
	if (PresetListView)
	{
		if (const FPresetItemPtr* Item = PresetItems.FindByPredicate([EntryIndex](const FPresetItemPtr& Candidate)
		{
			return Candidate.IsValid() && Candidate->EntryIndex == EntryIndex;
		}))
		{
			PresetListView->SetSelection(*Item);
		}
	}
	LoadSelectedEntryIntoProxy();
	RefreshPreview();
}

void SStylizedEmissiveLibraryWidget::HandleSearchChanged(const FText& InSearchText)
{
	SearchFilter = InSearchText.ToString();
	RefreshPresetItems();
}

void SStylizedEmissiveLibraryWidget::HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	ApplyProxyToSelectedEntry();
	RefreshPreview();
}

void SStylizedEmissiveLibraryWidget::ApplyProxyToSelectedEntry()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !EditProxy || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		return;
	}

	LibraryObject->Modify();
	const FName StableId = LibraryObject->Models[SelectedEntryIndex].ModelId;
	EditProxy->WriteToEntry(LibraryObject->Models[SelectedEntryIndex]);
	LibraryObject->Models[SelectedEntryIndex].ModelId = StableId;
	LibraryObject->MarkPackageDirty();
	StatusText = LOCTEXT("UnsavedStatus", "配置已更新，点击“保存预设库”写入磁盘。可立即拖动测试当前配置。");
	RefreshPresetItems(StableId);
}

void SStylizedEmissiveLibraryWidget::LoadSelectedEntryIntoProxy()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!EditProxy || !LibraryObject || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		if (DetailsView)
		{
			DetailsView->SetObject(nullptr);
		}
		return;
	}

	EditProxy->LoadFromEntry(LibraryObject->Models[SelectedEntryIndex]);
	if (DetailsView)
	{
		DetailsView->SetObject(EditProxy, true);
	}
}

void SStylizedEmissiveLibraryWidget::RefreshPreview()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	const FStylizedEmissiveModelEntry* Entry = LibraryObject && LibraryObject->Models.IsValidIndex(SelectedEntryIndex)
		? &LibraryObject->Models[SelectedEntryIndex]
		: nullptr;
	if (PreviewViewport)
	{
		PreviewViewport->SetPreset(Entry);
	}
}

FReply SStylizedEmissiveLibraryWidget::CreatePreset()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreatePresetTransaction", "创建风格化自发光预设"));
	LibraryObject->Modify();
	FStylizedEmissiveModelEntry& Entry = LibraryObject->Models.AddDefaulted_GetRef();
	Entry.ModelId = MakeUniquePresetId(TEXT("EmissivePreset"));
	Entry.DisplayName = LOCTEXT("NewPresetDisplayName", "新自发光预设");
	Entry.Description = LOCTEXT("NewPresetDescription", "选择模型和自发光材质，或者关闭“使用模型”创建无 Mesh 数据光源。");
	Entry.bUseMesh = false;
	Entry.Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/YogArt_Material/MasterMaterial/Efect/M_Emissive_Common.M_Emissive_Common"));
	LibraryObject->MarkPackageDirty();
	StatusText = LOCTEXT("NewPresetStatus", "已新建预设，请在右侧配置后保存。默认是无 Mesh 数据光源。");
	RefreshPresetItems(Entry.ModelId);
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::DuplicatePreset()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		return FReply::Handled();
	}

	ApplyProxyToSelectedEntry();
	const FScopedTransaction Transaction(LOCTEXT("DuplicatePresetTransaction", "复制风格化自发光预设"));
	LibraryObject->Modify();
	FStylizedEmissiveModelEntry Entry = LibraryObject->Models[SelectedEntryIndex];
	Entry.ModelId = MakeUniquePresetId(Entry.ModelId.ToString() + TEXT("_Copy"));
	Entry.DisplayName = FText::Format(LOCTEXT("CopiedPresetName", "{0} 副本"), GetEntryDisplayName(Entry));
	LibraryObject->Models.Add(Entry);
	LibraryObject->MarkPackageDirty();
	StatusText = LOCTEXT("DuplicatePresetStatus", "已复制预设。");
	RefreshPresetItems(Entry.ModelId);
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::DeletePreset()
{
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		return FReply::Handled();
	}

	const FText PresetName = GetEntryDisplayName(LibraryObject->Models[SelectedEntryIndex]);
	const FText ConfirmText = FText::Format(LOCTEXT("DeletePresetConfirm", "确定删除预设“{0}”吗？已经放置到场景中的 Actor 不会被删除，但会失去模型库引用。"), PresetName);
	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("DeletePresetTransaction", "删除风格化自发光预设"));
	LibraryObject->Modify();
	LibraryObject->Models.RemoveAt(SelectedEntryIndex);
	LibraryObject->MarkPackageDirty();
	SelectedEntryIndex = INDEX_NONE;
	StatusText = LOCTEXT("DeletePresetStatus", "预设已删除。");
	RefreshPresetItems();
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::SaveLibrary()
{
	ApplyProxyToSelectedEntry();
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject)
	{
		return FReply::Handled();
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(LibraryObject->GetOutermost());
	const FEditorFileUtils::EPromptReturnCode Result = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	StatusText = Result == FEditorFileUtils::PR_Success
		? LOCTEXT("SaveSuccessStatus", "预设库已保存。现在可以拖动预设到关卡视口。")
		: LOCTEXT("SaveFailedStatus", "预设库没有保存；请检查 P4 checkout、文件权限或保存提示。");
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::PlaceSelectedPreset()
{
	ApplyProxyToSelectedEntry();
	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		return FReply::Handled();
	}

	AStylizedEmissiveLight* Actor = UCelesLightEditorLibrary::CreateStylizedEmissiveLight(nullptr);
	if (Actor && Actor->ApplyLibraryPreset(LibraryObject, LibraryObject->Models[SelectedEntryIndex].ModelId))
	{
		Actor->SetActorLabel(FString::Printf(TEXT("Emissive_%s"), *GetEntryDisplayName(LibraryObject->Models[SelectedEntryIndex]).ToString()));
		StatusText = LOCTEXT("PlaceSuccessStatus", "已在当前视口前方放置预设；也可从左侧拖动到指定位置。");
	}
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::ShowLibraryInContentBrowser()
{
	if (UStylizedEmissiveModelLibrary* LibraryObject = Library.Get())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		TArray<UObject*> AssetsToSync;
		AssetsToSync.Add(LibraryObject);
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
	}
	return FReply::Handled();
}

FReply SStylizedEmissiveLibraryWidget::BeginPresetDrag(const int32 EntryIndex)
{
	SelectPresetByIndex(EntryIndex);
	ApplyProxyToSelectedEntry();

	UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !LibraryObject->Models.IsValidIndex(EntryIndex))
	{
		return FReply::Unhandled();
	}

	UActorFactoryStylizedEmissivePreset* Factory = NewObject<UActorFactoryStylizedEmissivePreset>(GetTransientPackage());
	Factory->Configure(LibraryObject, LibraryObject->Models[EntryIndex].ModelId);
	ActiveDragFactoryRoot = TStrongObjectPtr<UObject>(Factory);
	StatusText = LOCTEXT("DraggingStatus", "正在拖动预设：松开到关卡视口即可生成 Actor。");
	return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(FAssetData(LibraryObject), Factory));
}

FName SStylizedEmissiveLibraryWidget::MakeUniquePresetId(const FString& BaseName) const
{
	const UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	FString SanitizedBase = BaseName.IsEmpty() ? TEXT("EmissivePreset") : BaseName;
	SanitizedBase.ReplaceInline(TEXT(" "), TEXT("_"));
	FName Candidate(*SanitizedBase);
	int32 Suffix = 1;
	while (LibraryObject && LibraryObject->FindModel(Candidate))
	{
		Candidate = FName(*FString::Printf(TEXT("%s_%02d"), *SanitizedBase, Suffix++));
	}
	return Candidate;
}

bool SStylizedEmissiveLibraryWidget::HasSelectedPreset() const
{
	const UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	return LibraryObject && LibraryObject->Models.IsValidIndex(SelectedEntryIndex);
}

FText SStylizedEmissiveLibraryWidget::GetSelectedPresetTitle() const
{
	const UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	return LibraryObject && LibraryObject->Models.IsValidIndex(SelectedEntryIndex)
		? GetEntryDisplayName(LibraryObject->Models[SelectedEntryIndex])
		: LOCTEXT("NoPresetSelected", "请选择或新建预设");
}

FText SStylizedEmissiveLibraryWidget::GetSelectedPresetSummary() const
{
	const UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	if (!LibraryObject || !LibraryObject->Models.IsValidIndex(SelectedEntryIndex))
	{
		return LOCTEXT("NoPresetSummary", "左侧选择预设后，可在右侧直接配置模型、自发光材质与灯光参数。");
	}

	const FStylizedEmissiveModelEntry& Entry = LibraryObject->Models[SelectedEntryIndex];
	const FText VisualText = Entry.bUseMesh && Entry.Mesh
		? FText::FromString(Entry.Mesh->GetName())
		: LOCTEXT("DataOnlySummary", "无 Mesh 数据光源");
	return FText::Format(LOCTEXT("PresetSummaryFormat", "{0} · {1} · 半径 {2} · 强度 {3}"),
		VisualText,
		GetLightingOutputText(Entry.bUseMesh ? Entry.LightingOutput : EStylizedEmissiveLightingOutput::StylizedMaterial),
		FText::AsNumber(FMath::RoundToInt(Entry.AttenuationRadius)),
		FText::AsNumber(FMath::RoundToInt(Entry.Intensity)));
}

FText SStylizedEmissiveLibraryWidget::GetStatusText() const
{
	return StatusText.IsEmpty() ? LOCTEXT("DefaultStatus", "提示：按住左侧预设并拖动到关卡视口。") : StatusText;
}

EVisibility SStylizedEmissiveLibraryWidget::GetDataOnlyMessageVisibility() const
{
	const UStylizedEmissiveModelLibrary* LibraryObject = Library.Get();
	return LibraryObject && LibraryObject->Models.IsValidIndex(SelectedEntryIndex)
		&& LibraryObject->Models[SelectedEntryIndex].bUseMesh
		&& LibraryObject->Models[SelectedEntryIndex].Mesh
		? EVisibility::Collapsed
		: EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE
