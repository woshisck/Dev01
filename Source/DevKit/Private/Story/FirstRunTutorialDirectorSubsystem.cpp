#include "Story/FirstRunTutorialDirectorSubsystem.h"

#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "Engine/Texture2D.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Map/RewardPickup.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Story/StoryEngineSubsystem.h"
#include "System/YogGameInstanceBase.h"

namespace
{
constexpr const TCHAR* BurnRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn.DA_Rune512_Burn");
constexpr const TCHAR* KnockbackRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Knockback.DA_Rune512_Knockback");
constexpr const TCHAR* MoonlightRunePath = TEXT("/Game/Code/Weapon/TwoHandedSword/CombatCards/DA_Rune512_THSword_Moonlight.DA_Rune512_THSword_Moonlight");
constexpr const TCHAR* MoonlightFallbackRunePath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward.DA_Rune512_Moonlight_Forward");
constexpr const TCHAR* FinisherRunePath = TEXT("/Game/YogRuneEditor/Runes/DA_Rune_Finisher.DA_Rune_Finisher");
constexpr const TCHAR* PrayerRoomDataPath = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/DA_PrayRoom.DA_PrayRoom");

constexpr const TCHAR* MaterialIconPath = TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_MaterialQuestionIcon.T_MaterialQuestionIcon");
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

FLootOption MakeMoonlightLootOption()
{
	FLootOption Option = MakeRuneLootOption(MoonlightRunePath, TEXT("Moonlight"), MoonlightIconPath);
	if (!Option.RuneAsset)
	{
		Option.RuneAsset = LoadTutorialAsset<URuneDataAsset>(MoonlightFallbackRunePath);
		UE_LOG(LogTemp, Warning,
			TEXT("[FirstRunTutorialDirector] Primary Moonlight rune failed to load, fallback=%s Asset=%s"),
			MoonlightFallbackRunePath,
			*GetNameSafe(Option.RuneAsset.Get()));
	}

	return Option;
}

FLootOption MakeMaterialLootOption(int32 Amount)
{
	FLootOption Option;
	Option.LootType = ELootType::Material;
	Option.Amount = Amount;
	Option.MetaCurrencyTag = ARewardPickup::ResolveMaterialCurrencyTag(FGameplayTag());
	Option.DisplayName = FText::Format(NSLOCTEXT("FirstRunTutorial", "MaterialRewardFmt", "材料 x{0}"), Amount);
	Option.Icon = LoadTutorialAsset<UTexture2D>(MaterialIconPath);
	return Option;
}

}

void UFirstRunTutorialDirectorSubsystem::SetStage(EFirstRunTutorialStage InStage)
{
	UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Stage %d -> %d"), static_cast<int32>(Stage), static_cast<int32>(InStage));
	Stage = InStage;
	PersistStageToSave(InStage);
}

bool UFirstRunTutorialDirectorSubsystem::IsFirstRunTutorialActive() const
{
	const UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr;
	return SaveSys && SaveSys->IsFirstRunTutorialActive();
}

EFirstRunTutorialStage UFirstRunTutorialDirectorSubsystem::GetPersistedStage() const
{
	const UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr;
	const int32 RawStage = SaveSys ? SaveSys->GetFirstRunTutorialStage() : 0;
	return RawStage >= static_cast<int32>(EFirstRunTutorialStage::None)
		&& RawStage <= static_cast<int32>(EFirstRunTutorialStage::Completed)
		? static_cast<EFirstRunTutorialStage>(RawStage)
		: EFirstRunTutorialStage::None;
}

void UFirstRunTutorialDirectorSubsystem::RestoreStageFromSave()
{
	const EFirstRunTutorialStage PersistedStage = GetPersistedStage();
	if (PersistedStage != Stage)
	{
		UE_LOG(LogTemp, Log, TEXT("[FirstRunTutorialDirector] Restored stage %d -> %d"),
			static_cast<int32>(Stage),
			static_cast<int32>(PersistedStage));
		Stage = PersistedStage;
	}
}

void UFirstRunTutorialDirectorSubsystem::PersistStageToSave(EFirstRunTutorialStage InStage) const
{
	if (UYogSaveSubsystem* SaveSys = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UYogSaveSubsystem>()
		: nullptr)
	{
		SaveSys->SetFirstRunTutorialStage(static_cast<int32>(InStage));
	}
}

