// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/YogGameMode.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "FlowAsset.h"
#include "FlowComponent.h"
#include "GameFramework/PlayerController.h"
#include <Kismet/GameplayStatics.h>
#include "LevelFlow/LevelFlowAsset.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Story/StoryEngineSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "UI/YogHUD.h"

static FName ResolveRoomLevelNameForOpen(FName RequestedLevel, const URoomDataAsset* Room)
{
	const FString Requested = RequestedLevel.ToString();
	const FString RoomAssetName = GetNameSafe(Room);

	if (Requested.Equals(TEXT("L1_CommonLevel_Corridor_01b"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_Room_CL_corridor_01b"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_CL_Corridor_01b"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b"));
	}

	if (Requested.Equals(TEXT("PrayRoom"), ESearchCase::IgnoreCase)
		|| Requested.Equals(TEXT("L1_CommonLevel_PrayRoom"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_PrayRoom/L1_CommonLevel_PrayRoom"));
	}

	if (Requested.Equals(TEXT("ShopRoom"), ESearchCase::IgnoreCase)
		|| RoomAssetName.Equals(TEXT("DA_Room_512_Shop"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom"));
	}

	return RequestedLevel;
}

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
	DefaultPawnClass = nullptr;

	// 事件总线统一使用的 FlowComponent，同时只跑一个 Flow（新触发停旧的）
	LifecycleFlowComponent = CreateDefaultSubobject<UFlowComponent>(TEXT("LifecycleFlowComponent"));
}


void AYogGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (GI && GI->PersistentSaveData && GI->PersistentSaveData->SavedCharacterClass)
	{
		// 使用 PlayerStart 作为出生位置，避免在 (0,0,0) 发生碰撞
		AActor* StartSpot = FindPlayerStart(NewPlayer);
		const FVector  SpawnLoc = StartSpot ? StartSpot->GetActorLocation() : FVector::ZeroVector;
		const FRotator SpawnRot = StartSpot ? StartSpot->GetActorRotation() : FRotator::ZeroRotator;

		FActorSpawnParameters Params;
		Params.Owner = NewPlayer;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APlayerCharacterBase* LoadedChar = GetWorld()->SpawnActor<APlayerCharacterBase>(
			GI->PersistentSaveData->SavedCharacterClass,
			SpawnLoc, SpawnRot,
			Params
		);

		if (!LoadedChar)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleStartingNewPlayer: SpawnActor failed, falling back to default"));
			Super::HandleStartingNewPlayer_Implementation(NewPlayer);
			return;
		}

		NewPlayer->Possess(LoadedChar);

		// Possess 完成后 ASC 已初始化，可以安全恢复跑局状态
		LoadedChar->RestoreRunStateFromGI();
	}
	else
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer); // fallback
	}
}



void AYogGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!bAutoSpawnPlayer)
	{
		// Do nothing - prevent auto spawning
		UE_LOG(LogTemp, Warning, TEXT("Auto player spawning disabled"));
		return;
	}

	// Fall back to default behavior if enabled
	Super::RestartPlayer(NewPlayer);

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

}

void AYogGameMode::StartPlay()
{
	Super::StartPlay();
	//==SunChuankai start==
	// ResolveActiveCampaignData();
	// if (GetActiveCampaignData())
	// {
	// 	StartLevelSpawning();
	// }
	// else
	// {
	// 	FallbackToPreplacedEnemies();
	// }
	//==SunChuankai end==
}

void AYogGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

}

void AYogGameMode::SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName)
{
	//YogSpawnPoint_0
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	//TODO: NEED TO CHANGE IN THE FUTURE DEV
	APlayerController* PlayerController = World->GetFirstPlayerController();


	AActor* PlayerStarter = FindPlayerStart(PlayerController, TEXT("YogSpawnPoint_0"));
	if (PlayerStarter)
	{
		if (PlayerController)
		{
			//if (APawn* ExistingPawn = PlayerController->GetPawn())
			//{
			//	ExistingPawn->Destroy();
			//}


			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APlayerCharacterBase* NewCharacter = World->SpawnActor<APlayerCharacterBase>
			(
				DefaultPawnClass,
				PlayerStarter->GetActorLocation(),
				PlayerStarter->GetActorRotation(),
				SpawnParams
			);
			NewCharacter = player;

			if (NewCharacter)
			{
				
				PlayerController->Possess(Cast<APawn>(NewCharacter));
				UE_LOG(LogTemp, Log, TEXT("Player spawned manually at specified location"));
			}

		}
	}

}


