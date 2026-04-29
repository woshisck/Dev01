// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraph.h"
#include "ComboGraphNodeBase.generated.h"

class UInputAction;
struct FGameplayEventData;
class UGameplayEffect;
class UComboGraphEdge;

UENUM(BlueprintType)
enum class EACENodeLimit : uint8
{
	Unlimited,
	Limited
};

/**
 *  Base Class for all Combo Graph nodes (Edges, Anim based and conduit)
 *
 *  Holds information and API related to debug states.
 */
UCLASS()
class COMBOGRAPH_API UComboGraphNodeBase : public UObject
{
	GENERATED_BODY()

public:
	// TODO: Rework debug system to use BP debugger
	bool bDebugActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TArray<TObjectPtr<UComboGraphNodeBase>> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TArray<TObjectPtr<UComboGraphNodeBase>> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Combo Graph")
	TMap<TObjectPtr<UComboGraphNodeBase>, TObjectPtr<UComboGraphEdge>> Edges;

	/** When not empty, will draw title with specified value instead of using Animation Asset name (Montage or Sequence) */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	FText NodeTitle;

	/** Simply returns the value of NodeTitle, and is overridden in child classes to fallback to animation asset name if NodeTitle is empty. */
	virtual FText GetNodeTitle() const;

	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	virtual UComboGraphEdge* GetEdge(UComboGraphNodeBase* ChildNode);

	UFUNCTION(BlueprintCallable, Category = "Combo Graph")
	virtual bool IsLeafNode() const;

	/** Checks all child nodes and the edge they're connected with and return the */
	virtual UComboGraphEdge* GetEdgeWithInput(UInputAction* InputAction);

	/** Returns whether the class is not a direct descendant of native node classes, indicating it's implemented in BP */
	bool IsComboSubclassedInBlueprint() const;

#if WITH_EDITORONLY_DATA
	/**
	 * ContextMenuName is used in Combo Graph to generate context menu items (upon right click in the graph to add new nodes)
	 *
	 * Split up ContextMenuName by "|" to create a top category if there is more than one level.
	 *
	 * You can leave this empty to exclude this class from being considered when Combo Graph generates a context menu.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph", meta = (HideInComboGraphDetailsPanel))
	FText ContextMenuName;

	/** Set it to false to prevent context menu in graph to include the BP Class name */
	UPROPERTY(EditDefaultsOnly, Category = "Combo Graph", meta = (HideInComboGraphDetailsPanel))
	bool bIncludeClassNameInContextMenu = true;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	EACENodeLimit ParentLimitType = EACENodeLimit::Unlimited;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph" , meta = (ClampMin = "0", EditCondition = "ParentLimitType == ENodeLimit::Limited", EditConditionHides))
	int32 ParentLimit = 0;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph")
	EACENodeLimit ChildrenLimitType = EACENodeLimit::Unlimited;

	// UPROPERTY(EditDefaultsOnly, Category = "Combo Graph" , meta = (ClampMin = "0", EditCondition = "ChildrenLimitType == ENodeLimit::Limited", EditConditionHides))
	int32 ChildrenLimit = 0;
#endif

#if WITH_EDITOR
	virtual bool IsNameEditable() const;

	virtual void SetNodeTitle(const FText& NewTitle);

	virtual FLinearColor GetBackgroundColor() const { return FLinearColor::Black; };

	virtual bool CanCreateConnection(UComboGraphNodeBase* Other, FText& ErrorMessage);
	virtual bool CanCreateConnectionTo(UComboGraphNodeBase* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage);
	virtual bool CanCreateConnectionFrom(UComboGraphNodeBase* Other, int32 NumberOfParentNodes, FText& ErrorMessage);
#endif
};
