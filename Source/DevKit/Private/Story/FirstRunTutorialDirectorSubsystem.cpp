#include "Story/FirstRunTutorialDirectorSubsystem.h"

#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "Engine/Texture2D.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Story/StoryEngineSubsystem.h"
#include "System/YogGameInstanceBase.h"

namespace
{
constexpr const TCHAR* AttackRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack.DA_Rune512_Attack");
constexpr const TCHAR* HeavyRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Heavy.DA_Rune512_Heavy");
constexpr const TCHAR* SplitRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split.DA_Rune512_Split");
constexpr const TCHAR* MoonlightRunePath = TEXT("/Game/Code/Weapon/TwoHandedSword/CombatCards/DA_Rune512_THSword_Moonlight.DA_Rune512_THSword_Moonlight");
constexpr const TCHAR* FinisherRunePath = TEXT("/Game/YogRuneEditor/Runes/DA_Rune_Finisher.DA_Rune_Finisher");
constexpr const TCHAR* EnemyRoomAttackBuffPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Attack.DA_Rune512_EnemyRoom_Attack");
constexpr const TCHAR* PrayerRoomDataPath = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom.DA_PrayRoom");

constexpr const TCHAR* GoldIconPath = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.T_GoldCoinIcon");
constexpr const TCHAR* MaterialIconPath = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_MaterialQuestionIcon.T_MaterialQuestionIcon");
constexpr const TCHAR* AttackIconPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons/T_Rune512_Attack.T_Rune512_Attack");
constexpr const TCHAR* HeavyIconPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons/T_Rune512_THSword.T_Rune512_THSword");
constexpr const TCHAR* SplitIconPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons/T_Rune512_Split.T_Rune512_Split");
constexpr const TCHAR* MoonlightIconPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/Icons/T_Rune512_THSword_Moonlight.T_Rune512_THSword_Moonlight");

template <typename T>
T* LoadTutorialAsset(const TCHAR* Path)
{
	return LoadObject<T>(nullptr, Path);
}

FLootOption MakeRuneLootOption(const TCHAR* RunePath, const TCHAR* DisplayName, const TCHAR* IconPath)
{
	FLootOption Option;
	Option.LootType = ELootType::Rune;
	Option.RuneAsset = LoadTutorialAsset<URuneDataAsset>(RunePath);
	Option.DisplayName = FText::FromString(DisplayName);
	Option.Icon = LoadTutorialAsset<UTexture2D>(IconPath);
	return Option;
}

FLootOption MakeGoldLootOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Gold;
	Option.Amount = Amount;
	Option.DisplayName = FText::Format(NSLOCTEXT("FirstRunTutorial", "GoldRewardFmt", "金币 x{0}"), Amount);
	Option.Icon = LoadTutorialAsset<UTexture2D>(GoldIconPath);
	return Option;
}

FLootOption MakeMaterialLootOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Material;
	Option.Amount = Amount;
	Option.DisplayName = FText::Format(NSLOCTEXT("FirstRunTutorial", "MaterialRewardFmt", "材料 x{0}"), Amount);
	Option.Icon = LoadTutorialAsset<UTexture2D>(MaterialIconPath);
	return Option;
}

void AddEnemyRoomAttackBuff(FStoryNextRoomPlan& Plan)
{
	if (URuneDataAsset* BuffRune = LoadTutorialAsset<URuneDataAsset>(EnemyRoomAttackBuffPath))
	{
		FBuffEntry Entry;
		Entry.RuneDA = BuffRune;
		Entry.DifficultyScore = 1;
		Entry.ApplyChance = 1.0f;

		Plan.bOverrideBuffs = true;
		Plan.BuffsOverride.Add(Entry);
	}
}
}

void UFirstRunTutorialDirectorSubsystem::SetStage(EFirstRunTutorialStage InStage)
{
	UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Stage %d -> %d"), static_cast<int32>(Stage), static_cast<int32>(InStage));
	Stage = InStage;
}

bool UFirstRunTutorialDirectorSubsystem::IsFirstRunTutorialActive() const
{
	const UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr;
	return SaveSys && SaveSys->IsFirstRunTutorialActive();
}

