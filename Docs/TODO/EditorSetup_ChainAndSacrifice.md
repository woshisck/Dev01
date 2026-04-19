# 编辑器配置指南：链路系统 + 献祭恩赐系统

> 本文档对应代码版本：2026-04-19 第四次会话  
> 前提：代码已编译通过（`UnrealEditor-DevKit.dll` 已更新）  
> 阅读顺序：先配链路测试符文（验证逻辑），再配献祭恩赐

---

## 一、链路系统（Chain System）

### 概念速查

| 角色 | 含义 | 放置限制 |
|------|------|----------|
| **None**（普通符文） | 激活区覆盖时激活 | 无 |
| **Producer（链路传出）** | 在激活区内时，向指定方向传导激活信号到相邻格 | 必须放进激活区才能传导 |
| **Consumer（外圈限定）** | 接收链路信号后激活；效果通常比普通符文强力 | **禁止放入任何阶段的激活区格子** |

---

### 1.1 配置 Producer 符文 DA

1. 在 Content Browser 找到或新建一个 `DA_Rune_*` 资产（`URuneDataAsset`）
2. 打开 Details 面板，找到 **Rune Config → Chain** 折叠区

| 字段 | 值 |
|------|----|
| `Chain Role` | `Producer（链路传出）` |
| `Chain Directions` | 勾选你想传导的方向（例如只向右传导勾 **E**，全方向勾全部 8 个） |

3. 其余字段（RuneName、FlowAsset 等）正常填写
4. **验证**：保存后在背包格子放置此符文，确认它只有在激活区内时符文才激活

---

### 1.2 配置 Consumer 符文 DA

1. 新建一个 `DA_Rune_*` 资产
2. 找到 **Rune Config → Chain** 折叠区

| 字段 | 值 |
|------|----|
| `Chain Role` | `Consumer（外圈限定）` |
| `Chain Directions` | 留空（Consumer 不传导） |

3. **验证放置限制**：  
   - 尝试把此符文拖入 5×5 格子的激活区范围（例如中心 3×3）→ 系统应**阻止放置**（拖放失败）  
   - 把符文放到激活区外圈 → 应允许放置

4. **验证激活传导**：  
   - 将 Producer（ChainDirection 含 E）放在激活区最右列  
   - 将 Consumer 放在 Producer 正右方（激活区外）  
   - 调整热度使 Producer 所在格进入激活区 → Consumer 应激活（FlowAsset 启动）

---

### 1.3 多跳链路测试

```
激活区           外圈
[ Producer-E ] → [ Producer-E ] → [ Consumer ]
```

- 第一个 Producer：ChainRole = Producer，ChainDirections = {E}
- 第二个 Producer：ChainRole = Producer，ChainDirections = {E}（放在第一个右边，激活区外）
- Consumer：放在第二个右边

预期结果：Consumer 经过两跳 Producer 传导后激活。  
（第二个 Producer 不在直接激活区，但被第一个 Producer 的链路激活，继续传播）

---

### 1.4 热度阶段动态刷新测试

调整热度让激活区从小变大（Phase 升级）→ Producer 的覆盖格子进入激活区 → Consumer 自动激活。  
降低热度（Phase 降级）→ Producer 不再在激活区内 → Consumer 自动失活。

---

## 二、献祭恩赐系统（Sacrifice Grace）

### 2.1 创建衰退 FA（FlowAsset）

> 路径建议：`Content/BuffFlow/SacrificeGrace/FA_SacrificeGrace_Decay`

**步骤：**

1. 右键 Content Browser → **BuffFlow → Flow Asset** → 命名 `FA_SacrificeGrace_Decay`
2. 打开 Flow Graph
3. 添加节点：**BuffFlow | Sacrifice → Sacrifice Decay**
4. 连线：`[Start] → [Sacrifice Decay: In]`

**Sacrifice Decay 节点参数（在 Details 面板填写）：**

| 参数 | 说明 | 建议值 |
|------|------|--------|
| `Base Decay Rate` | 初始衰退速率（每秒扣热度点数） | `0.5` |
| `Decay Accel Per Second` | 每秒速率累加量 | `0.02` |
| `Max Decay Rate` | 速率上限 | `3.0` |
| `HP Drain Per Second` | Phase 0 时每秒扣血量 | `5.0` |

5. **Stop 引脚**：暂不连线（FA 停止时 `Cleanup()` 自动清理 Timer）
6. 保存 FA

---

### 2.2 创建 SacrificeGrace DA

> 路径建议：`Content/Data/SacrificeGrace/DA_SacrificeGrace_Default`

1. 右键 Content Browser → **Miscellaneous → Data Asset** → 选择 `SacrificeGraceDA` 类
2. 命名 `DA_SacrificeGrace_Default`，打开填写：

| 字段 | 值 |
|------|----|
| `Base Decay Rate` | `0.5`（与 FA 节点保持一致，便于策划查阅） |
| `Decay Accel Per Second` | `0.02` |
| `Max Decay Rate` | `3.0` |
| `HP Drain Per Second` | `5.0` |
| `Bonus Effect` | 选择一个已有 GE（如攻击力加成）；暂时可留空 |
| `Flow Asset` | 选择上一步创建的 `FA_SacrificeGrace_Decay` |
| `Display Name` | `献祭恩赐` |
| `Description` | `开局热度充满，但热度将持续衰退。Phase 0 时每秒扣血。` |

3. 保存

---

### 2.3 创建 BonusEffect GE（可选，暂跳过也能测试）

如果需要额外奖励效果：

