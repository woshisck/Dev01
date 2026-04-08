# BuffFlow 节点速查表

> 版本：Sprint 4.15（2026-04-08）
> 上级文档：[BuffFlow_DesignGuide.md](BuffFlow_DesignGuide.md)

---

## 触发节点（FA 起点，监听游戏事件）

| FA 显示名 | 触发时机 | 核心配置 | 常见用途 |
|---|---|---|---|
| **On Damage Dealt** | 造成伤害时 | `Once Per Swing`：同一帧攻击多人只算一次 | 命中叠加、命中流血 |
| **On Damage Received** | 受到伤害时 | — | 受伤暂停热度、受伤防盾 |
| **On Crit Hit** | 触发暴击时 | — | 暴击专属增益 |
| **On Kill** | 击杀目标时 | — | 击杀回血、击杀叠加 |
| **On Dash** | 闪避/冲刺时 | — | 闪避加速、无敌帧特效 |
| **On Buff Added** | 任意符文 FA 启动时 | — | 符文联动 |
| **On Buff Removed** | 任意符文 FA 停止时 | — | 符文消失清理 |
| **On Phase Up Ready** | 热度满 + 可升阶时 | — | 升阶流程入口 |
| **On Heat Reached Zero** | 热度从 >0 变为 0 | — | 降阶处理 |
| **On Periodic** | 每隔 N 秒重复触发 | `Interval`（秒）/ `Fire Immediately` | 定时循环效果 |

---

## 条件 / 数据节点（查询状态，控制分支）

| FA 显示名 | 功能 | 输出引脚 |
|---|---|---|
| **Has Tag** | 检查目标是否带有某个 Gameplay Tag | True / False |
| **Compare Float** | 比较两个浮点数（A 可连数据引脚，B 填阈值） | True / False |
| **Compare Int** | 比较整数 | True / False |
| **Check Target Type** | 判断上次伤害目标的身份 | 对敌人 / 对自己 |
| **Get Attribute** | 读取目标属性当前值，输出到 `CachedValue` | Out + CachedValue（Float） |
| **Get Rune Info** | 查询目标 ASC 上的 GE 运行状态 | Found / Not Found + StackCount / TimeRemaining |

**Get Rune Info 数据输出引脚：**

| 引脚 | 类型 | 说明 |
|---|---|---|
| `bIsActive` | Bool | GE 是否活跃 |
| `StackCount` | Int | 当前叠加层数 |
| `Level` | Float | GE 等级 |
| `TimeRemaining` | Float | 剩余秒数，-1 = 永久 |

---

## 效果节点（执行实际动作）

| FA 显示名 | 一句话功能 | 需要额外资产？ |
|---|---|---|
| **Apply Attribute Modifier** | 直接修改属性，无需 GE 资产 | ❌ |
| **Apply Effect** | 施加一个 Blueprint GE 类 | ✅ Blueprint GE |
| **Apply Execution** | 执行 C++ 计算公式（ExecCalc），无需 GE 资产 | ✅ 程序写 C++ ExecCalc |
| **Grant GA** | 授予目标一个 GameplayAbility | ✅ Blueprint GA |
| **Add Tag** | 永久添加 Loose Tag（FA 停止时自动移除） | ❌ |
| **Grant Tag** | 临时添加 Loose Tag，可设倒计时自动过期 | ❌ |
| **Remove Tag** | 移除 Tag | ❌ |
| **Do Damage** | 直接造成伤害（固定值或倍率） | ❌ |
| **Play Niagara** | 播放 Niagara 粒子特效 | ✅ Niagara 资产 |
| **Destroy Niagara** | 按名称销毁之前注册的粒子 | — |
| **Play Montage** | 在目标角色播放动画蒙太奇 | ✅ 动画资产 |
| **Spawn Actor At Location** | 在击杀点或指定位置生成 Actor | ✅ Actor 类 |
| **Finish Buff** | 终止整个 FA（触发 Cleanup） | — |

---

## 工具节点（辅助）

| FA 显示名 | 功能 | 备注 |
|---|---|---|
| **Delay** | 等待 N 秒后继续，支持 Cancel 提前取消 | Duration 支持数据引脚动态输入 |
| **Literal Float** | 输出固定浮点数值（纯数据，不接流程线） | 等价于蓝图 Make Literal Float |
| **Literal Int** | 输出固定整数 | 同上 |
| **Literal Bool** | 输出固定布尔值 | 同上 |
| **Math Float** | 浮点运算（+−×÷），Result 可连向下游 | — |
| **Math Int** | 整数运算 | — |

