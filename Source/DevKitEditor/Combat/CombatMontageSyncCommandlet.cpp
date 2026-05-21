#include "DevKitEditor/Combat/CombatMontageSyncCommandlet.h"

#include "Animation/AnimMontage.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimNotifyState_AddGameplayTag.h"
#include "AnimationBlueprintLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Data/MontageNotifyEntry.h"
#include "Data/WeaponComboConfigDA.h"
#include "FileHelpers.h"
#include "GenericGraphNode.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace CombatMontageSync
{
	const FString DefaultWeaponPath = TEXT("/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword");
	const FString GeneratedMontageRoot = TEXT("/Game/Code/Weapon/TwoHandedSword/GeneratedMontages");
	const FName ComboTrackName(TEXT("Combo"));

	struct FWindowSpec
	{
		int32 StartFrame = 18;
		int32 EndFrame = 27;
		int32 TotalFrames = 30;
		FString Source;
	};

	struct FNodeUsage
	{
		FName NodeId = NAME_None;
		TObjectPtr<UMontageConfigDA> MontageConfig = nullptr;
		bool bIsComboFinisher = false;
		FString Source;
	};

	struct FMontageRequest
	{
		TObjectPtr<UAnimMontage> Montage = nullptr;
		bool bNeedsComboWindow = false;
		FWindowSpec Window;
		TArray<FString> Sources;
		TArray<FNodeUsage> Usages;
		bool bAlsoUsedByFinisher = false;
		bool bConflictingWindow = false;
	};

	FString ToObjectPath(const FString& Path)
	{
		if (Path.Contains(TEXT(".")))
		{
			return Path;
		}

		return Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
	}

	template<typename T>
	T* LoadAsset(const FString& Path)
	{
		return LoadObject<T>(nullptr, *ToObjectPath(Path));
	}

	UObject* GetObjectPropertyValue(UObject* Owner, const FName PropertyName)
	{
		if (!Owner)
		{
			return nullptr;
		}

		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Owner->GetClass(), PropertyName);
		return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
	}

	bool SameWindow(const FWindowSpec& A, const FWindowSpec& B)
	{
		return A.StartFrame == B.StartFrame
			&& A.EndFrame == B.EndFrame
			&& A.TotalFrames == B.TotalFrames;
	}

	FString SanitizeAssetSuffix(FString Value)
	{
		Value.ReplaceInline(TEXT("."), TEXT("_"));
		Value.ReplaceInline(TEXT(" "), TEXT("_"));
		Value.ReplaceInline(TEXT("/"), TEXT("_"));
		Value.ReplaceInline(TEXT("\\"), TEXT("_"));
		Value.ReplaceInline(TEXT(":"), TEXT("_"));
		return Value;
	}

	FString GetFinisherDuplicatePackagePath(UAnimMontage* SourceMontage, FName NodeId)
	{
		if (!SourceMontage)
		{
			return FString();
		}

		const FString AssetName = FString::Printf(
			TEXT("%s_%s_Finisher"),
			*SourceMontage->GetName(),
			*SanitizeAssetSuffix(NodeId.ToString()));
		return GeneratedMontageRoot / AssetName;
	}

	FString GetNodeDuplicatePackagePath(UAnimMontage* SourceMontage, FName NodeId, bool bIsFinisher)
	{
		if (!SourceMontage)
		{
			return FString();
		}

		const FString AssetName = FString::Printf(
			TEXT("%s_%s_%s"),
			*SourceMontage->GetName(),
			*SanitizeAssetSuffix(NodeId.ToString()),
			bIsFinisher ? TEXT("Finisher") : TEXT("Combo"));
		return GeneratedMontageRoot / AssetName;
	}

	bool IsGeneratedMontage(const UAnimMontage* Montage)
	{
		return Montage && Montage->GetOutermost()->GetName().StartsWith(GeneratedMontageRoot);
	}

	FWindowSpec ResolveWindowFromMontageConfig(const UMontageConfigDA* MontageConfig, const FWeaponComboNodeConfig& Node)
	{
		FWindowSpec Result;
		Result.StartFrame = Node.ComboWindowStartFrame;
		Result.EndFrame = Node.ComboWindowEndFrame;
		Result.TotalFrames = Node.ComboWindowTotalFrames;
		Result.Source = FString::Printf(TEXT("NodeWindow:%s"), *Node.NodeId.ToString());

		if (MontageConfig)
		{
			Result.TotalFrames = FMath::Max(1, MontageConfig->TotalFrames);
			const TArray<UMNE_ComboWindow*> ComboWindows = MontageConfig->GetEntriesOfType<UMNE_ComboWindow>();
			if (ComboWindows.Num() > 0 && ComboWindows[0])
			{
				Result.StartFrame = ComboWindows[0]->StartFrame;
				Result.EndFrame = ComboWindows[0]->EndFrame;
				Result.Source = FString::Printf(TEXT("MontageConfig:%s"), *GetNameSafe(MontageConfig));
			}
		}

		Result.StartFrame = FMath::Max(0, Result.StartFrame);
		Result.EndFrame = FMath::Max(Result.StartFrame + 1, Result.EndFrame);
		Result.TotalFrames = FMath::Max(1, Result.TotalFrames);
		return Result;
	}

	void AddRequest(TMap<TObjectPtr<UAnimMontage>, FMontageRequest>& Requests, const FWeaponComboNodeConfig& Node)
	{
		const UMontageConfigDA* MontageConfig = Node.MontageConfig;
		UAnimMontage* Montage = MontageConfig ? MontageConfig->Montage : nullptr;
		if (!Montage)
		{
			return;
		}

		FMontageRequest& Request = Requests.FindOrAdd(Montage);
		Request.Montage = Montage;
		const FString SourceText = FString::Printf(TEXT("%s / %s"), *Node.NodeId.ToString(), *GetNameSafe(MontageConfig));
		Request.Sources.Add(SourceText);
		FNodeUsage& Usage = Request.Usages.AddDefaulted_GetRef();
		Usage.NodeId = Node.NodeId;
		Usage.MontageConfig = Node.MontageConfig;
		Usage.bIsComboFinisher = Node.bIsComboFinisher;
		Usage.Source = SourceText;

		if (Node.bIsComboFinisher)
		{
			Request.bAlsoUsedByFinisher = true;
			return;
		}

		const FWindowSpec Window = ResolveWindowFromMontageConfig(MontageConfig, Node);
		if (Request.bNeedsComboWindow && !SameWindow(Request.Window, Window))
		{
			Request.bConflictingWindow = true;
		}
		else if (!Request.bNeedsComboWindow)
		{
			Request.Window = Window;
		}
		Request.bNeedsComboWindow = true;
	}

	void GatherFromGraph(const UGameplayAbilityComboGraph* Graph, TMap<TObjectPtr<UAnimMontage>, FMontageRequest>& Requests)
	{
		if (!Graph)
		{
			return;
		}

		for (const UGenericGraphNode* GenericNode : Graph->AllNodes)
		{
			const UGameplayAbilityComboGraphNode* Node = Cast<UGameplayAbilityComboGraphNode>(GenericNode);
			if (!Node)
			{
				continue;
			}

			FWeaponComboNodeConfig Config = Node->BuildRuntimeConfig(Node->RootInputAction);
			Config.bOverrideComboWindow = Node->bUseNodeComboWindow;
			Config.ComboWindowStartFrame = Node->ComboWindowStartFrame;
			Config.ComboWindowEndFrame = Node->ComboWindowEndFrame;
			Config.ComboWindowTotalFrames = Node->ComboWindowTotalFrames;
			AddRequest(Requests, Config);
		}
	}

	void GatherFromConfig(const UWeaponComboConfigDA* Config, TMap<TObjectPtr<UAnimMontage>, FMontageRequest>& Requests)
	{
		if (!Config)
		{
			return;
		}

		for (const FWeaponComboNodeConfig& Node : Config->Nodes)
		{
			AddRequest(Requests, Node);
		}
	}

	bool NotifyHasCanCombo(const FAnimNotifyEvent& Event, const FGameplayTag& CanComboTag)
	{
		const UAnimNotifyState_AddGameplayTag* AddTagState = Cast<UAnimNotifyState_AddGameplayTag>(Event.NotifyStateClass);
		return AddTagState && AddTagState->Tags.HasTagExact(CanComboTag);
	}

	UAnimMontage* GetOrCreateFinisherDuplicate(UAnimMontage* SourceMontage, FName NodeId, TArray<UPackage*>& PackagesToSave)
	{
		if (!SourceMontage)
		{
			return nullptr;
		}

		const FString PackagePath = GetFinisherDuplicatePackagePath(SourceMontage, NodeId);
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		if (UAnimMontage* Existing = LoadAsset<UAnimMontage>(PackagePath))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UObject* DuplicatedObject = StaticDuplicateObject(SourceMontage, Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		UAnimMontage* DuplicatedMontage = Cast<UAnimMontage>(DuplicatedObject);
		if (!DuplicatedMontage)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(DuplicatedMontage);
		DuplicatedMontage->MarkPackageDirty();
		PackagesToSave.AddUnique(DuplicatedMontage->GetOutermost());
		return DuplicatedMontage;
	}

	UAnimMontage* GetOrCreateNodeDuplicate(UAnimMontage* SourceMontage, FName NodeId, bool bIsFinisher, TArray<UPackage*>& PackagesToSave)
	{
		if (!SourceMontage)
		{
			return nullptr;
		}

		const FString PackagePath = GetNodeDuplicatePackagePath(SourceMontage, NodeId, bIsFinisher);
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		if (UAnimMontage* Existing = LoadAsset<UAnimMontage>(PackagePath))
		{
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UObject* DuplicatedObject = StaticDuplicateObject(SourceMontage, Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		UAnimMontage* DuplicatedMontage = Cast<UAnimMontage>(DuplicatedObject);
		if (!DuplicatedMontage)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(DuplicatedMontage);
		DuplicatedMontage->MarkPackageDirty();
		PackagesToSave.AddUnique(DuplicatedMontage->GetOutermost());
		return DuplicatedMontage;
	}

	int32 RemoveExistingCanComboNotifies(UAnimMontage* Montage, const FGameplayTag& CanComboTag)
	{
		if (!Montage)
		{
			return 0;
		}

		int32 Removed = 0;
		for (int32 Index = Montage->Notifies.Num() - 1; Index >= 0; --Index)
		{
			if (NotifyHasCanCombo(Montage->Notifies[Index], CanComboTag))
			{
				Montage->Notifies.RemoveAt(Index);
				++Removed;
			}
		}
		return Removed;
	}

	bool AddCanComboNotify(UAnimMontage* Montage, const FWindowSpec& Window, const FGameplayTag& CanComboTag)
	{
		if (!Montage)
		{
			return false;
		}

		const float Length = FMath::Max(0.01f, Montage->GetPlayLength());
		const float StartTime = FMath::Clamp(static_cast<float>(Window.StartFrame) / static_cast<float>(Window.TotalFrames), 0.0f, 1.0f) * Length;
		float EndTime = FMath::Clamp(static_cast<float>(Window.EndFrame) / static_cast<float>(Window.TotalFrames), 0.0f, 1.0f) * Length;
		if (EndTime <= StartTime + KINDA_SMALL_NUMBER)
		{
			EndTime = FMath::Min(Length, StartTime + 0.05f);
		}
		const float Duration = FMath::Max(0.01f, EndTime - StartTime);

		if (!UAnimationBlueprintLibrary::IsValidAnimNotifyTrackName(Montage, ComboTrackName))
		{
			UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Montage, ComboTrackName, FLinearColor(0.8f, 0.1f, 0.1f, 1.0f));
		}

		UAnimNotifyState* NotifyState = UAnimationBlueprintLibrary::AddAnimationNotifyStateEvent(
			Montage,
			ComboTrackName,
			StartTime,
			Duration,
			UAnimNotifyState_AddGameplayTag::StaticClass());

		UAnimNotifyState_AddGameplayTag* AddTagState = Cast<UAnimNotifyState_AddGameplayTag>(NotifyState);
		if (!AddTagState)
		{
			return false;
		}

		AddTagState->Tags.Reset();
		AddTagState->Tags.AddTag(CanComboTag);
		Montage->RefreshCacheData();
		return true;
	}
}

UCombatMontageSyncCommandlet::UCombatMontageSyncCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UCombatMontageSyncCommandlet::Main(const FString& Params)
{
	using namespace CombatMontageSync;

	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	FString WeaponPath;
	if (!FParse::Value(*Params, TEXT("Weapon="), WeaponPath))
	{
		WeaponPath = DefaultWeaponPath;
	}

	UObject* Weapon = LoadAsset<UObject>(WeaponPath);
	if (!Weapon)
	{
		UE_LOG(LogTemp, Error, TEXT("[CombatMontageSync] Failed to load weapon: %s"), *WeaponPath);
		return 1;
	}

	TMap<TObjectPtr<UAnimMontage>, FMontageRequest> Requests;
	GatherFromGraph(Cast<UGameplayAbilityComboGraph>(GetObjectPropertyValue(Weapon, TEXT("GameplayAbilityComboGraph"))), Requests);
	GatherFromConfig(Cast<UWeaponComboConfigDA>(GetObjectPropertyValue(Weapon, TEXT("WeaponComboConfig"))), Requests);

	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
	TArray<UPackage*> PackagesToSave;
	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Combat Montage Sync Report"));
	ReportLines.Add(FString::Printf(TEXT("- Weapon: `%s`"), *WeaponPath));
	ReportLines.Add(FString::Printf(TEXT("- Apply: `%s`"), bApply ? TEXT("true") : TEXT("false")));
	ReportLines.Add(TEXT(""));

	int32 ChangedMontages = 0;
	int32 ReassignedMontageConfigs = 0;
	ReportLines.Add(TEXT("## Writable Montage Copies"));
	bool bHadWritableCopy = false;
	for (const TPair<TObjectPtr<UAnimMontage>, FMontageRequest>& Pair : Requests)
	{
		const FMontageRequest& Request = Pair.Value;
		for (const FNodeUsage& Usage : Request.Usages)
		{
			if (!Usage.MontageConfig || !Usage.MontageConfig->Montage || IsGeneratedMontage(Usage.MontageConfig->Montage))
			{
				continue;
			}

			bHadWritableCopy = true;
			const FString DuplicatePath = GetNodeDuplicatePackagePath(
				Usage.MontageConfig->Montage,
				Usage.NodeId,
				Usage.bIsComboFinisher);
			ReportLines.Add(FString::Printf(
				TEXT("- `%s` will use writable montage copy `%s`."),
				*Usage.Source,
				*DuplicatePath));

			if (!bApply)
			{
				continue;
			}

			UAnimMontage* DuplicateMontage = GetOrCreateNodeDuplicate(
				Usage.MontageConfig->Montage,
				Usage.NodeId,
				Usage.bIsComboFinisher,
				PackagesToSave);
			if (!DuplicateMontage)
			{
				ReportLines.Add(TEXT("  - Error: failed to create writable montage copy."));
				continue;
			}

			Usage.MontageConfig->Modify();
			Usage.MontageConfig->Montage = DuplicateMontage;
			Usage.MontageConfig->MarkPackageDirty();
			PackagesToSave.AddUnique(Usage.MontageConfig->GetOutermost());
			++ReassignedMontageConfigs;
		}
	}
	if (!bHadWritableCopy)
	{
		ReportLines.Add(TEXT("- None."));
	}
	ReportLines.Add(TEXT(""));

	if (bApply && bHadWritableCopy)
	{
		Requests.Empty();
		GatherFromGraph(Cast<UGameplayAbilityComboGraph>(GetObjectPropertyValue(Weapon, TEXT("GameplayAbilityComboGraph"))), Requests);
		GatherFromConfig(Cast<UWeaponComboConfigDA>(GetObjectPropertyValue(Weapon, TEXT("WeaponComboConfig"))), Requests);
	}

	ReportLines.Add(TEXT("## Conflict Fixes"));
	bool bHadConflictFix = false;
	for (TPair<TObjectPtr<UAnimMontage>, FMontageRequest>& Pair : Requests)
	{
		UAnimMontage* Montage = Pair.Key;
		FMontageRequest& Request = Pair.Value;
		if (!Montage || !Request.bNeedsComboWindow || !Request.bAlsoUsedByFinisher)
		{
			continue;
		}

		for (FNodeUsage& Usage : Request.Usages)
		{
			if (!Usage.bIsComboFinisher || !Usage.MontageConfig)
			{
				continue;
			}

			bHadConflictFix = true;
			const FString DuplicatePath = GetFinisherDuplicatePackagePath(Montage, Usage.NodeId);
			ReportLines.Add(FString::Printf(
				TEXT("- `%s` uses `%s` as a finisher while the montage also needs CanCombo; finisher will use `%s`."),
				*Usage.Source,
				*Montage->GetPathName(),
				*DuplicatePath));

			if (!bApply)
			{
				continue;
			}

			UAnimMontage* DuplicateMontage = GetOrCreateFinisherDuplicate(Montage, Usage.NodeId, PackagesToSave);
			if (!DuplicateMontage)
			{
				ReportLines.Add(TEXT("  - Error: failed to create finisher montage duplicate."));
				continue;
			}

			DuplicateMontage->Modify();
			const int32 RemovedFromDuplicate = RemoveExistingCanComboNotifies(DuplicateMontage, CanComboTag);
			if (RemovedFromDuplicate > 0)
			{
				DuplicateMontage->MarkPackageDirty();
				PackagesToSave.AddUnique(DuplicateMontage->GetOutermost());
				ReportLines.Add(FString::Printf(TEXT("  - Removed `%d` CanCombo notify from finisher duplicate."), RemovedFromDuplicate));
			}

			Usage.MontageConfig->Modify();
			Usage.MontageConfig->Montage = DuplicateMontage;
			Usage.MontageConfig->MarkPackageDirty();
			PackagesToSave.AddUnique(Usage.MontageConfig->GetOutermost());
			++ReassignedMontageConfigs;
		}
	}
	if (!bHadConflictFix)
	{
		ReportLines.Add(TEXT("- None."));
	}
	ReportLines.Add(TEXT(""));

	for (const TPair<TObjectPtr<UAnimMontage>, FMontageRequest>& Pair : Requests)
	{
		UAnimMontage* Montage = Pair.Key;
		const FMontageRequest& Request = Pair.Value;
		if (!Montage)
		{
			continue;
		}

		int32 ExistingCanComboCount = 0;
		for (const FAnimNotifyEvent& Event : Montage->Notifies)
		{
			if (NotifyHasCanCombo(Event, CanComboTag))
			{
				++ExistingCanComboCount;
			}
		}

		ReportLines.Add(FString::Printf(TEXT("## `%s`"), *Montage->GetPathName()));
		ReportLines.Add(FString::Printf(TEXT("- Sources: `%s`"), *FString::Join(Request.Sources, TEXT("`, `"))));
		ReportLines.Add(FString::Printf(TEXT("- Existing CanCombo Notifies: `%d`"), ExistingCanComboCount));
		ReportLines.Add(FString::Printf(TEXT("- Needs CanCombo: `%s`"), Request.bNeedsComboWindow ? TEXT("true") : TEXT("false")));
		if (Request.bNeedsComboWindow)
		{
			ReportLines.Add(FString::Printf(
				TEXT("- Window: `%d-%d / %d` from `%s`"),
				Request.Window.StartFrame,
				Request.Window.EndFrame,
				Request.Window.TotalFrames,
				*Request.Window.Source));
		}
		if (Request.bAlsoUsedByFinisher && Request.bNeedsComboWindow)
		{
			ReportLines.Add(TEXT("- Warning: same montage is also used by a finisher node; montage-level CanCombo affects both branches."));
		}
		if (Request.bConflictingWindow)
		{
			ReportLines.Add(TEXT("- Warning: multiple nodes requested different windows; first window was used."));
		}

		bool bChanged = false;
		if (bApply)
		{
			Montage->Modify();
			const int32 Removed = RemoveExistingCanComboNotifies(Montage, CanComboTag);
			if (Removed > 0)
			{
				bChanged = true;
				ReportLines.Add(FString::Printf(TEXT("- Removed: `%d`"), Removed));
			}

			if (Request.bNeedsComboWindow)
			{
				if (AddCanComboNotify(Montage, Request.Window, CanComboTag))
				{
					bChanged = true;
					ReportLines.Add(TEXT("- Added: `ANS_AddGameplayTag(PlayerState.AbilityCast.CanCombo)`"));
				}
				else
				{
					ReportLines.Add(TEXT("- Error: failed to add CanCombo notify."));
				}
			}

			if (bChanged)
			{
				Montage->MarkPackageDirty();
				PackagesToSave.AddUnique(Montage->GetOutermost());
				++ChangedMontages;
			}
		}
		ReportLines.Add(TEXT(""));
	}

	if (bApply && PackagesToSave.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TEXT("CombatMontageSyncReport.md"), ReportLines, ReportPath, SharedReportPath);
	UE_LOG(LogTemp, Display, TEXT("[CombatMontageSync] Requests=%d ChangedMontages=%d ReassignedConfigs=%d Report=%s Shared=%s"),
		Requests.Num(),
		ChangedMontages,
		ReassignedMontageConfigs,
		*ReportPath,
		*SharedReportPath);

	return 0;
}
