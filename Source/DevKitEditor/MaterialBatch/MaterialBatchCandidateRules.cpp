#include "MaterialBatch/MaterialBatchCandidateRules.h"

FMaterialBatchCandidateDecision FMaterialBatchCandidateRules::ClassifyComponent(const FMaterialBatchComponentScanInput& Input)
{
	if (!Input.bIsStaticMeshComponent)
	{
		return { false, EMaterialBatchCandidateRejectReason::NotStaticMeshComponent };
	}

	if (!Input.bHasStaticMobility)
	{
		return { false, EMaterialBatchCandidateRejectReason::DynamicMobility };
	}

	if (Input.bHasExplicitExcludeTag)
	{
		return { false, EMaterialBatchCandidateRejectReason::ExplicitExcludeTag };
	}

	if (Input.bHasGameplayCriticalTag)
	{
		return { false, EMaterialBatchCandidateRejectReason::GameplayCriticalTag };
	}

	if (Input.bHasInteractiveTag)
	{
		return { false, EMaterialBatchCandidateRejectReason::InteractiveTag };
	}

	if (Input.MaterialSlotCount <= 0)
	{
		return { false, EMaterialBatchCandidateRejectReason::NoMaterialSlots };
	}

	if (Input.LodCount <= 0)
	{
		return { false, EMaterialBatchCandidateRejectReason::NoLods };
	}

	return { true, EMaterialBatchCandidateRejectReason::None };
}

FMaterialBatchComponentScanInput FMaterialBatchCandidateRules::BuildInputFromTags(
	const FMaterialBatchComponentScanInput& BaseInput,
	const TArray<FName>& ActorTags)
{
	FMaterialBatchComponentScanInput Result = BaseInput;
	for (const FName& Tag : ActorTags)
	{
		const FString TagString = Tag.ToString();
		if (TagString.Equals(TEXT("EnvBatch.Include"), ESearchCase::IgnoreCase))
		{
			Result.bHasExplicitIncludeTag = true;
		}
		else if (TagString.Equals(TEXT("EnvBatch.Exclude"), ESearchCase::IgnoreCase))
		{
			Result.bHasExplicitExcludeTag = true;
		}
		else if (TagString.Equals(TEXT("GameplayCritical"), ESearchCase::IgnoreCase)
			|| TagString.Equals(TEXT("EnvBatch.GameplayCritical"), ESearchCase::IgnoreCase))
		{
			Result.bHasGameplayCriticalTag = true;
		}
		else if (TagString.Equals(TEXT("Interactive"), ESearchCase::IgnoreCase)
			|| TagString.Equals(TEXT("EnvBatch.Interactive"), ESearchCase::IgnoreCase))
		{
			Result.bHasInteractiveTag = true;
		}
	}
	return Result;
}

FString FMaterialBatchCandidateRules::RejectReasonToString(EMaterialBatchCandidateRejectReason Reason)
{
	switch (Reason)
	{
	case EMaterialBatchCandidateRejectReason::None:
		return TEXT("None");
	case EMaterialBatchCandidateRejectReason::NotStaticMeshComponent:
		return TEXT("NotStaticMeshComponent");
	case EMaterialBatchCandidateRejectReason::DynamicMobility:
		return TEXT("DynamicMobility");
	case EMaterialBatchCandidateRejectReason::NoMaterialSlots:
		return TEXT("NoMaterialSlots");
	case EMaterialBatchCandidateRejectReason::NoLods:
		return TEXT("NoLods");
	case EMaterialBatchCandidateRejectReason::ExplicitExcludeTag:
		return TEXT("ExplicitExcludeTag");
	case EMaterialBatchCandidateRejectReason::GameplayCriticalTag:
		return TEXT("GameplayCriticalTag");
	case EMaterialBatchCandidateRejectReason::InteractiveTag:
		return TEXT("InteractiveTag");
	default:
		return TEXT("Unknown");
	}
}
