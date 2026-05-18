# 历史遗留蓝图配置方案 + Phase 7/8

---

## Part A：历史遗留蓝图配置

> 部分任务已在 C++ 中完成，其余为纯编辑器操作，Codex 无法自动化。
> 本文档给出分类清单和精确手动步骤。

---

### 已完成（C++ 已有，无需任何操作）

| 任务 | 位置 | 状态 |
|------|------|------|
| GA_PlayerDash 接入 SetDashMode | `GA_PlayerDash.cpp:358,382` — dash 开始/结束各调一次 `CM->SetDashMode()` | ✅ |
| 右摇杆接入 SetCameraInputAxis | `YogPlayerControllerBase.cpp:750` — `CM->SetCameraInputAxis(Value)` | ✅ |
| GE_WeaponHitDamage 检查无敌帧 Tag | `DamageAttributeSet.cpp:243` — 检查 `Buff.Status.DashInvincible` 后 `return` | ✅ |

---

### 需手动在 Editor 中操作

#### 1. BP_YogHUD → LevelEndEffect DA 配置

**目的**：让 HUD 知道使用哪个特效参数资产（圆形揭幕/慢动作参数）。

**步骤**：
1. 在 Content Browser 找到 `BP_YogHUD`（继承自 `AYogHUD`）
2. 打开 Blueprint → Details 面板
3. 找到 `Level End Effect DA`（`TObjectPtr<ULevelEndEffectDA> LevelEndEffectDA`）属性
4. 指定已有的 `LevelEndEffectDA` 资产（若无则先创建：右键 → Blueprint → 继承 `ULevelEndEffectDA`）
5. 编译并保存 BP_YogHUD

---

#### 2. 关卡中放置 ALevelIntroCameraMarker

**目的**：每个关卡开场镜头 — 镜头先切到 Marker 位置停留，再平滑移回玩家。

**步骤**：
1. 打开目标关卡（游戏中每个战斗关卡均需放置）
2. Place Actors → 搜索 `LevelIntroCameraMarker` → 拖入关卡
3. 设置 Marker 的 Transform（放在关卡入口处的高处或戏剧性位置）
4. 在 LevelFlow Blueprint（`BP_LevelFlow`）的 `BeginPlay` 事件中：
   - 获取场景中的 `ALevelIntroCameraMarker` 引用
   - 调用 `TriggerIntro()` 节点
5. 保存关卡

---

#### 3. 刷怪时对敌人施加 HealthMultiplier/DamageMultiplier GE

**目的**：关卡 Buff 的血量/伤害倍率应在敌人生成后施加。

**步骤**：
1. 打开刷怪 GA（`GA_SpawnWave` 或类似名称）
2. 在 Spawn Actor 节点后，获取生成的敌人 ASC
3. 调用 `Apply Gameplay Effect to Self`，指定 `GE_EnemyFloorScaling`（若无则创建：继承 `UGameplayEffect`，添加 `Attribute.HealthMultiplier` 和 `Attribute.DamageMultiplier` 的 Magnitude）
4. 从 `RoomDataAsset → BuffPool` 中读取对应倍率作为 Magnitude 来源
5. 编译保存

---

#### 4. 传送门约束 MinOpenPortals / MaxOpenPortals

**状态**：代码库中未找到对应属性定义（可能已移除或规划中未实现）。

**操作**：与策划确认是否仍需此功能；如需，先在 `APortal.h` 或 `RoomDataAsset.h` 中增加属性，再在 Portal 管理逻辑中读取。

---

#### 5. HB_PlayerMain → Loot → LootSelectionWidgetClass

**目的**：让 HUD 使用 `WBP_LootSelection` 作为战利品选择弹窗。

**步骤**：
1. 打开 `BP_YogHUD`
2. Details → `Loot Selection Widget Class` 属性
3. 指定 `WBP_LootSelection`
4. 编译保存

---

