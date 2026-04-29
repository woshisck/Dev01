// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "Interfaces/Interface_PreviewMeshProvider.h"
#include "ComboGraph.generated.h"

class UInputMappingContext;
class UInputAction;
class UComboGraphNodeSequence;
class UComboGraphNodeMontage;
class UComboGraphNodeEntry;
class UComboGraphNodeAnimBase;
class UComboGraphEdge;
class UComboGraphNodeBase;


/** Last Selected Editor App Mode */
UENUM()
enum class EComboGraphSelectedAppMode : uint8
{
	/** Persona Mode */
	Persona UMETA(DisplayName="Persona Preview Mode"),

	/** Graph Mode */
	Graph UMETA(DisplayName="Combo Graph Mode"),
};


/**
 * Combo Graph Assets provide a way to visually design combo strings and link montages together.
 *
 * Combo Graphs are then run with custom Ability Tasks from within a Gameplay Ability.
 */
UCLASS(BlueprintType, AutoExpandCategories=("Combo Graph | Nodes", "Combo Graph | Transitions", "Combo Graph | Icons"))
class COMBOGRAPH_API UComboGraph : public UObject, public IInterface_PreviewMeshProvider
{
	GENERATED_BODY()

public:
	UComboGraph();

	/** Represents the top level combo node direct child of entry node. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeEntry> EntryNode;

	/** Represents the top level combo node direct child of entry node. */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeBase> FirstNode;

	/**
	 * Holds any combo nodes with no direct parent
	 *
	 * Note: should usually hold only one node, the first one. If more than one,
	 * means that some combo nodes are present in graph but not linked to a valid
	 * path from entry node as all nodes below entry should have direct
	 * parent / child relationship.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TArray<TObjectPtr<UComboGraphNodeBase>> RootNodes;

	/** Holds all combo nodes defined for this graph */
	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TArray<TObjectPtr<UComboGraphNodeBase>> AllNodes;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	/** Internal Node Class type used to know which classes to consider when drawing out context menu in graphs */
	TSubclassOf<UComboGraphNodeAnimBase> NodeType;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	/** Not used currently, but might be if we allow customization of edge classes */
	TSubclassOf<UComboGraphEdge> EdgeType;

	/**
	 * Node Class type used for montages assets when dropped in a graph, or dropped on an existing node pin.
	 *
	 * This property lets you customize which class Combo Graph will use to create a new montage node in these situations.
	 * Mainly useful if you have a custom Combo Node Montage BP or native subclass.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Nodes")
	TSubclassOf<UComboGraphNodeMontage> DefaultNodeMontageType;

	/**
	 * Node Class type used for sequence assets when dropped in a graph, or dropped on an existing node pin.
	 *
	 * This property lets you customize which class Combo Graph will use to create a new sequence node in these situations.
	 * Mainly useful if you have a custom Combo Node Montage BP or native subclass.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Nodes")
	TSubclassOf<UComboGraphNodeSequence> DefaultNodeSequenceType;

	/**
	 * Default Input Action to use when creating new connections.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Transitions")
	TObjectPtr<UInputAction> DefaultInputAction;

	void ClearGraph();
	int32 GetLevelNum() const;

	//~ IInterface_PreviewMeshProvider
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty) override;
	virtual USkeletalMesh* GetPreviewMesh() const override;
	//~ End IInterface_PreviewMeshProvider

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UEdGraph> EditorGraph;

	/** Last selected app mode, stored here to persist across re-opening and restart of the editor */
	UPROPERTY(VisibleDefaultsOnly, Category = "Combo Graph", meta=(HideInComboGraphDetailsPanel))
	EComboGraphSelectedAppMode SelectedAppMode = EComboGraphSelectedAppMode::Persona;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	bool bCanRenameNode = true;

	/** Determine if we can have cycles or not in a graph */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Transitions")
	bool bCanBeCyclical = true;

	/** Enhanced Input Context Mapping to use to draw edge (transition) icons in Graphs (if not set, will fallback to the one defined in Project Settings) */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Icons")
	TSoftObjectPtr<UInputMappingContext> ContextMapping;

	/** Icon preference to draw edge (transition) icons in Graph. Can be either Keyboard or Gamepad based */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph | Icons")
	EComboGraphIconType IconPreference;
#endif

private:

#if WITH_EDITORONLY_DATA
	/** The default skeletal mesh to use when previewing this asset */
	UPROPERTY(duplicatetransient, AssetRegistrySearchable)
	TSoftObjectPtr<class USkeletalMesh> PreviewSkeletalMesh;
#endif
};
