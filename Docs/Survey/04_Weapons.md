# 04 武器系统 — 已完成功能盘点

> 范围：武器拾取 / 装备 / 切关恢复 / 火绳枪 6 GA / 弹药 HUD / 输入接入 / 热度发光。
> 武器拾取浮窗 / HUD 飞行图标 → [06_UI_HUD.md](06_UI_HUD.md)；热度发光的材质实现 → [05_FeedbackLayer.md](05_FeedbackLayer.md)。

---

## 功能总览

| 功能名称 | 需求简介 | 完成状态（代码）| 完成状态（编辑器）|
|---|---|---|---|
| 武器拾取 + 装备 + 切关恢复 (WEAPON-FW / COMBAT-003b) | 主武器系统 + 换武器旧 Spawner 恢复原色 | ✅ 编译通过 | ✅ 现有 DA_Weapon_* 已配 |
| 武器初始符文自动放置 (FEAT-027) | 拾取自动塞热度一区 | ✅ 编译通过 | ⚙ DA_Weapon_*.InitialRunes 待填 |
| 火绳枪 6 GA 框架（轻 / 重 / 冲刺 + 单 / 全 / 冲刺装填）| 第一类远程武器 | ✅ C++ 框架完成 | ⚙ BP GAs 待派生（MUSKET-001/002/003）|
| 火绳枪输入接入：Reload + 重攻击松键 + GA Tag 激活 (INPUT-001) | 蓄力松键发射 + R 装填 | ✅ 编译通过 | ⚙ IA_Reload + IMC_Default 待配 |
| 弹药计数器 HUD AmmoCounter (WEAPON-001 / UI-017) | 火绳枪图标式弹量 | ✅ 编译通过 | ⚙ WBP_AmmoCounter 待建（MUSKET-004）|
| 武器随热度发光（WeaponGlowOverlay.ush）(HEAT-GLOW) | Fresnel 边缘光 + 颜色跳变 + 切关追赶同步 | ✅ 编译通过 | ✅ 材质 / DA 已配 |

> 状态口径：✅ 完成 / ⚙ 待配置（注明待办）/ — 不涉及编辑器配置

---

## 武器框架

### [WEAPON-FW / COMBAT-003b] 武器拾取 + 装备 + 切关恢复
> （ID 拆分说明：原 `COMBAT-003` 同时表示武器与三选一，此处取武器部分；三选一部分见 [03_RunLoop.md](03_RunLoop.md) 的 [COMBAT-003a]）
- **设计需求**：一把主武器 — 走近 Spawner 弹浮窗 → 按 E 拾取（与 RewardPickup 同模式）→ 切关后状态保留；换武器时旧 Spawner 恢复原色（不再黑化），旧 WeaponInstance 销毁，热度委托解绑。
- **状态**：✅ C++完成（含 [FIX-009] 拾取黑化移除）
- **核心文件**：
  - `Public/Item/Weapon/WeaponSpawner.h` + `Private/Item/Weapon/WeaponSpawner.cpp`（`TryPickupWeapon`）
  - `Public/Item/Weapon/WeaponInstance.h` + `Private/Item/Weapon/WeaponInstance.cpp`
  - `Public/Item/Weapon/WeaponDefinition.h`（DA — `InitialRunes` / `HeatOverlayMaterial` / `WeaponInfo`）
  - `Public/Item/Weapon/WeaponInfoDA.h`（`WeaponName` / `WeaponDescription` / `Thumbnail` / `Zone1/2/3Image`）
- **设计文档**：[WeaponSystem_Technical](../Systems/Weapon/WeaponSystem_Technical.md)
- **验收方式**：
  1. 走近 WeaponSpawner 应弹浮窗；按 E 应装备武器并隐藏浮窗
  2. 切关后武器应自动恢复装备（FRunState.WeaponDefinition）
  3. 换武器时旧 Spawner 应恢复原 mesh 颜色，新武器装备成功

