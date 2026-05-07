#include "Customization/RuneDataAssetDetails.h"

#include "Data/RuneDataAsset.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "RuneDataAssetDetails"

DEFINE_LOG_CATEGORY_STATIC(LogRuneDADetails, Log, All);

TSharedRef<IDetailCustomization> FRuneDataAssetDetails::MakeInstance()
{
	return MakeShared<FRuneDataAssetDetails>();
}

void FRuneDataAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
	if (SelectedObjects.Num() != 1)
	{
		return; // 多选时不展示快捷按钮（避免歧义）
	}
	TWeakObjectPtr<URuneDataAsset> WeakDA = Cast<URuneDataAsset>(SelectedObjects[0].Get());
	if (!WeakDA.IsValid())
	{
		return;
	}

	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(
		TEXT("RuneTools"),
		LOCTEXT("RuneToolsCategory", "Rune Tools"),
		ECategoryPriority::Important);

	Cat.AddCustomRow(LOCTEXT("RuneToolsRow", "Rune Quick Actions"))
	.WholeRowContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("CopyTagBtn", "Copy RuneIdTag"))
			.ToolTipText(LOCTEXT("CopyTagTip", "拷贝 RuneIdTag 到剪贴板，便于在 Excel/Notion 引用"))
			.OnClicked_Lambda([WeakDA]() -> FReply
			{
				if (URuneDataAsset* DA = WeakDA.Get())
				{
					const FString TagStr = DA->GetRuneIdTag().ToString();
					FPlatformApplicationMisc::ClipboardCopy(*TagStr);
					UE_LOG(LogRuneDADetails, Log, TEXT("[%s] 已复制 RuneIdTag: %s"), *DA->GetName(), *TagStr);
				}
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(LOCTEXT("ValidateBtn", "Validate This"))
			.ToolTipText(LOCTEXT("ValidateTip", "对本条 RuneDA 跑一次字段校验，结果输出到 Output Log"))
			.OnClicked_Lambda([WeakDA]() -> FReply
			{
				URuneDataAsset* DA = WeakDA.Get();
				if (!DA) return FReply::Handled();

				int32 Issues = 0;
				if (!DA->GetRuneIdTag().IsValid())
				{
					UE_LOG(LogRuneDADetails, Warning, TEXT("[%s] RuneIdTag 未配置"), *DA->GetName()); ++Issues;
				}
				if (DA->GetRuneName().IsNone())
				{
					UE_LOG(LogRuneDADetails, Warning, TEXT("[%s] RuneName 为空"), *DA->GetName()); ++Issues;
				}
				if (DA->GetGoldCost() < 0)
				{
					UE_LOG(LogRuneDADetails, Error, TEXT("[%s] GoldCost < 0 (=%d)"), *DA->GetName(), DA->GetGoldCost()); ++Issues;
				}
				if (DA->GetChainRole() == ERuneChainRole::Producer && DA->GetChainDirections().Num() == 0)
				{
					UE_LOG(LogRuneDADetails, Warning, TEXT("[%s] ChainRole=Producer 但 ChainDirections 为空"), *DA->GetName()); ++Issues;
				}
				UE_LOG(LogRuneDADetails, Log, TEXT("[%s] 校验完成，发现 %d 处问题"), *DA->GetName(), Issues);
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8, 0).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("RuneSummary", "Rarity: {0}  GoldCost: {1}"),
				FText::AsNumber(static_cast<int32>(WeakDA->GetRarity())),
				FText::AsNumber(WeakDA->GetGoldCost())))
		]
	];
}

#undef LOCTEXT_NAMESPACE
