#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Data/EnemyData.h"
#include "Misc/NotifyHook.h"
#include "Styling/SlateColor.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
struct FPropertyAndParent;
class IDetailsView;
class SBox;
class SSearchBox;
class UAbilityData;
class UEnemyDataActorFactory;
class UGASTemplate;

enum class EEnemyLibraryCategory : uint8
{
	Official,
	Placeholder,
	Legacy,
	Test,
};

enum class EEnemyManagerPage : uint8
{
	Overview,
	CharacterData,
	AbilityData,
	Blueprint,
	AIAndCombat,
	SpawnAndRewards,
};

enum class EEnemyAbilityInspector : uint8
{
	AbilityData,
	GasTemplate,
};

struct FEnemyManagerItem
{
	FAssetData AssetData;
	TWeakObjectPtr<UEnemyData> EnemyData;
	TSharedPtr<FAssetThumbnail> Thumbnail;
	FString EnemyId;
	FString EnglishName;
	FText DisplayName;
	EEnemyCombatTier CombatTier = EEnemyCombatTier::Normal;
	EEnemyProductionStatus ProductionStatus = EEnemyProductionStatus::Placeholder;
	EEnemyLibraryCategory LibraryCategory = EEnemyLibraryCategory::Placeholder;
	bool bPlaceholder = false;
	bool bRequired = true;
	bool bTestOnly = false;
	FString SearchText;
	FText ValidationText;
	FSlateColor ValidationColor = FSlateColor::UseForeground();
};

using FEnemyManagerItemPtr = TSharedPtr<FEnemyManagerItem>;

class SEnemyManagerWidget final : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SEnemyManagerWidget)
		: _SkipInitialAssetScan(false)
		{
		}
		SLATE_ARGUMENT(bool, SkipInitialAssetScan)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SelectEnemyForInteraction(FEnemyManagerItemPtr Item);
	FReply BeginEnemyDrag(FEnemyManagerItemPtr Item);
	virtual void NotifyPostChange(
		const FPropertyChangedEvent& PropertyChangedEvent,
		FProperty* PropertyThatChanged) override;

#if WITH_DEV_AUTOMATION_TESTS
	bool HasCorePanelsForTesting() const;
	static int32 GetRequiredFormalPlaceholderCountForTesting();
	static int32 GetOptionalTestPlaceholderCountForTesting();
	static TArray<FString> GetRequiredFormalEnemyIdsForTesting();
	static TArray<FString> GetOptionalTestEnemyIdsForTesting();
	static bool IsOfficialEnemyPathForTesting(const FString& PackageName);
	static bool IsTestEnemyPathForTesting(const FString& PackageName);
#endif

