#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericGraph.h"
#include "GenericGraphEdge.h"
#include "GenericGraphNode.h"
#include "GameplayAbilityComboGraph.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EYogComboGraphInputAction : uint8
{
	Light UMETA(DisplayName = "Light"),
	Heavy UMETA(DisplayName = "Heavy"),
	Dash UMETA(DisplayName = "Dash"),
	Any UMETA(DisplayName = "Any")
};

UENUM(BlueprintType)
enum class EYogComboGraphDashSaveMode : uint8
{
	None UMETA(DisplayName = "None"),
	PreserveIfSourceAllows UMETA(DisplayName = "Preserve If Source Allows"),
	ForcePreserve UMETA(DisplayName = "Force Preserve")
};

/**
 * 决定 ComboGraph 节点在运行时激活哪一个 GA。
 * Melee → UGA_MeleeAttack（默认）
 * Range → UGA_RangeAttack
 * 项目侧 (ComboRuntimeComponent) 负责根据此枚举选择激活类。
 */
UENUM(BlueprintType)
enum class EYogComboGraphAttackType : uint8
{
	Melee UMETA(DisplayName = "Melee"),
	Range UMETA(DisplayName = "Range")
};

UCLASS(BlueprintType, Blueprintable)
class YOGCOMBOGRAPH_API UGameplayAbilityComboGraphNode : public UGenericGraphNode
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphNode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FName NodeId = NAME_None;

	/** Player input that starts this node when it is a root node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EYogComboGraphInputAction RootInputAction = EYogComboGraphInputAction::Any;

	/** Melee → GA_MeleeAttack, Range → GA_RangeAttack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EYogComboGraphAttackType AttackType = EYogComboGraphAttackType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = "1"))
	int32 TotalFrames = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bAllowDashSave = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	EYogComboGraphDashSaveMode DashSaveMode = EYogComboGraphDashSaveMode::PreserveIfSourceAllows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "0.0"))
	float DashSaveExpireSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	bool bSavePendingLinkContext = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	bool bClearCombatTagsOnDashEnd = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	bool bBreakComboOnDashCancel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window")
	bool bUseNodeComboWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowStartFrame = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowEndFrame = 27;

	/** Optional trigger-timing tag (project-defined, e.g. Combo.TriggerTiming.OnHit / OnCommit). Empty = project default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card", meta = (AdvancedDisplay))
	FGameplayTag TriggerTimingTag;

	virtual FText GetDescription_Implementation() const override;

#if WITH_EDITORONLY_DATA
	bool bDebugActive = false;
#endif

#if WITH_EDITOR
	virtual FLinearColor GetBackgroundColor() const override;
	virtual FText GetNodeTitle() const override;
	virtual bool CanCreateConnectionTo(UGenericGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage) override;
#endif
};

UCLASS(BlueprintType, Blueprintable)
class YOGCOMBOGRAPH_API UGameplayAbilityComboGraphEdge : public UGenericGraphEdge
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphEdge();

	/** Player input that traverses this edge. Any accepts any player combo input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EYogComboGraphInputAction InputAction = EYogComboGraphInputAction::Any;

	/** Optional gate against the owner's owned gameplay tags. Empty = no gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FGameplayTagQuery StateRequirement;

#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
#endif
};

UCLASS(BlueprintType, Blueprintable)
class YOGCOMBOGRAPH_API UGameplayAbilityComboGraph : public UGenericGraph
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraph();

	const UGameplayAbilityComboGraphNode* FindRootComboNode(EYogComboGraphInputAction InputAction) const;
	const UGameplayAbilityComboGraphNode* FindChildComboNode(FName ParentNodeId, EYogComboGraphInputAction InputAction, const FGameplayTagContainer* OwnedTags = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ValidateComboGraph(TArray<FText>& OutWarnings) const;
};
