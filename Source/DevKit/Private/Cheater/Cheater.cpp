// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheater/Cheater.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Combat/FinisherDeprecation.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "GameModes/YogGameMode.h"
#include "GameFramework/PlayerController.h"
#include "System/YogGameInstanceBase.h"
#include "System/YogRuntimeGMSubsystem.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

// ─── 内部辅助 ─────────────────────────────────────────────────────────────────

namespace
{
	const TCHAR* GYogMoonlightLinkCardPaths[] = {
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward.DA_Rune512_Moonlight_Forward"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Reversed.DA_Rune512_Moonlight_Reversed"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn.DA_Rune512_Burn"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Poison.DA_Rune512_Poison"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Shield.DA_Rune512_Shield"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Pierce.DA_Rune512_Pierce"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack.DA_Rune512_Attack"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_ReduceDamage.DA_Rune512_ReduceDamage"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Splash.DA_Rune512_Splash"),
		TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split.DA_Rune512_Split"),
	};

	APlayerCharacterBase* FindCheatPlayer(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
	}

	void GiveMoonlightLinkCardsToWorld(UWorld* World)
	{
		APlayerCharacterBase* Char = FindCheatPlayer(World);
		UCombatDeckComponent* CombatDeck = Char ? Char->CombatDeckComponent : nullptr;
		if (!CombatDeck)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveMoonlightLinkCards: CombatDeckComponent not found."));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[GM] Moonlight cards failed: no combat deck."));
			}
			return;
		}

		int32 AddedCount = 0;
		TArray<FString> MissingCards;
		for (const TCHAR* CardPath : GYogMoonlightLinkCardPaths)
		{
			URuneDataAsset* RuneAsset = LoadObject<URuneDataAsset>(nullptr, CardPath);
			if (!RuneAsset)
			{
				MissingCards.Add(CardPath);
				UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveMoonlightLinkCards: failed to load %s"), CardPath);
				continue;
			}

			if (CombatDeck->AddCardFromRuneReward(RuneAsset))
			{
				++AddedCount;
			}
			else
			{
				MissingCards.Add(CardPath);
				UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveMoonlightLinkCards: failed to add %s"), CardPath);
			}
		}

		CombatDeck->RefreshDeckView();

		const FString Message = FString::Printf(
			TEXT("[GM] Moonlight link cards added: %d/%d"),
			AddedCount,
			UE_ARRAY_COUNT(GYogMoonlightLinkCardPaths));
		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
		if (!MissingCards.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] Missing moonlight cards: %s"), *FString::Join(MissingCards, TEXT(", ")));
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				6.f,
				MissingCards.IsEmpty() ? FColor::Green : FColor::Yellow,
				Message);
		}
	}

	void SetLevelEndedForWorld(UWorld* World)
	{
		AYogGameMode* GameMode = World ? World->GetAuthGameMode<AYogGameMode>() : nullptr;
		if (!GameMode)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_SetLevelEnded: GameMode not found."));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[GM] Level end failed: no GameMode."));
			}
			return;
		}

		if (GameMode->CurrentPhase != ELevelPhase::Arrangement)
		{
			GameMode->CurrentPhase = ELevelPhase::Combat;
			GameMode->EnterArrangementPhase();
		}

		APlayerCharacterBase* Char = FindCheatPlayer(World);
		if (UBackpackGridComponent* Backpack = Char ? Char->GetBackpackGridComponent() : nullptr)
		{
			Backpack->SetLocked(false);
		}

		const FString Message = TEXT("[GM] Level set to ended / Arrangement phase.");
		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green, Message);
		}
	}

	APlayerController* FindRuntimeGMPlayerControllerInWorld(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		return World->GetFirstPlayerController();
	}

	bool IsRuntimeGMPlayableWorld(const UWorld* World)
	{
		return World
			&& (World->WorldType == EWorldType::PIE
				|| World->WorldType == EWorldType::Game
				|| World->WorldType == EWorldType::GamePreview);
	}

	APlayerController* FindRuntimeGMPlayerController(UWorld* PreferredWorld)
	{
		if (IsRuntimeGMPlayableWorld(PreferredWorld))
		{
			if (APlayerController* PC = FindRuntimeGMPlayerControllerInWorld(PreferredWorld))
			{
				return PC;
			}
		}

		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* CandidateWorld = WorldContext.World();
			if (CandidateWorld == PreferredWorld || !IsRuntimeGMPlayableWorld(CandidateWorld))
			{
				continue;
			}

			if (APlayerController* PC = FindRuntimeGMPlayerControllerInWorld(CandidateWorld))
			{
				return PC;
			}
		}

		return nullptr;
	}

	void ToggleRuntimeGMPanelForWorld(UWorld* World)
	{
#if !UE_BUILD_SHIPPING
		APlayerController* PC = FindRuntimeGMPlayerController(World);
		UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr;
		UYogRuntimeGMSubsystem* RuntimeGM = GameInstance ? GameInstance->GetSubsystem<UYogRuntimeGMSubsystem>() : nullptr;
		if (!PC || !RuntimeGM)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RuntimeGM] Yog.GM failed: PlayerController or RuntimeGM subsystem not found. CommandWorld=%s WorldType=%d"),
				*GetNameSafe(World),
				World ? static_cast<int32>(World->WorldType) : -1);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[RuntimeGM] Yog.GM failed: no playable world."));
			}
			return;
		}

		RuntimeGM->ToggleGMPanel(PC);