private:
	friend class SEnemyManagerDragCard;

	struct FEnemyCatalogDefinition
	{
		FString EnemyId;
		FText DisplayName;
		EEnemyCombatTier CombatTier = EEnemyCombatTier::Normal;
		bool bTestOnly = false;
		bool bRequired = true;
		TArray<FString> Aliases;
	};

	static const TCHAR* FormalEnemyRoot;
	static const TCHAR* TestEnemyRoot;
	static const TCHAR* LegacyEnemyRoot;

	static const TArray<FEnemyCatalogDefinition>& GetCatalog();
	static bool IsUnderRoot(const FString& PackageName, const FString& Root);
	static bool IsOfficialEnemyPackage(const FString& PackageName);
	static bool IsTestEnemyPackage(const FString& PackageName);
	static FString NormalizeEnemyIdentity(const FString& Value);
	static FText GetTierText(EEnemyCombatTier Tier);
	static FString GetTierFolder(EEnemyCombatTier Tier);
	static FText GetLibraryCategoryText(EEnemyLibraryCategory Category);

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildCenterPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildPageTabs();
	TSharedRef<SWidget> BuildPageControls();
	TSharedRef<SWidget> BuildLibraryFilters();
	TSharedRef<SWidget> BuildLibraryFilterButton(EEnemyLibraryCategory Category, const FText& Label);
	TSharedRef<SWidget> BuildDependencyPanel();
	TSharedRef<SWidget> BuildValidationPanel();
	TSharedRef<SWidget> BuildValidationMessage(
		const FText& Message,
		const FLinearColor& Color,
		bool bStrong) const;
	TSharedRef<ITableRow> GenerateEnemyRow(FEnemyManagerItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<SWidget> BuildPlaceholderInspector(const FEnemyManagerItemPtr& Item);
	TSharedRef<SWidget> BuildEmptyInspector(const FText& Message) const;

	void RefreshEntries(bool bKeepSelection);
	void RebuildFilteredEntries();
	void RefreshInspector();
	void RefreshRightPanel();
	void RefreshSelectedValidation();
	void LoadEnemyForItem(const FEnemyManagerItemPtr& Item);
	void RebuildCenterPage();
	bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	void HandleSearchChanged(const FText& NewText);
	void HandleSelectionChanged(FEnemyManagerItemPtr Item, ESelectInfo::Type SelectInfo);
	void HandleLibraryFilterChanged(ECheckBoxState NewState, EEnemyLibraryCategory Category);
	ECheckBoxState IsLibraryFilterChecked(EEnemyLibraryCategory Category) const;
	FReply HandleEnemyListKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	FReply SetPage(EEnemyManagerPage NewPage);
	FReply SetAbilityInspector(EEnemyAbilityInspector NewInspector);
	FSlateColor GetPageButtonColor(EEnemyManagerPage Page) const;
	FReply CreateSelectedPlaceholder();
	FReply SaveSelectedEnemy();
	FReply OpenSelectedEnemy() const;
	FReply SyncSelectedEnemy();
	FReply OpenReferencedAsset(UObject* Asset) const;
	void HandleEnemyClassChanged(const UClass* NewClass);
	const UClass* GetSelectedEnemyClass() const;
	void HandleAbilityDataChanged(const FAssetData& AssetData);
	void HandleGasTemplateChanged(const FAssetData& AssetData);
	FString GetAbilityDataObjectPath() const;
	FString GetGasTemplateObjectPath() const;

	void CountValidationIssues(const FEnemyManagerItemPtr& Item, int32& OutErrors, int32& OutWarnings) const;
	FText BuildValidationText(const FEnemyManagerItemPtr& Item) const;
	FSlateColor BuildValidationColor(const FEnemyManagerItemPtr& Item) const;
	bool CatalogMatchesEnemy(const FEnemyCatalogDefinition& Definition, const UEnemyData* Enemy, const FAssetData& AssetData) const;
	FString BuildPlaceholderPackageName(const FEnemyManagerItemPtr& Item) const;
	void SetStatus(const FText& InStatus, bool bError = false);

	FText GetSelectedTitle() const;
	FText GetSelectedSubtitle() const;
	FText GetDependencySummary() const;
	FText GetSelectedValidationText() const;
	FSlateColor GetSelectedValidationColor() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;

	TArray<FEnemyManagerItemPtr> AllEntries;
	TArray<FEnemyManagerItemPtr> FilteredEntries;
	FEnemyManagerItemPtr SelectedItem;
	TSet<EEnemyLibraryCategory> EnabledLibraryFilters;
	FString SearchFilter;
	FText StatusText;
	bool bStatusError = false;
	EEnemyManagerPage CurrentPage = EEnemyManagerPage::Overview;
	EEnemyAbilityInspector CurrentAbilityInspector = EEnemyAbilityInspector::AbilityData;

	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SListView<FEnemyManagerItemPtr>> EnemyListView;
	TSharedPtr<SBox> InspectorHost;
	TSharedPtr<SBox> CenterPanelBox;
	TSharedPtr<SBox> PageControlsHost;
	TSharedPtr<SBox> RightPanelHost;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TStrongObjectPtr<UObject> ActiveDragFactoryRoot;
};
