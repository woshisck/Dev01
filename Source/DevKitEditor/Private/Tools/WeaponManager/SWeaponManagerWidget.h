#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "GameplayTagContainer.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/SListView.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
class IDetailsView;
class SBox;
class SEditableTextBox;
class SSearchBox;
class UAbilityData;
class UAnimMontage;
class UGA_WeaponSkill;
class UWeaponDefinitionActorFactory;
class UWeaponDefinition;
class UWeaponSkillDataAsset;

enum class EWeaponManagerPage : uint8
{
	Actions,
	Skills,
	Details,
};

enum class EWeaponManagerActionMode : uint8
{
	Basic,
	Passive,
};

enum class EWeaponManagerLibraryCategory : uint8
{
	Official,
	Test,
};

FORCEINLINE uint32 GetTypeHash(EWeaponManagerLibraryCategory Category)
{
	return static_cast<uint32>(Category);
}

struct FWeaponManagerWeaponItem
{
	FAssetData AssetData;
	TWeakObjectPtr<UWeaponDefinition> Weapon;
	TSharedPtr<FAssetThumbnail> Thumbnail;
	FText DisplayName;
	FText WeaponTypeText;
	FText ValidationText;
	FSlateColor ValidationColor;
	EWeaponManagerLibraryCategory LibraryCategory = EWeaponManagerLibraryCategory::Test;
	FString SearchText;
};

struct FWeaponManagerSkillType
{
	FGameplayTag SkillTag;
	FText DisplayName;
	FText Description;
	TSubclassOf<UGA_WeaponSkill> AbilityClass;
	TSubclassOf<UWeaponSkillDataAsset> DataAssetClass;
	TArray<FGameplayTag> RequiredMontageSlots;
};

class SWeaponManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponManagerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SelectWeaponForInteraction(TSharedPtr<FWeaponManagerWeaponItem> Item);
	FReply BeginWeaponDrag(TSharedPtr<FWeaponManagerWeaponItem> Item);

#if WITH_DEV_AUTOMATION_TESTS
	int32 GetBasicActionRowCountForTesting() const;
	int32 GetPassiveActionRowCountForTesting() const;
	int32 GetNativeSkillTypeCountForTesting() const;
	int32 GetRequiredMontageSlotCountForTesting(const FGameplayTag& SkillTag) const;
	bool HasCorePanelsForTesting() const;
	static bool IsOfficialWeaponPathForTesting(const FString& PackageName);
#endif

private:
	using FWeaponItemPtr = TSharedPtr<FWeaponManagerWeaponItem>;
	using FSkillTypePtr = TSharedPtr<FWeaponManagerSkillType>;

	void RefreshWeapons(bool bKeepSelection = true);
	void RefreshSkillTypes();
	void RebuildFilteredWeapons();
	void RebuildCenterPanel();
	void RefreshDetailsPanel();
	void RefreshStatus();

	TSharedRef<ITableRow> GenerateWeaponRow(FWeaponItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleWeaponSelectionChanged(FWeaponItemPtr Item, ESelectInfo::Type SelectInfo);
	void HandleSearchChanged(const FText& NewText);
	FReply HandleWeaponListKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	void HandleLibraryFilterChanged(ECheckBoxState NewState, EWeaponManagerLibraryCategory Category);
	ECheckBoxState IsLibraryFilterChecked(EWeaponManagerLibraryCategory Category) const;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildLibraryFilterButtons();
	TSharedRef<SWidget> BuildLibraryFilterButton(EWeaponManagerLibraryCategory Category, const FText& Label);
	TSharedRef<SWidget> BuildCenterPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildPageTabs();
	TSharedRef<SWidget> BuildActionPage();
	TSharedRef<SWidget> BuildBasicActionRows();
	TSharedRef<SWidget> BuildPassiveActionRows();
	TSharedRef<SWidget> BuildSkillPage();
	TSharedRef<SWidget> BuildWeaponDetailsPage();
	TSharedRef<SWidget> BuildSelectedSkillRequirements();
	TSharedRef<SWidget> BuildActionRow(const FText& Label, const TCHAR* TagName, const FText& Description, bool bPassive, bool bRuntimeReady);
	TSharedRef<SWidget> BuildSkillRow(FSkillTypePtr SkillType);
	TSharedRef<SWidget> BuildEmptyState(const FText& Title, const FText& Description, TSharedPtr<SWidget> ActionWidget = nullptr);

	FReply SetPage(EWeaponManagerPage NewPage);
	FReply SetActionMode(EWeaponManagerActionMode NewMode);
	FReply SaveManagedAssets();
	FReply OpenCreateWeaponDialog();
	FReply ConfirmCreateWeapon(TSharedPtr<class SWindow> Dialog);
	FReply CreateAttackData();
	FReply CreatePassiveData();
	FReply CreateSkill(FSkillTypePtr SkillType);
	FReply RemoveSkill(FSkillTypePtr SkillType);
	FReply SetDefaultSkill(FSkillTypePtr SkillType);
	FReply SelectSkill(FSkillTypePtr SkillType);
	FReply OpenAsset(UObject* Asset) const;
	FReply SyncAsset(UObject* Asset) const;

	UWeaponDefinition* GetSelectedWeapon() const;
	UWeaponSkillDataAsset* FindWeaponSkill(const FGameplayTag& SkillTag) const;
	UAbilityData* GetCurrentActionData(bool bPassive) const;
	UAnimMontage* GetMontage(const FGameplayTag& ActionTag, bool bPassive) const;
	void SetMontage(const FGameplayTag& ActionTag, bool bPassive, const FAssetData& AssetData);
	FString GetMontageObjectPath(FGameplayTag ActionTag, bool bPassive) const;
	bool HasConfiguredMontage(const FGameplayTag& ActionTag, bool bPassive) const;
	void SetSkillMontage(const FGameplayTag& MontageSlot, const FAssetData& AssetData);
	FString GetSkillMontageObjectPath(FGameplayTag MontageSlot) const;

	UObject* CreateAsset(UClass* AssetClass, const FString& PackageName, const FString& AssetName, TArray<UPackage*>& OutPackages) const;
	bool SavePackages(const TArray<UPackage*>& Packages, bool bPromptForCheckout) const;
	FString GetWeaponFolder() const;
	FString GetWeaponStem() const;
	FText GetSelectedWeaponTitle() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetWeaponValidationText(const UWeaponDefinition* Weapon) const;
	int32 CountWeaponIssues(const UWeaponDefinition* Weapon, int32& OutWarnings) const;
	void SetStatus(const FText& InStatus, bool bIsError = false);

	TArray<FWeaponItemPtr> AllWeapons;
	TArray<FWeaponItemPtr> FilteredWeapons;
	TArray<FSkillTypePtr> SkillTypes;
	TSet<EWeaponManagerLibraryCategory> EnabledLibraryFilters;
	FWeaponItemPtr SelectedWeaponItem;
	FSkillTypePtr SelectedSkillType;

	TSharedPtr<SListView<FWeaponItemPtr>> WeaponListView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SBox> CenterPanelBox;
	TSharedPtr<SBox> SkillRequirementsBox;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TStrongObjectPtr<UObject> ActiveDragFactoryRoot;

	EWeaponManagerPage CurrentPage = EWeaponManagerPage::Actions;
	EWeaponManagerActionMode CurrentActionMode = EWeaponManagerActionMode::Basic;
	FString SearchFilter;
	FString PendingNewWeaponName = TEXT("NewWeapon");
	FString PendingNewWeaponFolder = TEXT("/Game/Code/Weapon/NewWeapon");
	FText StatusText;
	bool bStatusIsError = false;
};