#endif
	}

	FAutoConsoleCommandWithWorld GGiveMoonlightLinkCardsCommand(
		TEXT("Yog_GiveMoonlightLinkCards"),
		TEXT("Adds Moonlight link test cards directly to the current player's combat deck."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&GiveMoonlightLinkCardsToWorld));

	FAutoConsoleCommandWithWorld GGiveMoonlightLinkCardsAliasCommand(
		TEXT("Yog.GiveMoonlightLinkCards"),
		TEXT("Adds Moonlight link test cards directly to the current player's combat deck."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&GiveMoonlightLinkCardsToWorld));

	FAutoConsoleCommandWithWorld GSetLevelEndedCommand(
		TEXT("Yog_SetLevelEnded"),
		TEXT("Forces the current level into ended / Arrangement phase for card and backpack testing."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&SetLevelEndedForWorld));

	FAutoConsoleCommandWithWorld GSetLevelEndedAliasCommand(
		TEXT("Yog.SetLevelEnded"),
		TEXT("Forces the current level into ended / Arrangement phase for card and backpack testing."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&SetLevelEndedForWorld));

#if !UE_BUILD_SHIPPING
	FAutoConsoleCommandWithWorld GToggleRuntimeGMPanelCommand(
		TEXT("Yog.GM"),
		TEXT("Toggles the Runtime GM panel for the current PIE, Standalone, or Development game world."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ToggleRuntimeGMPanelForWorld));

	FAutoConsoleCommandWithWorld GToggleRuntimeGMPanelAliasCommand(
		TEXT("Yog.GM.Toggle"),
		TEXT("Alias for Yog.GM. Toggles the Runtime GM panel for the current game world."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ToggleRuntimeGMPanelForWorld));
#endif
}

APlayerCharacterBase* UYogCheatManager::GetPlayerChar() const
{
	APlayerController* PC = GetOuterAPlayerController();
	return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
}

UBackpackGridComponent* UYogCheatManager::GetBGC() const
{
	APlayerCharacterBase* Char = GetPlayerChar();
	return Char ? Char->BackpackGridComponent : nullptr;
}

// ─── 热度 ─────────────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_SetHeat(float Value)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_SetHeat: 找不到玩")); return; }

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Clamped = FMath::Clamp(Value, 0.f, ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute()));
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), Clamped);

	// 通知 BGC 更新热度阶段状
	if (UBackpackGridComponent* BGC = GetBGC())
	{
		BGC->OnHeatValueChanged(Clamped);
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度设为 %.1f"), Clamped);
}

void UYogCheatManager::Yog_SetPhase(int32 Phase)
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_SetPhase: 找不BGC")); return; }

	const int32 Clamped = FMath::Clamp(Phase, 0, 3);
	BGC->RestorePhase(Clamped);
	UE_LOG(LogTemp, Log, TEXT("[GM] 热度阶段设为 %d"), Clamped);
}

