# 开发方案 v2：数据编辑接口与统一访问器（DA 主导路线）

> 基于 2026-05-07 用户决策 + Codex 审查整合修订。  
> 上一版方案见 `Docs/WorkSession/codex_plan_review.md` 的审查对象（搬数值到表格的路线已废弃）。

---

## 需求描述

项目数值（伤害、属性、Buff加成、关卡难度）散落在多个 DA 字段里，策划平衡时需要在数十个 .uasset 间反复跳转，无法横向比对。  
**目标**：保持 DA 作为主数据载体，但通过统一访问器、自定义编辑器界面、批量编辑工具，让 DA 的编辑体验逼近表格化。

---

## 关键决策（已确认）

| 项 | 决策 | 备注 |
|---|---|---|
| 主数据载体 | **DA** | 不再强推 DT/CSV 迁移 |
| RuneID 类型 | **GameplayTag** | 替代现有 int32 RuneID |
| ERuneRarity | **保留枚举** | 权重交给 CT_RarityByDepth 集中管理 |
| Fragment 内部数值 | **P1.5 单独设计** | 不阻塞 P0 |
| 角色/武器 DT | **保持现状** | 只补访问器，不动结构 |
| 旧字段清理策略 | **访问器 + 分阶段** | Codex 建议，避免一次性破坏 |
| 编辑接口形态 | **EUW 批量编辑器 + 自定义 DA Detail Panel** | 双管齐下 |

---

## 方案设计

### 一、分层职责（修订）

```
┌────────────────────────────────────────────────────────────┐
│ DA 层 — 主数据载体                                         │
│   RuneDA / EffectDA: 身份(Tag) + 数值字段 + 资产引用       │
│   CharacterData / WeaponData: 身份 + DT RowHandle          │
├────────────────────────────────────────────────────────────┤
│ 访问器层 — 业务代码唯一入口                                │
│   ResolveRuneNumbers() / GetGoldCost() / GetRarity()       │
│   隔离字段实现细节，便于后续重构                           │
├────────────────────────────────────────────────────────────┤
│ 工具层 — 编辑体验提升                                      │
│   EUW: 批量编辑/横向对比/筛选/排序                         │
│   IDetailCustomization: 单条编辑增强（折叠/滑块/联动预览） │
│   DataValidator: 静态校验（Tag 唯一性、引用断链、范围）    │
├────────────────────────────────────────────────────────────┤
│ CurveTable 层 — 成长曲线（仅用于"输入→输出"映射）          │
│   CT_RarityByDepth, CT_DifficultyByWave                    │
└────────────────────────────────────────────────────────────┘
```

### 二、Tag 命名空间扩展

新增三个 Tag 命名空间（更新 `Docs/Tags/Tag_Namespaces.md`）：

- `Rune.ID.<Name>` — 替代 RuneID int32，例如 `Rune.ID.BurningEdge`
- `Effect.ID.<Name>` — EffectDA 身份，例如 `Effect.ID.Bleed`
- `Card.ID.<Name>` — 已存在，沿用

### 三、统一访问器规范

所有 DA 数值字段必须通过访问器访问：

```cpp
// RuneDA
FGameplayTag       URuneDataAsset::GetRuneIdTag() const;
int32              URuneDataAsset::GetGoldCost() const;
ERuneRarity        URuneDataAsset::GetRarity() const;
ERuneTriggerType   URuneDataAsset::GetTriggerType() const;

// EffectDA  
float              UEffectDataAsset::GetDuration() const;
float              UEffectDataAsset::GetPeriod() const;
int32              UEffectDataAsset::GetMaxStack() const;
FGameplayTag       UEffectDataAsset::GetEffectTag() const;

// CharacterData / WeaponData（已有 DT 调用，加一层语义化访问器）
const FYogBaseAttributeData& UCharacterData::GetAttributes() const;
float UCharacterData::GetMaxHealth() const;  // 等价于 GetAttributes().MaxHealth
```

业务代码迁移规则：
- 凡是 `RuneDA->GoldCost` 这种直接字段读取，全部改为 `RuneDA->GetGoldCost()`
- 旧字段加 `UE_DEPRECATED` 宏，编译期警告
- 整个项目搜索过滤 `->RuneInfo.RuneConfig.GoldCost` 等模式逐个替换

### 四、Editor Utility Widget 设计

**EUW_RuneBalanceEditor**（核心工具）：

- 顶部筛选栏：按 Rarity / TriggerType / ChainRole 多选过滤
- 中部表格：每行一个 RuneDA，列出 `RuneIdTag / Name / GoldCost / Rarity / Duration / Period / Magnitude` 等关键数值
- 单元格双击直接编辑，回车保存
- 支持列排序、批量赋值（选中多行后右键"统一设置 GoldCost = 50"）
- 顶部"导出 CSV 快照"按钮（只读快照，便于策划在 Excel 中横向比对，不能反向导入）

