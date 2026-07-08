#include "Tools/RVTMeshDecal/SDevKitRVTMeshDecalWidget.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "IContentBrowserSingleton.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"
#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"
#include "VT/RuntimeVirtualTexture.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDevKitRVTMeshDecalWidget"

namespace
{
	FString ToObjectPath(const FAssetData& AssetData)
	{
		return AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
	}

	TSharedRef<SWidget> MakeLabel(const FText& Text)
	{
		return SNew(STextBlock)
			.Text(Text);
	}
}

void SDevKitRVTMeshDecalWidget::Construct(const FArguments& InArgs)
{
	PlaneMeshObjectPath = FDevKitRVTMeshDecalService::GetDefaultPlaneMeshObjectPath();
	StatusText = LOCTEXT("InitialStatus", "就绪。选择 Plane、贴花材质和目标 RVT 后，创建或更新可用于植被模式刷出的 RVT 网格贴花 FoliageType。");

	ChildSlot
	[
		SNew(SBorder)
		.Padding(16.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "RVT 网格贴花刷"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "创建以 Plane 为基础的 FoliageType：写入目标 RVT、主通道按 From Virtual Texture 隐藏、按 Priority 分层。创建后在 UE 植被模式中使用该 FoliageType 刷实例。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLabel(LOCTEXT("FolderLabel", "FoliageType 保存目录"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(FoliageTypeFolderTextBox, SEditableTextBox)
					.HintText(LOCTEXT("FolderHint", "/Game/Art/Map/Map_Data/LevelName/BakeInfo/RVTDecalFoliage"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLabel(LOCTEXT("NameOverrideLabel", "FoliageType 名称覆盖"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(FoliageTypeNameOverrideTextBox, SEditableTextBox)
					.HintText(LOCTEXT("NameOverrideHint", "留空则使用 FT_RVTDecal_<材质名>_P<Priority>"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLabel(LOCTEXT("PlaneMeshLabel", "基础 Plane Mesh"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UStaticMesh::StaticClass())
					.ObjectPath(this, &SDevKitRVTMeshDecalWidget::GetPlaneMeshObjectPath)
					.OnObjectChanged(this, &SDevKitRVTMeshDecalWidget::OnPlaneMeshChanged)
					.AllowClear(false)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLabel(LOCTEXT("MaterialLabel", "贴花材质"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UMaterialInterface::StaticClass())
					.ObjectPath(this, &SDevKitRVTMeshDecalWidget::GetMaterialObjectPath)
					.OnObjectChanged(this, &SDevKitRVTMeshDecalWidget::OnMaterialChanged)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLabel(LOCTEXT("RVTLabel", "目标 RVT"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(URuntimeVirtualTexture::StaticClass())
					.ObjectPath(this, &SDevKitRVTMeshDecalWidget::GetRuntimeVirtualTextureObjectPath)
					.OnObjectChanged(this, &SDevKitRVTMeshDecalWidget::OnRuntimeVirtualTextureChanged)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[MakeLabel(LOCTEXT("PriorityLabel", "Priority"))]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSpinBox<int32>)
							.MinValue(-1000)
							.MaxValue(1000)
							.Value(TranslucencySortPriority)
							.OnValueChanged_Lambda([this](int32 NewValue) { TranslucencySortPriority = NewValue; })
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[MakeLabel(LOCTEXT("MinScaleLabel", "最小尺寸"))]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSpinBox<float>)
							.MinValue(0.01f)
							.MaxValue(100.0f)
							.Value(MinScale)
							.OnValueChanged_Lambda([this](float NewValue) { MinScale = NewValue; })
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[MakeLabel(LOCTEXT("MaxScaleLabel", "最大尺寸"))]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SSpinBox<float>)
							.MinValue(0.01f)
							.MaxValue(100.0f)
							.Value(MaxScale)
							.OnValueChanged_Lambda([this](float NewValue) { MaxScale = NewValue; })
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 24.f, 0.f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bAlignToNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bAlignToNormal = NewState == ECheckBoxState::Checked; })
						[
							SNew(STextBlock).Text(LOCTEXT("AlignToNormalLabel", "对齐地面法线"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bRandomYaw ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bRandomYaw = NewState == ECheckBoxState::Checked; })
						[
							SNew(STextBlock).Text(LOCTEXT("RandomYawLabel", "随机旋转"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("UseCurrentLevelPaths", "使用当前关卡路径"))
						.OnClicked(this, &SDevKitRVTMeshDecalWidget::UseCurrentLevelPaths)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("CreateOrUpdate", "创建/更新 FoliageType"))
						.OnClicked(this, &SDevKitRVTMeshDecalWidget::CreateOrUpdateFoliageType)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetStatusText)
					.ColorAndOpacity(this, &SDevKitRVTMeshDecalWidget::GetStatusColor)
					.AutoWrapText(true)
				]
			]
		]
	];

	RefreshDefaultPaths();
}

FString SDevKitRVTMeshDecalWidget::GetCurrentWorldPackagePath() const
{
	if (!GEditor)
	{
		return FString();
	}

	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World || !World->GetOutermost())
	{
		return FString();
	}

	return World->GetOutermost()->GetName();
}

void SDevKitRVTMeshDecalWidget::RefreshDefaultPaths()
{
	const FString WorldPackagePath = GetCurrentWorldPackagePath();
	if (WorldPackagePath.IsEmpty())
	{
		return;
	}

	const FString FoliageFolder = FDevKitRVTMeshDecalService::InferDefaultFoliageTypeFolderFromWorldPackage(WorldPackagePath);
	if (FoliageTypeFolderTextBox.IsValid())
	{
		FoliageTypeFolderTextBox->SetText(FText::FromString(FoliageFolder));
	}

	const FString BakeInfoFolder = FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(WorldPackagePath);
	const FString RVTName = FDevKitLevelRVTService::BuildDefaultGroundRVTNameFromWorldPackage(WorldPackagePath);
	RuntimeVirtualTextureObjectPath = FString::Printf(TEXT("%s/%s.%s"), *BakeInfoFolder, *RVTName, *RVTName);
}

FText SDevKitRVTMeshDecalWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SDevKitRVTMeshDecalWidget::GetStatusColor() const
{
	return bStatusIsError
		? FSlateColor(FLinearColor(0.9f, 0.2f, 0.15f))
		: FSlateColor::UseForeground();
}

void SDevKitRVTMeshDecalWidget::SetStatus(const FText& InStatus, bool bInIsError)
{
	StatusText = InStatus;
	bStatusIsError = bInIsError;
}

FString SDevKitRVTMeshDecalWidget::GetPlaneMeshObjectPath() const
{
	return PlaneMeshObjectPath;
}

FString SDevKitRVTMeshDecalWidget::GetMaterialObjectPath() const
{
	return MaterialObjectPath;
}

FString SDevKitRVTMeshDecalWidget::GetRuntimeVirtualTextureObjectPath() const
{
	return RuntimeVirtualTextureObjectPath;
}

void SDevKitRVTMeshDecalWidget::OnPlaneMeshChanged(const FAssetData& AssetData)
{
	PlaneMeshObjectPath = ToObjectPath(AssetData);
}

void SDevKitRVTMeshDecalWidget::OnMaterialChanged(const FAssetData& AssetData)
{
	MaterialObjectPath = ToObjectPath(AssetData);
}

void SDevKitRVTMeshDecalWidget::OnRuntimeVirtualTextureChanged(const FAssetData& AssetData)
{
	RuntimeVirtualTextureObjectPath = ToObjectPath(AssetData);
}

FReply SDevKitRVTMeshDecalWidget::UseCurrentLevelPaths()
{
	RefreshDefaultPaths();
	SetStatus(LOCTEXT("UsedCurrentLevelPaths", "已根据当前关卡刷新保存目录和默认 RVT。"), false);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::CreateOrUpdateFoliageType()
{
	const FDevKitRVTMeshDecalRequest Request{
		FoliageTypeFolderTextBox.IsValid() ? FoliageTypeFolderTextBox->GetText().ToString() : FString(),
		PlaneMeshObjectPath,
		MaterialObjectPath,
		RuntimeVirtualTextureObjectPath,
		TranslucencySortPriority,
		MinScale,
		MaxScale,
		bAlignToNormal,
		bRandomYaw,
		FoliageTypeNameOverrideTextBox.IsValid() ? FoliageTypeNameOverrideTextBox->GetText().ToString() : FString()
	};

	const FDevKitRVTMeshDecalResult Result = FDevKitRVTMeshDecalService::CreateOrUpdateFoliageType(Request);
	SetStatus(Result.Message, !Result.bSuccess);

	if (Result.bSuccess && Result.FoliageType.IsValid())
	{
		IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
		ContentBrowser.SyncBrowserToAssets({FAssetData(Result.FoliageType.Get())}, false, true);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