void UYogCheatManager::Yog_MaxHeat()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Max = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), Max);

	if (UBackpackGridComponent* BGC = GetBGC())
	{
		BGC->OnHeatValueChanged(Max);
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度设为满%.1f"), Max);
}

void UYogCheatManager::Yog_FreezeHeat(bool bFreeze)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	// Buff.Heat.Active 存在GE_HeatDecay PreGameplayEffectExecute 阻断
	const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(FName("Buff.Heat.Active"));

	if (bFreeze)
	{
		ASC->AddLooseGameplayTag(ActiveTag);
		UE_LOG(LogTemp, Log, TEXT("[GM] 热度衰减已冻"));
	}
	else
	{
		ASC->RemoveLooseGameplayTag(ActiveTag);
		UE_LOG(LogTemp, Log, TEXT("[GM] 热度衰减已恢"));
	}
}

// ─── 背包 / 符文 ──────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_GiveRune(int32 RuneID)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveRune: 找不到玩")); return; }

	// 遍历所有已加载RuneDataAsset，按 RuneID 匹配
	for (TObjectIterator<URuneDataAsset> It; It; ++It)
	{
		URuneDataAsset* DA = *It;
		if (!IsValid(DA) || DA->HasAnyFlags(RF_ClassDefaultObject)) continue;

		if (DA->GetLegacyRuneID() == RuneID)
		{
			FRuneInstance Instance = DA->CreateInstance();
			Char->AddRuneToInventory(Instance);
			UE_LOG(LogTemp, Log, TEXT("[GM] 已给予符%s（ID=%d），加入待放置列"), *DA->GetRuneName().ToString(), RuneID);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveRune: 未找RuneID=%d DataAsset（确认已加载"), RuneID);
}

void UYogCheatManager::Yog_GiveMoonlightLinkCards()
{
	GiveMoonlightLinkCardsToWorld(GetWorld());
}

void UYogCheatManager::Yog_SetLevelEnded()
{
	SetLevelEndedForWorld(GetWorld());
}

void UYogCheatManager::Yog_ClearRunes()
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) return;

	// 收集所Guid 后批量移除（避免迭代中修改数组）
	TArray<FGuid> Guids;
	for (const FPlacedRune& Placed : BGC->GetAllPlacedRunes())
	{
		if (!Placed.bIsPermanent)
		{
			Guids.Add(Placed.Rune.RuneGuid);
		}
	}

	for (const FGuid& Guid : Guids)
	{
		BGC->RemoveRune(Guid);
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 已清空背包符文（%d 个）"), Guids.Num());
}

void UYogCheatManager::Yog_SetGold(int32 Amount)
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) return;

	const int32 Current = BGC->Gold;
	if (Amount >= Current)
	{
		BGC->AddGold(Amount - Current);
	}
	else
	{
		BGC->SpendGold(Current - Amount);
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 金币设为 %d"), Amount);
}

// ─── 玩家属─────────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_GodMode()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	bGodModeActive = !bGodModeActive;

	if (bGodModeActive)
	{
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxHealthAttribute(), 99999.f);
		ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), 99999.f);
		UE_LOG(LogTemp, Log, TEXT("[GM] 无敌模式开启（HP=MaxHP=99999"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[GM] 无敌模式关闭（HP 已设99999，需手动重置属性）"));
	}
}

void UYogCheatManager::Yog_SetHP(float Value)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Max = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float Clamped = FMath::Clamp(Value, 0.f, Max);
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), Clamped);

	UE_LOG(LogTemp, Log, TEXT("[GM] HP 设为 %.1f"), Clamped);
}

void UYogCheatManager::Yog_FullHP()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Max = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), Max);

	UE_LOG(LogTemp, Log, TEXT("[GM] HP 恢复满%.1f"), Max);
}

void UYogCheatManager::Yog_SetAttack(float Value)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), Value);
	UE_LOG(LogTemp, Log, TEXT("[GM] Attack 属性设%.1f"), Value);
}

