#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterPointDataAsset.generated.h"

class UFlowAsset;

/**
 * A designer-authored story/tutorial point.
 *
 * This is the asset level designers place on triggers, and graph nodes bind to it for visual flow authoring.
 */
UCLASS(BlueprintType, meta = (DisplayName = "剧情教学点"))
class DEVKIT_API UStoryEncounterPointDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点")
	EStoryEncounterNodeKind Kind = EStoryEncounterNodeKind::Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点", meta = (MultiLine = true))
	FText PlayerFacingEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情点")
	EStoryEncounterFirePolicy FirePolicy = EStoryEncounterFirePolicy::Once;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发条件")
	FStoryEncounterCondition Condition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "触发结果")
	TArray<FStoryEncounterAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "节点 Flow")
	TObjectPtr<UFlowAsset> NodeEventFlow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "关卡放置")
	FName PlacementLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "关卡放置")
	FName PlacementName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "编辑器")
	FVector2D EditorPosition = FVector2D::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	FName GetStableNodeId() const;

	UFUNCTION(BlueprintPure, Category = "剧情点")
	FStoryEncounterNode ToEncounterNode() const;
};
