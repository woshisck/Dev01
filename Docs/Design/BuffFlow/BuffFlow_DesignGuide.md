# BuffFlow 符文系统 — 策划 / 美术指南

> 版本：Sprint 4.15（2026-04-08）
> 子文档：[节点速查](BuffFlow_NodeReference.md) · [符文制作流程](BuffFlow_RuneWorkflow.md)
> 关联文档：[Buff_Tag_Spec.md](Buff_Tag_Spec.md) · [TestRune_CreationGuide.md](TestRune_CreationGuide.md)

---

## 一、系统是什么？

**一句话：** 在 Flow Graph 里连线就能描述符文行为，不用写代码。

| 层 | 是什么 | 策划/美术负责？ |
|---|---|---|
| **FA**（Flow Asset） | 描述"什么时候、对谁、做什么"的可视化逻辑图 | ✅ 主要在这里工作 |
| **GAS** | 执行属性计算、Tag 管理（引擎功能） | ❌ 自动运行 |
| **DA**（RuneDataAsset） | 符文配置表：名称、图标、占格、挂哪个 FA | ✅ 填表 |

---

## 二、三个核心概念

### 2.1 RuneDataAsset（符文配置表）

> 类比：这是一张"卡牌配置表"，决定符文叫什么、长什么样、占几格、激活时执行哪个 FA。

| 字段 | 说明 | 举例 |
|---|---|---|
| `RuneConfig.RuneName` | 符文名称 | `AttackUp` |
| `RuneConfig.RuneIcon` | 图标贴图 | 拖入 Texture2D |
| `RuneConfig.RuneDescription` | 描述文本 | "激活时 +10 攻击力" |
| `RuneConfig.RuneType` | Buff / Debuff / None | `Buff` |
| `RuneConfig.RuneID` | 策划表 ID | `1001` |
| `Shape.Cells` | 占格坐标数组 | `(0,0)` = 1×1；`(0,0)(1,0)` = 横向 2格 |
| `Flow.FlowAsset` | 激活时启动的 FA | 拖入 FA 资产 |

**创建：** Content Browser → 右键 → Miscellaneous → Data Asset → RuneDataAsset

---

### 2.2 Flow Asset（FA，符文逻辑图）

> 类比：就像蓝图，但专门描述 buff 行为。节点连线 = 逻辑流程。

- 符文放入激活区 → FA **自动启动**
- 符文移出激活区 → FA **自动停止**，所有效果自动清理

**创建：** Content Browser → 右键 → Flow → Flow Asset

---

### 2.3 激活区与热度阶段

> 类比：背包是 5×5 棋盘，激活区是中央"热区"，热度越高热区越大。

| 热度阶段 | 激活区范围 |
|---|---|
| Phase 0（冷） | 中心 1×1 格 |
| Phase 1 | 中心 2×2 格 |
| Phase 2 | 中心 4×4 格 |
| Phase 3（超凡） | 全部格子 |

**永久符文**（`PermanentRunes` 数组）：始终激活，不受激活区限制。

---

## 三、判断需不需要找程序

| 需求 | 策划自己能做？ | 实现方式 |
|---|---|---|
| 修改属性（固定值） | ✅ | Apply Attribute Modifier |
| 每秒触发属性修改 | ✅ | Apply Attribute Modifier + Period 字段 |
| 临时状态标记 | ✅ | Grant Tag + Duration |
| 堆叠型效果 | ✅ | Apply Attribute Modifier Stackable |
| 延迟效果 | ✅ | Delay 节点 |
| 播放粒子特效 | ✅（需美术资产） | Play Niagara |
| 受伤暂停热度 | ⚠️ 需程序 | Blueprint GE + OngoingTagRequirements |
| 复杂伤害公式 | ⚠️ 需程序 | C++ ExecCalc + Apply Execution |
| 物理冲量 / 弹飞 | ⚠️ 需程序 | Blueprint GA + Grant GA |

---

## 四、目标选择器

| 选择器 | 含义 | 使用场景 |
|---|---|---|
| `BuffOwner` | 符文拥有者（玩家自己） | **被动符文必须用这个** |
| `LastDamageTarget` | 上次造成伤害的目标 | 给敌人施加 Debuff |
| `DamageCauser` | 伤害来源方 | 反弹效果 |
| `BuffGiver` | 符文施加者（通常同 BuffOwner） | 特殊情况 |

> ⚠️ **常见错误：** 被动符文用了 `LastDamageTarget`，因没有目标直接 Failed。**被动符文必须用 `BuffOwner`**。

---

## 五、常见问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 被动符文没效果 | Target 填了 LastDamageTarget | 改为 BuffOwner |
| 效果不消除 | FinishBuff 节点提前终止 FA | 确认 bFinish=false |
| Period 不执行 | DurationType=Instant | 改为 Infinite 或 HasDuration |
| GrantTag 没自动消失 | Duration=0 | Duration > 0 才有倒计时 |

---

> 详细节点速查表 → [BuffFlow_NodeReference.md](BuffFlow_NodeReference.md)
> 符文制作 Step by Step → [BuffFlow_RuneWorkflow.md](BuffFlow_RuneWorkflow.md)