void UFirstRunTutorialDirectorSubsystem::HandleArrangementPhase(AYogGameMode* GameMode)
{
	if (!GameMode || !IsFirstRunTutorialActive())
	{
		return;
	}

	EFirstRunTutorialStage PlanningStage = Stage;
	if (PlanningStage == EFirstRunTutorialStage::None && GameMode->CurrentFloor <= 1)
	{
		PlanningStage = EFirstRunTutorialStage::GoldRoomCleared;
	}

	FStoryNextRoomPlan Plan;
	if (!BuildDefaultNextRoomPlanForStage(PlanningStage, Plan))
	{
		return;
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->SetPendingStoryNextRoomPlan(Plan);
	}

	SetStage(GetNextStageAfterPlanning(PlanningStage));
	UE_LOG(LogTemp, Log,
		TEXT("[FirstRunTutorialDirector] Planned next room. Stage=%d NextStage=%d ForcePortal=%d RewardOverride=%d RewardCount=%d SuppressClearReward=%d SpecialEnemy=%d"),
		static_cast<int32>(PlanningStage),
		static_cast<int32>(Stage),
		Plan.bForceSinglePortal ? 1 : 0,
		Plan.bOverrideRewardOptions ? 1 : 0,
		Plan.RewardOptionsOverride.Num(),
		Plan.bSuppressRoomClearRewardPickup ? 1 : 0,
		Plan.bMarkLastEnemyAsSpecialRewardEnemy ? 1 : 0);
}

void UFirstRunTutorialDirectorSubsystem::HandleRewardRuneAdded(URuneDataAsset* RuneAsset, APlayerCharacterBase* Player)
{
	if (!RuneAsset || !IsFirstRunTutorialActive())
	{
		return;
	}

	if (IsRuneAtPath(RuneAsset, MoonlightRunePath))
	{
		BroadcastTutorialStoryEvent(FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.MoonlightObtained"), false), Player);
	}
}

void UFirstRunTutorialDirectorSubsystem::HandleSacrificeConfirmed(URuneDataAsset* GrantedRune, APlayerCharacterBase* Player)
{
	URuneDataAsset* EffectiveRune = ResolveSacrificeRewardOverride(GrantedRune);
	if (!Player || !EffectiveRune || !IsFirstRunTutorialActive())
	{
		return;
	}

	if (Stage != EFirstRunTutorialStage::PrayerRoom || !IsRuneAtPath(EffectiveRune, FinisherRunePath))
	{
		return;
	}

	if (Player->CombatDeckComponent)
	{
		Player->CombatDeckComponent->AddCardFromRuneReward(EffectiveRune);
	}

	BroadcastTutorialStoryEvent(FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.FinisherObtained"), false), Player);
	SetStage(EFirstRunTutorialStage::ForcedSurvival);

	if (AYogGameMode* GameMode = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(Player)))
	{
		GameMode->StartForcedSurvivalEncounter();
	}
}

URuneDataAsset* UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardOverride(URuneDataAsset* DefaultRune) const
{
	return ResolveSacrificeRewardForStage(Stage, IsFirstRunTutorialActive(), DefaultRune);
}

bool UFirstRunTutorialDirectorSubsystem::IsPrayerSacrificeOverrideActive() const
{
	return IsFirstRunTutorialActive()
		&& Stage == EFirstRunTutorialStage::PrayerRoom
		&& LoadFirstRunFinisherRune() != nullptr;
}

bool UFirstRunTutorialDirectorSubsystem::ShouldHandleScriptedDefeatDeath() const
{
	return Stage == EFirstRunTutorialStage::ForcedSurvival && IsFirstRunTutorialActive();
}

void UFirstRunTutorialDirectorSubsystem::HandleScriptedDefeatDeath(AYogGameMode* GameMode)
{
	if (!GameMode || !ShouldHandleScriptedDefeatDeath())
	{
		return;
	}

	if (UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>())
	{
		SaveSys->MarkFirstRunTutorialCompleted();
		SaveSys->ClearRunCheckpoint();
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ClearRunState();
	}

	SetStage(EFirstRunTutorialStage::Completed);
}

bool UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(EFirstRunTutorialStage InStage, FStoryNextRoomPlan& OutPlan)
{
	OutPlan = FStoryNextRoomPlan();
	OutPlan.bForceSinglePortal = true;
	OutPlan.PortalIndex = 0;

	if (InStage == EFirstRunTutorialStage::GoldRoomCleared)
	{
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = {
			MakeRuneLootOption(MoonlightRunePath, TEXT("Moonlight"), MoonlightIconPath),
		};
		return true;
	}

	if (InStage == EFirstRunTutorialStage::BuffCardRoom)
	{
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = { MakeMaterialLootOption(1) };
		return true;
	}

	if (InStage == EFirstRunTutorialStage::MoonlightRoom)
	{
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = {
			MakeRuneLootOption(AttackRunePath, TEXT("Attack"), AttackIconPath),
			MakeRuneLootOption(HeavyRunePath, TEXT("Heavy"), HeavyIconPath),
			MakeRuneLootOption(SplitRunePath, TEXT("Split"), SplitIconPath),
		};
		AddEnemyRoomAttackBuff(OutPlan);
		return true;
	}

	if (InStage == EFirstRunTutorialStage::TransitionRoom01)
	{
		OutPlan.RoomDataOverride = LoadTutorialAsset<URoomDataAsset>(PrayerRoomDataPath);
		return true;
	}

	if (InStage == EFirstRunTutorialStage::TransitionRoom02)
	{
		OutPlan.RoomDataOverride = LoadTutorialAsset<URoomDataAsset>(PrayerRoomDataPath);
		return true;
	}

	switch (InStage)
	{
	case EFirstRunTutorialStage::GoldRoomCleared:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = {
			MakeRuneLootOption(AttackRunePath, TEXT("攻击"), AttackIconPath),
			MakeRuneLootOption(HeavyRunePath, TEXT("重击"), HeavyIconPath),
			MakeRuneLootOption(SplitRunePath, TEXT("分裂"), SplitIconPath),
		};
		AddEnemyRoomAttackBuff(OutPlan);
		return true;

	case EFirstRunTutorialStage::BuffCardRoom:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = {
			MakeRuneLootOption(MoonlightRunePath, TEXT("月光"), MoonlightIconPath),
		};
		OutPlan.bSuppressRoomClearRewardPickup = true;
		OutPlan.bMarkLastEnemyAsSpecialRewardEnemy = true;
		OutPlan.SpecialRewardEnemyLootOptions = OutPlan.RewardOptionsOverride;
		return true;

	case EFirstRunTutorialStage::MoonlightRoom:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = { MakeGoldLootOption(30) };
		return true;

	case EFirstRunTutorialStage::TransitionRoom01:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = { MakeMaterialLootOption(1) };
		return true;

	case EFirstRunTutorialStage::TransitionRoom02:
		return true;

	default:
		break;
	}

	OutPlan = FStoryNextRoomPlan();
	return false;
}

void UFirstRunTutorialDirectorSubsystem::BuildDefaultPostTutorialDeck(TArray<URuneDataAsset*>& OutDeck)
{
	OutDeck.Reset();
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(AttackRunePath));
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(AttackRunePath));
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(MoonlightRunePath));
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(FinisherRunePath));
}

