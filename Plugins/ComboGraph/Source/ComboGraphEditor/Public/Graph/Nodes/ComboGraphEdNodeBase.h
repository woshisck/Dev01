// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEdNodeBase.generated.h"

// TODO: Has to inherit from UK2Node to avoid ensure condition error on editor startup (see #3), but I don't like that

/** Shared Base class for all Editor Graph nodes */
UCLASS(MinimalAPI, abstract)
// class UComboGraphEdNodeBase : public UK2Node
class UComboGraphEdNodeBase : public UEdGraphNode
{
	GENERATED_BODY()

public:

	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

	//~ Begin UK2Node Interface
	// virtual bool IsNodePure() const override { return true; }
	// virtual bool ShouldShowNodeProperties() const override { return true; }
	//~ End UK2Node Interface

	//~ Begin UEdGraphNode Interface
	// Make those noop like in UEdGraphNode. UK2Node does CastChecked on Graph Schema to be UEdGraphSchema_K2 and don't want to make schema K2.
	// Nodes are now inheriting from UK2Node to avoid ensure condition on startup with pure check (see #3)
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override {}
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override {};
	//~ End UEdGraphNode Interface
};