void AYogGameMode::UpdateFinishLevel(int count)
{
	this->MonsterKillCount += count;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SS = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			SS->RecordEnemyKilled(count);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("MonsterKillCount: %d"), this->MonsterKillCount);
}

void AYogGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 触发游戏开始事件（一次性，跨 PIE 重启会重置 — 单机 Roguelite 行为可接受）
	TriggerLifecycleEvent(EGameLifecycleEvent::GameStart);
}

void AYogGameMode::HandlePlayerDeath(APlayerCharacterBase* Player)
{
	if (bPlayerDeathPending || bGameOverTriggered)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogSaveSubsystem* SS = GI->GetSubsystem<UYogSaveSubsystem>())
		{
			SS->RecordPlayerDeath();
		}
	}

	bPlayerDeathPending = true;
	PendingDeathPlayer = Player;

	if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
	{
		StoryEngine->BroadcastStoryEventWithPayload(
			FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Player.Died"), false),
			FGameplayTag(),
			FGameplayTag(),
			FGameplayTag(),
			Player,
			Player ? Cast<APlayerController>(Player->GetController()) : GetWorld()->GetFirstPlayerController());
	}

	TriggerLifecycleEvent(EGameLifecycleEvent::PlayerDeath);
	BeginPlayerDeathVisuals(Player);

	if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetShowMouseCursor(false);
	}

	bScriptedDefeatGameOver = false;

	if (!bScriptedDefeatGameOver && CanOfferPlayerDeathRevive(bGameOverTriggered, bPlayerDeathReviveUsed) && Player)
	{
		Player->PrepareForDeathReviveChoice();
	}

	ClearPlayerDeathGameOverTicker();
	TWeakObjectPtr<AYogGameMode> WeakThis(this);
	PlayerDeathGameOverTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->FinishPlayerDeathGameOver();
			}
			return false;
		}),
		FMath::Max(0.f, PlayerDeathGameOverDelay));

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Player died. Game over will finalize in %.2fs."), PlayerDeathGameOverDelay);
}

bool AYogGameMode::CanOfferPlayerDeathRevive(bool bInGameOverTriggered, bool bInPlayerDeathReviveUsed)
{
	return !bInGameOverTriggered && !bInPlayerDeathReviveUsed;
}

bool AYogGameMode::ShouldBroadcastRunSummaryForPlayerDeathGameOver(bool /*bCanRevive*/)
{
	return false;
}

