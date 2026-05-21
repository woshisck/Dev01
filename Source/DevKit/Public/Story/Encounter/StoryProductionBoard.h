#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StoryProductionBoard.generated.h"

UENUM(BlueprintType)
enum class EStoryProductionStatus : uint8
{
	Designed UMETA(DisplayName = "设计中"),
	InSimulator UMETA(DisplayName = "已进模拟器"),
	InEncounterMap UMETA(DisplayName = "已进流程图"),
	PlacedInLevel UMETA(DisplayName = "已在关卡放置"),
	Connected UMETA(DisplayName = "已接入触发"),
	PIEVerified UMETA(DisplayName = "已PIE验证"),
	Done UMETA(DisplayName = "完成"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryProductionRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName RequirementId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName FlowId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FText PointName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FText PlayerAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FText ExperienceGoal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName EncounterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName LevelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	FName PlacementName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	EStoryProductionStatus Status = EStoryProductionStatus::Designed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单", meta = (MultiLine = true))
	FText Notes;
};

UCLASS(BlueprintType)
class DEVKIT_API UStoryProductionBoardDA : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "制作清单")
	TArray<FStoryProductionRow> Rows;

	const FStoryProductionRow* FindRow(FName RequirementId) const;
};
