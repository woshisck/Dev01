// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphPin.h"

class SComboGraphConduitOutputPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SComboGraphConduitOutputPin){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);
protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	// End SGraphPin interface

	const FSlateBrush* GetPinBorder() const;
};
