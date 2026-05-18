#pragma once

#include "CoreMinimal.h"
#include "GenericGraphAssetEditor/AssetEditor_GenericGraph.h"
#include "Containers/Ticker.h"

/**
 * Custom asset editor for UGameplayAbilityComboGraph.
 * Extends the generic graph editor with PIE debug highlighting:
 * during Play-In-Editor the currently active combo node turns green,
 * mirroring the behaviour of AnimBP state machine debuggers.
 */
class FAssetEditor_ComboGraph : public FAssetEditor_GenericGraph
{
public:
	FAssetEditor_ComboGraph();
	virtual ~FAssetEditor_ComboGraph();

private:
	void OnPIEStarted(bool bIsSimulating);
	void OnPIEEnded(bool bIsSimulating);

	bool TickDebugger(float DeltaTime);
	void ClearDebugState();

	FTSTicker::FDelegateHandle DebugTickHandle;
	FDelegateHandle PIEStartHandle;
	FDelegateHandle PIEEndHandle;
};
