// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UComboGraphAbilityTask_StartGraph;
class UComboGraphNodeBase;
class UComboGraph;

class COMBOGRAPHEDITOR_API FComboGraphDebugger : public FTickableGameObject
{
public:
	FComboGraphDebugger();
	~FComboGraphDebugger();

	//~ Begin FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(FComboGraphEditorTickHelper, STATGROUP_Tickables); }
	//~ End FTickableGameObject

	/** Refresh the debugging information we are displaying (only when paused, as Tick() updates when running) */
	void Setup(UComboGraph* InComboGraphAsset, TSharedRef<class FComboGraphAssetEditor> InEditorOwner);

	bool IsDebuggerReady() const;
	bool IsDebuggerRunning() const;

	UComboGraphAbilityTask_StartGraph* GetDebuggedTaskForSelectedActor() const;
	AActor* GetSelectedActor() const;

	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);
	void OnPausePIE(const bool bIsSimulating);

	void OnObjectSelected(UObject* Object);
	void OnComboGraphStarted(const UComboGraphAbilityTask_StartGraph& InOwnerTask, const UComboGraph& InComboGraphAsset);
	void OnComboGraphEnded(const UComboGraphAbilityTask_StartGraph& InOwnerTask, const UComboGraph& InComboGraphAsset);

	static bool IsPlaySessionPaused();
	static bool IsPlaySessionRunning();
	static bool IsPIESimulating();
	static bool IsPIENotSimulating();

	FString GetDebuggedInstanceDesc() const;
	FString GetActorLabel(const AActor* InActor, const bool bIncludeNetModeSuffix = true, const bool bIncludeSpawnedContext = true) const;
	FString DescribeInstance(const AActor& ActorToDescribe) const;
	void OnInstanceSelectedInDropdown(AActor* SelectedActor);
	void GetMatchingInstances(TArray<UComboGraphAbilityTask_StartGraph*>& MatchingInstances, TArray<AActor*>& MatchingActors);

private:

	/** owning editor */
	TWeakPtr<FComboGraphAssetEditor> EditorOwner;

	/** asset for debugging */
	UComboGraph* ComboGraphAsset;

	/** root node in asset's graph */
	TWeakObjectPtr<UComboGraphNodeBase> RootNode;

	/** instance for debugging */
	TWeakObjectPtr<AActor> ActorInstance;

	/** all known ComboGraph task instances, cached for dropdown list */
	TArray<TWeakObjectPtr<UComboGraphAbilityTask_StartGraph>> KnownInstances;

	/** all known ComboGraph owner instances, cached for dropdown list */
	TArray<TWeakObjectPtr<AActor>> KnownActors;

	/** cached PIE state */
	bool bIsPIEActive = false;
};
