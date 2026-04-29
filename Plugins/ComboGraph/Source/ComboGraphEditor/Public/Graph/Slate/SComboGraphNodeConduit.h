// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UComboGraphEdNodeConduit;

class SComboGraphNodeConduit : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SComboGraphNodeConduit){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UComboGraphEdNodeConduit* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual void CreatePinWidgets() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	// End of SGraphNode interface

protected:
	FSlateColor GetBorderBackgroundColor() const;

	virtual const FSlateBrush* GetNameIcon() const;
};
