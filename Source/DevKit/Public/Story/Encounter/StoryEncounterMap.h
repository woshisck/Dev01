#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/Encounter/StoryEncounterTypes.h"
#include "StoryEncounterMap.generated.h"

UCLASS(BlueprintType)
class DEVKIT_API UStoryEncounterMap : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情流程")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情流程")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "剧情流程")
	TArray<FStoryEncounterNode> Nodes;

	const FStoryEncounterNode* FindNode(FName NodeId) const;
	FStoryEncounterNode* FindNodeMutable(FName NodeId);
};
