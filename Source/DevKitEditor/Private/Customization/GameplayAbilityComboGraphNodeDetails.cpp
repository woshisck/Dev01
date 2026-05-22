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
	Category.AddCustomRow(LOCTEXT("ComboManagerNodeClassFilter", "Node Class"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NodeClassLabel", "Node Class"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(240.f)
		[
			SNew(STextBlock)
			.Text(this, &FGameplayAbilityComboGraphNodeDetails::GetNodeClassText)
			.ToolTipText(this, &FGameplayAbilityComboGraphNodeDetails::GetNodeClassTooltipText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

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

FText FGameplayAbilityComboGraphNodeDetails::GetNodeClassText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	const UClass* NodeClass = Node ? Node->GetClass() : nullptr;
	return NodeClass
		? FText::FromString(NodeClass->GetName())
		: LOCTEXT("NoNodeClass", "None");
}

FText FGameplayAbilityComboGraphNodeDetails::GetNodeClassTooltipText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	const UClass* NodeClass = Node ? Node->GetClass() : nullptr;
	return NodeClass
		? FText::FromString(NodeClass->GetPathName())
		: LOCTEXT("NoNodeClassTooltip", "No combo node selected.");
}

#undef LOCTEXT_NAMESPACE
