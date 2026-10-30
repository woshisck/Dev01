// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheater/Cheater.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Combat/FinisherDeprecation.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "GameModes/YogGameMode.h"
#include "GameFramework/PlayerController.h"
#include "System/YogGameInstanceBase.h"
#include "System/YogRunEconomySubsystem.h"
#include "System/YogRuntimeGMSubsystem.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

// ─── 内部辅助 ─────────────────────────────────────────────────────────────────

static APlayerCharacterBase* FindCheatPlayer(UWorld* World)
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
}

static APlayerController* FindRuntimeGMPlayerControllerInWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	return World->GetFirstPlayerController();
}

static bool IsRuntimeGMPlayableWorld(const UWorld* World)
{
	return World
		&& (World->WorldType == EWorldType::PIE
			|| World->WorldType == EWorldType::Game
			|| World->WorldType == EWorldType::GamePreview);
}

static APlayerController* FindRuntimeGMPlayerController(UWorld* PreferredWorld)
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

static void ToggleRuntimeGMPanelForWorld(UWorld* World)
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

#if !UE_BUILD_SHIPPING
static FAutoConsoleCommandWithWorld GToggleRuntimeGMPanelCommand(
	TEXT("Yog.GM"),
	TEXT("Toggles the Runtime GM panel for the current PIE, Standalone, or Development game world."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&ToggleRuntimeGMPanelForWorld));

static FAutoConsoleCommandWithWorld GToggleRuntimeGMPanelAliasCommand(
	TEXT("Yog.GM.Toggle"),
	TEXT("Alias for Yog.GM. Toggles the Runtime GM panel for the current game world."),
	FConsoleCommandWithWorldDelegate::CreateStatic(&ToggleRuntimeGMPanelForWorld));
#endif

APlayerCharacterBase* UYogCheatManager::GetPlayerChar() const
{
	APlayerController* PC = GetOuterAPlayerController();
	return PC ? Cast<APlayerCharacterBase>(PC->GetPawn()) : nullptr;
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

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度设为 %.1f"), Clamped);
}

void UYogCheatManager::Yog_MaxHeat()
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Max = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), Max);

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

void UYogCheatManager::Yog_SetGold(int32 Amount)
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UYogRunEconomySubsystem* Economy = GI ? GI->GetSubsystem<UYogRunEconomySubsystem>() : nullptr;
	if (!Economy) return;

	Economy->RestoreGold(Amount);

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

	const int32 RequiredBattles = 3;
	GameMode->CompletedCombatBattleCount = FMath::Max(GameMode->CompletedCombatBattleCount, RequiredBattles);

	if (UYogGameInstanceBase* GI = World ? Cast<UYogGameInstanceBase>(World->GetGameInstance()) : nullptr)
	{
		if (GI->PendingRunState.bIsValid)
		{
			GI->PendingRunState.CompletedCombatBattleCount = GameMode->CompletedCombatBattleCount;
		}
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
	if (!ASC) return;

	const float Heat    = ASC->GetNumericAttribute(UBaseAttributeSet::GetHeatAttribute());
	const float MaxHeat = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute());

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度: %.1f / %.1f"), Heat, MaxHeat);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
		FString::Printf(TEXT("[GM] Heat: %.1f / %.1f"), Heat, MaxHeat));
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


void UYogCheatManager::Yog_Help()
{
	const TCHAR* Help =
		TEXT("[GM Commands 星骸降临]\n")
		TEXT("── 热度 ──\n")
		TEXT("  Yog_SetHeat <float>       设置热度值\n")
		TEXT("  Yog_MaxHeat               热度设为满值\n")
		TEXT("  Yog_FreezeHeat <0/1>      冻结/解冻热度衰减\n")
		TEXT("── 背包 ──\n")
		TEXT("  Yog_GiveRune <RuneID>     给予符文（入待放置列表）\n")
		TEXT("  Yog_SetGold <int>         设置金币\n")
		TEXT("── 玩家属──\n")
		TEXT("  Yog_GodMode               切换无敌模式\n")
		TEXT("  Yog_SetHP <float>         设置 HP\n")
		TEXT("  Yog_FullHP                HP 恢复满值\n")
		TEXT("  Yog_SetAttack <float>     设置 Attack 属性\n")
		TEXT("── 敌人 ──\n")
		TEXT("  Yog_KillAll               杀死所有敌人\n")
		TEXT("  Yog_FreezeEnemies <0/1>   冻结/解冻所有敌人\n")
		TEXT("── 对话气泡（控制台命令，非 Exec）──\n")
		TEXT("  Yog.Bubble <Text>         按任意文字弹气泡\n")
		TEXT("  Yog.BubbleRow <RowName>   按 DT_BubbleMessages 行名弹气泡\n")
		TEXT("── Debug ──\n")
		TEXT("  Yog_PrintHeat             打印热度/阶段\n")
		TEXT("  Yog_PrintTags             打印玩家所Tag\n")
		TEXT("  Yog_PrintAttributes       打印所有属性值\n")
		TEXT("  Yog_Help                  显示此帮");

	UE_LOG(LogTemp, Log, TEXT("%s"), Help);
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::White, TEXT("[GM] 命令列表已打印到 Output Log"));
}