float AYogGameMode::CalculatePlayerReviveHealth(float MaxHealth, float ReviveHealthPercent)
{
	if (MaxHealth <= 0.f || ReviveHealthPercent <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(MaxHealth * ReviveHealthPercent, 1.f, MaxHealth);
}

void AYogGameMode::FinishPlayerDeathGameOver()
{
	ClearPlayerDeathGameOverTicker();
	const bool bCanRevive = CanOfferPlayerDeathRevive(bGameOverTriggered, bPlayerDeathReviveUsed)
		&& PendingDeathPlayer.IsValid();

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Finalize death. CanRevive=%d ReviveUsed=%d PendingPlayer=%s"),
		(int32)bCanRevive,
		(int32)bPlayerDeathReviveUsed,
		*GetNameSafe(PendingDeathPlayer.Get()));

	if (!bCanRevive)
	{
		bGameOverTriggered = true;

		if (APlayerCharacterBase* Player = PendingDeathPlayer.Get())
		{
			Player->ResetToDefaultUnarmedCombatState();
		}

		// 广播本局结算数据（展示结算界面、触发成长货币结算存档）
		if (ShouldBroadcastRunSummaryForPlayerDeathGameOver(bCanRevive))
		{
			if (UYogMetaProgressionSubsystem* Meta = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr)
			{
				Meta->BroadcastRunEnded(CurrentFloor, MonsterKillCount);
			}
		}

		if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
		{
			GI->ClearRunState();
			// 死亡才清磁盘存档点；中途退出走 ReturnToMainMenu，不清除
			if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
			{
				SaveSys->ClearRunCheckpoint();
			}
		}
	}

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
	if (APlayerCharacterBase* Player = PendingDeathPlayer.Get())
	{
		Player->CustomTimeDilation = 1.f;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ShowGameOverScreen(bCanRevive, bScriptedDefeatGameOver);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Game paused after player death."));
}

bool AYogGameMode::RevivePlayerFromDeath()
{
	// Gameplay-only revive: flip the death flags, unpause, restore time dilation, hand control
	// back to the pawn. UI teardown (death menu, mouse cursor, InputMode, SetBlockGameInput) is
	// owned by the death-menu widget that called this; do not touch UI state from here.
	APlayerCharacterBase* Player = PendingDeathPlayer.Get();
	if (!bPlayerDeathPending || bPlayerDeathReviveUsed || !Player)
	{
		return false;
	}

	ClearPlayerDeathGameOverTicker();
	bPlayerDeathPending = false;
	bPlayerDeathReviveUsed = true;
	PendingDeathPlayer.Reset();

	// Restore global time dilation and clear the per-pawn dilation override set in BeginPlayerDeathVisuals.
	// (HUD->EndDeathEffect() inside this helper is the one UI bit still piggy-backing here — extract
	// it to the death-menu widget's OnDeactivated next pass.)
	EndPlayerDeathVisuals(Player);

	UGameplayStatics::SetGamePaused(GetWorld(), false);
	Player->ReviveFromDeath(PlayerReviveHealthPercent, PlayerReviveProtectionDuration);

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		if (UStoryEngineSubsystem* StoryEngine = GetGameInstance()->GetSubsystem<UStoryEngineSubsystem>())
		{
			StoryEngine->BroadcastStoryEventWithPayload(
				FGameplayTag::RequestGameplayTag(TEXT("Story.Event.Player.Revived"), false),
				FGameplayTag(),
				FGameplayTag(),
				FGameplayTag(),
				Player,
				PC);
		}

		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->EnableInput(PC);
		PC->SetPause(false);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Player revived at %.0f%% HP with %.1fs protection."),
		PlayerReviveHealthPercent * 100.f, PlayerReviveProtectionDuration);
	return true;
}

void AYogGameMode::ClearPlayerDeathGameOverTicker()
{
	if (PlayerDeathGameOverTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PlayerDeathGameOverTickerHandle);
		PlayerDeathGameOverTickerHandle.Reset();
	}
}

void AYogGameMode::BeginPlayerDeathVisuals(APlayerCharacterBase* Player)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float DilationScale = FMath::Clamp(PlayerDeathTimeDilationScale, 0.01f, 1.f);
	UGameplayStatics::SetGlobalTimeDilation(World, DilationScale);
	if (Player)
	{
		Player->CustomTimeDilation = 1.f / DilationScale;
	}
	if (AYogHUD* HUD = Cast<AYogHUD>(UGameplayStatics::GetPlayerController(World, 0)
		? UGameplayStatics::GetPlayerController(World, 0)->GetHUD()
		: nullptr))
	{
		HUD->BeginDeathEffect();
	}
}

void AYogGameMode::EndPlayerDeathVisuals(APlayerCharacterBase* Player)
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
		if (AYogHUD* HUD = Cast<AYogHUD>(UGameplayStatics::GetPlayerController(World, 0)
			? UGameplayStatics::GetPlayerController(World, 0)->GetHUD()
			: nullptr))
		{
			HUD->EndDeathEffect();
		}
	}
	if (Player)
	{
		Player->CustomTimeDilation = 1.f;
	}
}

