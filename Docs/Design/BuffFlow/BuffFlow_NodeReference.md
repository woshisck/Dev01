# BuffFlow 节点速查表

> 版本：Sprint 4.15（2026-04-08）
> 上级文档：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)

---

## 触发节点（FA 起点，监听游戏事件）

| FA 显示名 | 触发时机 | 核心配置 | 常见用途 |
|---|---|---|---|
| **On Damage Dealt** | 造成伤害时 | `Once Per Swing`：同帧多目标只算一次 | 命中叠加、命中流血 |
| **On Damage Received** | 受到伤害时 | — | 受伤暂停热度、受伤防盾 |
| **On Crit Hit** | 触发暴击时 | — | 暴击专属增益 |
| **On Kill** | 击杀目标时 | — | 击杀回血、击杀叠加 |
| **On Dash** | 闪避/冲刺时 | — | 闪避加速、无敌帧特效 |
| **On Buff Added** | 任意符文 FA 启动时 | — | 符文联动 |
| **On Buff Removed** | 任意符文 FA 停止时 | — | 符文消失清理 |
| **On Phase Up Ready** | 热度满 + 可升阶时 | — | 升阶流程入口 |
| **On Heat Reached Zero** | 热度从 >0 变为 0 | — | 降阶处理 |
| **On Periodic** | 每隔 N 秒重复触发 | `Interval`（秒）/ `Fire Immediately` | 定时循环效果 |

> 所有触发节点都有 **In**（开始监听）和 **Stop**（停止监听）输入引脚。

---

## 条件 / 数据节点

| FA 显示名 | 功能 | 输出引脚 |
|---|---|---|
| **Has Tag** | 检查目标是否有某个 Tag | Yes / No |
| **Compare Float** | 比较两个浮点数 | True / False |
| **Compare Int** | 比较两个整数 | True / False |
| **Check Target Type** | 判断上次伤害目标的身份 | 对敌人 / 对自己 |
| **Get Attribute** | 读取目标属性当前值 | Out + `CachedValue`（Float 数据引脚） |
| **Get Rune Info** | 查询 GE 运行状态（层数/剩余时间等） | Found / Not Found + 数据引脚 |

**Get Rune Info 数据输出引脚：**

| 引脚 | 类型 | 说明 |
|---|---|---|
| `bIsActive` | Bool | GE 是否活跃 |
| `StackCount` | Int | 当前叠加层数 |
| `Level` | Float | GE 等级 |
| `TimeRemaining` | Float | 剩余秒数，-1 = 永久 |

---

## 效果节点

| FA 显示名 | 一句话功能 | 需要额外资产？ |
|---|---|---|
| **Apply Attribute Modifier** | 直接修改属性，数值在节点上配置，无需 GE 资产 | ❌ |
| **Apply Gameplay Effect Class** | 施加一个 Blueprint GE 类，支持 SetByCaller 传值 | ✅ Blueprint GE |
| **Apply Execution Calculation** | 执行 C++ ExecCalc，适合复杂伤害公式 | ✅ 程序写 C++ |
| **Grant GA** | 授予目标一个 GameplayAbility | ✅ Blueprint GA |
| **Add Tag** | 添加 Loose Tag，FA 停止时自动移除 | ❌ |
| **Grant Tag (Timed)** | 添加 Loose Tag，支持 N 秒后自动过期 | ❌ |
| **Remove Tag** | 立即移除 Tag | ❌ |
| **Do Damage** | 对目标造成伤害（固定值或基于上次伤害的倍率） | ✅ Blueprint GE（伤害 GE） |
| **Play Niagara** | 播放 Niagara 粒子特效 | ✅ Niagara 资产 |
| **Destroy Niagara** | 按名称销毁之前注册的粒子 | — |
| **Play Montage** | 在目标角色播放动画蒙太奇 | ✅ 动画资产 |
| **Spawn Actor At Location** | 在击杀点或指定位置生成 Actor | ✅ Actor 类 |
| **Finish Buff** | 立即终止整个 FA（触发 Cleanup） | — |

---

## Phase（热度阶段）节点

| FA 显示名 | 功能 | 说明 |
|---|---|---|
| **Increment Phase** | 热度阶段 +1（最高 3） | 配合 On Phase Up Ready 使用 |
| **Decrement Phase** | 热度阶段 -1（最低 0） | 配合 On Heat Reached Zero 使用 |
| **Phase Decay Timer** | 等待 N 秒后触发 Out，可提前 Cancel | 用于升阶前的倒计时窗口 |