1. 右键 → **Gameplay Effect** → 命名 `GE_SacrificeGrace_Bonus`
2. Duration Policy = `Infinite`
3. Modifiers：添加你想要的额外属性加成（如 Attack Power +10%）
4. 回到 DA，`Bonus Effect` 字段指向此 GE

---

### 2.4 创建拾取物 BP（核心）

> 路径建议：`Content/Blueprints/Pickups/BP_SacrificeGracePickup`

1. 新建 Blueprint Class → 父类选 **Actor**
2. 添加组件：`StaticMesh`（选一个醒目 Mesh，如发光球体）；可选 `Billboard` 在编辑器中可见

**添加以下变量：**

| 变量名 | 类型 | 说明 |
|--------|------|------|
| `SacrificeGraceDA` | `SacrificeGraceDA` (Object Reference) | 由 GameMode 注入 |

**添加函数 `SetSacrificeGraceDA`（供 GameMode 注入 DA）：**

```
函数名：SetSacrificeGraceDA
输入参数：DA (SacrificeGraceDA Object Reference)
节点：Set SacrificeGraceDA → DA
```

**添加碰撞触发（或按 E 拾取）：**

1. 添加 `Sphere Collision` 组件（半径 100）
2. 绑定 `On Component Begin Overlap` 事件
3. 判断 Overlapping Actor 是否是 `PlayerCharacterBase`

**拾取后显示接受/拒绝弹窗：**

```
On Overlap:
  → Cast to PlayerCharacterBase → 成功
  → Create Widget (WBP_GameDialog 或已有是/否弹窗)
  → 设置弹窗标题 = SacrificeGraceDA.DisplayName
  → 设置弹窗内容 = SacrificeGraceDA.Description
  → 绑定 OnConfirm → PlayerChar.AcquireSacrificeGrace(SacrificeGraceDA)
                   → DestroyActor (self)
  → 绑定 OnCancel  → DestroyActor (self)
  → AddToViewport
```

> 如果没有现成的是/否弹窗，最简单的替代方案：  
> 重叠时**直接调用** `AcquireSacrificeGrace`，无需弹窗（先验证逻辑，之后再加 UI）

3. 保存 BP

---

### 2.5 配置 GameMode BP

打开 `BP_YogGameMode`（或项目使用的 GameMode Blueprint），在 Details 面板找到：

**SacrificeGrace 分类：**

| 字段 | 填写内容 |
|------|----------|
| `Sacrifice Grace Pool` | 添加 `DA_SacrificeGrace_Default`（可添加多个，随机抽取） |
| `Sacrifice Drop Chance` | `0.15`（15%） |
| `Sacrifice Pickup Class` | 选择 `BP_SacrificeGracePickup` |

保存。

---

### 2.6 测试流程

#### 快速测试（跳过掉落，直接触发）

在 Level Blueprint 或测试 GA 中：

```
BeginPlay / 按键事件:
  → Get Player Character → Cast to PlayerCharacterBase
  → AcquireSacrificeGrace(DA = DA_SacrificeGrace_Default)
```

**验证清单：**
- [ ] 调用后热度立即跳至最大值
- [ ] BonusEffect GE 生效（若已配置，看属性变化）
- [ ] 等待 1 秒 → 热度开始下降
- [ ] 等待更长时间 → 下降速率逐渐加快，上限 3.0/秒
- [ ] 手动降低热度到 Phase 0（CurrentPhase = 0） → 每秒 HP -5（HP 不会降到 5 以下）
- [ ] 进入新关卡（OpenLevel）→ BeginPlay 重新激活，热度重充满，衰退速率归零

#### 掉落测试

1. 确保 GameMode 配置了 `SacrificePickupClass` 和 `SacrificeGracePool`
2. 打开一个非主城关卡（`bIsHubRoom = false` 的 RoomDA）
3. 击杀所有敌人 → `EnterArrangementPhase` 触发
4. 多次重复（约 7 次），平均 1~2 次应出现额外的献祭拾取物

---

## 三、已知限制与后续计划

| 限制 | 说明 |
|------|------|
| Consumer 武器切换后不重验证 | 切换武器激活区变化后，已放置的 Consumer 不会被强制移出（临时方案） |
| BFNode_SacrificeDecay 节点参数与 DA 参数分离 | FA 节点上填的参数独立于 DA，建议统一以 FA 节点为准 |
| 献祭恩赐仅在 PlayerCharacterBase 内存中持久 | 跨存档不持久化（`SaveGame` 标记未加），需后期处理 |
| 弹窗 UI | 当前使用碰撞直触或临时弹窗；后续接入 `WBP_GameDialogWidget` 标准流程 |

---

## 四、文件索引速查

| 文件 | 用途 |
|------|------|
| `Source/DevKit/Public/Data/RuneDataAsset.h` | `ERuneChainRole`、`EChainDirection`、`FRuneConfig.ChainRole/.ChainDirections` |
| `Source/DevKit/Public/Component/BackpackGridComponent.h/.cpp` | 链路 BFS 逻辑、Consumer 放置验证 |
| `Source/DevKit/Public/Data/SacrificeGraceDA.h` | 献祭 DA 数据结构 |
| `Source/DevKit/Public/BuffFlow/Nodes/BFNode_SacrificeDecay.h/.cpp` | 衰退 Timer 节点 |
| `Source/DevKit/Private/Character/PlayerCharacterBase.cpp` | `AcquireSacrificeGrace()`、BeginPlay 重激活 |
| `Source/DevKit/Private/GameModes/YogGameMode.cpp` | `EnterArrangementPhase` 15% 掉落逻辑 |
