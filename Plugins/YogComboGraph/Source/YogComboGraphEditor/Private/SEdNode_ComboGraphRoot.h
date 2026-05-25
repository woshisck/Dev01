#pragma once

#include "CoreMinimal.h"
#include "GenericGraphAssetEditor/SEdNode_GenericGraphNode.h"

class UEdNode_ComboGraphRoot;

class SEdNode_ComboGraphRoot : public SEdNode_GenericGraphNode
{
public:
	SLATE_BEGIN_ARGS(SEdNode_ComboGraphRoot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdNode_ComboGraphRoot* InNode);

	// Root has no rename — guard the null GenericGraphNode dereference in the base.
	virtual bool IsNameReadOnly() const override { return true; }
};