// ─── 敌人 ─────────────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_KillAll()
{
	UWorld* World = GetWorld();
	if (!World) return;

	int32 Count = 0;
	for (TActorIterator<AEnemyCharacterBase> It(World); It; ++It)
	{
		AEnemyCharacterBase* Enemy = *It;
		if (IsValid(Enemy) && !Enemy->bIsDead)
		{
			Enemy->Die();
			++Count;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 已击杀 %d 个敌"), Count);
}

void UYogCheatManager::Yog_FreezeEnemies(bool bFreeze)
{
	UWorld* World = GetWorld();
	if (!World) return;

	int32 Count = 0;
	for (TActorIterator<AEnemyCharacterBase> It(World); It; ++It)
	{
		AEnemyCharacterBase* Enemy = *It;
		if (IsValid(Enemy) && !Enemy->bIsDead)
		{
			Enemy->CustomTimeDilation = bFreeze ? 0.f : 1.f;
			++Count;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] %s %d 个敌"), bFreeze ? TEXT("冻结") : TEXT("解冻"), Count);
}

// ─── Debug 打印 ───────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_UnlockFinisher()
{
#if UE_BUILD_SHIPPING && !DEVKIT_ENABLE_SHIPPING_CHEATS
	return;
#else
	if (DevKit::Combat::IsFinisherAbilityDeprecated())
	{
		const FString Message = TEXT("[GM] Finisher ability is deprecated; unlock command skipped.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
		}
		return;
	}

	UWorld* World = GetWorld();
	AYogGameMode* GameMode = World ? World->GetAuthGameMode<AYogGameMode>() : nullptr;
	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_UnlockFinisher: GameMode not found"));
		return;
	}

	APlayerCharacterBase* Char = GetPlayerChar();
	UCombatDeckComponent* CombatDeck = Char ? Char->CombatDeckComponent : nullptr;
	const int32 RequiredBattles = CombatDeck
		? FMath::Max(0, CombatDeck->TemporaryFinisherUnlockCompletedBattles)
		: 3;
	GameMode->CompletedCombatBattleCount = FMath::Max(GameMode->CompletedCombatBattleCount, RequiredBattles);

	if (UYogGameInstanceBase* GI = World ? Cast<UYogGameInstanceBase>(World->GetGameInstance()) : nullptr)
	{
		if (GI->PendingRunState.bIsValid)
		{
			GI->PendingRunState.CompletedCombatBattleCount = GameMode->CompletedCombatBattleCount;
		}
	}

	if (CombatDeck)
	{
		CombatDeck->RefreshDeckView();
	}

	const FString Message = FString::Printf(
		TEXT("[GM] Finisher unlocked (%d/%d)"),
		GameMode->CompletedCombatBattleCount,
		RequiredBattles);
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
	}
#endif
}

void UYogCheatManager::Yog_PrintHeat()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	UBackpackGridComponent* BGC = GetBGC();
	if (!ASC || !BGC) return;

	const float Heat    = ASC->GetNumericAttribute(UBaseAttributeSet::GetHeatAttribute());
	const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
	const int32 Phase   = BGC->GetCurrentPhase();

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度: %.1f / %.1f  |  阶段: %d"), Heat, MaxHeat, Phase);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
		FString::Printf(TEXT("[GM] Heat: %.1f / %.1f  Phase: %d"), Heat, MaxHeat, Phase));
}

void UYogCheatManager::Yog_PrintTags()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	FGameplayTagContainer Tags;
	ASC->GetOwnedGameplayTags(Tags);

	UE_LOG(LogTemp, Log, TEXT("[GM] 玩家 Tag 列表（共 %d 个）:"), Tags.Num());
	for (const FGameplayTag& Tag : Tags)
	{
		UE_LOG(LogTemp, Log, TEXT("  - %s"), *Tag.ToString());
	}

	GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow,
		FString::Printf(TEXT("[GM] Tags: %d 个，详见 Output Log"), Tags.Num()));
}