URuneDataAsset* UFirstRunTutorialDirectorSubsystem::LoadFirstRunFinisherRune()
{
	return LoadTutorialAsset<URuneDataAsset>(FinisherRunePath);
}

URuneDataAsset* UFirstRunTutorialDirectorSubsystem::ResolveSacrificeRewardForStage(
	EFirstRunTutorialStage InStage,
	bool bFirstRunTutorialActive,
	URuneDataAsset* DefaultRune)
{
	if (bFirstRunTutorialActive && InStage == EFirstRunTutorialStage::PrayerRoom)
	{
		if (URuneDataAsset* FinisherRune = LoadFirstRunFinisherRune())
		{
			return FinisherRune;
		}
	}

	return DefaultRune;
}

bool UFirstRunTutorialDirectorSubsystem::IsRuneAtPath(const URuneDataAsset* Rune, const TCHAR* Path)
{
	return Rune && Path && Rune->GetPathName().Equals(Path, ESearchCase::IgnoreCase);
}

EFirstRunTutorialStage UFirstRunTutorialDirectorSubsystem::GetNextStageAfterPlanning(EFirstRunTutorialStage PlanningStage)
{
	switch (PlanningStage)
	{
	case EFirstRunTutorialStage::GoldRoomCleared:
		return EFirstRunTutorialStage::BuffCardRoom;
	case EFirstRunTutorialStage::BuffCardRoom:
		return EFirstRunTutorialStage::MoonlightRoom;
	case EFirstRunTutorialStage::MoonlightRoom:
		return EFirstRunTutorialStage::TransitionRoom01;
	case EFirstRunTutorialStage::TransitionRoom01:
		return EFirstRunTutorialStage::PrayerRoom;
	case EFirstRunTutorialStage::TransitionRoom02:
		return EFirstRunTutorialStage::PrayerRoom;
	default:
		return PlanningStage;
	}
}

void UFirstRunTutorialDirectorSubsystem::BroadcastTutorialStoryEvent(FGameplayTag EventTag, APlayerCharacterBase* Player) const
{
	if (!EventTag.IsValid())
	{
		return;
	}

	if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>()
		: nullptr)
	{
		APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
		StoryEngine->BroadcastStoryEventWithPayload(EventTag, EventTag, EventTag, FGameplayTag(), Player, PC);
	}
}
