#pragma once

#include "IDetailCustomization.h"

class UGameplayAbilityComboGraphNode;

class FGameplayAbilityComboGraphNodeDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TWeakObjectPtr<UGameplayAbilityComboGraphNode> EditingNode;

	FReply MigrateFromFirstNotify();
	FText GetSummaryText() const;
	bool CanMigrateFromFirstNotify() const;
};