void UYogCheatManager::Yog_PrintAttributes()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	auto PrintAttr = [&](const FGameplayAttribute& Attr, const TCHAR* Name)
	{
		UE_LOG(LogTemp, Log, TEXT("  %-15s = %.2f"), Name, ASC->GetNumericAttribute(Attr));
	};

	UE_LOG(LogTemp, Log, TEXT("[GM] 玩家属"));
	PrintAttr(UBaseAttributeSet::GetHealthAttribute(),    TEXT("Health"));
	PrintAttr(UBaseAttributeSet::GetMaxHealthAttribute(), TEXT("MaxHealth"));
	PrintAttr(UBaseAttributeSet::GetHeatAttribute(),      TEXT("Heat"));
	PrintAttr(UBaseAttributeSet::GetMaxHeatAttribute(),   TEXT("MaxHeat"));
	PrintAttr(UBaseAttributeSet::GetAttackAttribute(),    TEXT("Attack"));
	PrintAttr(UBaseAttributeSet::GetAttackPowerAttribute(),TEXT("AttackPower"));
	PrintAttr(UBaseAttributeSet::GetMoveSpeedAttribute(), TEXT("MoveSpeed"));
	PrintAttr(UBaseAttributeSet::GetShieldAttribute(),    TEXT("Shield"));

	GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green, TEXT("[GM] 属性已打印Output Log"));
}

void UYogCheatManager::Yog_PrintRunes()
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) return;

	const TArray<FPlacedRune>& Runes = BGC->GetAllPlacedRunes();
	UE_LOG(LogTemp, Log, TEXT("[GM] 背包符文（共 %d 个）:"), Runes.Num());

	for (const FPlacedRune& Placed : Runes)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%s] ID=%d  Lv=%d  Pivot=(%d,%d)  Active=%s  Permanent=%s"),
			*Placed.Rune.RuneConfig.RuneName.ToString(),
			Placed.Rune.RuneConfig.RuneID,
			Placed.Rune.UpgradeLevel,
			Placed.Pivot.X, Placed.Pivot.Y,
			Placed.bIsActivated ? TEXT("Y") : TEXT("N"),
			Placed.bIsPermanent ? TEXT("Y") : TEXT("N"));
	}

	GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Orange,
		FString::Printf(TEXT("[GM] 符文: %d 个，详见 Output Log"), Runes.Num()));
}

void UYogCheatManager::Yog_Help()
{
	const TCHAR* Help =
		TEXT("[GM Commands 星骸降临]\n")
		TEXT("── 热度 ──\n")
		TEXT("  Yog_SetHeat <float>       设置热度值\n")
		TEXT("  Yog_SetPhase <0-3>        强制设置热度阶段\n")
		TEXT("  Yog_MaxHeat               热度设为满值\n")
		TEXT("  Yog_FreezeHeat <0/1>      冻结/解冻热度衰减\n")
		TEXT("── 背包 ──\n")
		TEXT("  Yog_GiveRune <RuneID>     给予符文（入待放置列表）\n")
		TEXT("  Yog_ClearRunes            清空已放置符文\n")
		TEXT("  Yog_SetGold <int>         设置金币\n")
		TEXT("── 玩家属──\n")
		TEXT("  Yog_GodMode               切换无敌模式\n")
		TEXT("  Yog_SetHP <float>         设置 HP\n")
		TEXT("  Yog_FullHP                HP 恢复满值\n")
		TEXT("  Yog_SetAttack <float>     设置 Attack 属性\n")
		TEXT("── 敌人 ──\n")
		TEXT("  Yog_KillAll               杀死所有敌人\n")
		TEXT("  Yog_FreezeEnemies <0/1>   冻结/解冻所有敌人\n")
		TEXT("── Debug ──\n")
		TEXT("  Yog_PrintHeat             打印热度/阶段\n")
		TEXT("  Yog_PrintTags             打印玩家所Tag\n")
		TEXT("  Yog_PrintAttributes       打印所有属性值\n")
		TEXT("  Yog_PrintRunes            打印背包符文列表\n")
		TEXT("  Yog_Help                  显示此帮");

	UE_LOG(LogTemp, Log, TEXT("%s"), Help);
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::White, TEXT("[GM] 命令列表已打印到 Output Log"));
}
