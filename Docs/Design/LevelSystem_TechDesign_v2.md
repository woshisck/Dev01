# 关卡系统技术设计文档 v2.0
**日期：** 2026-04-03  
**状态：** 设计确认，待实现

---

## 一、系统总览

关卡系统由以下几个子系统构成：

```
UCampaignDataAsset（局内全局配置）
  └─ FloorTable[]：每一关 → 对应 URoomDataAsset + 难度等级

URoomDataAsset（单个房间配置）
  ├─ EnemyPool[]       敌人池（含难度分）
  ├─ BuffPool[]        关卡Buff池（给所有敌人）
  ├─ LootPool[]        玩家战利品池（符文三选一）
  ├─ LowConfig         低难度参数
  ├─ MediumConfig      中难度参数
  └─ HighConfig        高难度参数

YogGameMode（运行时）
  ├─ 读取当前关是第几关 → 查 FloorTable → 得到难度等级
  ├─ 按难度配置生成波次计划（预算分配）
  ├─ 执行波次刷怪（触发条件 + 刷怪方式 + 敌人选择，三层随机）
  ├─ 监控结束条件（波次刷完 + 场内清空）
  └─ 触发结算（金币 + 符文三选一 → 整理阶段 → 下一关）
```

---

## 二、核心数据结构

### 2.1 敌人池条目

```cpp
USTRUCT(BlueprintType)
struct FEnemyEntry
{
    GENERATED_BODY()

    // 敌人 Actor 类
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    TSubclassOf<AEnemyCharacterBase> EnemyClass;

    // 该敌人在难度分系统中的"费用"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    int32 DifficultyScore = 3;

    // 是否为精英专属（仅精英关可刷出）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    bool bEliteOnly = false;

    // 高难度版本（如有配置，高难度关卡下用此类替换；若未配置，原敌人附加霸体GE）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
    TSubclassOf<AEnemyCharacterBase> EliteVariantClass;
};
```

### 2.2 刷怪触发条件

控制"这一波结束后，下一波何时刷出"，本身消耗难度分预算：

```cpp
UENUM(BlueprintType)
enum class ESpawnTriggerType : uint8
{
    AllEnemiesDead      UMETA(DisplayName = "全部死亡"),   // 0分：最安全，等清场后再刷
    PercentKilled_50    UMETA(DisplayName = "死亡50%"),    // 2分：场内还有一半就刷
    PercentKilled_20    UMETA(DisplayName = "死亡20%"),    // 3分：刚死几只就刷，压力最大
    TimeInterval_5s     UMETA(DisplayName = "5秒后"),      // 2分：不管死没死，5秒后刷
};

USTRUCT(BlueprintType)
struct FSpawnTriggerOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESpawnTriggerType TriggerType = ESpawnTriggerType::AllEnemiesDead;

    // 此触发条件消耗的难度分
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 DifficultyScore = 0;
};
```

### 2.3 刷怪方式

控制一波内敌人"如何出现"，同样消耗难度分预算：

```cpp
UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    OneByOne    UMETA(DisplayName = "逐个刷入"),   // 1分：固定时间间隔，一只一只出现
    Wave        UMETA(DisplayName = "波次同时"),   // 1分：选好的敌人同时全部刷出
};

USTRUCT(BlueprintType)
struct FSpawnModeOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ESpawnMode SpawnMode = ESpawnMode::Wave;

    // 此刷怪方式消耗的难度分
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 DifficultyScore = 1;

    // OneByOne 时，每只怪之间的间隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="SpawnMode==ESpawnMode::OneByOne"))
    float OneByOneInterval = 3.0f;
};
```

### 2.4 难度等级配置

```cpp
USTRUCT(BlueprintType)
struct FDifficultyConfig
{
    GENERATED_BODY()

    // ---- 波次数量 ----
    // 从 [WaveCountMin, WaveCountMax] 随机选，策划可写固定值（Min==Max）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMin = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    int32 WaveCountMax = 3;

    // 每波的难度分预算（数组长度 = WaveCountMax，不足时循环使用最后一个）
    // 例：[15, 20, 15] 表示第1波15分，第2波20分，第3波15分
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Waves")
    TArray<int32> WaveBudgets = { 15, 20, 15 };

    // ---- 可用的触发条件池 ----
    // 策划填写此难度下允许出现哪些触发条件（系统从中随机抽）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    TArray<FSpawnTriggerOption> AllowedTriggers;

    // ---- 可用的刷怪方式池 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    TArray<FSpawnModeOption> AllowedSpawnModes;

    // ---- 敌人数值倍率 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float HealthMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    float DamageMultiplier = 1.0f;

    // ---- 关卡 Buff 数量 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    int32 BuffCount = 2;

    // ---- 金币奖励范围 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMin = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    int32 GoldMax = 20;
};
```

