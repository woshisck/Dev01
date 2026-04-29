// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEdNodeBase.h"
#include "ComboGraphEdNodeConduit.generated.h"

class UComboGraphNodeBase;

/** Editor Graph Node for conduit nodes in Combat Graphs. Based off UAnimStateConduitNode for state machine graphs in Anim BP. */
UCLASS(MinimalAPI)
class UComboGraphEdNodeConduit : public UComboGraphEdNodeBase
{
	GENERATED_BODY()

public:
	UComboGraphEdNodeConduit();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeBase> RuntimeNode;

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanDuplicateNode() const override { return false; }
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual FString GetDocumentationExcerptName() const override;
	virtual FString GetDocumentationLink() const override;
	//~ End UEdGraphNode Interface
};
