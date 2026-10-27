# 敌人可交互尸体 / 鞭尸爆奖励 — 待执行方案

| 项目 | 内容 |
| --- | --- |
| 适用范围 | 敌人死亡掉落流程（`Die` → `GA_Dead` → `FinishDying` → `RollAndSpawnKillRewards`） |
| 适用人群 | 程序（实现）、策划（配置） |
| 配套文档 | [EnemyDeathDisappear_TODO.md](EnemyDeathDisappear_TODO.md) |
| 最后更新 | 2026-10-27 |
| 状态 | **C++ 已写完，未编译、未 PIE 验证**；资产配置待做（见文末） |

## 需求

1. 敌人死亡时**不再掉落** `ARewardPickup`。尸体本身变成可交互物：像拾取物一样有双胶囊碰撞，
   玩家进入范围时走 `IYogInteractable` 注册 / 显示提示，长按 E 拾取。
2. 玩家对尸体攻击 N 次（默认 3 次）后尸体爆开，掉出旧的 `ARewardPickup`（仍可交互拾取）。

## 现状

```
HP 归零 → Die()          关闭胶囊碰撞、计入击杀、SetLifeSpan(8 + 延迟) 兜底
        → GA_Dead 蒙太奇 → StartDeathDelay → (消解 Cue) → 计时器
        → FinishDying()   RollAndSpawnKillRewards() → 在尸体处生成 ARewardPickup
                          → 广播 OnCharacterDied → Destroy()
```

| 位置 | 说明 |
| --- | --- |
| `EnemyCharacterBase.cpp` `Die()` / `FinishDying()` / `RollAndSpawnKillRewards()` | 掉落发生在 `FinishDying` |
| `YogCharacterBase.cpp` `Die()` | 关闭胶囊碰撞 |
| `YogCharacterBase.cpp` `FinishDying()` | 广播死亡委托 + `Destroy()` |
| `GA_Dead.cpp` `StartDeathDelay()` / `OnDeathDelayExpired()` | 消解 Cue + 延迟调用 `FinishDying` |
| `YogTargetType_Melee.cpp` `UYogTargetType_Player::GetTargets_Implementation` | Pawn 球形 Overlap + `IsAlive()` 过滤 → **尸体目前打不到** |
| `GA_MeleeAttack.cpp` `OnEventReceived`（约 1883 行） | 无目标时直接 return |
| `YogGameMode.cpp` `SpawnEnemyKillRewardPickup` | 生成击杀奖励拾取物 |
| `WeaponSpawner.h` | 双胶囊参考：`PlayerInteractVolume` + `BlockVolume` |

## 目标流程

```
Die()          立即 Roll 掉落（从 FinishDying 前移）
               ├─ 无掉落 → 保持旧行为
               └─ 有掉落 → bLootableCorpse = true
GA_Dead        可交互尸体：冻结死亡姿势，跳过消解 Cue
FinishDying()  可交互尸体：广播死亡委托但不 Destroy，
               清除 8 秒 SetLifeSpan 兜底，开启尸体碰撞体
                 ├─ 玩家长按 E（IYogInteractable）→ 发放奖励 → 尸体销毁
                 └─ 玩家攻击 N 次（默认 3）       → 爆炸特效 → 生成旧 ARewardPickup → 尸体销毁
```

## 实施步骤

### 1. 掉落改为死亡时 Roll（`EnemyCharacterBase.h/.cpp`）

- [x] 拆分 `RollAndSpawnKillRewards()`：
  - `RollKillRewards()` → 写入 `PendingCorpseBundledLoot` / `PendingCorpseSkillLoot`
    （两个数组而非计划里的一个：技能奖励必须单独一个拾取物）
  - `SpawnKillRewardPickups(bPlayFocusCue, ScatterRadius, OutPickups)` → 读取上述数组生成拾取物
- [x] `Die()` 中、`Super::Die()` **之前**调用 `RollKillRewards()`（无蒙太奇的敌人会在 `Super::Die()` 内
      同步走到 `GA_Dead::StartDeathDelay`，那时必须已知是否为可交互尸体）；
      结果非空且 `bLootableCorpse` 开启时 `bLootableCorpse = true`

### 2. 尸体双胶囊（参考 `AWeaponSpawner`）

| 组件 | 碰撞设置 | 用途 |
| --- | --- | --- |
| `CorpseInteractVolume` | QueryOnly，Overlap Pawn，半径较大 | Begin/End Overlap → `RegisterInteractable` / `UnregisterInteractable` + 提示显隐 |
| `CorpseHitVolume` | ObjectType = WorldDynamic，贴合身体 | 近战命中检测用 |

- [x] 改为在 `EnterLootableCorpseState()` 中运行时创建（普通敌人不带额外组件 / 控件），
      创建后调用 `UpdateOverlaps()`（玩家已站在尸体上时也能注册）
- [x] 同时创建 `UInteractPromptComponent`（接口自动查找）

### 3. `AEnemyCharacterBase` 实现 `IYogInteractable`

- [x] `CanInteract`：可交互尸体且未被消耗（`IsCorpseInteractable()`）
- [x] `GetInteractPriority`：新增 `YogInteractPriority::Corpse = 45`（介于 Pickup 50 与 Altar 40 之间）
- [x] `TryInteract`：通过 `SpawnEnemyKillRewardPickup` 生成拾取物后立即对其调用 `TryPickup(Player)`，
      再消耗尸体。复用现有金币 / 材料 / 技能发放、BP 特效事件、符文三选一 UI，不重复写发放逻辑

### 4. 尸体保持倒地（`GA_Dead.cpp`）

