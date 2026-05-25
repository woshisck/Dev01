#pragma once

#include "CoreMinimal.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "EdNode_ComboGraphRoot.generated.h"

/**
 * Purely visual entry-point node shown at the top of every combo graph editor.
 * It is always present, non-deletable, and has no backing UGenericGraphNode,
 * so RebuildGenericGraph skips it — combo nodes wired from its output pin have
 * no recorded parent and are correctly treated as runtime RootNodes.
 */
UCLASS()
class YOGCOMBOGRAPHEDITOR_API UEdNode_ComboGraphRoot : public UEdNode_GenericGraphNode
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetBackgroundColor() const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
};
