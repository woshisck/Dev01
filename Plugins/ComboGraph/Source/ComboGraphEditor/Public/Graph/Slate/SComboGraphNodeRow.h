// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UComboGraphNodeBase;

/**
 * Delegate fired to get the path to start picking from.
 * @return the path the picker will use to start from
 */
DECLARE_DELEGATE_RetVal(FText, FACEOnGetValueText);

/** Draws a combo node property row to render label / values for a given property */
class COMBOGRAPHEDITOR_API SComboGraphNodeRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SComboGraphNodeRow) {}

	SLATE_ARGUMENT(FText, LabelText)
	SLATE_ARGUMENT(FText, LabelTooltipText)
	SLATE_ARGUMENT(FText, ValueTooltipText)
	SLATE_ARGUMENT(bool, IsTransparent)

	/** Delegate fired to get the actual value from GetValueText */
	SLATE_ARGUMENT(FACEOnGetValueText, OnGetValueText)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, UComboGraphNodeBase* InRuntimeNode);

protected:

	UComboGraphNodeBase* RuntimeNode = nullptr;
	FText LabelText;
	FText LabelTooltipText;
	FText ValueTooltipText;
	bool bIsTransparent = false;

	FACEOnGetValueText OnGetValueText;

	FText GetValueText() const;
};
