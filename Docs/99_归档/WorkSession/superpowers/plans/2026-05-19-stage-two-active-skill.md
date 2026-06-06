> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# 阶段二：主动技能系统方案

> 前置条件：阶段一闭环（DT、WBP、HUD、Hub、Campaign）已完成并验证通过。

---

## 系统定位

本项目有三套独立机制，互不干涉：

| 机制 | 组件 | 键位 | 来源 |
|---|---|---|---|
| 战斗道具 | `UCombatItemComponent` | F 使用 / Q 切换 | 拾取 |
| 卡组 | `UCombatDeckComponent` | 攻击自动出牌 | 背包构筑 |
| **主动技能** | `UPlayerActiveSkillComponent` | R 使用 / T 切换 | Hub 出发前配置 |

主动技能通过局外解锁获得（MetaProgression），每局出发前在 Hub 选定。

---

## 架构概览

```
MetaProgression
  UnlockFeature(Feature.Combat.ActiveSkill)    → 解锁系统，HUD 槽位显示
  UnlockFeature(Feature.Combat.ActiveSkillSlot2) → 扩展到 2 槽
  UnlockFeature(Feature.Combat.ActiveSkillSlot3) → 扩展到 3 槽

Hub L_HubTown
  BP_HubLoadoutTerminal → WBP_LoadoutSelect
    玩家选择 1-3 个 UActiveSkillDataAsset
    写入 UYogSaveGame.SelectedSkillLoadout

Run 开始
  UPlayerActiveSkillComponent::InitLoadout(SelectedSkills)
  → 按槽挂载各技能，启动 Feature 检查
  → OnSkillSlotsChanged → WBP_ActiveSkillHUD 刷新

战斗中
  IA_UseActiveSkill → UseActiveSkill()
    → 查 Feature.Combat.ActiveSkill 已解锁
    → 检查 CooldownRemaining <= 0
    → GrantAndActivate GA (via ASC)
    → 重置 CooldownRemaining = CooldownDuration
    → Tick 每帧减少 CooldownRemaining
    → OnSkillUsed 广播 → HUD 刷新冷却条

  IA_SelectNextSkill → SelectNextSkill()
    → ActiveSlotIndex = (ActiveSlotIndex + 1) % ActiveSlotCount
    → OnSkillSlotsChanged
```

---

## 新增 C++ 文件

### 1. `UActiveSkillDataAsset` (UPrimaryDataAsset)

**路径：** `Source/DevKit/Public/Data/ActiveSkillDataAsset.h`

```cpp
// 字段：
FText DisplayName;
FText Description;
TObjectPtr<UTexture2D> Icon;
TSubclassOf<UGameplayAbility> GAClass;   // 技能触发的 GA（允许为 None，后期添加）
FGameplayTag SkillTag;                   // 如 Skill.Active.ShieldBurst
float CooldownDuration = 8.0f;          // 冷却秒数
int32 MaxUses = 0;                       // 0=无限次，>0=用完技能槽灰掉
```

### 2. `FActiveSkillSlotView` (USTRUCT, BlueprintType)

镜像 `FCombatItemSlotView`：

```cpp
FName SkillId;
FText DisplayName;
UTexture2D* Icon;
float CooldownRemaining;    // HUD 进度条用
float CooldownDuration;
int32 UsesRemaining;        // 0=无限
bool bSelected;
bool bOnCooldown;
```

### 3. `UPlayerActiveSkillComponent` (UActorComponent)

**路径：** `Source/DevKit/Public/Component/PlayerActiveSkillComponent.h`

镜像 `UCombatItemComponent`，核心 API：

