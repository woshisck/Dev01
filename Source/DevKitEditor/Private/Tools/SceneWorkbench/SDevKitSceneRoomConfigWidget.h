#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class IDetailTreeNode;
class IPropertyHandle;
class IPropertyRowGenerator;
class SBox;
class URoomDataAsset;

/** Flat room authoring cards. Property handles retain the existing asset/undo contract. */
class SDevKitSceneRoomConfigWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitSceneRoomConfigWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetRoom(URoomDataAsset* InRoom);
	TSharedRef<SWidget> MakeDefaultsPanel();
	TSharedRef<SWidget> MakeGameplayPanel();
	URoomDataAsset* GetRoom() const;

#if WITH_DEV_AUTOMATION_TESTS
	bool HasPropertyForTesting(FName Name) const;
	bool AddArrayItemForTesting(FName Name);
	bool DeleteArrayItemForTesting(FName Name, int32 Index);
	TSharedPtr<IPropertyHandle> GetArrayElementPropertyForTesting(FName ArrayName,
		int32 Index, FName ChildName = NAME_None) const;
#endif

private:
	struct FFieldColumn
	{
		FName Name;
		FText Label;
		float Weight = 1.0f;
	};

	void GatherHandles(const TArray<TSharedRef<IDetailTreeNode>>& Nodes);
	void RebuildPanels();
	void QueueRebuild();
	void OnPropertiesChanged(const FPropertyChangedEvent& Event);
	TSharedPtr<IPropertyHandle> FindHandle(FName Name) const;
	bool IsHandleEditable(TSharedPtr<IPropertyHandle> Handle) const;
	TSharedRef<SWidget> BuildDefaults();
	TSharedRef<SWidget> BuildGameplay();
	TSharedRef<SWidget> MakeCard(const FText& Title, const FText& Description,
		TSharedRef<SWidget> Content, FLinearColor Accent, bool bAdvanced = false);
	TSharedRef<SWidget> MakeField(FName Name, const FText& Label, const FText& Hint = FText::GetEmpty());
	TSharedRef<SWidget> MakeHandleField(TSharedPtr<IPropertyHandle> Handle,
		const FText& Label, const FText& Hint = FText::GetEmpty());
	TSharedRef<SWidget> MakeValue(TSharedPtr<IPropertyHandle> Handle);
	TSharedRef<SWidget> MakeArray(TSharedPtr<IPropertyHandle> Handle, const FText& EntryLabel,
		const TArray<FFieldColumn>& Columns = {});
	TSharedRef<SWidget> MakeDifficulty();
	TSharedRef<SWidget> MakePortals();
	TSharedRef<SWidget> MakeEmptyState() const;
	FReply AddArrayItem(TSharedPtr<IPropertyHandle> Handle);
	FReply DeleteArrayItem(TSharedPtr<IPropertyHandle> Handle, int32 Index);

	TStrongObjectPtr<URoomDataAsset> Room;
	TSharedPtr<IPropertyRowGenerator> RowGenerator;
	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;
	TSharedPtr<SBox> DefaultsPanel;
	TSharedPtr<SBox> GameplayPanel;
	TMap<FString, bool> ExpandedCards;
	bool bRebuildQueued = false;
};
