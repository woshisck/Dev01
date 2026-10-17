#pragma once

#include "CoreMinimal.h"
#include "Data/StateConflictDataAsset.h"
#include "UObject/StrongObjectPtr.h"

/** One editor diagnostic. Warnings never pretend that the whole GAS policy was validated. */
struct FDevKitActionWorkbenchIssue
{
	bool bError = false;
	FText Message;
	int32 RuleIndex = INDEX_NONE;
};

struct FDevKitActionWorkbenchAction
{
	FGameplayTag Id;
	FText DisplayName;
	bool bBuiltin = false;
};

/** Row = incoming request, column = current state. This is a tag/table projection, not GAS admission. */
struct FDevKitActionWorkbenchCell
{
	bool bBlocked = false;
	bool bCancels = false;
	bool bInheritedBlock = false;
	bool bInheritedCancel = false;
	bool bHasCurrentRule = false;
	bool bHasRequestRule = false;
	bool bAmbiguous = false;
	FText Explanation;
};

/**
 * Detached draft editor for the EXISTING StateConflict asset.
 * No source control operations, package saves, ability activation, or runtime refresh happen here.
 */
class FDevKitActionWorkbenchModel
{
public:
	bool Load(UStateConflictDataAsset* Asset, FText& OutMessage);
	bool Reset(FText& OutMessage);
	UStateConflictDataAsset* GetSource() const { return Source.Get(); }
	UStateConflictDataAsset* GetDraft() const { return Draft.Get(); }
	bool IsDirty() const;
	bool HasPendingApply() const { return bOwnsDirtySource; }
	bool HasSourceChanged() const;
	TArray<FDevKitActionWorkbenchIssue> Validate() const;
	static TArray<FDevKitActionWorkbenchIssue> ValidateAsset(const UStateConflictDataAsset* Asset);

	/** Explicitly changes only Source in an undoable transaction. Caller owns any save prompt. */
	bool ApplyDraft(FText& OutMessage);
	/** Call only immediately after a successful, explicit save of this model's source package. */
	bool AcknowledgeSavedSource(FText& OutMessage);
	bool SetBlocked(FGameplayTag Request, FGameplayTag Current, bool bEnabled, FText& OutMessage);
	bool SetCancel(FGameplayTag Request, FGameplayTag Current, bool bEnabled, FText& OutMessage);

	static TArray<FDevKitActionWorkbenchAction> GetActions(const UStateConflictDataAsset* Asset);
	static FDevKitActionWorkbenchCell EvaluateCell(const UStateConflictDataAsset* Asset,
		FGameplayTag Request, FGameplayTag Current);
	/** Deterministic JSON of actual rule data; rule order is intentionally retained. */
	static FString Snapshot(const UStateConflictDataAsset* Asset);

private:
	bool SetTarget(FGameplayTag Active, FGameplayTag Target, bool bCancel, bool bEnabled, FText& OutMessage);
	void CaptureBaseline();
	static FString DiskFingerprint(const UStateConflictDataAsset* Asset);

	TStrongObjectPtr<UStateConflictDataAsset> Source;
	TStrongObjectPtr<UStateConflictDataAsset> Draft;
	FString Baseline;
	FString BaselineDiskFingerprint;
	bool bSourceWasDirty = false;
	bool bOwnsDirtySource = false;
};