```cpp
// 加载当局技能（从 SaveGame 读取，Hub 出发前调用）
void InitLoadout(const TArray<UActiveSkillDataAsset*>& Skills);

// 使用当前选中技能（由 IA_UseActiveSkill 绑定）
bool UseActiveSkill();

// 切换到下一个槽（由 IA_SelectNextSkill 绑定）
void SelectNextSkill();

// 直接跳到指定槽
void SetActiveSlotIndex(int32 NewIndex);

// 当前有效槽数（受 Feature.Combat.ActiveSkillSlot2/3 控制）
int32 GetActiveSlotCount() const;

// UI 用视图数据
TArray<FActiveSkillSlotView> GetSlotViews() const;

// 委托（供 WBP 绑定）
FActiveSkillSlotsChangedDelegate OnSkillSlotsChanged;
FActiveSkillUsedDelegate OnSkillUsed;
FActiveSkillUseFailedDelegate OnSkillUseFailed;
```

冷却实现：每帧 TickComponent 减少 `CooldownRemaining`，不依赖 `USkillChargeComponent`（后期可升级为 GAS 属性驱动）。

### 4. 在 `PlayerCharacterBase` 添加组件

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Active Skill")
TObjectPtr<UPlayerActiveSkillComponent> ActiveSkillComponent;
```

---

## 新增 GameplayTags

追加到 `Config/DefaultGameplayTags.ini`：

```ini
+GameplayTagList=(Tag="Feature.Combat.ActiveSkill",DevComment="主动技能系统已解锁")
+GameplayTagList=(Tag="Feature.Combat.ActiveSkillSlot2",DevComment="主动技能第2槽已解锁")
+GameplayTagList=(Tag="Feature.Combat.ActiveSkillSlot3",DevComment="主动技能第3槽已解锁")
+GameplayTagList=(Tag="Skill.Active.ShieldBurst",DevComment="测试技能：护盾爆发")
```

---

## 新增 Input Actions

存放路径：`Content/Code/Core/Input/Actions/`

| 资产名 | 键位建议 | 说明 |
|---|---|---|
| `IA_UseActiveSkill` | R | 使用当前选中技能 |
| `IA_SelectNextSkill` | T | 切换到下一个技能槽 |

键位为建议值，在 `IMC_Default` 中配置，用户可在编辑器里修改。

---

## 新增 WBP 资产

### `WBP_ActiveSkillSlot`

**父类：** CommonActivatableWidget 或 UserWidget  
BindWidget 名（对应 C++ 无绑定，纯 Blueprint 实现）：

| 控件 | 类型 | 用途 |
|---|---|---|
| `Img_SkillIcon` | Image | 技能图标 |
| `ProgressBar_CD` | ProgressBar | 冷却进度（CooldownRemaining/CooldownDuration） |
| `Txt_Uses` | TextBlock | 剩余使用次数（MaxUses>0 才显示） |
| `Border_Selected` | Border | 选中高亮框 |

每帧在 `EventTick` 更新 ProgressBar_CD（或用 AnimationTimeline）。

### `WBP_ActiveSkillHUD`

**父类：** UserWidget  
动态创建 1–3 个 `WBP_ActiveSkillSlot`，订阅 `OnSkillSlotsChanged` 刷新。  
添加到 `BP_YogHUD` 的 Viewport（`Feature.Combat.ActiveSkill` 解锁后才 AddToViewport）。

### `WBP_LoadoutSelect`

**父类：** `YogCommonActivatableWidget` 或 `CommonActivatableWidget`  
功能：展示可用技能列表，玩家拖拽或点击选择 1–3 个技能进入出发槽。  
选定后写入 `UYogSaveGame.SelectedSkillLoadout`。

---

## 新增 Hub 资产

### `BP_HubLoadoutTerminal`

**父类：** `BP_HubFacilityActor`  
配置：`WidgetClass = WBP_LoadoutSelect`，`FacilityDisplayName = 技能配置`

在 `L_HubTown` 放置于 `BP_HubUpgradeTerminal` 旁边（如 Location X=500, Y=0）。  
仅当 `Feature.Combat.ActiveSkill` 已解锁时该设施才可交互（在 `BP_OnInteract` 中检查）。

---

## 存档扩展

在 `UYogSaveGame` 添加：

```cpp
UPROPERTY(SaveGame)
TArray<FSoftObjectPath> SelectedSkillLoadout;
// 最多 3 条，存 DataAsset 软引用
// 出发前由 WBP_LoadoutSelect 写入
// 进入战斗关卡后由 PlayerActiveSkillComponent::InitLoadout 读取
```

---

## MetaProgression 扩展

在 `DT_MetaUpgradeNodes` 添加 2 个新节点（编辑器内操作）：

| RowName | DisplayName | EffectType | FeatureTag | Side | MaxLevel | Cost |
|---|---|---|---|---|---|---|
| `Node.Skill.Unlock` | 主动技能：初觉 | FeatureUnlock | Feature.Combat.ActiveSkill | Mystic | 1 | 30 |
| `Node.Skill.Slot2` | 主动技能：双携 | FeatureUnlock | Feature.Combat.ActiveSkillSlot2 | Mystic | 1 | 50 |

Prerequisites：`Node.Skill.Slot2` 依赖 `Node.Skill.Unlock`。

---

## 测试技能 DataAsset

创建 1 个测试技能（编辑器内操作）：

```text
路径：Content/Data/ActiveSkills/DA_ActiveSkill_ShieldBurst.uasset
类：ActiveSkillDataAsset
DisplayName：护盾爆发
GAClass：（暂时留空，或指向一个只播特效的简单 GA）
CooldownDuration：8.0
MaxUses：0
```

---

## 实施顺序

| 步骤 | 类型 | 需要编译？ |
|---|---|---|
| 1. 添加 GameplayTags | 配置文件 | 否 |
| 2. 新增 `UActiveSkillDataAsset` 类 | C++ | **是** |
| 3. 新增 `UPlayerActiveSkillComponent` 类 | C++ | **是** |
| 4. 在 `PlayerCharacterBase` 挂载组件 | C++ | **是** |
| 5. 在存档类添加 SelectedSkillLoadout | C++ | **是** |
| 6. 编译通过后：创建 2 个 Input Actions（IA_ 资产） | 编辑器 | 否 |
| 7. 在 `IMC_Default` 绑定 R / T 键 | 编辑器 | 否 |
| 8. 创建 `WBP_ActiveSkillSlot` | 编辑器 | 否 |
| 9. 创建 `WBP_ActiveSkillHUD`，接入 BP_YogHUD | 编辑器 | 否 |
| 10. 创建 `WBP_LoadoutSelect` | 编辑器 | 否 |
| 11. 创建 `BP_HubLoadoutTerminal`，放入 L_HubTown | 编辑器 | 否 |
| 12. DT_MetaUpgradeNodes 添加 2 个新节点 | 编辑器 | 否 |
| 13. 创建测试技能 DA_ActiveSkill_ShieldBurst | 编辑器 | 否 |
| 14. PIE 验证：解锁 → 配置 → 出发 → 使用 → 冷却 | 测试 | 否 |

步骤 1–5 是一批 C++ 改动，一次编译解决。步骤 6 起全是编辑器内操作。

---

## 验收标准

- 新存档：主动技能 HUD 不显示，Hub 配置终端不可交互。
- 购买 `Node.Skill.Unlock` 后：HUD 出现 1 个技能槽，Hub 配置终端可交互。
- 在 Hub 选择 `DA_ActiveSkill_ShieldBurst`，出发进入战斗关卡。
- 战斗中按 R 触发技能（GA 激活 / 特效播放），HUD 冷却条开始减少。
- 冷却期间按 R 无反应（`OnSkillUseFailed` 广播）。
- 冷却完成，技能可再次使用。
- 购买 `Node.Skill.Slot2`：HUD 出现 2 个槽，T 键可在槽间切换。

---

## 阶段三边界

阶段二完成后，进入阶段三：

- 制作 2–3 个正式技能 GA（冲刺斩、召唤短暂护盾、范围冰冻等）
- 商店、祭坛、进阶连携开放（FeatureUnlock 控制）
- 更多 Meta 节点（攻击强化、生命强化 II–III 级）
- 音效和特效补充

