# 开发需求整理 — 星骸降临

> 记录开发者（策划+程序）对各系统的核心手感要求与设计决策。
> 供后续开发时快速对齐意图，避免重复讨论相同问题。
>
> 最后更新：2026-04-18

---

## 一、相机系统

### 1.1 手感核心原则

| 要求 | 说明 | 对应参数/做法 |
| --- | --- | --- |
| 跟随不领先 | 默认关闭 LookAhead 前瞻。移动时相机向前冲又快速回弹会造成眩晕感 | `bEnableLookAhead = false` |
| 静止=静止 | 玩家停下后相机立即稳定，不允许长时间漂移 | `StationarySettleSpeed = 5`（可调高到 15） |
| 分速度管理 | 移动 / 静止 / 冲刺 三个独立速度，手感差异明显 | 见下表 |
| 不追敌 | 相机不主动偏移向敌人，战斗时视野保持中性由玩家自主控制 | DetermineState 不触发 CombatFocus/CombatSearch |
| 无鼠标偏移 | 鼠标移动不影响相机位置，仅手柄右摇杆有效 | 代码中彻底移除读取鼠标的逻辑 |

**默认速度参数（BP Details 可无需重编译调整）：**

| 场景 | 参数 | 推荐值 |
| --- | --- | --- |
| 玩家移动 | `MovingFollowSpeed` | 8 |
| 玩家静止 | `StationarySettleSpeed` | 5 |
| 冲刺 | `DashFollowSpeed` | 18 |

### 1.2 可保留但默认关闭的功能

- **LookAhead 前瞻**：关卡行程大（如大地图）时可开启 `bEnableLookAhead = true`
- **战斗偏移**（CombatFocus / CombatSearch）：枚举和 ComputeStateOffset 实现已保留，需要时在 DetermineState 加回触发逻辑

### 1.3 状态优先级（当前）

```
Dash  >  PickupFocus（整理阶段有拾取物）  >  LookAhead/FocusCharacter
```

---

## 二、输入系统

### 2.1 手柄键位分配

| 按键 | 功能 |
| --- | --- |
| 右摇杆（Axis2D）| 相机偏移（`IA_CameraLook` → `SetCameraInputAxis`） |
| Special Left（Select/View）| 背包开关（`IA_OpenBackpack`） |
| D-Pad | 背包格子导航 / 三选一卡片切换 |
| A | 确认 / 抓取符文 |
| B | 取消 / 放下符文 |
| Y | 移除符文 |

### 2.2 鼠标/键盘

- 鼠标移动：**不**影响相机
- Tab 键：背包开关（与手柄 Special Left 并存）

---

## 三、背包 UI 系统

### 3.1 架构原则

1. **视觉与逻辑分离**：颜色/尺寸等视觉参数统一写入 `DA_BackpackStyle`（BackpackStyleDataAsset），策划改视觉无需重编译
2. **拖拽自管**：所有拖拽事件（DragDetected / DragOver / Drop / Cancelled）由 `BackpackScreenWidget` 接管；格子 Widget 设为 HitTestInvisible，不依赖蓝图 DragDropOperation BP
3. **浮空图标不用 DefaultDragVisual**：UE 默认 DefaultDragVisual 从屏幕 (0,0) 飞向鼠标，无法直接出现在鼠标下方。改用 Canvas 根层 `GrabbedRuneIcon` Image，NativeTick 每帧用 `LastMouseAbsPos` 定位
4. **子 Widget 解耦**：BackpackGridWidget / PendingGridWidget / RuneInfoCardWidget 均为 BindWidgetOptional，蓝图 Designer 放对应名称实例即可

### 3.2 蓝图待完成清单

- `WBP_BackpackScreen`：添加 `RuneInfoCard`（Visibility=Collapsed）、Details 填入 `DA_BackpackStyle`
- `WBP_RuneInfoCard`：放 CardIcon / CardName / CardDesc / CardUpgrade 四个控件
- `WBP_BackpackGrid`：命名"BackpackGridWidget"挂入 BackpackScreen
- `WBP_PendingGrid`：命名"PendingGridWidget"挂入 BackpackScreen

---

## 四、战斗 GA 系统

### 4.1 玩家近战攻击

- **基类 `UGA_PlayerMeleeAttack`**：构造函数自动绑定 `GE_StatBeforeATK` / `GE_StatAfterATK`，所有子 GA 无需手动填写
- **具体 GA**：LightAtk1~4、HeavyAtk1~4、DashAtk — 蓝图子类直接替换旧 GA

### 4.2 冲刺连招桥接（DashSave）

- 攻击蒙太奇在桥接帧通过 AnimNotifyState 授予 `Action.Combo.DashSavePoint` Tag
- 此窗口内冲刺 → 保存当前连招进度 → 冲刺结束后以 LooseTag 恢复 → 接下一击
- 2 秒内未接攻击则 Tag 自动过期，防止残留

---

## 五、通用代码规范（本项目 C++ 约定）

| 规范 | 原因 |
| --- | --- |
| 相机 VInterpTo 起点用 `GetCameraLocation()` | OutVT.POV.Location 在 Super() 后已 snap 到 SpringArm 新位置，以此为起点平滑量为零，造成僵硬感 |
| 可调参数写 `UPROPERTY(EditAnywhere)` | 策划/蓝图侧无需重编译即可调数值，减少迭代摩擦 |
| 互斥参数组加 `EditCondition` | 避免在蓝图 Details 中看到无效参数，减少配置错误 |
| 可选子 Widget 用 `BindWidgetOptional` | 蓝图不放对应控件时不报错，允许渐进式搭建 UI |
| 功能完成后写入 `Docs/FeatureLog.md` | 出 Bug 时直接把对应 log 条目发给 Claude 即可快速定位 |
| 编译前关闭 UE 编辑器 | DLL 被编辑器锁定时 Link 失败（LNK1104） |

---

## 六、遗留待续任务（截至 2026-04-18）

### 关卡系统

- [ ] 血量/伤害倍率施加：`HealthMultiplier` / `DamageMultiplier` 已在 `FDifficultyConfig`，刷怪时未对敌人 ASC 施加对应 GE
- [ ] 传送门随机约束：MinOpenPortals / MaxOpenPortals 配置字段（当前真随机）
- [ ] 符文三选一 UI：绑定 `OnLootGenerated` 委托
- [ ] 金币 HUD：绑定 `OnGoldChanged` 委托

### 战斗系统

- [ ] 无敌帧接入伤害管道：GE_WeaponHitDamage 中检查 `Buff.Status.DashInvincible` 跳过伤害
- [ ] 可穿越几何体：薄墙/家具网格体 Object Type 改为 DashThrough
- [ ] 受击系统：绑定 `GetASC().ReceivedDamage` → 触发 GetHit GA → 播放受击蒙太奇
- [ ] 敌人 GA 动作韧性：敌人 GA 需手动写入 `CurrentActionPoiseBonus`

### 相机系统

- [ ] CameraShake 资产：创建 UCameraShakeBase 蓝图，分配到 `BP_PlayerCameraManager` 的 HeavyHitShakeClass / CritHitShakeClass
