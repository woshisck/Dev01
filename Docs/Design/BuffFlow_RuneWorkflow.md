# BuffFlow 符文制作流程

> 版本：Sprint 4.15（2026-04-08）
> 上级文档：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)
> 详细案例：[TestRune_CreationGuide.md](TestRune_CreationGuide.md)

---

## 制作前：明确设计意图

| 问题 | 常见选项 |
|---|---|
| 触发时机？ | 被动常驻 / 命中时 / 击杀时 / 受伤时 |
| 效果目标？ | 自己（BuffOwner）/ 敌人（LastDamageTarget） |
| 持续多久？ | 永久 / 有时限 / 立即一次 |
| 需要叠加？ | 不叠 / 刷新时间 / 叠层 |
| 需要每秒触发？ | 是 → Period 字段；否 → 不填 |
| 需要额外资产？ | 简单属性 → 不需要；物理冲量/复杂公式 → 找程序 |

---

## Step 1：选择正确节点

```
效果是修改属性？
  ├─ 固定数值               →  Apply Attribute Modifier（无需 GE 资产）
  ├─ 每秒 +N               →  Apply Attribute Modifier + Period 字段
  ├─ 复杂公式               →  找程序写 C++ ExecCalc + Apply Execution
  └─ 特殊 GE 参数           →  找程序配合 Blueprint GE + Apply Effect

效果是状态标记？
  ├─ 永久（符文卸下才消失）  →  Add Tag
  └─ 有倒计时              →  Grant Tag + Duration

效果需要能力执行？
  └─  找程序写 Blueprint GA + Grant GA 节点
```

---

## Step 2：创建 Flow Asset（FA）

1. Content Browser → 右键 → **Flow → Flow Asset**
2. 路径：`Content/Game/Runes/<符文名>/`
3. 命名：`FA_Rune_<功能名>`（例：`FA_Rune_AttackUp`）
4. 双击打开，在 FA 图里连线，**Start 节点** 为入口

---

## Step 3：创建 RuneDataAsset（DA）

1. Content Browser → 右键 → **Miscellaneous → Data Asset → RuneDataAsset**
2. 路径：`Content/Game/Runes/<符文名>/`
3. 命名：`DA_Rune_<功能名>`（例：`DA_Rune_AttackUp`）

**必填字段：**

| 字段 | 说明 |
|---|---|
| `RuneConfig.RuneName` | 符文名称 |
| `RuneConfig.RuneID` | 策划表 ID |
| `RuneConfig.RuneType` | Buff / Debuff / None |
| `Shape.Cells` | 至少填 `(0,0)` = 1格 |
| `Flow.FlowAsset` | 拖入 FA |

---

## Step 4：测试

1. 将 DA 填入角色的 `DebugTestRunes` 数组（或 `PermanentRunes`）
2. 运行游戏，打开 **GAS Debugger**（按 `'` 键）

**验证要点：**

| 验证项 | 在哪看 |
|---|---|
| 属性变化是否正确 | GAS Debugger → Attributes |
| Tag 是否正确授予 | GAS Debugger → Granted Tags |
| 激活 GE 列表 | GAS Debugger → Active Effects |
| 移出激活区后效果是否清理 | 拖出格子后再看 Debugger |

---

## 资产目录结构参考

```
Content/Game/Runes/
├── AttackUp/
│   ├── DA_Rune_AttackUp
│   └── FA_Rune_AttackUp
├── SpeedStack/
│   ├── DA_Rune_SpeedStack
│   └── FA_Rune_SpeedStack
├── HeatUp/
│   ├── DA_Rune_HeatUp
│   ├── FA_Rune_HeatUp
│   └── GE_HeatTick          ← 仅此符文需要 Blueprint GE
└── Knockback/
    ├── DA_Rune_Knockback
    ├── FA_Rune_Knockback
    ├── GE_KnockbackStagger   ← SetByCaller，必须用 Blueprint GE
    └── GA_Knockback          ← 物理冲量，必须用 Blueprint GA
```