void AYogGameMode::TransitionToLevel(FName NextLevel, URoomDataAsset* NextRoom)
{
	if (NextLevel.IsNone()) return;
	const FName ResolvedNextLevel = ResolveRoomLevelNameForOpen(NextLevel, NextRoom);
	if (ResolvedNextLevel != NextLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("TransitionToLevel: resolved map [%s] -> [%s] for room [%s]"),
			*NextLevel.ToString(), *ResolvedNextLevel.ToString(), *GetNameSafe(NextRoom));
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->PendingNextFloor = CurrentFloor + 1;

		// 写入下一关的房间配置（由传送门的骰子结果决定）
		GI->PendingRoomData = NextRoom;

		if (Player)
		{
			// 保存跑局状态
			FRunState NewState;
			NewState.bIsValid    = true;

			if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
			{
				NewState.CurrentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());

				// 热度切关：默认归零，符文可保留一阶或两阶
				static const FGameplayTag TagRetainTwo = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.TwoPhase"), false);
				static const FGameplayTag TagRetainOne = FGameplayTag::RequestGameplayTag(TEXT("Buff.HeatCarry.OnePhase"), false);
				const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
				if (TagRetainTwo.IsValid() && ASC->HasMatchingGameplayTag(TagRetainTwo))
					NewState.CurrentHeat = MaxHeat * 2.f;
				else if (TagRetainOne.IsValid() && ASC->HasMatchingGameplayTag(TagRetainOne))
					NewState.CurrentHeat = MaxHeat;
				else
					NewState.CurrentHeat = 0.f;
			}

			// 保存当前武器
			NewState.EquippedWeaponDef = Player->EquippedWeaponDef;

			// 保存整理阶段选出但尚未放入格子的符文
			NewState.PendingRunes = Player->PendingRunes;

			Player->CaptureCombatLoadoutForRunState(NewState);
			NewState.CompletedCombatBattleCount = CompletedCombatBattleCount;

			// 保存献祭恩赐
			NewState.ActiveSacrificeGrace = Player->ActiveSacrificeGrace;
			NewState.SacrificeOfferingCosts = Player->GetSacrificeOfferingCosts();
			if (Player->ActiveSkillComponent)
			{
				for (UActiveSkillDataAsset* Skill : Player->ActiveSkillComponent->GetSkillLoadout())
				{
					NewState.SelectedSkillLoadout.Add(Skill);
				}
			}

			GI->PendingRunState = NewState;
			UE_LOG(LogTemp, Warning, TEXT("[RunState] SAVE (Portal) — HP=%.1f Phase=%d Runes=%d Weapon=%s Room=%s"),
				NewState.CurrentHP, NewState.CurrentPhase, NewState.PlacedRunes.Num(),
				NewState.EquippedWeaponDef ? *NewState.EquippedWeaponDef->GetName() : TEXT("none"),
				NextRoom ? *NextRoom->GetName() : TEXT("null"));

			// 保存角色类，供下一关 HandleStartingNewPlayer_Implementation 重建玩家
			if (!GI->PersistentSaveData)
			{
				GI->PersistentSaveData = NewObject<UYogSaveGame>(GI);
			}
			GI->PersistentSaveData->SavedCharacterClass = Player->GetClass();
		}
	}

	UGameplayStatics::OpenLevel(GetWorld(), ResolvedNextLevel);
}

// ─────────────────────────────────────────────────────────────────────────────
// Enemy registry (camera combat awareness)
// ─────────────────────────────────────────────────────────────────────────────

void AYogGameMode::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	if (!IsValid(Enemy)) return;

	// 去重（防止同一敌人注册两次）
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (W.Get() == Enemy) return;
	}

	// 清理失效条目（防止数组无限增长）
	AliveEnemies.RemoveAllSwap([](const TWeakObjectPtr<AEnemyCharacterBase>& W)
	{
		return !W.IsValid();
	});

	AliveEnemies.Add(Enemy);

	if (Enemy->GetCombatTier() == EEnemyCombatTier::Boss)
	{
		OnBossRegisteredNative.Broadcast(Enemy);
	}

	OnEnemyRegisteredNative.Broadcast(Enemy);
}

void AYogGameMode::UnregisterEnemy(AEnemyCharacterBase* Enemy)
{
	// MobSpawner and Die() can both unregister the same enemy; only the call that removes it may broadcast.
	const bool bWasRegistered = AliveEnemies.ContainsByPredicate([Enemy](const TWeakObjectPtr<AEnemyCharacterBase>& W)
	{
		return W.Get() == Enemy;
	});

	AliveEnemies.RemoveAllSwap([Enemy](const TWeakObjectPtr<AEnemyCharacterBase>& W)
	{
		return !W.IsValid() || W.Get() == Enemy;
	});

	if (bWasRegistered)
	{
		OnEnemyUnregisteredNative.Broadcast(Enemy);
	}
}

bool AYogGameMode::HasAliveEnemies() const
{
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (E->IsAlive()) return true;
		}
	}
	return false;
}

FVector AYogGameMode::GetEnemyCentroid(FVector Origin, float WithinRadius) const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	const float RadiusSq = WithinRadius * WithinRadius;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			const float DistSq = FVector::DistSquared(E->GetActorLocation(), Origin);
			if (DistSq <= RadiusSq)
			{
				Sum += E->GetActorLocation();
				Count++;
			}
		}
	}

	return (Count > 0) ? (Sum / static_cast<float>(Count)) : Origin;
}