---

## 工具节点

| FA 显示名 | 功能 | 备注 |
|---|---|---|
| **Delay** | 等待 N 秒后继续，支持 Cancel 提前取消 | Duration 支持数据引脚 |
| **Literal Float** | 输出固定浮点值（纯数据，无执行引脚） | 等价于蓝图 Make Literal Float |
| **Literal Int** | 输出固定整数 | 同上 |
| **Literal Bool** | 输出固定布尔值 | 同上 |
| **Math Float** | 浮点运算（+−×÷），Result 可连向下游 | — |
| **Math Int** | 整数运算 | — |

---

## Apply Attribute Modifier 字段详解

| 字段 | 说明 | 示例 |
|---|---|---|
| Attribute | 目标属性 | `BaseAttributeSet.Attack` / `BaseAttributeSet.MoveSpeed` |
| Mod Op | 运算方式 | `Additive`（加）/ `Multiplicative`（乘）/ `Override`（覆盖） |
| Value | 修改量（支持数据引脚连线） | `10.0` |
| Duration Type | 持续方式 | `Infinite`（持续到 FA 停止）/ `Has Duration`（时限）/ `Instant`（瞬发永久） |
| Duration | 持续秒数（Has Duration 时生效） | `3.0` |
| Period (0=Off) | 每隔 N 秒执行一次（0=禁用） | `1.0` = 每秒触发 |
| Fire Immediately | Period>0 时，是否在施加瞬间立即执行一次 | 勾=立即触发；不勾=等第一个间隔 |
| Target | 施加目标 | `BuffOwner` / `LastDamageTarget` |
| Stack Mode | 堆叠方式 | `None` / `Unique`（刷新时间）/ `Stackable`（叠层） |
| Stack Limit Count | 最大叠层数（Stackable 时生效） | `5` |

---

## Grant Tag (Timed) 引脚说明

| 引脚类型 | 引脚名 | 说明 |
|---|---|---|
| 输入 | In | 授予 Tag，启动倒计时（已授予时重置） |
| 输入 | Remove | 手动立即移除 |
| 输出 | Out | 授予成功后立即触发 |
| 输出 | Expired | 倒计时到期、Tag 自动移除后触发 |
| 输出 | Removed | 手动 Remove 后触发 |
| 输出 | Failed | 目标无效或无 ASC |

---

## 两个"施加效果"节点对比

| 对比项 | Apply Attribute Modifier | Apply Gameplay Effect Class |
|---|---|---|
| 需要 GE 资产 | ❌ 不需要 | ✅ 需要 Blueprint GE |
| 配置位置 | 节点上直接填 | GE 蓝图里配置 |
| GAS Debugger 可见 | ❌ 不显示（transient GE） | ✅ 显示 GE 名 |
| SetByCaller | ❌ 不支持 | ✅ 最多 3 个槽 |
| 适合场景 | 简单属性加减，快速调试 | 需要调试可视化、复杂 GE 逻辑 |

---

## 设计模式速查

| 模式 | 名称 | 节点连线思路 |
|---|---|---|
| A | 被动属性加成 | Start → **Apply Attribute Modifier**（Infinite, BuffOwner） |
| B | 条件触发 | **On Kill** → **Has Tag** → Yes/No 两支效果 |
| C | 命中叠加 | **On Damage Dealt** → **Apply Attribute Modifier**（Stackable, Has Duration） |
| D | 每秒 +N | **Apply Attribute Modifier**（Period=1.0, Infinite） |
| E | 受伤暂停热度 | **On Damage Received** → **Grant Tag (Timed)** HeatInhibit, 5s |
| F | 升阶流程 | **On Phase Up Ready** → **Phase Decay Timer** → **Increment Phase** |
| G | 降阶流程 | **On Heat Reached Zero** → **Decrement Phase** |
| H | 一次性消耗 Buff | **On Damage Dealt** → **Apply Attribute Modifier**.Remove |
| I | N 次后爆发 | **On Kill** → **Counter**（Goal=5）→ 爆发效果 |
| J | 随机效果 | **On Kill** → **Multi Gate**（Random）→ 多效果分支 |
| K | 延迟效果 | **On Kill** → **Delay**（2s）→ 效果 |
| L | 动态数值 | **Get Attribute**.CachedValue →（数据线）→ **Apply Attribute Modifier**.Value |
