#include "Customization/GameplayAbilityComboGraphNodeDetails.h"

#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimMontage.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraphNodeDetails"

namespace
{
	UAN_MeleeDamage* FindFirstDamageNotify(const UGameplayAbilityComboGraphNode* Node)
	{
		const UMontageConfigDA* MontageConfig = Node ? Node->MontageConfig.Get() : nullptr;
		UAnimMontage* Montage = MontageConfig ? MontageConfig->Montage.Get() : nullptr;
		if (!Montage)
		{
			return nullptr;
		}

		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(NotifyEvent.Notify))
			{
				return DamageNotify;
			}
		}
		return nullptr;
	}
}

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

	Category.AddCustomRow(LOCTEXT("MigrateNotifyFilter", "Migrate Notify"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("MigrateNotifyButton", "Migrate First AN_MeleeDamage To Node Attack Config"))
			.IsEnabled(this, &FGameplayAbilityComboGraphNodeDetails::CanMigrateFromFirstNotify)
			.OnClicked(this, &FGameplayAbilityComboGraphNodeDetails::MigrateFromFirstNotify)
		];
}

FText FGameplayAbilityComboGraphNodeDetails::GetSummaryText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	if (!Node)
	{
		return LOCTEXT("NoNode", "No combo node selected.");
	}

	const UAN_MeleeDamage* Notify = FindFirstDamageNotify(Node);
	const FComboAttackConfig& Config = Node->NodeAttackConfig;
	return FText::FromString(FString::Printf(
		TEXT("NodeAttack: %s | Damage %.0f | Range %.0f | HitBoxes %d | MontageNotify: %s"),
		Config.bEnabled ? TEXT("Enabled") : TEXT("Fallback"),
		Config.ActDamage,
		Config.ActRange,
		Config.HitboxTypes.Num(),
		Notify ? *Notify->GetName() : TEXT("None")));
}

bool FGameplayAbilityComboGraphNodeDetails::CanMigrateFromFirstNotify() const
{
	return FindFirstDamageNotify(EditingNode.Get()) != nullptr;
}

FReply FGameplayAbilityComboGraphNodeDetails::MigrateFromFirstNotify()
{
	UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	UAN_MeleeDamage* Notify = FindFirstDamageNotify(Node);
	if (!Node || !Notify)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("MigrateNotifyToComboNode", "Migrate AN_MeleeDamage to Combo Graph Node"));
	if (Node->Graph)
	{
		Node->Graph->Modify();
	}
	Node->Modify();
	Node->NodeAttackConfig.CopyFromNotify(Notify);
	Node->MarkPackageDirty();
	if (Node->Graph)
	{
		Node->Graph->MarkPackageDirty();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