FVector AYogGameMode::GetNearestEnemyDirection(FVector Origin) const
{
	float MinDistSq = FLT_MAX;
	FVector NearestDir = FVector::ZeroVector;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			const float DistSq = FVector::DistSquared(E->GetActorLocation(), Origin);
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				NearestDir = (E->GetActorLocation() - Origin).GetSafeNormal();
			}
		}
	}
	return NearestDir;
}

TArray<AEnemyCharacterBase*> AYogGameMode::GetNearbyEnemies(FVector Origin, float WithinRadius) const
{
	TArray<AEnemyCharacterBase*> Result;
	const float RadiusSq = WithinRadius * WithinRadius;

	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get())
		{
			if (!E->IsAlive()) continue;
			if (FVector::DistSquared(E->GetActorLocation(), Origin) <= RadiusSq)
			{
				Result.Add(E);
			}
		}
	}
	return Result;
}

TArray<AEnemyCharacterBase*> AYogGameMode::GetAllAliveEnemies() const
{
	TArray<AEnemyCharacterBase*> Result;
	for (const TWeakObjectPtr<AEnemyCharacterBase>& W : AliveEnemies)
	{
		if (AEnemyCharacterBase* E = W.Get(); E && E->IsAlive())
			Result.Add(E);
	}
	return Result;
}


// =========================================================
// Lifecycle event bus
// =========================================================

bool AYogGameMode::IsOneShotEvent(EGameLifecycleEvent Event)
{
	switch (Event)
	{
	case EGameLifecycleEvent::HeatPhaseEntered:
	case EGameLifecycleEvent::PlayerDeath:
	case EGameLifecycleEvent::GameStart:
		return true;
	default:
		return false;
	}
}

void AYogGameMode::TriggerLifecycleEvent(EGameLifecycleEvent Event)
{
	// 一次性事件去重（提前查询，但延后到"成功启动 Flow"才记录，
	// 否则未配 LFA 时会永久吞掉事件，将来配了也不再触发）
	const bool bIsOneShot = IsOneShotEvent(Event);
	if (bIsOneShot && FiredOnceEvents.Contains(Event))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[LifecycleEvents] 事件 %d 已触发过，忽略"), (int32)Event);
		return;
	}

	TObjectPtr<ULevelFlowAsset>* AssetPtr = LifecycleEventFlows.Find(Event);
	if (!AssetPtr || !AssetPtr->Get())
	{
		// 没配映射 = 沉默（用户不想要这个事件的表演），且不写 FiredOnceEvents 留给后续配置生效
		UE_LOG(LogTemp, Log, TEXT("[LifecycleEvents] TriggerLifecycleEvent(%d) 未配 LFA，跳过"), (int32)Event);
		return;
	}

	if (!LifecycleFlowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LifecycleEvents] LifecycleFlowComponent 为空，无法跑 Flow"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LifecycleEvents] TriggerLifecycleEvent(%d) -> %s"),
		(int32)Event, *AssetPtr->Get()->GetName());

	// 停旧 Flow 再启新 Flow（同一时间只跑一个）
	LifecycleFlowComponent->FinishRootFlow(LifecycleFlowComponent->RootFlow, EFlowFinishPolicy::Keep);
	LifecycleFlowComponent->RootFlow = AssetPtr->Get();
	LifecycleFlowComponent->StartRootFlow();

	// 成功启动 Flow 之后再记录，避免静默吞事件
	if (bIsOneShot)
		FiredOnceEvents.Add(Event);
}

bool AYogGameMode::RunStoryLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow)
{
	if (!FlowAsset)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[StoryEvent] RunStoryLevelFlow skipped: FlowAsset is null"));
		return false;
	}

	if (!LifecycleFlowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryEvent] RunStoryLevelFlow failed: LifecycleFlowComponent is null"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[StoryEvent] RunStoryLevelFlow -> %s"), *FlowAsset->GetName());
	if (bStopExistingFlow)
	{
		LifecycleFlowComponent->FinishRootFlow(LifecycleFlowComponent->RootFlow, EFlowFinishPolicy::Keep);
	}
	LifecycleFlowComponent->RootFlow = FlowAsset;
	LifecycleFlowComponent->StartRootFlow();
	return true;
}

