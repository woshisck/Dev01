// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphRuntimeTypes.h"
#include "ComboGraphEditorTypes.generated.h"

/** DataTable used to draw edge (transition) icons in Graph. Determine mappings between Keys and Icon textures */
USTRUCT(BlueprintType)
struct COMBOGRAPHEDITOR_API FComboGraphInputsMetaData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo Graph")
	FKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo Graph")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo Graph")
	EComboGraphIconType IconType = EComboGraphIconType::Keyboard;
};

/** Holds Graph Pin Categories Names as constants */
UCLASS()
class UComboGraphPinNames : public UObject
{
	GENERATED_UCLASS_BODY()

	static const FName PinCategory_EntryOut;
	static const FName PinCategory_MultipleNodes;
	static const FName PinCategory_Transition;
	static const FName PinCategory_Edge;

	static const FName PinName_InLeft;
	static const FName PinName_InTop;
	static const FName PinName_OutRight;
	static const FName PinName_OutBottom;
};


/** Predicate class for sorting operation in auto arrange */
struct FComboGraphCompareNodeXLocation
{
	FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
	{
		const UEdGraphNode* NodeA = A.GetOwningNode();
		const UEdGraphNode* NodeB = B.GetOwningNode();

		if (NodeA->NodePosX == NodeB->NodePosX)
		{
			return NodeA->NodePosY < NodeB->NodePosY;
		}

		return NodeA->NodePosX < NodeB->NodePosX;
	}
};

UENUM()
enum class EComboGraphAutoArrangeStrategy : uint8
{
	/** Setup Event */
	Vertical,

	/** Update Event */
	Horizontal,

	/** None - invalid */
	None UMETA(Hidden),
};

struct FComboGraphEditorModes
{
	// Mode constants
	static const FName ComboGraphPersonaModeID;
	static const FName ComboGraphDefaultModeID;

	static FText GetLocalizedMode(const FName InMode)
	{
		static TMap<FName, FText> LocModes;

		if (LocModes.Num() == 0)
		{
			LocModes.Add(ComboGraphPersonaModeID, NSLOCTEXT("ComboGraphEditorModes", "ComboGraphPersonaMode", "Persona Preview"));
			LocModes.Add(ComboGraphDefaultModeID, NSLOCTEXT("ComboGraphEditorModes", "ComboGraphDefaultMode", "Combo Graph"));
		}

		check(InMode != NAME_None);
		const FText* OutDesc = LocModes.Find(InMode);
		check(OutDesc);
		return *OutDesc;
	}
private:
	FComboGraphEditorModes() {}
};