- [x] `StartDeathDelay` 中若为可交互尸体：`GetMesh()->bPauseAnims = true`，
      防止蒙太奇 BlendOut 后 AnimBP 回到 Idle 站起来
- [x] 可交互尸体跳过消解 Cue；延迟计时器照常执行，保证 `FinishDying` 仍被调用
      （通过新增的虚函数 `AYogCharacterBase::IsLootableCorpse()` 判断）

### 5. 拆分 `AYogCharacterBase::FinishDying()`

- [x] 抽出 `BroadcastCharacterDied()`（`OnCharacterDied` + `OnCharacterDiedNative`）
- [x] 尸体路径按原时机广播但不 `Destroy`，保证 Story 特殊奖励等监听者照常触发
- [x] 尸体路径加重入保护（`GA_Dead` 与 `BFNode_FinishLifecycle` 都可能调用 `FinishDying`）；
      同时 `SetLifeSpan(0)` 清掉 `Die()` 里的 8 秒兜底

### 6. 近战命中计数（`YogTargetType_Melee` / `GA_MeleeAttack.cpp`）

- [x] 新增 `UYogTargetType_Player::CollectLootableCorpses(...)`：WorldDynamic 球形 Overlap，
      过滤可交互尸体，复用 `IsTargetHit` 判定扇形
- [x] 在 `OnEventReceived` 的"无目标 → return"之前调用（仅 `UYogAnimNotifyState_Damage` 伤害窗口分支），
      保证只打到尸体也能计数
- [x] 尸体**不进入** `OutActors`：不吃伤害 GE、不触发顿帧、不结算卡组
- [x] `Corpse->RegisterCorpseHit(ActiveAttackGuid)`：按攻击 GUID 去重（一个伤害窗口会多次触发事件），
      每次挥砍只计 1 次；GUID 无效时退化为 0.3 秒时间窗；
      第 N 次调用 `ExplodeCorpse()` → 特效 → 小范围（90 cm）散布生成拾取物 → `Destroy()`
- [ ] 已知缺口：只有 `AN_MeleeDamage` 单点通知（非伤害窗口）的玩家蒙太奇不会计数

### 7. 策划配置（`EnemyData.h`，新分组 `Enemy|Corpse`）

| 字段 | 默认值 | 说明 |
| --- | --- | --- |
| `bLootableCorpse` | true | 关闭 = 旧的死亡即掉落行为；还要求本次至少 Roll 到一个奖励 |
| `CorpseHitsToExplode` | 3 | 爆尸所需攻击次数 |
| `CorpseInteractRadius` | 140 cm | 长按 E 交互范围（新增，计划里没有） |
| `CorpseExplosionFX` | 空 | 爆尸 Niagara |
| `CorpseHitFX` | 空 | 每次命中尸体的反馈（可选） |

### 8. 文档

- [ ] 更新 [EnemyDeathDisappear_TODO.md](EnemyDeathDisappear_TODO.md)：有掉落的尸体不再按延迟消失
- [x] 登记 FeatureLog（`[CORPSE-001]`）

## 已采用的假设

| 问题 | 当前实现 |
| --- | --- |
| 长按 E 交互尸体的结果 | 直接发放奖励；爆尸只是另一种拿同样奖励的方式。若爆尸应为唯一途径或奖励更多，需改 `TryInteract` |
| 尸体是否阻挡玩家 | 不阻挡：两个体积都是 QueryOnly（已放弃 `bCorpseBlocksPawns` 方案） |
| 拾取物聚焦慢动作 | E 键 / 爆尸生成的拾取物不播（玩家已在交互中）；死亡即掉落路径仍播 |

## 资产待办（C++ 之外）

| 资产 | 动作 | 必需 |
| --- | --- | --- |
| 各 `UEnemyData`（如 `DA_Rat`） | 已有 `KillRewards` 的敌人自动变成尸体；不想要的设 `bLootableCorpse = false` | 是 |
| GameMode BP | `RewardPickupClass` 必须已指定，否则尸体无法领取 / 爆开 | 是 |
| `CorpseExplosionFX` / `CorpseHitFX` | 新建 Niagara 并填入 `UEnemyData` | 否（不配则无特效） |
| 各敌人死亡蒙太奇 | 确认存在，否则尸体冻结在死亡瞬间姿势 | 建议 |

## 验证清单（PIE）

- [ ] 编译通过（新增 `UPROPERTY`，需整关编辑器重编译）
- [ ] 有奖励的敌人死亡：不掉拾取物，倒地不起身，出现 E 提示
- [ ] 长按 E：奖励发放，尸体消失
- [ ] 攻击 3 次：爆开，散出拾取物，拾取物仍可交互；一次挥砍只计 1 次
- [ ] 无奖励 / `bLootableCorpse = false` 的敌人：行为与改动前一致
- [ ] 玩家站在尸体上时敌人死亡，提示能正常出现
- [ ] 带 Story 特殊奖励的敌人：死亡监听照常触发

## 风险与注意事项

- **关卡清怪 / 传送门**：击杀仍在 `Die()` 计数，尸体还在时门就会打开；尸体随关卡卸载销毁
- **`CorpseHitVolume` 不能用 Pawn 类型**：否则投射物、`BFNode_ApplyGEInRadius` 等所有 Pawn 查询都会命中尸体
- **`ATrainingDummyCharacter`** 重写了 `FinishDying` 且无 `KillRewards`，不受影响
- **`BFNode_FinishLifecycle`** 直接调用 `FinishDying()`，同样会走尸体路径
- **必须整关编辑器重编译**：新增 `UPROPERTY`，Live Coding 无法热更