**EUW_EffectBalanceEditor**：同结构，针对 EffectDA。

### 五、自定义 IDetailCustomization

`FRuneDataAssetDetails`（C++ 编辑器模块）：

- `RuneInfo.RuneConfig` 分组重排：`身份 / 经济 / 链路 / 触发 / 通用效果`
- 数值字段用 `SSpinBox` 替代默认 `SNumericEntryBox`，提供范围限制和拖拽
- `Rarity` 切换时实时预览"该稀有度在第 1/5/10 关的掉落权重"（读 CT_RarityByDepth）
- 加一个"在 EUW 中打开"按钮，跳转到 EUW_RuneBalanceEditor 并定位本条

---

## 实现步骤

### P0 — 访问器统一 + Tag 化（一周）

1. **新建 `RuneIdTag` 字段**
   - `FRuneConfig` 加 `FGameplayTag RuneIdTag`，`int32 RuneID` 标 `UE_DEPRECATED`
   - 写迁移工具：扫描所有 RuneDA，从现有 RuneID 生成对应 Tag，加进 `Config/Tags/RuneIDs.ini`
   - 业务代码搜索 `RuneID == X` → `RuneIdTag.MatchesTag(...)` 替换

2. **补统一访问器**
   - `URuneDataAsset` / `UEffectDataAsset` 加完整 Get* 方法
   - `UCharacterData` / `UWeaponData` 已有 `GetMovementData()` 等，补齐缺失的字段访问器

3. **业务代码迁移**
   - grep 项目中所有直接字段访问，逐个改为访问器调用
   - 重点路径：商店逻辑、背包 UI、掉落系统、BuffFlow 节点

4. **Codex 建议的双源比对验证（可选但推荐）**
   - 写一个 Editor 命令：遍历所有 DA，调用新访问器和旧字段，diff 输出报告
   - 验证迁移过程中无逻辑变化

### P1 — 编辑器工具（两周，核心价值）

5. **EUW_RuneBalanceEditor**
   - C++ 工具函数：`UDataEditorLibrary::GetAllRuneDAs()` / `BatchSetGoldCost(TArray<URuneDataAsset*>, int32)`
   - EUW 蓝图：调用上述函数，构建表格 UI
   - "导出 CSV 快照"功能（只读，便于 Excel 比对）

6. **EUW_EffectBalanceEditor**
   - 同结构，针对 EffectDA 的 Duration / Period / MaxStack

7. **DataValidator 工具**
   - 静态扫描：RuneIdTag 唯一性、EffectTag 冲突、CharacterData 引用断链、字段范围（GoldCost > 0）
   - 输出报告到 Output Log，可在 EUW 顶部按钮触发

### P1.5 — Fragment 内部数值（单独设计，1 周）

8. **接口设计选择**（届时再定）：
   - 方案 A：给 `URuneEffectFragment` 加 `FDataTableRowHandle MagnitudesRow`
   - 方案 B：BuffFlow 节点 `BFNode_ApplyEffect` 接收 `TMap<FGameplayTag, float>` 参数，运行时通过 SetByCaller 注入

9. **`URuneEffectFragment::ApplyToGE()` 签名扩展**
   - 当前签名 `ApplyToGE(UGameplayEffect*)` 不接 Spec/上下文
   - 改为 `ApplyToGE(UGameplayEffect*, const FRuneEffectContext&)`，Context 携带 SetByCaller 来源

### P2 — 曲线（一周）

10. **CT_RarityByDepth**
    - 四个 Curve：Common/Rare/Epic/Legendary 的权重 vs 关卡深度
    - 修改掉落抽奖逻辑：`GenerateLootBatch()` 读 CurveTable 计算每张 RuneDA 的权重

11. **CT_DifficultyByWave**
    - 难度系数 vs 波次
    - `FRoomDifficultyTier` 字段保留，但运行时叠加 CurveTable 调整

### P3 — 伤害公式集中（远期，按需推进）

12. **先只统一近战命中入口**（Codex 建议）
    - 新建 `UDamageCalculation::ComputeMeleeDamage(Source, Target, Action)`
    - `AN_MeleeDamage` 改为调用此函数
    - Projectile / DoT / BuffFlow 暂不动，逐个评估迁移成本

---

## 涉及文件

### 新建