### [FEAT-027] 武器初始符文自动放置
- **设计需求**：武器拾取时自动把"起始符文"塞进热度一激活区 — 玩家不用手动从背包拖。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/Item/Weapon/WeaponSpawner.cpp`（`TryPickupWeapon` 步骤 4b：`GetActivationZoneCellsForPhase(0)` → 逐格 `TryPlaceRune`）
  - `Public/Item/Weapon/WeaponDefinition.h`（`InitialRunes : TArray<URuneDataAsset*>`）
- **验收方式**：DA_Weapon_Test 配 InitialRunes = [DA_Rune_TestA, DA_Rune_TestB] → 拾取武器后开背包应看到这两个符文已放在热度一激活区

---

## 火绳枪（第一类远程武器）

### [Musket GAs] 火绳枪 6 GA 框架
- **设计需求**：第一类远程武器：轻 / 重 / 冲刺攻击 + 单发 / 全弹 / 冲刺装填，6 个 GA。
- **状态**：✅ C++ 框架完成；⚙ BP GAs 待派生（MUSKET-001~003）
- **核心文件**：
  - `Public/AbilitySystem/Abilities/Musket/GA_MusketBase.h` + `Private/.../GA_MusketBase.cpp`
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h` + `.cpp`
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h` + `.cpp`（含按住蓄力 / 松键发射逻辑）
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_SprintAttack.h` + `.cpp`
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_Reload_Single.h` + `.cpp`
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_Reload_All.h` + `.cpp`
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_SprintReload.h` + `.cpp`
  - `Public/AbilitySystem/AbilityTask/AbilityTask_MusketCharge.h`
  - `Public/Projectile/MusketBullet.h` + `Private/Projectile/MusketBullet.cpp`
  - `Public/AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h`
- **设计文档**：[Musket_System_Guide](../Systems/Weapon/Musket_System_Guide.md)
- **验收方式**：BP GA 派生后按 IA_LightAttack 应发射子弹；按 IA_Reload 应触发装填蒙太奇

### [INPUT-001] 火绳枪输入接入（Reload 绑定 + 重攻击松键修复 + GA Tag 激活）
- **设计需求**：重攻击要能"按住蓄力松键发射"；R 键装填；项目用 `TryActivateAbilitiesByTag` 不走 InputID，旧 `WaitInputRelease` 不触发要换实现。
- **状态**：✅ C++完成；⚙ 编辑器创建 IA_Reload + 赋给 BP_PlayerController + IMC_Default 加 IA_Reload
- **核心文件**：
  - `Public/Character/YogPlayerControllerBase.h` + `Private/Character/YogPlayerControllerBase.cpp`（`Input_HeavyAttack` 加 `ETriggerEvent::Completed` → `HeavyAttackReleased()` → `HandleGameplayEvent(GameplayEvent.Musket.HeavyRelease)`；`Input_Reload` 新增）
  - `Public/AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h` + `.cpp`（改用 `UAbilityTask_WaitGameplayEvent` 等待 HeavyRelease）
  - 6 个 Musket GA 在 `AbilityTags` 补 `PlayerState.AbilityCast.*`；LightAttack 加 `ActivationBlockedTags: Buff.Status.DashInvincible`
  - 新 Tags：`GameplayEvent.Musket.HeavyRelease` / `PlayerState.AbilityCast.LightAtk/HeavyAtk/Dash/Reload`
- **验收方式**：
  1. 按住鼠标右键蓄力 → 松键应发射重击子弹
  2. 按 R 键应触发装填动画
  3. 冲刺无敌帧期间按左键 → LightAttack 应被 BlockedTags 阻断不激活

---

## 弹药系统

### [WEAPON-001 / UI-017] 弹药计数器 HUD（AmmoCounter）
- **设计需求**：火绳枪要有图标式弹量显示 — 横排 MaxAmmo 个图标，当前弹量金色 / 空仓灰色，无需 BP 代码自动刷新。
- **状态**：✅ C++完成；⚙ WBP_AmmoCounter 蓝图待创建（MUSKET-004）
- **核心文件**：
  - `Public/UI/AmmoCounter.h`（基类）
  - `Public/UI/WBP_AmmoCounter.h`（C++ 类，订阅 ASC 的 `CurrentAmmo` / `MaxAmmo` 属性变化）
- **设计文档**：[Musket_System_Guide](../Systems/Weapon/Musket_System_Guide.md)
- **验收方式**：装备火绳枪后开 PIE，HUD 应出现 6 个图标；开一枪应有一个变灰；装填后全恢复金色

---

## 武器视觉表现

### [HEAT-GLOW] 武器随热度阶段发光（WeaponGlowOverlay.ush）
- **设计需求**：玩家热度升阶时武器有 Fresnel 边缘光 + 颜色跳变（Phase 1=白 / 2=绿 / 3=橙黄 / 4=过热红）；切关后武器装备时追赶同步当前热度阶段。
- **状态**：✅ C++完成
- **核心文件**：
  - `Private/Item/Weapon/WeaponInstance.cpp`（`OnHeatPhaseChanged(Phase)` → Overlay Material `EmissiveColor` 参数）
  - `Public/Character/PlayerCharacterBase.h` + `.cpp`（`RegisterGameplayTagEvent` 监听 `Buff.Status.Heat.Phase.1/2/3` → 广播 `OnHeatPhaseChanged`）
  - `Source/DevKit/Shaders/WeaponGlowOverlay.ush`
  - `Source/DevKit/Shaders/GlassRim.ush`（共享 `GR_HueToRGB` / `GR_Iridescence` / `GR_Fresnel` 函数）
- **设计文档**：[Material_Authoring_Guide](../Systems/VFX/Material_Authoring_Guide.md)
- **验收方式**：
  1. GM 命令升阶 → 武器应发对应颜色边缘光
  2. 拾取时玩家已是 Phase 2 → 武器装备瞬间应同步显示绿色光

---

## 待配置清单（本分册涉及）

| ID | 内容 | 文档 |
|---|---|---|
| MUSKET-001 | 火绳枪 BP GAs：Light / Heavy / Sprint Attack（派生自对应 C++ 类） | [Musket_System_Guide](../Systems/Weapon/Musket_System_Guide.md) |
| MUSKET-002 | 火绳枪 BP GAs：Reload_Single / Reload_All / Sprint_Reload | 同上 |
| MUSKET-003 | GE 伤害 + 弹药 Attribute 初始值 | 同上 |
| MUSKET-004 | WBP_AmmoCounter 蓝图配置（父类 = AmmoCounter / WBP_AmmoCounter C++ 类） | [AmmoCounter.h](../../Source/DevKit/Public/UI/AmmoCounter.h) |
| INPUT-001 编辑器 | 创建 `IA_Reload`（Digital，Keyboard R）→ 赋给 `BP_PlayerController.Input_Reload`；`IMC_Default` 加入 IA_Reload | [TASKS](../PM/TASKS.md) |
