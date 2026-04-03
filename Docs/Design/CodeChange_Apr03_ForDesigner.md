# 今天改了什么？—— 策划版
**日期：** 2026-04-03

---

## 三件事

1. **修好了背包** — 程序同学搭好了框架但内部都是空的，今天全部填实，背包现在可以真正运行
2. **写了关卡流程** — 打完怪 → 金币结算 → 符文三选一 → 整理背包 → 进下一关
3. **写了刷怪系统** — 难度分预算波次系统，策划可以在表格里直接配置

---

## 一、背包修了什么

### 修之前的状态

程序同学搭了正确的"架子"，但里面很多函数都是空的，导致：

- 格子判断函数永远返回"不合法" → **没法放任何符文**
- 坐标换算函数永远返回 0 → **所有格子都定位到同一个点**
- 热度变化函数有写法错误 → **游戏到这里必崩溃**
- 符文放置/移动/激活/取消激活的逻辑全是空的

### 修完能做什么

| 功能 | 状态 |
|------|------|
| 把符文放到背包格子上 | ✅ |
| 移动格子上的符文 | ✅ |
| 移除格子上的符文 | ✅ |
| 热度变化时自动扩大/缩小激活区 | ✅ |
| 符文完全在激活区内时自动施加 GE 效果 | ✅ |
| 符文离开激活区时自动移除 GE | ✅ |
| 战斗阶段锁背包，整理阶段解锁 | ✅ |

**激活规则**：符文的**所有格子**都必须在激活区内才算激活，贴在边上一半不算。

---

## 二、关卡流程

### 流程

```
战斗阶段
  敌人刷出 → 玩家打 → 全部打完
       ↓ 自动触发
整理阶段
  背包解锁
  系统随机从掉落池抽 3 个符文给玩家选
       ↓ 玩家选 1 个
  符文进入"待放置区"
  玩家在背包格子上拖放整理
       ↓ 玩家点"确认"
切换中
  背包锁住
  加载下一关
       ↓
战斗阶段（循环）
```

### 你需要创建的数据资产

#### DA_LevelSequence_Run01（`ULevelSequenceDataAsset`）

> 这个控制"进下一关"，每个关卡需要一个

| 字段 | 填什么 |
|------|--------|
| NextLevelName | 下一关的关卡名（和 UE Content Browser 里的场景名完全一致） |
| KillTarget | 这关要打多少只怪（暂备用，新系统用 CampaignData 控制） |
| LootPool | 可能掉落的符文 DataAsset（至少放 3 个） |

#### 在 GameMode BP 里挂上

打开关卡的 GameMode Blueprint，把上面创建好的资产拖到对应槽位。

---

## 三、新刷怪系统

### 核心概念：难度分预算

每一波敌人有一个"分数预算"，系统用这些分数"购买"三样东西：

```
本波预算 = 15分

① 购买"触发条件"（下一波什么时候刷）
   - 全部死亡后刷  = 0分（最安全）
   - 死亡50%后刷   = 2分（压力中等）
   - 死亡20%后刷   = 3分（压力最大）
   - 5秒后刷       = 2分

② 购买"刷怪方式"
   - 一起全刷出    = 1分
   - 逐个刷出      = 1分

③ 用剩余分数"购买"敌人
   - 每只怪有自己的分数（普通怪 2-4 分，精英怪 6-10 分）
   - 一直买到钱不够为止
   - 保证至少买到 1 只怪（即使超出预算也要买）
```

### 关卡难度表

你只需要维护一张表（`DA_Campaign_MainRun`），手动写每一关用什么场景、什么难度：

| 关卡序号 | 场景名 | 难度 |
|---------|--------|------|
| 1 | Level_Prison | 低难度 |
| 2 | Level_Prison | 低难度 |
| 3 | Level_Prison | 精英关 |
| ... | ... | ... |

**优势**：可以精确控制第几关必定出精英，第几关升难度。

### 你需要创建的数据资产（新系统）

#### DA_Room_\<关卡名\>（`URoomDataAsset`）

> 每个关卡场景对应一个，描述"这个场景里有什么"

| 字段 | 填什么 |
|------|--------|
| bIsEliteRoom | 是否是精英关（打勾后可以刷出精英专属怪） |
| EnemyPool | 把敌人类拖进来，每个填一个难度分 |
| LootPool | 符文候选池（三选一用） |
| BuffPool | 关卡词条候选池（给所有敌人的 GE，暂不需要填） |
| LowConfig | 低难度参数（见下表） |
| MediumConfig | 中难度参数 |
| HighConfig | 高难度参数 |

**LowConfig 推荐填法（刚开始测试用）：**

| 字段 | 推荐值 |
|------|--------|
| WaveCountMin / Max | 2 / 2（固定2波） |
| WaveBudgets | [15, 20]（第1波15分，第2波20分） |
| AllowedTriggers | AllEnemiesDead（0分）一个就够 |
| AllowedSpawnModes | Wave（同时刷出）一个就够 |
| BuffCount | 0（暂不填词条） |
| GoldMin / Max | 10 / 20 |

**EnemyPool 填法示例：**

| 敌人类 | DifficultyScore | bEliteOnly |
|--------|----------------|-----------|
| BP_Enemy_Zombie | 3 | 不打勾 |
| BP_Enemy_Archer | 4 | 不打勾 |
| BP_Enemy_Elite_Knight | 8 | 打勾（精英关才出） |

#### DA_Campaign_MainRun（`UCampaignDataAsset`）

> 整个局内的关卡顺序表，一个游戏只需要一个

按顺序填 FloorTable，每一行一关：
- `FloorNumber`：关卡编号（排序用）
- `RoomData`：拖入对应的 DA_Room_XX
- `Difficulty`：Low / Medium / High / Elite
- `LevelName`：这关加载哪个场景（名字要和 UE 里完全一致）

#### 在 GameMode BP 里挂上

把 `DA_Campaign_MainRun` 拖到 GameMode BP 的 `CampaignData` 槽位，系统自动读取并开始刷怪。

---

## 四、今天之后你可以做的事

> 编译成功后，按顺序来

1. **创建 DA_Room_Prison_Normal**，填上 EnemyPool（至少 1 种怪 + 难度分）和 LootPool（至少 3 个符文）
2. **创建 DA_Campaign_MainRun**，FloorTable 填 2-3 关测试用
3. **创建 DA_LevelSequence_Run01**，填 NextLevelName
4. **打开关卡的 GameMode BP**，把 CampaignData 和 LevelSequenceData 挂上去
5. **进游戏跑一遍**，验证：怪能刷出 → 打完触发结算 → 符文三选一 → 进下一关

---

## 五、文件速查

| 你想找的 | 文件路径 |
|---------|---------|
| 背包逻辑 | `Private/Component/BackpackGridComponent.cpp` |
| 符文数据结构 | `Private/Data/RuneDataAsset.cpp` |
| 玩家角色（待放置符文列表） | `Public/Character/PlayerCharacterBase.h` |
| 关卡流程（进阶段、选战利品） | `Public/GameModes/LevelFlowTypes.h` |
| 刷怪类型定义（难度分、波次等） | `Public/GameModes/SpawnTypes.h` |
| 房间数据资产 | `Public/Data/RoomDataAsset.h` |
| 局内序列数据资产 | `Public/Data/CampaignDataAsset.h` |
| GameMode 完整实现 | `Private/GameModes/YogGameMode.cpp` |