### 2.5 房间数据资产（核心配置单元）

```cpp
// 命名规范：DA_Room_<场景名>_<类型>
// 例：DA_Room_Prison_Normal、DA_Room_Prison_Elite
UCLASS(BlueprintType)
class DEVKIT_API URoomDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // ---- 房间标识 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    FName RoomName;

    // 是否为精英关（精英关才能刷出 bEliteOnly 的敌人）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
    bool bIsEliteRoom = false;

    // ---- 敌人池 ----
    // 所有可能出现的敌人（含 bEliteOnly 标记）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemies")
    TArray<FEnemyEntry> EnemyPool;

    // ---- 关卡 Buff 池（给所有敌人的）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
    TArray<TSubclassOf<UGameplayEffect>> BuffPool;

    // ---- 玩家战利品池（符文三选一的候选）----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<TObjectPtr<URuneDataAsset>> LootPool;

    // ---- 三套难度配置 ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig LowConfig;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig MediumConfig;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
    FDifficultyConfig HighConfig;
};
```

### 2.6 关卡序列表（全局局内配置）

```cpp
UENUM(BlueprintType)
enum class EDifficultyTier : uint8
{
    Low     UMETA(DisplayName = "低难度"),
    Medium  UMETA(DisplayName = "中难度"),
    High    UMETA(DisplayName = "高难度"),
    Elite   UMETA(DisplayName = "精英关"),
};

// 局内关卡序列中，一格的配置
USTRUCT(BlueprintType)
struct FFloorEntry
{
    GENERATED_BODY()

    // 第几关（从1开始）
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 FloorNumber = 1;

    // 使用哪个房间资产
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<URoomDataAsset> RoomData;

    // 这一关的难度等级（系统自动选对应的 Config）
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EDifficultyTier Difficulty = EDifficultyTier::Low;

    // 对应的 UE 场景名（OpenLevel 用）
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName LevelName;
};

// 命名规范：DA_Campaign_<run名>，例：DA_Campaign_MainRun
UCLASS(BlueprintType)
class DEVKIT_API UCampaignDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    // 关卡序列表（策划手动填写，精确控制每一关的难度和场景）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    TArray<FFloorEntry> FloorTable;
};
```

---

## 三、核心算法：难度分预算刷怪

### 3.1 波次预算分配

```
输入：DifficultyConfig（已选中的难度档）

1. 随机波次数 WaveCount ∈ [WaveCountMin, WaveCountMax]
2. 逐波处理（i = 0..WaveCount-1）：
   Budget = WaveBudgets[min(i, WaveBudgets.Num()-1)]
   执行 GenerateWave(Budget)
```

### 3.2 单波生成（GenerateWave）

```
输入：Budget（本波总分）

Step 1：随机选触发条件
  候选 = AllowedTriggers（按 DifficultyScore <= Budget 过滤）
  若候选为空 → 强制使用 AllEnemiesDead（0分）
  选中触发条件 T → Budget -= T.DifficultyScore

Step 2：随机选刷怪方式
  候选 = AllowedSpawnModes（按 DifficultyScore <= Budget 过滤）
  若候选为空 → 强制使用 Wave（1分），Budget 不足就不扣
  选中方式 M → Budget -= M.DifficultyScore

Step 3：用剩余 Budget 填充敌人列表
  EnemyList = []
  while Budget > 0:
    候选 = EnemyPool（过滤：
      - DifficultyScore <= Budget
      - 若非精英关，排除 bEliteOnly == true 的敌人）
    若候选为空 → 跳出循环
    随机选一只怪 E → EnemyList.Add(E) → Budget -= E.DifficultyScore

Step 4：记录本波计划
  FWavePlan { TriggerType=T, SpawnMode=M, Enemies=EnemyList }
```

### 3.3 执行刷怪

```
执行 FWavePlan：

if SpawnMode == Wave：
  同时 Spawn EnemyList 中所有敌人（从随机 MobSpawner 位置）

if SpawnMode == OneByOne：
  每隔 OneByOneInterval 秒，依次 Spawn 一只

监听触发条件，决定何时执行下一波：
  AllEnemiesDead    → 监听场内敌人数量 == 0
  PercentKilled_50  → 监听本波已死数量 >= 本波总数 * 50%
  PercentKilled_20  → 监听本波已死数量 >= 本波总数 * 20%
  TimeInterval_5s   → 计时器 5 秒后触发
```

### 3.4 关卡结束判定

```
条件：所有波次已刷完 AND 场内敌人数量 == 0
特殊：Boss 或"召唤者"类敌人死亡时，直接触发结束（不等全清）
```

---

## 四、结算流程

