// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ComboGraphEdNodeBase.h"
#include "ComboGraphEdNode.generated.h"

class UComboGraphNodeBase;
class UComboGraphNodeAnimBase;
class UComboGraphEdNodeEdge;
class SComboGraphNode;

UCLASS(MinimalAPI)
class UComboGraphEdNode : public UComboGraphEdNodeBase
{
	GENERATED_BODY()

public:
	UComboGraphEdNode();

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Combo Graph")
	TObjectPtr<UComboGraphNodeAnimBase> RuntimeNode;

	SComboGraphNode* SlateNode;

	virtual FLinearColor GetBackgroundColor() const;

	//~ UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual void PrepareForCopying() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
	//~ End UEdGraphNode interface

	const UComboGraphNodeBase* GetDebuggedNode() const;

	bool IsDebugActive() const { return bIsDebugActive; }
	bool WasDebugActive() const { return bWasDebugActive; }
	float GetDebugTotalTime() const { return DebugTotalTime; }
	float GetDebugElapsedTime() const { return DebugElapsedTime; }

	/** Called from associated Slate Node widget tick to pass in time ticks and update debug information. */
	virtual void UpdateTime(float DeltaTime);

	/** Updates Error Text based on Skeletal Mesh passed in, checking if anim asset defined with this node has a skeleton compatible with passed in Mesh */
	void UpdateErrorReporting(USkeletalMesh* InSkeletalMesh, FText ErrorMessage) const;

protected:
	bool bIsDebugActive = false;
	bool bWasDebugActive = false;

	/** Resets when debug active states change. */
	float DebugTotalTime = 0.f;

	/** The amount of time the observed node was active */
	float DebugElapsedTime = 0.f;

	/** Returns amount of time this node has been marked as was active */
	float WasActiveTime() const;

	/** Update time logic for when runtime node is not the same as debugged node (node changed) */
	void UpdateTimeRuntimeNodeChanged(float DeltaTime, float DebugMaxTime);

	/** Update time logic for when runtime is currently active and same as as debugged node */
	void UpdateTimeCurrentNode(float DeltaTime, float DebugMaxTime);
};
