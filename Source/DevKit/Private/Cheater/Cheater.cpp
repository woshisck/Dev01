// Fill out your copyright notice in the Description page of Project Settings.

#include "Cheater/Cheater.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/RuneDataAsset.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

// ─── 内部辅助 ─────────────────────────────────────────────────────────────────

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
	if (!Char) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_SetHeat: 找不到玩家")); return; }

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	const float Clamped = FMath::Clamp(Value, 0.f, ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHeatAttribute()));
	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetHeatAttribute(), Clamped);

	// 通知 BGC 更新热度阶段状态
	if (UBackpackGridComponent* BGC = GetBGC())
	{
		BGC->OnHeatValueChanged(Clamped);
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度设为 %.1f"), Clamped);
}

void UYogCheatManager::Yog_SetPhase(int32 Phase)
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_SetPhase: 找不到 BGC")); return; }

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

	UE_LOG(LogTemp, Log, TEXT("[GM] 热度设为满值 %.1f"), Max);
}

void UYogCheatManager::Yog_FreezeHeat(bool bFreeze)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	// Buff.Status.Heat.Active 存在时 GE_HeatDecay 被 PreGameplayEffectExecute 阻断
	const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(FName("Buff.Status.Heat.Active"));

	if (bFreeze)
	{
		ASC->AddLooseGameplayTag(ActiveTag);
		UE_LOG(LogTemp, Log, TEXT("[GM] 热度衰减已冻结"));
	}
	else
	{
		ASC->RemoveLooseGameplayTag(ActiveTag);
		UE_LOG(LogTemp, Log, TEXT("[GM] 热度衰减已恢复"));
	}
}

// ─── 背包 / 符文 ──────────────────────────────────────────────────────────────

void UYogCheatManager::Yog_GiveRune(int32 RuneID)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) { UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveRune: 找不到玩家")); return; }

	// 遍历所有已加载的 RuneDataAsset，按 RuneID 匹配
	for (TObjectIterator<URuneDataAsset> It; It; ++It)
	{
		URuneDataAsset* DA = *It;
		if (!IsValid(DA) || DA->HasAnyFlags(RF_ClassDefaultObject)) continue;

		if (DA->GetLegacyRuneID() == RuneID)
		{
			FRuneInstance Instance = DA->CreateInstance();
			Char->AddRuneToInventory(Instance);
			UE_LOG(LogTemp, Log, TEXT("[GM] 已给予符文 %s（ID=%d），加入待放置列表"), *DA->GetRuneName().ToString(), RuneID);
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] Yog_GiveRune: 未找到 RuneID=%d 的 DataAsset（确认已加载）"), RuneID);
}

void UYogCheatManager::Yog_ClearRunes()
{
	UBackpackGridComponent* BGC = GetBGC();
	if (!BGC) return;

	// 收集所有 Guid 后批量移除（避免迭代中修改数组）
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

	UE_LOG(LogTemp, Log, TEXT("[GM] 已清空背包符文（共 %d 个）"), Guids.Num());
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

// ─── 玩家属性 ─────────────────────────────────────────────────────────────────

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
		UE_LOG(LogTemp, Log, TEXT("[GM] 无敌模式开启（HP=MaxHP=99999）"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[GM] 无敌模式关闭（HP 已设为 99999，需手动重置属性）"));
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

	UE_LOG(LogTemp, Log, TEXT("[GM] HP 恢复满值 %.1f"), Max);
}

void UYogCheatManager::Yog_SetAttack(float Value)
{
	APlayerCharacterBase* Char = GetPlayerChar();
	if (!Char) return;

	UYogAbilitySystemComponent* ASC = Char->GetASC();
	if (!ASC) return;

	ASC->SetNumericAttributeBase(UBaseAttributeSet::GetAttackAttribute(), Value);
	UE_LOG(LogTemp, Log, TEXT("[GM] Attack 属性设为 %.1f"), Value);
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

	UE_LOG(LogTemp, Log, TEXT("[GM] 已击杀 %d 个敌人"), Count);
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

	UE_LOG(LogTemp, Log, TEXT("[GM] 已%s %d 个敌人"), bFreeze ? TEXT("冻结") : TEXT("解冻"), Count);
}

// ─── Debug 打印 ───────────────────────────────────────────────────────────────

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

	UE_LOG(LogTemp, Log, TEXT("[GM] 玩家属性:"));
	PrintAttr(UBaseAttributeSet::GetHealthAttribute(),    TEXT("Health"));
	PrintAttr(UBaseAttributeSet::GetMaxHealthAttribute(), TEXT("MaxHealth"));
	PrintAttr(UBaseAttributeSet::GetHeatAttribute(),      TEXT("Heat"));
	PrintAttr(UBaseAttributeSet::GetMaxHeatAttribute(),   TEXT("MaxHeat"));
	PrintAttr(UBaseAttributeSet::GetAttackAttribute(),    TEXT("Attack"));
	PrintAttr(UBaseAttributeSet::GetAttackPowerAttribute(),TEXT("AttackPower"));
	PrintAttr(UBaseAttributeSet::GetMoveSpeedAttribute(), TEXT("MoveSpeed"));
	PrintAttr(UBaseAttributeSet::GetShieldAttribute(),    TEXT("Shield"));

	GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green, TEXT("[GM] 属性已打印到 Output Log"));
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
		TEXT("[GM Commands — 星骸降临]\n")
		TEXT("── 热度 ──\n")
		TEXT("  Yog_SetHeat <float>       设置热度值\n")
		TEXT("  Yog_SetPhase <0-3>        强制设置热度阶段\n")
		TEXT("  Yog_MaxHeat               热度设为满值\n")
		TEXT("  Yog_FreezeHeat <0/1>      冻结/解冻热度衰减\n")
		TEXT("── 背包 ──\n")
		TEXT("  Yog_GiveRune <RuneID>     给予符文（入待放置列表）\n")
		TEXT("  Yog_ClearRunes            清空已放置符文\n")
		TEXT("  Yog_SetGold <int>         设置金币\n")
		TEXT("── 玩家属性 ──\n")
		TEXT("  Yog_GodMode               切换无敌模式\n")
		TEXT("  Yog_SetHP <float>         设置 HP\n")
		TEXT("  Yog_FullHP                HP 恢复满值\n")
		TEXT("  Yog_SetAttack <float>     设置 Attack 属性\n")
		TEXT("── 敌人 ──\n")
		TEXT("  Yog_KillAll               杀死所有敌人\n")
		TEXT("  Yog_FreezeEnemies <0/1>   冻结/解冻所有敌人\n")
		TEXT("── Debug ──\n")
		TEXT("  Yog_PrintHeat             打印热度/阶段\n")
		TEXT("  Yog_PrintTags             打印玩家所有 Tag\n")
		TEXT("  Yog_PrintAttributes       打印所有属性值\n")
		TEXT("  Yog_PrintRunes            打印背包符文列表\n")
		TEXT("  Yog_Help                  显示此帮助");

	UE_LOG(LogTemp, Log, TEXT("%s"), Help);
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::White, TEXT("[GM] 命令列表已打印到 Output Log"));
}