void UFirstRunTutorialDirectorSubsystem::HandleArrangementPhase(AYogGameMode* GameMode)
{
	if (!GameMode || !IsFirstRunTutorialActive())
	{
		return;
	}

	RestoreStageFromSave();
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

	RestoreStageFromSave();
	const FGameplayTag MoonlightIdTag = FGameplayTag::RequestGameplayTag(TEXT("Card.ID.Moonlight"), false);
	const FGameplayTag MoonlightEffectTag = FGameplayTag::RequestGameplayTag(TEXT("Card.Effect.Moonlight"), false);
	const FCombatCardConfig& CombatCard = RuneAsset->RuneInfo.CombatCard;
	const bool bIsMoonlightCard =
		IsRuneAtPath(RuneAsset, MoonlightRunePath)
		|| (MoonlightIdTag.IsValid() && CombatCard.CardIdTag == MoonlightIdTag)
		|| (MoonlightEffectTag.IsValid() && CombatCard.CardEffectTags.HasTagExact(MoonlightEffectTag));

	if (bIsMoonlightCard)
	{
		BroadcastTutorialStoryEvent(FGameplayTag::RequestGameplayTag(TEXT("Story.Event.FirstRun.MoonlightObtained"), false), Player);
	}
}

void UFirstRunTutorialDirectorSubsystem::HandleSacrificeConfirmed(URuneDataAsset* GrantedRune, APlayerCharacterBase* Player)
{
	RestoreStageFromSave();
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
	const EFirstRunTutorialStage EffectiveStage = Stage != EFirstRunTutorialStage::None ? Stage : GetPersistedStage();
	return ResolveSacrificeRewardForStage(EffectiveStage, IsFirstRunTutorialActive(), DefaultRune);
}

bool UFirstRunTutorialDirectorSubsystem::IsPrayerSacrificeOverrideActive() const
{
	const EFirstRunTutorialStage EffectiveStage = Stage != EFirstRunTutorialStage::None ? Stage : GetPersistedStage();
	return IsFirstRunTutorialActive()
		&& EffectiveStage == EFirstRunTutorialStage::PrayerRoom
		&& LoadFirstRunFinisherRune() != nullptr;
}

bool UFirstRunTutorialDirectorSubsystem::ShouldHandleScriptedDefeatDeath() const
{
	const EFirstRunTutorialStage EffectiveStage = Stage != EFirstRunTutorialStage::None ? Stage : GetPersistedStage();
	return EffectiveStage == EFirstRunTutorialStage::ForcedSurvival && IsFirstRunTutorialActive();
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
		GI->QueueFirstRunWorldRewindHint();
	}

	SetStage(EFirstRunTutorialStage::Completed);
}

bool UFirstRunTutorialDirectorSubsystem::BuildDefaultNextRoomPlanForStage(EFirstRunTutorialStage InStage, FStoryNextRoomPlan& OutPlan)
{
	OutPlan = FStoryNextRoomPlan();
	OutPlan.bForceSinglePortal = true;
	OutPlan.PortalIndex = 0;

	switch (InStage)
	{
	case EFirstRunTutorialStage::GoldRoomCleared:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = {
			MakeMoonlightLootOption(),
		};
		return true;

	case EFirstRunTutorialStage::BuffCardRoom:
		OutPlan.bOverrideRewardOptions = true;
		OutPlan.RewardOptionsOverride = { MakeMaterialLootOption(1) };
		return true;

	case EFirstRunTutorialStage::MoonlightRoom:
		// 第 4 关：让玩家通过 01b 的 portal[0] 走到 WaterDungeon；保留默认奖励池。
		return true;

	case EFirstRunTutorialStage::TransitionRoom01:
		// 第 4 关（WaterDungeon）：默认 portal[0] = [01a, 01b]，随机回一间走廊。
		return true;

	case EFirstRunTutorialStage::TransitionRoom02:
		// 玩家此时在 01a 或 01b 第二次出现的房间。两间走廊都把 portal[1] 配成
		// [PrayerRoom]，强制开 portal[1] 走 portal 机制；同时 RoomDataOverride
		// 锁定到 DA_PrayRoom —— 否则 SelectRoomByTag 的类型滚动（多半 Normal）
		// 会把唯一的 Event 型 PrayerRoom 过滤掉，回退到 Campaign 的 Normal 池里
		// 任意一张（WD/01a/01b），玩家就进不去祈祷室。
		OutPlan.PortalIndex = 1;
		OutPlan.RoomDataOverride = LoadTutorialAsset<URoomDataAsset>(PrayerRoomDataPath);
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
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(BurnRunePath));
	OutDeck.Add(LoadTutorialAsset<URuneDataAsset>(KnockbackRunePath));
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
		return EFirstRunTutorialStage::TransitionRoom02;
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
