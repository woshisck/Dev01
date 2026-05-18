#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagQuery.h"
#include "GenericGraph.h"
#include "GenericGraphEdge.h"
#include "GenericGraphNode.h"
#include "GameplayAbilityComboGraph.generated.h"

class UAnimMontage;
class UDataAsset;
class USoundBase;
class UFXSystemAsset;

/** Scope of the time dilation applied when a hit lands during a combo node's montage. */
UENUM(BlueprintType)
enum class EComboHitDilationScope : uint8
{
	None    UMETA(DisplayName = "None"),     // No dilation.
	Global  UMETA(DisplayName = "Global"),   // World time dilation (everyone slows down).
	Self    UMETA(DisplayName = "Self"),     // Only the player's CustomTimeDilation.
};

USTRUCT(BlueprintType)
struct YOGCOMBOGRAPH_API FComboHitDilationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDilation")
	EComboHitDilationScope Scope = EComboHitDilationScope::None;

	/** Time dilation factor while active. 0.05 ≈ near freeze, 1.0 = no slowdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDilation",
		meta = (EditCondition = "Scope != EComboHitDilationScope::None", ClampMin = "0.01", ClampMax = "1.0"))
	float DilationFactor = 0.15f;

	/** Real-time duration (in seconds) the dilation lasts before restoring 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDilation",
		meta = (EditCondition = "Scope != EComboHitDilationScope::None", ClampMin = "0.0"))
	float DurationSeconds = 0.08f;
};

USTRUCT(BlueprintType)
struct YOGCOMBOGRAPH_API FComboNodeFxBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TObjectPtr<USoundBase> Sound = nullptr;

	/** Niagara or legacy ParticleSystem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	TObjectPtr<UFXSystemAsset> ParticleSystem = nullptr;

	/** Optional socket on the player to attach the particle to. NAME_None = world-spawn at the actor location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	FName AttachSocket = NAME_None;
};

UCLASS(BlueprintType, Blueprintable)
class YOGCOMBOGRAPH_API UGameplayAbilityComboGraphNode : public UGenericGraphNode
{
	GENERATED_BODY()

public:
	UGameplayAbilityComboGraphNode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FName NodeId = NAME_None;

	/** Required input tag for this node to qualify as a root entry. Invalid/empty = matches any input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FGameplayTag RootInputTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage", meta = (ClampMin = "1"))
	int32 TotalFrames = 30;

	/** Optional project-defined payload (e.g. attack data DA). The plugin stores it loosely typed so it can be
	 *  reused outside the original project; consumers cast to their concrete type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<UDataAsset> AttackDataOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bIsComboFinisher = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bAllowDashSave = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window")
	bool bUseNodeComboWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowStartFrame = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Window", meta = (EditCondition = "bUseNodeComboWindow", ClampMin = "0"))
	int32 ComboWindowEndFrame = 27;

	/** Optional trigger-timing tag (project-defined, e.g. Combo.TriggerTiming.OnHit / OnCommit). Empty = project default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card", meta = (AdvancedDisplay))
	FGameplayTag TriggerTimingTag;

	/** Freeze/slow-motion applied when a hit lands during this node's montage. Scope = None disables it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|HitReaction")
	FComboHitDilationSettings HitSuccessDilation;

	/** SFX/VFX played when this node's montage starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|FX")
	FComboNodeFxBinding OnMontageStartFx;

	/** SFX/VFX played when a hit lands during this node's montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|FX")
	FComboNodeFxBinding OnHitSuccessFx;

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

	/** Inputs that may traverse this edge (OR semantics). Empty = matches any input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	FGameplayTagContainer AcceptedInputTags;

	/** Optional gate against the owner's owned gameplay tags (e.g. must be grounded, must have parry-success buff, must not be stunned). Empty = no gate. */
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

	const UGameplayAbilityComboGraphNode* FindRootComboNode(FGameplayTag InputTag) const;
	const UGameplayAbilityComboGraphNode* FindChildComboNode(FName ParentNodeId, FGameplayTag InputTag, const FGameplayTagContainer* OwnedTags = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ValidateComboGraph(TArray<FText>& OutWarnings) const;
};