---

## FlowGraph 内置节点

| FA 显示名 | 功能 | 用途举例 |
|---|---|---|
| **Execution Sequence** | 依次触发多个输出 | 升阶：加属性 → 播特效 → 显示 UI |
| **Multi Gate** | 顺序或随机触发多个输出 | 击杀随机加攻/防/速 |
| **Logical AND** | 所有输入都触发才输出 | 热度≥50 AND 连击末帧同时满足 |
| **Logical OR** | 任一输入触发即输出 | 击杀 OR 暴击 触发同一效果 |
| **Timer** | 周期计时 + 完成信号 | 通用倒计时 |
| **Counter** | 计数到目标值后触发 | 击杀 5 次触发爆发 |
| **Sub Graph** | 引用另一个 FA 作为子逻辑 | 通用加成模块复用 |
| **Branch** | 条件分支（配合 AddOn 谓词） | — |
| **Log** | 打印调试信息到屏幕 / 日志 | 开发期排查执行路径 |

---

## Apply Attribute Modifier 字段详解

| 字段 | 说明 | 示例 |
|---|---|---|
| Attribute | 目标属性 | `AttackDamage` / `MoveSpeed` / `Heat` |
| Mod Op | 修改方式 | `Additive`（加）/ `Multiplicative`（乘）/ `Override`（覆盖） |
| Value | 修改量（支持数据引脚连线） | `10.0` / `-300.0` |
| Duration Type | 持续方式 | `Infinite`（永久）/ `Has Duration`（有时限）/ `Instant`（瞬发） |
| Duration | 持续秒数 | `3.0`，Has Duration 时生效 |
| Period (0=Off) | 每隔 N 秒执行一次 | `1.0` = 每秒触发；`0` = 不启用 |
| Fire Immediately | Period>0 时，施加瞬间是否立即执行一次 | 勾上=立即；不勾=等第一个 Period |
| Target | 施加给谁 | 见目标选择器 |
| Stack Mode | 堆叠方式 | `None` / `Unique`（刷新时间）/ `Stackable`（叠层） |
| Stack Limit Count | 最大层数（Stackable 时生效） | `5` |

---

## Grant Tag 引脚说明

| 类型 | 引脚名 | 触发时机 |
|---|---|---|
| 输入 | In | 授予 Tag（已授予时重置倒计时） |
| 输入 | Remove | 手动立即移除 |
| 输出 | Out | 授予成功后立即触发 |
| 输出 | Expired | 倒计时到期自动触发 |
| 输出 | Removed | 手动 Remove 后触发 |
| 输出 | Failed | 找不到目标 |

---

## 设计模式速查

| 模式 | 名称 | 节点连线思路 |
|---|---|---|
| A | 被动属性加成 | Start → **Apply Attribute Modifier**（Infinite, BuffOwner） |
| B | 条件触发 | **On Kill** → **Has Tag** → True/False 两支效果 |
| C | 命中叠加 | **On Damage Dealt** → **Apply Attribute Modifier**（Stackable, Has Duration） |
| D | 每秒 +N | **Apply Attribute Modifier**（Period=1.0, Infinite） |
| E | 受伤暂停 | **On Damage Received** → **Grant Tag** HeatInhibit, 5s |
| G | 递归守卫 | **On Damage Dealt** → **Has Tag** 守卫 → False: **Add Tag** → 效果 → **Remove Tag** |
| H | N 次后爆发 | **On Kill** → **Counter**（Goal=5）→ 爆发效果 |
| I | 随机效果 | **On Kill** → **Multi Gate**（Random）→ 多效果分支 |
| J | 多条件同步 | **On Kill** + **On Crit Hit** → **Logical AND** → 大额加成 |
| K | 子图复用 | 封装为子 FA，多处用 **Sub Graph** 引用 |
| L | 延迟效果 | **On Kill** → **Delay**（2s）→ 效果 |
| M | 字面量数据引脚 | **Literal Float**（5.0）→（数据线）→ **Apply Attribute Modifier**.Value |