- `Source/DevKit/Public/Data/DataEditorLibrary.h/.cpp` — EUW 用工具函数
- `Source/DevKitEditor/Private/Customization/RuneDataAssetDetails.h/.cpp` — IDetailCustomization
- `Source/DevKitEditor/Private/Customization/EffectDataAssetDetails.h/.cpp` — 同上
- `Source/DevKitEditor/Private/Tools/DataValidator.cpp` — 静态校验工具
- `Content/Editor/EUW_RuneBalanceEditor.uasset` — 批量编辑器
- `Content/Editor/EUW_EffectBalanceEditor.uasset` — 同上
- `Content/Data/Curves/CT_RarityByDepth.uasset` — P2
- `Content/Data/Curves/CT_DifficultyByWave.uasset` — P2
- `Config/Tags/RuneIDs.ini` — Rune.ID.* Tag 定义
- `Config/Tags/EffectIDs.ini` — Effect.ID.* Tag 定义
- `Docs/Conventions/DataAuthoring.md` — 规范文档（访问器/Tag/EUW 用法）

### 修改

- [RuneDataAsset.h](Source/DevKit/Public/Data/RuneDataAsset.h) — 加 `RuneIdTag`，旧 `RuneID` 标 Deprecated，加访问器
- [EffectDataAsset.h](Source/DevKit/Public/Data/EffectDataAsset.h) — 加访问器（字段不动）
- [CharacterData.h](Source/DevKit/Public/Data/CharacterData.h) — 加细粒度访问器（GetMaxHealth 等）
- [WeaponData.h](Source/DevKit/Public/Data/WeaponData.h) — 同上
- 商店/背包/掉落/BuffFlow 节点的所有直接字段访问 — 改访问器调用

### 文档

- 更新 [Docs/INDEX.md](Docs/INDEX.md) 添加 DataAuthoring 入口
- 更新 [Docs/PM/TASKS.md](Docs/PM/TASKS.md) 加 P0/P1/P1.5/P2/P3 任务
- 更新 [Docs/Tags/Tag_Namespaces.md](Docs/Tags/Tag_Namespaces.md) 加 Rune.ID / Effect.ID 命名空间
- 更新 [Docs/Conventions/BuffFlow.md](Docs/Conventions/BuffFlow.md) 说明访问器使用

---

## 潜在风险

1. **`RuneID` 迁移到 Tag 影响存档**：如果存档里存了 int RuneID，迁移到 Tag 后老存档加载失败。需要在 SaveGame 加 fallback 映射或版本号。

2. **`GoldCost` 已被多处直接读取**（Codex 发现）：商店、背包、UI 同时使用，迁移时需一次性改完，否则新旧并存会出现"商店显示新价、背包显示旧价"。建议加一个 PR 集中迁移。

3. **IDetailCustomization 维护成本**：UE 自定义 Detail 模块语法繁琐，字段增删时容易遗漏。建议 P1 先做 EUW（性价比更高），Detail Customization 评估后再决定是否做。

4. **EUW 性能**：表格展示几百条 RuneDA 时滚动可能卡顿。建议用 ListView 虚拟化，不要一次性创建所有行 Widget。

5. **DataValidator 对未加载资产无效**（Codex 提）：用 AssetRegistry 扫描 .uasset 元数据而非 LoadObject，避免遗漏。

6. **CT_RarityByDepth 与现有三档稀有度不一致**（Codex 提）：当前 `FRoomDifficultyTier` 只有 Common/Rare/Epic 权重，没有 Legendary。需要先决定 Legendary 是否引入，再设计 Curve。

---

## 待确认问题（剩余少量）

1. **`RuneIdTag` 迁移完成后是否删除 `int32 RuneID`？**  
   建议设定截止版本（如 v0.5），届时所有引用迁移完后删除。

2. **EUW_RuneBalanceEditor 的"批量赋值"是否需要 Undo 支持？**  
   UE 编辑器修改 DA 默认走 Transaction，但批量操作如果包装在 Editor Utility 里需要手动 `FScopedTransaction`，决定是否做。

3. **是否引入 Legendary 稀有度？**  
   决定 CT_RarityByDepth 的 Curve 数量。

4. **P0 是否需要做"双源比对验证"工具**（Codex 建议）？  
   多花半天，但能保证迁移期间零回归。建议做。

---

## 工作量估算

| 阶段 | 工时 | 累计 |
|------|------|------|
| P0 访问器统一 + Tag 化 | 1 周 | 1 周 |
| P1 EUW 编辑器 + 校验工具 | 2 周 | 3 周 |
| P1.5 Fragment 数值接口 | 1 周 | 4 周 |
| P2 CurveTable 曲线 | 1 周 | 5 周 |
| P3 伤害公式集中 | 按需 | — |

P0+P1 共 3 周即可让数值编辑体验有质的提升。P1.5/P2/P3 可按业务需要逐个推进。

---

**下一步**：

- 如果决策无误，开始 P0 编码：先做 `RuneIdTag` 字段 + 第一个访问器 `GetGoldCost()`，跑通迁移流程
- 代码完成后执行 `/codex-review-code Source/DevKit/Public/Data/RuneDataAsset.h`（或多个文件）让 Codex 做代码级审查
