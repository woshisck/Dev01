# 代码修改说明（程序版）
**日期：** 2026-04-03  
**操作者：** Claude（AI）  
**状态：** 已写入，待编译验证

---

## 一、背包系统修复（Bug Fix）

### 1. `Data/RuneDataAsset.cpp` — 实现之前是桩

**`FRuneShape::Rotate90()`**：顺时针旋转 90°，公式 `(col, row) → (-row, col)`，结果归零到左上角。

**`URuneDataAsset::CreateInstance()`**：复制 RuneTemplate，生成新 `FGuid::NewGuid()`。

---

### 2. `Component/BackpackGridComponent.cpp` — 全量实现

原文件几乎全是桩，含以下严重 Bug：

| Bug | 影响 |
|-----|------|
| `IsCellValid()` 永远返回 false | 无法放置任何符文 |
| `CellToIndex()` 永远返回 0 | 格子索引全部错误 |
| `GetRuneCells()` 返回空数组 | 所有形状操作失效 |
| `TryPlaceRune()` 创建空 FPlacedRune | 数据未写入，GridOccupancy 未更新 |
| `OnHeatPercentChanged` 访问 `[2]` | 2元素数组越界，**运行时崩溃** |
| `BeginPlay()` 未初始化 GridOccupancy | 所有格子查询崩溃 |
| `FActivationZoneConfig::MakeDefault()` 未实现 | 默认激活区为空 |

**全部修复，关键实现要点：**

- `BeginPlay`：`GridOccupancy.Init(-1, GridWidth * GridHeight)`；ZoneShapes 为空时调用 `MakeDefault()`
- `IsCellValid`：`X ∈ [0, GridWidth) && Y ∈ [0, GridHeight)`
- `CellToIndex`：`Y * GridWidth + X`
- `GetRuneCells`：遍历 `Shape.Cells`，每个 Offset + Pivot
- `TryPlaceRune`：填充 FPlacedRune → 更新 GridOccupancy → RefreshAllActivations → 广播
- `RemoveRune`：Deactivate → 清 GridOccupancy → RemoveAt → 修复后续下标偏移 → RefreshAllActivations
- `MoveRune`：临时清旧位置 → CanPlaceRune → 失败回滚，成功更新
- `OnHeatPercentChanged`：改用 `[0]/[1]`，加 `Num() < 2` 保护，仅 Tier 变化时广播+刷新
- `IsRuneInActivationZone`：**全部格子**都在 ZoneSet 内才返回 true（ALL，非 ANY）
- `ActivateRune`：MakeEffectContext → MakeOutgoingSpec → ApplyGESpecToSelf，存 Handle
- `DeactivateRune`：RemoveActiveGameplayEffect，清 Handle
- `MakeDefault`：Tier1=1×1(2,2)，Tier2=2×2(2~3,2~3)，Tier3=4×4(1~4,1~4)

---

### 3. `Character/PlayerCharacterBase.h / .cpp` — 新增成员

**新增 include：** `#include "Data/RuneDataAsset.h"`

**新增属性：**
```cpp
UPROPERTY(BlueprintReadOnly, Category = "Backpack")
TArray<FRuneInstance> PendingRunes;  // 待放置符文列表（整理阶段从此处拖放到格子）
```

**新增方法：**
```cpp
UFUNCTION(BlueprintCallable, Category = "Backpack")
void AddRuneToInventory(const FRuneInstance& Rune);  // 由 GameMode.SelectLoot 调用
```

**BeginPlay 新增：**
```cpp
if (BackpackGridComponent && GetAbilitySystemComponent())
    BackpackGridComponent->InitWithASC(GetAbilitySystemComponent());
```

---

## 二、关卡流程新增

### 4. `GameModes/LevelFlowTypes.h` — 新文件

包含：
- `ELevelPhase`（Combat / Arrangement / Transitioning）
- `ELootType`（Rune，预留扩展）
- `FLootOption`（单个战利品选项）
- `ULevelSequenceDataAsset`（NextLevelName + KillTarget + LootPool）

---

