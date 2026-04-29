// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEdNodeBase.h"
#include "ComboGraphEdNodeEntry.generated.h"

class UComboGraphNodeEntry;
/** Editor Graph Node for entry point in Combat Graphs. Based off UAnimStateEntryNode for state machine graphs in Anim BP. */
UCLASS(MinimalAPI)
class UComboGraphEdNodeEntry : public UComboGraphEdNodeBase
{
	GENERATED_BODY()

public:
	UComboGraphEdNodeEntry();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeEntry> RuntimeNode;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	virtual bool CanUserDeleteNode() const override	{ return false; }
	virtual bool CanDuplicateNode() const override	{ return false; }
	//~ End UEdGraphNode Interface

	COMBOGRAPHEDITOR_API UEdGraphNode* GetOutputNode();
};
