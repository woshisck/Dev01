#pragma once

#include "CoreMinimal.h"

enum class EMaterialBatchCandidateRejectReason : uint8
{
	None,
	NotStaticMeshComponent,
	DynamicMobility,
	NoMaterialSlots,
	NoLods,
	ExplicitExcludeTag,
	GameplayCriticalTag,
	InteractiveTag
};

struct FMaterialBatchComponentScanInput
{
	bool bIsStaticMeshComponent = false;
	bool bHasStaticMobility = false;
	bool bHasExplicitIncludeTag = false;
	bool bHasExplicitExcludeTag = false;
	bool bHasGameplayCriticalTag = false;
	bool bHasInteractiveTag = false;
	int32 MaterialSlotCount = 0;
	int32 LodCount = 0;
};

struct FMaterialBatchCandidateDecision
{
	bool bEligible = false;
	EMaterialBatchCandidateRejectReason Reason = EMaterialBatchCandidateRejectReason::None;
};

class FMaterialBatchCandidateRules
{
public:
	static FMaterialBatchCandidateDecision ClassifyComponent(const FMaterialBatchComponentScanInput& Input);
	static FMaterialBatchComponentScanInput BuildInputFromTags(const FMaterialBatchComponentScanInput& BaseInput, const TArray<FName>& ActorTags);
	static FString RejectReasonToString(EMaterialBatchCandidateRejectReason Reason);
};