### 5. `GameModes/YogGameMode.h / .cpp` — 关卡流程扩展

**新增 include：** `#include "GameModes/LevelFlowTypes.h"`

**新增 Delegates：**
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, ELevelPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootGenerated, const TArray<FLootOption>&, LootOptions);
```

**新增属性：** `CurrentPhase`、`LevelSequenceData`、`CurrentLootOptions`、`OnPhaseChanged`、`OnLootGenerated`

**新增方法：**

| 方法 | 行为 |
|------|------|
| `EnterArrangementPhase()` | Combat → Arrangement，解锁背包，生成战利品 |
| `SelectLoot(int32)` | 选战利品 → AddRuneToInventory |
| `ConfirmArrangementAndTransition()` | Arrangement → Transitioning，锁背包，OpenLevel |
| `GenerateLootOptions()` | Fisher-Yates 洗牌 LootPool，取前3个，广播 |

**`UpdateFinishLevel` 修改：** 击杀目标达成后自动调用 `EnterArrangementPhase()`

---

## 三、新刷怪系统（难度分预算波次）

### 6. `GameModes/SpawnTypes.h` — 新文件

包含：
- `EDifficultyTier`（Low / Medium / High / Elite）
- `ESpawnTriggerType`（AllEnemiesDead=0分 / PercentKilled_50=2分 / PercentKilled_20=3分 / TimeInterval_5s=2分）
- `ESpawnMode`（Wave=1分 / OneByOne=1分）
- `FSpawnTriggerOption`、`FSpawnModeOption`（含 DifficultyScore）
- `FEnemyEntry`（EnemyClass + DifficultyScore + bEliteOnly + EliteVariantClass）
- `FDifficultyConfig`（WaveCount范围 + WaveBudgets + AllowedTriggers + AllowedSpawnModes + 倍率 + BuffCount + 金币范围）

---

### 7. `Data/RoomDataAsset.h / .cpp` — 新文件

`URoomDataAsset`（`UPrimaryDataAsset`）：

```cpp
bool bIsEliteRoom;
TArray<FEnemyEntry>                    EnemyPool;
TArray<TSubclassOf<UGameplayEffect>>   BuffPool;
TArray<TObjectPtr<URuneDataAsset>>     LootPool;
FDifficultyConfig LowConfig, MediumConfig, HighConfig;
```

---

### 8. `Data/CampaignDataAsset.h / .cpp` — 新文件

`FFloorEntry`（FloorNumber + RoomData + Difficulty + LevelName）

`UCampaignDataAsset`（`UPrimaryDataAsset`）：
```cpp
TArray<FFloorEntry> FloorTable;  // 策划手动排列关卡序列
```

---

### 9. `GameModes/YogGameMode.h / .cpp` — 刷怪系统扩展

**新增 includes：** `SpawnTypes.h`、`Data/CampaignDataAsset.h`、`Character/EnemyCharacterBase.h`、`Data/RoomDataAsset.h`

**新增公开属性：**
```cpp
TObjectPtr<UCampaignDataAsset> CampaignData;  // 在 GameMode BP 中指定
int32 CurrentFloor = 1;
```

**新增公开方法：**
```cpp
UFUNCTION(BlueprintCallable) void StartLevelSpawning();
```

**新增私有运行时结构：**
```cpp
struct FWavePlan { ESpawnTriggerType, ESpawnMode, OneByOneInterval, EnemiesToSpawn[], TotalSpawnedInWave, TotalKilledInWave };
```

**核心算法（`BuildWavePlan`，三步预算分配）：**
1. 从 `AllowedTriggers` 随机选触发条件，扣分
2. 从 `AllowedSpawnModes` 随机选刷怪方式，扣分
3. 循环从 `EnemyPool` 随机选怪，扣分（**第一只保证刷出，允许超出预算**），直到预算 ≤ 0

**`UpdateFinishLevel` 修改：**
- 配置了 `CampaignData` 时走新路径：递减 `TotalAliveEnemies`，累加 `TotalKilledInWave`，调用 `CheckWaveTrigger` + `CheckLevelComplete`
- 未配置时保留旧行为

**`StartPlay` 新增：** 若 `CampaignData != nullptr`，自动调用 `StartLevelSpawning()`

**触发条件监听实现：**
- `AllEnemiesDead`：`CheckWaveTrigger` 内检测 `TotalAliveEnemies == 0`
- `TimeInterval_5s`：`SetupWaveTrigger` 设 5 秒定时器
- `PercentKilled_50/20`：`CheckWaveTrigger` 内检测 `TotalKilledInWave >= Ceil(Spawned * 0.5/0.2)`

**关卡完成条件：** `bAllWavesSpawned && TotalAliveEnemies <= 0` → 调用 `EnterArrangementPhase()`

---

## 四、文件变更完整列表

| 文件 | 类型 | 性质 |
|------|------|------|
| `Private/Data/RuneDataAsset.cpp` | 修改 | Bug Fix（桩→实现） |
| `Private/Component/BackpackGridComponent.cpp` | 修改 | Bug Fix + 全量实现 |
| `Public/Character/PlayerCharacterBase.h` | 修改 | 新增 AddRuneToInventory + PendingRunes |
| `Private/Character/PlayerCharacterBase.cpp` | 修改 | 新增 InitWithASC + AddRuneToInventory 实现 |
| `Public/GameModes/LevelFlowTypes.h` | **新建** | 关卡流程类型 |
| `Public/GameModes/SpawnTypes.h` | **新建** | 刷怪系统类型定义 |
| `Public/Data/RoomDataAsset.h` | **新建** | 房间数据资产 |
| `Private/Data/RoomDataAsset.cpp` | **新建** | （空，结构体无实现） |
| `Public/Data/CampaignDataAsset.h` | **新建** | 局内关卡序列数据资产 |
| `Private/Data/CampaignDataAsset.cpp` | **新建** | （空） |
| `Public/GameModes/YogGameMode.h` | 修改 | 新增关卡流程 + 刷怪系统接口 |
| `Private/GameModes/YogGameMode.cpp` | 修改 | 新增关卡流程 + 刷怪系统实现 |

---

## 五、编译注意事项

- VS Code 会显示 `CoreMinimal.h not found` 红线，属 IntelliSense 配置问题，**不影响 VS/UBT 实际编译**
- 编译入口：VS 中右键 `DevKit` → **Build**
- 若遇 EPERM 错误，对相关文件执行 `chmod u+w` 或在 P4 中 Check Out
- 新增文件（SpawnTypes.h、RoomDataAsset、CampaignDataAsset）无需修改 Build.cs，UBT 自动收录

---

## 六、策划配置说明（编译成功后）

### 需要创建的 DataAsset

**1. DA_Room_\<关卡名\>**（`URoomDataAsset`）
- `EnemyPool`：填入敌人类 + 难度分（普通怪 2-4 分，精英 6-10 分）
- `LowConfig.WaveBudgets`：例 `[15, 20]`
- `LowConfig.AllowedTriggers`：至少填一个 `AllEnemiesDead`（0分）
- `LowConfig.AllowedSpawnModes`：至少填一个 `Wave`（1分）
- `LowConfig.BuffCount`：2
- `LootPool`：至少3个符文DataAsset

**2. DA_Campaign_MainRun**（`UCampaignDataAsset`）
- `FloorTable[0]`：RoomData=DA_Room_XX，Difficulty=Low，LevelName=关卡资产名
- `FloorTable[1]`：...

**3. 在 GameMode BP / World Settings 中**
- `CampaignData` 槽位 → 拖入 DA_Campaign_MainRun
- `LevelSequenceData` 槽位 → 拖入 DA_LevelSequence_Run01（关卡流程用）

### TODO（后续迭代）

- `SpawnEnemyFromPool` 中对新生成敌人施加 `ActiveRoomBuffs`（需 MobSpawner 返回 Actor 引用）
- `EliteVariantClass`：高难度下替换敌人类型（现在统一用普通版）
- 精英关敌人（`bEliteOnly`）已有过滤逻辑，填好数据即生效