#### 6. 清理 WBP_LootSelection 的 Event On Focus Lost 节点

**目的**：删除意外触发失焦关闭的 Blueprint 节点。

**步骤**：
1. 打开 `WBP_LootSelection`
2. 切换到 Graph 视图
3. 找到 `Event On Focus Lost` 节点（或 `Event On Deactivated`）
4. 确认节点内的逻辑是否应删除（若触发了不期望的关闭行为，删除连线或整个节点）
5. 编译保存

---

#### 7. CameraShake 资产创建并分配

**目的**：冲刺结束/受击时触发屏幕震动。

**步骤**：
1. Content Browser → 右键 → Blueprint Class → 搜索并继承 `UMatineeCameraShake`（或 `UCameraShakeBase`）
2. 创建两个资产：`BS_DashShake`、`BS_HitShake`
3. 配置参数：
   - `BS_DashShake`：OscillationDuration=0.2, Amplitude XYZ=2,2,2, Frequency=20
   - `BS_HitShake`：OscillationDuration=0.15, Amplitude XYZ=4,4,4, Frequency=25
4. 在 `GA_PlayerDash` Blueprint 中，`Commit Ability` 之后添加 `Play World Camera Shake` 节点，指定 `BS_DashShake`
5. 在受击 GA 或 `DamageAttributeSet` 触发处同样添加（或通过 `BP_PlayerController::ClientPlayCameraShake`）

---

#### 8. 金币 HUD 绑定 OnGoldChanged

**目的**：背包金币数量变化时更新金币显示控件。

**步骤**：
1. 打开金币显示 Widget（`WBP_GoldDisplay` 或 `WBP_HUD_Economy`）
2. 在 `Event Construct` 中：
   - 获取玩家角色 → `Get Backpack Grid Component`
   - 绑定 `On Gold Changed` 事件
   - 在回调中调用 `Set Text`（将 `NewGold` 转为字符串）
3. 编译保存

---

## Part B：Phase 7 — 统计写入 + 打造符文授予

> 以下为 C++ 代码变更，Codex 直接实现。

### 涉及文件

| 文件 | 变更 |
|------|------|
| `Source/DevKit/Public/SaveGame/YogSaveSubsystem.h` | 新增 4 个统计写入函数 + `MigrateSaveGame` 声明 |
| `Source/DevKit/Private/SaveGame/YogSaveSubsystem.cpp` | 实现统计函数 + TriggerCheckpoint 更新 HighestFloor |
| `Source/DevKit/Private/System/YogGameInstanceBase.cpp` | `StartNewRunFromFrontend` 调用 `RecordRunStarted` |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | `UpdateFinishLevel` 调用 `RecordEnemyKilled`；`HandlePlayerDeath` 调用 `RecordPlayerDeath` |
| `Source/DevKit/Public/Character/PlayerCharacterBase.h` | 声明 `GrantCraftedStarterRunesAsync()` |
| `Source/DevKit/Private/Character/PlayerCharacterBase.cpp` | 实现 `GrantCraftedStarterRunesAsync()`（AsyncLoad → AddHiddenPassiveRune） |
| `Source/DevKit/Private/Character/YogPlayerControllerBase.cpp` | 在 SetTimerForNextTick lambda 末尾调用 `GrantCraftedStarterRunesAsync()` |
| `Source/DevKit/Private/Component/BackpackGridComponent.cpp` | `AddGold` 内调用 `RecordGoldEarned` |

---

## Part C：Phase 8 — 存档版本迁移框架

| 文件 | 变更 |
|------|------|
| `Source/DevKit/Private/SaveGame/YogSaveSubsystem.cpp` | `SelectSlot` 加版本检查；实现 `MigrateSaveGame(Save, FromVersion, ToVersion)` |
| `Source/DevKit/Public/SaveGame/YogSaveSubsystem.h` | 私有声明 `MigrateSaveGame` |
