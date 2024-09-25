// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "ComboGraphEdGraph.generated.h"

class FComboGraphDebugger;
class UComboGraphEdNodeBase;
class UComboGraphEdNodeConduit;
class UComboGraphNodeBase;
class UComboGraphNodeAnimBase;
class UComboGraph;
class UComboGraphAbility;
class UComboGraphEdge;
class UComboGraphEdNode;
class UComboGraphEdNodeEdge;
class UComboGraphEdNodeEntry;

UCLASS()
class COMBOGRAPHEDITOR_API UComboGraphEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	/** Entry node within the state machine */
	UPROPERTY()
	TObjectPtr<UComboGraphEdNodeEntry> EntryNode;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UComboGraphNodeBase>, TObjectPtr<UComboGraphEdNodeBase>> NodeMap;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UComboGraphEdge>, TObjectPtr<UComboGraphEdNodeEdge>> EdgeMap;

	/** Shared ref to asset editor, namely to access debugger info and debugged node */
	TSharedPtr<FComboGraphDebugger> Debugger;

	UComboGraph* GetComboGraphModel() const;

	virtual void RebuildGraph();
	void RebuildGraphForEdge(UComboGraph* OwningGraph, UComboGraphEdNodeEdge* EdGraphEdge);
	void RebuildGraphForNodeBase(UComboGraph* OwningGraph, UComboGraphEdNodeBase* NodeBase);
	void RebuildGraphForEntry(UComboGraph* OwningGraph, UComboGraphEdNodeEntry* NodeEntry);

	/** Goes through each nodes and run a validation pass */
	void ValidateNodes(FCompilerResultsLog* LogResults);

	//~ UObject interface
	virtual bool Modify(bool bAlwaysMarkDirty) override;
	virtual void PostEditUndo() override;
	//~ End UObject interface

	/** Returns all graph nodes that are of node type (not edges) */
	TArray<UComboGraphEdNode*> GetAllNodes() const;

	/** Re-organize graph nodes automatically */
	void AutoArrange(bool bVertical);

protected:
	void Clear();

	// TODO: Is it needed anymore ?
	void SortNodes(UComboGraphNodeBase* RootNode);
};