```
关卡结束
  ↓
1. 发放金币（GoldMin~GoldMax 随机，自动加入玩家账户）
2. 进入整理阶段（EnterArrangementPhase）
   ↓
   背包解锁
   从 LootPool 随机抽 3 个符文 → 显示给玩家三选一
   玩家选一个 → AddRuneToInventory
   玩家整理背包
   玩家点"确认"→ ConfirmArrangementAndTransition
   ↓
3. 背包锁定
4. 加载下一关（按 FloorTable[CurrentFloor+1].LevelName）
```

---

## 五、关卡 Buff 系统（给敌人）

```
关卡进入时（BeginPlay）：
  从 BuffPool 随机选 BuffCount 个 GE（按难度等级决定数量）
  存入 ActiveRoomBuffs[]
  
新敌人 Spawn 时：
  GameMode 对每个新生成的敌人调用：
    foreach GE in ActiveRoomBuffs：
      ASC->ApplyGESpecToSelf(GE)

敌人死亡时：GE 随敌人销毁，无需手动移除
```

---

## 六、MobSpawner 扩展（高难度版本）

```
MobSpawner 新增属性：
  bool bSpawnEliteVariant（默认 false）

Spawn 敌人时：
  if bSpawnEliteVariant && Enemy.EliteVariantClass != nullptr：
    Spawn EliteVariantClass
  elif bSpawnEliteVariant && Enemy.EliteVariantClass == nullptr：
    Spawn EnemyClass，然后附加"霸体"GE
  else：
    Spawn EnemyClass（正常版）
```

---

## 七、4月15日实现范围

### ✅ 必须做（Demo 能跑通）

| 任务 | 说明 |
|------|------|
| `FEnemyEntry` 结构体 | 敌人 + 难度分 |
| `URoomDataAsset` | 含敌人池 + LootPool + 三套 DifficultyConfig |
| `UCampaignDataAsset` | FloorTable（手动填几关测试用） |
| GameMode 读取当前关难度 | 根据 FloorTable 选 Config |
| 波次预算生成算法 | GenerateWave 完整实现 |
| 触发条件：AllEnemiesDead + TimeInterval | 两种条件够用 |
| 刷怪方式：Wave + OneByOne | 两种都实现 |
| 结束判定（波次完 + 场内清空） | 替换现有 KillCount 逻辑 |
| 金币自动发放 | 一行代码 |
| 关卡结束 → 符文三选一（已实现） | 已有 |

### ⚠️ 简化处理

| 功能 | 简化方式 |
|------|---------|
| 触发条件 PercentKilled_20/50 | 推迟，Demo 先用 AllEnemiesDead + 5s |
| 关卡 Buff 给敌人 | 架构预留，Demo 不填数据不触发 |
| MobSpawner 高难度勾选 | 推迟，Demo 直接刷普通版 |

### ❌ 推迟（完整版再做）

- 精英关系统（bEliteOnly 过滤）
- 难度曲线自动推进（Demo 固定使用 Low 难度）
- 房间 Buff 预览界面（进关前显示词条）
- 遗物/被动道具系统

---

## 八、策划配置指南

### 需要创建的 DataAsset

**1. DA_Room_<关卡名>**（每个关卡场景一个）

| 字段 | 怎么填 |
|------|--------|
| EnemyPool | 把想出现的敌人类拖进来，每个填难度分（建议：普通怪 2-4 分，精英怪 6-10 分） |
| LootPool | 符文 DataAsset 拖进来，至少 3 个 |
| LowConfig.WaveBudgets | 例 [15, 20]，两波分别 15 分和 20 分 |
| LowConfig.BuffCount | 低难度 2 个 |
| LowConfig.GoldMin/Max | 例 10-20 |
| AllowedTriggers | 低难度建议只放 AllEnemiesDead（0分）和 TimeInterval_5s（2分） |
| AllowedSpawnModes | 低难度建议放 Wave（1分） |

**2. DA_Campaign_MainRun**（整个局内一个）

| FloorNumber | RoomData | Difficulty | LevelName |
|-------------|----------|------------|-----------|
| 1 | DA_Room_Prison | Low | Level_Prison |
| 2 | DA_Room_Prison | Low | Level_Prison |
| 3 | DA_Room_Prison_Elite | Elite | Level_Prison |
| ... | ... | ... | ... |

---

## 九、文件规划

| 新文件 | 路径 | 说明 |
|--------|------|------|
| `RoomDataAsset.h/.cpp` | `Public/Data/` `Private/Data/` | 房间数据结构 |
| `CampaignDataAsset.h/.cpp` | `Public/Data/` `Private/Data/` | 局内序列表 |
| `SpawnTypes.h` | `Public/GameModes/` | 所有枚举和结构体 |
| `YogGameMode.h/.cpp` | 修改 | 替换现有刷怪逻辑，接入新系统 |

---

*本文档涵盖 2026-04-03 QA 所有答案，v2.0 对 v1.0 进行了全面重写。*
