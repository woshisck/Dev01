# Dev01 动作工作台：原生编辑器版

## P4 / UGS 部署入口（2026-09-20）

本文件随动作工作台代码进入 `//Dev01/main`；下文 `X:\Project\Dev01` 和源码引擎的测试是初始开发验证记录，不代表 UGS 已经更新。团队入口应在匹配本次代码 CL 的云端 PCB 发布后使用：

- 同步自己的 UGS 主工作区（本机为 `X:\Project\YogProject\Dev01`），由 UGS 下载匹配 PCB 后启动。
- 菜单 `Designer → 动作工作台`；也可从 `YogTool → 开发工作台 → 角色、战斗与玩法 → 动作 Tag 与阻断` 打开同一个停靠页。
- 工作流入口保留独立标签页及未保存草稿关闭保护，不创建第二个编辑实例。
- 不复制源码引擎版本 DLL，不手动改 BuildId，不点击 Missing Modules 的 Yes。
- 最终代码 CL / PCB revision / BuildId / 云端及客户端验收应以本次发布报告为准；本地开发的 10 项通过不能替代云端构建。

## 打开与使用

本版位于项目 `DevKitEditor` 模块，是真正的 UE Slate 停靠页，不是嵌入网页，也不需要修改引擎源码。

- 项目：`X:\Project\Dev01\DevKit.uproject`
- 本轮编译引擎：`X:\Dev-BuildEngine`
- 菜单：`Designer → 动作工作台`
- 控制台：`DevKit.ActionWorkbench.Open`
- 默认来源：`DefaultGame.ini` 中 `DevAssetManager.StateConflictData` 指定的真实资产。打开只复制出临时草稿，不保存源资产。

推荐流程：**选择规则来源 → 选择矩阵格 → 修改草稿 → 校验 → 应用到资产 → 保存资产 → 在隔离环境重新进入 PIE 验证**。

测试时先复制规则资产为测试副本，并在顶部选中副本；不要直接改默认全局表。没有经过验证的存档隔离宿主时，不进入正式游戏 PIE。

## 四个页面

| 页面 | 已实现 |
| --- | --- |
| 衔接矩阵 | 行=新请求、列=当前状态。查看和独立编辑阻止激活、状态进入后取消；保留不对称规则。 |
| 状态与资源 | 在独立草稿上使用 UE 原生 Tag 选择器编辑 Rules 与 BlockCategoryMap；移动/AI 分类沿用现有运行时。 |
| 运行诊断 | 选择 PIE 中带 ASC 的 Actor，读取状态、真实 blocked ability tags、活跃能力及实际规则表路径，可复制报告。 |
| 校验与范围 | 列出无效/重复规则、未使用的 Priority、来源冲突等；明确展示尚未接通的能力。 |

左侧是既有状态/目标标签目录，核心动作使用中文名称。现有普攻动画槽位仍为 `Character.State.Skill.Attack.Combo1–4`，本版不重命名 Tag、不自动注册 `.Attack.1`。

## 必须理解的规则语义

- `ActiveTag` 精确触发。目标 `BlockTags / CancelTags` 按 GAS 标签层级匹配。
- “阻止激活”来自**当前状态**的 BlockTags；“进入后取消”来自**新状态**的 CancelTags。
- 一个格子可同时存在两者；这不是自动中断事务。请求若尚未通过准入，不能指望自己尚未出现的状态先取消别人。
- “本表未限制”不是“允许”。GA/蓝图、额外 AbilityTags、成本、冷却、蒙太奇窗口、死亡和对话等仍有独立约束。
- 父目标影响多个子动作时，工作台拒绝用单个子格静默删除共享父目标；需在高级数据中核对后显式调整。
- `StateConflict.Priority` 当前不参与运行时决策。本版不会假装该数值能决定动作优先级。

## 数据安全与协作

草稿是 transient 副本。Apply 使用 UE 撤销事务，只改当前源资产的 Rules/BlockCategoryMap；Save 是单独的确认动作。已应用未保存期间暂停草稿编辑，先完成保存或撤销，避免混合两个版本。UE 的正式 Save All/外部资产保存成功事件也会确认本工作台的待保存版本；Autosave 不算正式保存。

源数据、规则顺序、映射或磁盘摘要变化时拒绝覆盖。只读文件和载入时已包含其他未保存改动的源包也会拒绝 Apply。外部 P4 同步改变磁盘后，需要在内容浏览器真正 Reload 或重启，不能只重新创建草稿绕过检查。

PIE/Simulate 及其世界上下文过渡期间禁止修改、应用和保存；诊断不调用 CanActivateAbility、激活、取消、清锁或 SetConflictTable。它只是当前快照，不是完整 owner/历史锁账本。

工具不提交 P4/Git、不发布 PCB；签出/占用按项目现有源控制流程处理。未保存草稿关闭/切源前提示；导出 JSON 位于 `Saved/ActionWorkbench/Exports`，仅用于评审，不提供自动导入。

## 本版边界

本版落实既有 StateConflict 的可视化管理和只读诊断。此前 HTML 是更完整的设计方向，以下仍未实现：新动作向导、预设继承、动画时间轴、输入排队、共存/窗口化中断、原子取消/激活、Owner/Token 残锁治理和表格批量导入。没有默认改变冲刺规则，也没有宣称修复所有旧入口的残锁。

## 验证与发布

完整测试步骤、停止条件与实际结果见 [Dev01_ActionWorkbench_TestPlan.md](Dev01_ActionWorkbench_TestPlan.md)。自动化前缀为 `DevKit.ActionWorkbench`。

本地通过不等于 UGS 已有此工具。要让同事使用，需另行核对 P4 opened/差异白名单，再走项目代码同步、云端项目编译、匹配 Installed Build 的 PCB 和同事端验收；本次没有重发引擎或提交版本控制。
