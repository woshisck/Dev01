#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterPointDataAsset.generated.h"

/**
 * A designer-authored story/tutorial point.
 *
 * This is the asset level designers place on triggers, and graph nodes bind to it for visual flow authoring.
 */
UCLASS(BlueprintType)
class DEVKIT_API UStoryEncounterPointDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point")
	EStoryEncounterNodeKind Kind = EStoryEncounterNodeKind::Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point", meta = (MultiLine = true))
	FText PlayerFacingEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Point")
	EStoryEncounterFirePolicy FirePolicy = EStoryEncounterFirePolicy::Once;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Conditions")
	FStoryEncounterCondition Condition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger Results")
	TArray<FStoryEncounterAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Node Flow")
	TObjectPtr<UStoryFlowAsset> NodeEventFlow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Placement")
	FName PlacementLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Placement")
	FName PlacementName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Editor")
	FVector2D EditorPosition = FVector2D::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	FName GetStableNodeId() const;

	UFUNCTION(BlueprintPure, Category = "Story Point")
	FStoryEncounterNode ToEncounterNode() const;
};
