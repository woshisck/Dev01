#include "CelesLightCaptureBoxDetails.h"

#include "Actors/CelesLightCaptureBox.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CelesLightBlueprintLibrary.h"
#include "ContentBrowserModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FCelesLightCaptureBoxDetails"

TSharedRef<IDetailCustomization> FCelesLightCaptureBoxDetails::MakeInstance()
{
	return MakeShared<FCelesLightCaptureBoxDetails>();
}

void FCelesLightCaptureBoxDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	SelectedCaptureBoxes.Reset();

	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	for (const TWeakObjectPtr<UObject>& CustomizedObject : CustomizedObjects)
	{
		if (ACelesLightCaptureBox* CaptureBox = Cast<ACelesLightCaptureBox>(CustomizedObject.Get()))
		{
			SelectedCaptureBoxes.Add(CaptureBox);
		}
	}

	IDetailCategoryBuilder& DescriptionCategory = DetailBuilder.EditCategory(
		TEXT("Celes Light|说明"),
		LOCTEXT("DescriptionCategory", "Celes Light|说明"));

	DescriptionCategory.AddCustomRow(LOCTEXT("DescriptionFilter", "说明"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.AutoWrapText(true)
		.Text(LOCTEXT(
			"CaptureBoxDescription",
			"采集盒体会把盒体范围内的 CelesPointLight / PointLight 信息编码到指定 Render Target。Max Light Count 默认 4、最高 16；超过数量的灯光会进入 Overflow 列表，不写入 RT。材质只需要自行采样这张 RT；插件不会创建动态材质，也不会改写场景物体材质。"))
	];

	IDetailCategoryBuilder& ActionCategory = DetailBuilder.EditCategory(
		TEXT("Celes Light|操作"),
		LOCTEXT("ActionCategory", "Celes Light|操作"));

	ActionCategory.AddCustomRow(LOCTEXT("CreateRTFilter", "创建 RT"))
	.WholeRowContent()
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.Text(LOCTEXT("CreateRTButton", "创建并引用 Light Info RT..."))
		.ToolTipText(LOCTEXT("CreateRTTooltip", "选择保存路径并创建 CanvasRenderTarget2D，然后自动引用到当前采集盒体。"))
		.OnClicked(this, &FCelesLightCaptureBoxDetails::CreateAndAssignRenderTarget)
	];

	ActionCategory.AddCustomRow(LOCTEXT("UpdateFilter", "Update Celes Light"))
	.WholeRowContent()
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.Text(LOCTEXT("UpdateButton", "Update Celes Light"))
		.ToolTipText(LOCTEXT("UpdateTooltip", "立即读取盒体内灯光并写入当前 RT。"))
		.OnClicked(this, &FCelesLightCaptureBoxDetails::UpdateSelectedCaptureBoxes)
	];
}

FReply FCelesLightCaptureBoxDetails::CreateAndAssignRenderTarget()
{
	const int32 LightInfoCount = GetFirstSelectedLightCount();

	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DefaultPath = TEXT("/Game/CelesLight");
	SaveAssetDialogConfig.DefaultAssetName = TEXT("CRT_CelesLightInfo");
	SaveAssetDialogConfig.AssetClassNames.Add(UCanvasRenderTarget2D::StaticClass()->GetClassPathName());
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
	SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("CreateRTDialogTitle", "创建 Celes Light RT");

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	const FString ObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	if (ObjectPath.IsEmpty())
	{
		return FReply::Handled();
	}

	UTextureRenderTarget2D* RenderTarget = CreateRenderTargetAsset(ObjectPath, LightInfoCount);
	if (!RenderTarget)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AssignRTTransaction", "引用 Celes Light RT"));
	for (const TWeakObjectPtr<ACelesLightCaptureBox>& CaptureBoxPtr : SelectedCaptureBoxes)
	{
		ACelesLightCaptureBox* CaptureBox = CaptureBoxPtr.Get();
		if (!CaptureBox)
		{
			continue;
		}

		CaptureBox->Modify();
		CaptureBox->LightInfoRenderTarget = RenderTarget;
		CaptureBox->UpdateCelesLight();
	}

	return FReply::Handled();
}

FReply FCelesLightCaptureBoxDetails::UpdateSelectedCaptureBoxes()
{
	for (const TWeakObjectPtr<ACelesLightCaptureBox>& CaptureBoxPtr : SelectedCaptureBoxes)
	{
		if (ACelesLightCaptureBox* CaptureBox = CaptureBoxPtr.Get())
		{
			CaptureBox->UpdateCelesLight();
		}
	}

	return FReply::Handled();
}

UTextureRenderTarget2D* FCelesLightCaptureBoxDetails::CreateRenderTargetAsset(const FString& ObjectPath, const int32 LightInfoCount) const
{
	const FString PackageName = FPackageName::ObjectPathToPackageName(ObjectPath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
	if (PackageName.IsEmpty() || AssetName.IsEmpty())
	{
		return nullptr;
	}

	if (UTextureRenderTarget2D* ExistingRenderTarget = Cast<UTextureRenderTarget2D>(StaticLoadObject(UTextureRenderTarget2D::StaticClass(), nullptr, *ObjectPath)))
	{
		ExistingRenderTarget->Modify();
		UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(ExistingRenderTarget, LightInfoCount);
		ExistingRenderTarget->MarkPackageDirty();
		return ExistingRenderTarget;
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}

	UCanvasRenderTarget2D* RenderTarget = NewObject<UCanvasRenderTarget2D>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!RenderTarget)
	{
		return nullptr;
	}

	UCelesLightBlueprintLibrary::ConfigureLightInfoRenderTarget(RenderTarget, LightInfoCount);
	FAssetRegistryModule::AssetCreated(RenderTarget);
	RenderTarget->MarkPackageDirty();
	Package->MarkPackageDirty();
	UEditorLoadingAndSavingUtils::SavePackages({ Package }, true);
	return RenderTarget;
}

int32 FCelesLightCaptureBoxDetails::GetFirstSelectedLightCount() const
{
	for (const TWeakObjectPtr<ACelesLightCaptureBox>& CaptureBoxPtr : SelectedCaptureBoxes)
	{
		if (const ACelesLightCaptureBox* CaptureBox = CaptureBoxPtr.Get())
		{
			return FMath::Clamp(CaptureBox->MaxLightCount, 1, CelesLight::MaxLightInfoCount);
		}
	}

	return CelesLight::DefaultLightInfoCount;
}

#undef LOCTEXT_NAMESPACE
