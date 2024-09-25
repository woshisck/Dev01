// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SNodePanel.h"
#include "SGraphNode.h"


class UComboGraphEdNodeEntry;

class SComboGraphNodeEntry : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SComboGraphNodeEntry){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UComboGraphEdNodeEntry* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	// End of SGraphNode interface

protected:
	FSlateColor GetBorderBackgroundColor() const;
	FText GetPreviewCornerText() const;
};
