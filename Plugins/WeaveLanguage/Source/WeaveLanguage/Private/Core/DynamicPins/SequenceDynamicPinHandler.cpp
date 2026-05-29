#include "SequenceDynamicPinHandler.h"
#include "Core/WeaveInterpreter.h"
#include "K2Node_ExecutionSequence.h"

void FSequenceDynamicPinHandler::PreScanLinks(const TArray<FWeaveLinkStmt>& Links,
                                              const TMap<FString, UK2Node*>& CreatedNodes)
{
	SequenceMaxPinIndex.Empty();

	for (const FWeaveLinkStmt& Link : Links)
	{
		if (Link.FromPin.StartsWith(TEXT("then_")))
		{
			if (FString IndexStr = Link.FromPin.RightChop(5); IndexStr.IsNumeric())
			{
				const int32 Index = FCString::Atoi(*IndexStr);
				if (int32& MaxIndex = SequenceMaxPinIndex.FindOrAdd(Link.FromNode, -1); Index > MaxIndex)
				{
					MaxIndex = Index;
				}
			}
		}
	}
}

void FSequenceDynamicPinHandler::AddDynamicPins(const TMap<FString, UK2Node*>& CreatedNodes)
{
	for (const auto& Pair : SequenceMaxPinIndex)
	{
		if (UK2Node* const* NodePtr = CreatedNodes.Find(Pair.Key))
		{
			if (UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(*NodePtr))
			{
				int32 MaxIndex = Pair.Value;
				for (int32 i = 2; i <= MaxIndex; ++i)
				{
					SeqNode->AddInputPin();
					UE_LOG(LogTemp, Log, TEXT("[Weaver] Added dynamic pin then_%d to Sequence node %s"), i, *Pair.Key);
				}
			}
		}
	}
}
