#include "Customization/GameplayAbilityComboGraphNodeDetails.h"

#include "Data/GameplayAbilityComboGraph.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraphNodeDetails"

TSharedRef<IDetailCustomization> FGameplayAbilityComboGraphNodeDetails::MakeInstance()
{
	return MakeShared<FGameplayAbilityComboGraphNodeDetails>();
}

void FGameplayAbilityComboGraphNodeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	EditingNode = Objects.Num() > 0 ? Cast<UGameplayAbilityComboGraphNode>(Objects[0].Get()) : nullptr;

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Combo Manager"));
	Category.AddCustomRow(LOCTEXT("ComboManagerSummaryFilter", "Combo Manager"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text(this, &FGameplayAbilityComboGraphNodeDetails::GetSummaryText)
			.AutoWrapText(true)
		];
}

FText FGameplayAbilityComboGraphNodeDetails::GetSummaryText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	if (!Node)
	{
		return LOCTEXT("NoNode", "No combo node selected.");
	}

	return FText::FromString(FString::Printf(
		TEXT("RootInput: %s | Montage: %s | ComboWindow: %d-%d/%d"),
		StaticEnum<EYogComboGraphInputAction>()
			? *StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(Node->RootInputAction))
			: TEXT("-"),
		*GetNameSafe(Node->Montage.Get()),
		Node->ComboWindowStartFrame,
		Node->ComboWindowEndFrame,
		Node->TotalFrames));
}

#undef LOCTEXT_NAMESPACE
