# Dev01 动作规则框架使用与验收

第一版使用 Excel 或结构化文本维护动作关系，Python 转换成 JSON，Lua 执行复杂条件，C++ 接入 GAS。动作工作台负责候选校验、模拟、显式挂载和运行状态查看，不再要求逐格编辑完整矩阵。

代码开发于 `X:\Project\Dev01`。下方“本轮验证记录”是 2026-10-02 的本地验收记录，当时尚未发布 P4 或 UGS；不能把它当作当前发布凭证。2026-10-05 的发布合并以 P4 CL148 为基线，在独立副本中验证，不恢复 CL144 已回滚的 TagReaction 功能，也不覆盖最新 LookDev / Workflow 工具。正式规则仍默认关闭，不修改原有存档或规则资产。UGS 必须下载匹配的新 PCB 后才具有这些功能。

## 开始使用

1. 复制 `Tools/ActionRules/Templates/ActionRules_Template.xlsx` 作为你的作者文件。`Actions` 表登记动作身份，`Rules` 表只写有关系的动作对。模板的示例规则全部为 `FALSE`。
2. 修改需要的规则行，再用 Python 转换。在项目根目录运行：

   ```powershell
   python Tools/ActionRules/action_rules.py --input Tools/ActionRules/Templates/ActionRules_Template.xlsx --id PlayerCombat.Development --tags-file Config/Tags/CharacterTag.ini --output Content/ActionRules/player_combat.json
   ```

   替换已有输出需明确添加 `--force`。错误输入不会覆盖旧输出。完整命令与 Markdown、CSV、Python 示例见 [作者工具说明](../../Tools/ActionRules/README.md)。

3. 用本地源码引擎打开本 Git 项目进行验收：

   ```powershell
   & 'X:\Dev-BuildEngine\Engine\Binaries\Win64\UnrealEditor.exe' 'X:\Project\Dev01\DevKit.uproject'
   ```

4. 在 `Designer → 动作工作台` 打开默认“表格 / Lua 规则源”页。菜单之外也可执行控制台命令 `DevKit.ActionWorkbench.Open`。
5. 选择 JSON，点击“校验候选”，查看规则 ID、方向、版本和启用状态。校验不会挂载规则。只读模拟可输入当前状态、请求 Tag、主体和数值。
6. 确认后勾选启用，并点击“保存挂载配置”。这一步写入 `Config/DefaultGame.ini`，只对下次 PIE 或新世界生效；PIE/SIE 期间禁止保存。不自动签出 P4、保存资产或创建 CL。

文件改动使已有校验失效，保存前也会重读 JSON/Lua 并比较版本。配置同样可以在项目设置的 `Game → Action Rules` 中查看。第一版默认不开启外部规则。

## 表格和规则方向

| 表 | 列 |
| --- | --- |
| Actions | ID, DisplayName, AbilityTags, StateTags |
| Rules | ID, Enabled, Subject, Current, Request, Effect, Condition, Reason |

`Current` 是已经持有的状态，`Request` 是新请求，两列引用 Actions 的 ID。`AbilityTags` 匹配 GA 请求身份，`StateTags` 匹配角色状态；不要把显示名称当成 Tag。父标签规则会匹配子标签，同一请求不能同时匹配两个动作身份。

- `block`：Current 存在时拒绝 Request。
- `interrupt`：请求通过准入检查后，按进入动作前的快照取消 Current 的能力。
- `Condition` 留空为无附加条件；非空只填 Lua 函数名，不填代码。
- 任意匹配 block 优先于 interrupt。没有匹配规则仅表示本规则源不阻止，不代表 GAS 最终允许。
- `Enabled` 必须显式为布尔值。没有全局“强制允许”或清空其他系统锁的功能。

表格中的 Tag 必须来自项目字典，示例采用 `Character.State.Skill.Attack`、`Character.State.Movement.Dash`、`Character.State.Skill.WeaponSkill`。框架不会自动注册新 Tag，也不会把 `.1` 等自定义命名自动换成现有 `.Combo1`。

## Lua 条件接口

将 Lua 与 JSON 放在同一目录，在转换时加 `--lua-file conditions.lua`。JSON 的 `luaFile` 只能指向同目录的 Lua 文件。

```lua
return {
    functions = {
        low_stamina = function(ctx)
            return ctx.values.stamina < 20
        end,
        in_cancel_window = function(ctx)
            return ctx.owned_tags["PlayerState.AbilityCast.CanCombo"] == true
        end
    }
}
```

`ctx.owned_tags` 和 `ctx.request_tags` 为 Tag 到 true 的集合，包含父标签；`ctx.values` 为数值快照；`ctx.subject` 为 `player`、`enemy` 或其他角色的 `other`。表格 `Subject=any` 适用于所有主体。

属性不会自动从 UObject 读取。程序或蓝图通过角色 ASC 的 `SetActionRuleValue("stamina", value)` 提供快照；名称必须为 1–64 字符 ASCII 标识符，数值必须有限。`ClearActionRuleValue` 移除该输入。示例缺少 stamina 时会产生明确错误并拒绝受影响请求，不把缺失值伪装成零。

Lua 只返回 boolean，不直接激活动作、写 Tag、操作 UObject、读写存档、访问文件或网络。每次求值使用独立 VM，闭包和全局变量不跨请求保存。限制为 64 KiB UTF-8 源码、4 MiB Lua 内存、100,000 条 VM 指令；每次请求最多执行 16 个不同条件函数，相同条件在本次请求内复用结果。超限或错误拒绝请求并清除该请求的取消计划。

这是可信团队脚本的受限运行环境，不是供任意恶意代码使用的安全沙箱。JSON 最多 1 MiB、1,024 个动作、8,192 条规则；按请求动作索引规则，不按每帧全表执行。大量 AI 请求仍需性能分析。

## 运行时和锁生命周期

`FDevKitActionRuleSet` 负责解析、校验和只读求值；`UDevKitActionRuleSubsystem` 为每个游戏世界保存独立快照；`UYogAbilitySystemComponent` 接入 GAS 的激活检查与取消阶段。诊断返回规则版本、命中 ID、原因和拟取消能力标签。

取消使用新动作获得自身状态 Tag **之前**的快照，避免“新冲刺已经带 Dash Tag”改变刚才的请求结果。可取消性在请求检查时验证；不允许取消的目标会阻止新请求。规则驱动的 interrupt 仅支持实例化请求能力。同一 ASC 上，具有相同请求 AssetTags 的已授予能力不能混用 NonInstanced 和实例化策略：GAS 的 Tag 准入接口没有请求句柄，本版会拒绝该组有取消计划的请求，并指出冲突能力；需先统一为实例化能力。常规 InstancedPerActor 与 InstancedPerExecution 均有对应接入。

新规则的 block 是查询式结果，不持有持续增加的 Tag 计数。旧 StateConflict 兼容层仍使用 GAS block 计数，但会记录自身实际施加的贡献，成对释放，排队处理嵌套状态回调，并在切表时重建自身贡献而不重放取消。不会清零外部系统的计数。

旧 Movement/AI 分类仍有直接暂停和恢复副作用。有关状态活跃时禁止热换对应映射；先结束状态，再换表。这是防止丢失恢复路径的保护，不是已经把移动、AI、输入和蒙太奇都改成通用锁句柄。

开发构建可显式调用世界子系统 `ReloadRules`，无效候选保留上一份有效快照；工作台不会自动重载正在运行的世界。Shipping 禁止重载，初始规则仍从随包 UFS 文件读取。若启用了规则却没有任何有效初始快照，能力请求会失败关闭并提供加载错误，应立即修复配置或退出运行后禁用。

## 保留的边界

- 旧 StateConflict、GA/CDO、蓝图、输入和 Montage 窗口依然生效。新增动作关系应只写一份作者源；迁移旧关系时先审查重复限制，不能期待表格中的 interrupt 绕过既有锁。
- GAS 的 PreActivate 早于 CommitAbility。这里不提供“旧动作已取消后，后续费用/游戏逻辑失败自动恢复旧动作”的事务回滚。
- 激活通知中若其他原生代码再次修改目标可取消性，GAS 仍可能拒绝取消。诊断快照不是取消成功凭证；验收必须观察实际活跃能力和结果。
- 玩家与敌人的数值上下文由调用方提供；客户端预测和服务器必须使用匹配版本和确定的上下文。本次不声称完成联网预测或 Shipping 包验收。
- Markdown 支持约定的结构化双表，不直接把任意自然语言需求当作可执行代码。自由描述先整理为明确方向、条件和预期结果，再转换。

## 自动和手工验收

自动套件包括 `DevKit.ActionRules`、`DevKit.StateConflict.Lifecycle` 和既有 `DevKit.ActionWorkbench`。作者工具运行 `python -m unittest discover -s Tools/ActionRules/tests -v`。Lua 独立 smoke 入口见 [插件说明](../../Plugins/DevKitLuaRules/README.md)。实际本轮结果补充在下方，不以测试文件存在代替通过。

手工在隔离测试地图中进行，先复制项目/规则并备份存档，不使用正式通关档：

1. 工作台只读打开、选择和模拟，不改变资产脏标记或项目配置。修改 JSON/Lua 后直接保存挂载，应要求重新校验。
2. 用测试专用规则启用两个方向：冲刺挡攻击，攻击可被冲刺取消；再加入冲刺挡重复冲刺。检查真实活跃 GA，不能只看矩阵颜色。
3. 把被打断能力设为不可取消，新请求应被拒绝，原动作继续。使用会在 CanActivate 失败的能力，旧动作应保持。
4. 测试两个来源同时阻止同一动作，结束一个不提前放行，全部结束后恢复。反复切表、取消、销毁角色，不积累 block。
5. Lua 写语法错误、返回非布尔、缺少上下文、死循环和超配额条件：不应崩溃、卡死或放行；错误重载保留旧版本。
6. 两个 PIE 世界分别验证规则状态互不串扰；退出再启动读取新配置。未经确认不把测试规则写到正式资产。
7. 后续发布前补真实打包游戏启动、规则 UFS/stage、服务器预测、云端 Installed Build 编译和同事端 UGS 冷同步测试。

## 本轮验证记录

验收日期：2026-10-02。使用源码引擎 `X:\Dev-BuildEngine` 与 Git 项目 `X:\Project\Dev01`，不是 UGS Installed Build 工作区。

| 验证 | 实际结果 | 证据 |
| --- | --- | --- |
| UE Development Editor 模块编译 | 成功；最后两处私有 CPP 修正使用同轮 UBT 原始 OBJ/DLL 响应文件编译、链接成功 | `Saved/ActionRulesValidation/Build-Verified-20261002.log`、`Replay-PrivateCppFixes-20261002.log` |
| UE 三组自动化 | 35/35 通过，0 失败、0 未运行；33 项无警告，2 项含旧 StateConflict 的类别/AI 诊断日志，共 8 条 warning、0 error | `Saved/ActionRulesValidation/Automation-Final-20261002/index.json` |
| Python 作者工具 | 35/35 通过，0 跳过；涵盖 XLSX/CSV/Markdown/JSON 转换、覆盖保护与输入限制 | `outputs/action-rules-20260926/unittest-20261002.log` |
| Lua 独立 MSVC smoke | 20/20 通过，包括错误、隔离、内存/指令配额与能力限制 | `Plugins/DevKitLuaRules/Intermediate/Standalone/smoke.log` |
| 原生 Slate UI | 默认工作台和已校验候选面板各 3 张；1600×900、1024×720、1920×1080@150%，颜色断言通过并人工检查可读性与滚动 | `Saved/ActionRuleSourcesValidation/SlateCaptures/` |

首轮 UE 报告为 33 成功、2 失败：实际保护警告已出现，但测试的预期日志使用未转义方括号，被正则匹配误读。仅修正匹配文本后完整重跑，最终报告如上。不要仅以进程退出码 0 判定测试通过，必须检查报告中的 failed/notRun。

UE 测试使用 `/Engine/Maps/Entry`，不启动正式关卡；运行时夹具为无 GameInstance 的隔离世界。配置、规则资产与现有 4 个存档在最终复测后再次比较 SHA256，全部与测试前一致：

- `Config/DefaultGame.ini`：`9781E2E5E082C8143D65313E2AF77A96A525A188E9F6DE8477457D5F5ED4D6F7`。
- `DA_Base_StateConflict_Initial.uasset`：`4C3A580621243D4DCC6A4BC100FDC318B2288F137F44516AAD07E4A3271673F9`。
- `SaveGame_01.sav`、`SaveSlot_0.sav`、`SaveSlot_2.sav`、`Settings.sav` 均未改变；测试创建的规则临时目录已清理，仅保留原有 `Content/ActionRules/Examples`。

本地编译命令：

```powershell
& 'X:\Dev-BuildEngine\Engine\Build\BatchFiles\Build.bat' DevKitEditor Win64 Development '-Project=X:\Project\Dev01\DevKit.uproject' -WaitMutex -NoUBA -NoEngineChanges -UsePrecompiled -Module=DevKit -Module=DevKitEditor -Module=DevKitLuaRules -MaxParallelActions=4
```

此 UE 5.8 源码环境的 `-UsePrecompiled` SharedPCH 目录与依赖缓存分区不一致，会扩大增量重编译；本轮没有修改 UBT。限定模块构建不执行项目元数据步骤，因此在编译成功后调用官方 `WriteMetadata` 模式，使用同轮 UBT 生成且已检查输出范围的 `TargetMetadata.json`：

```powershell
& 'X:\Dev-BuildEngine\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe' 'X:\Dev-BuildEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll' -Mode=WriteMetadata '-Input=X:\Project\Dev01\Intermediate\Build\Win64\x64\DevKitEditor\Development\TargetMetadata.json' -Version=2
```

该输入的 Version 为 null，全部 manifest 输出均在项目目录；不可替换为 EngineMetadata.json，也不可编辑 BuildId 强行匹配。本轮引擎 NetCore DLL、引擎 `.modules` / `.version` 哈希未变，项目与 Lua 模块自然复用源码引擎 BuildId `4b62cf96-fa6d-4dad-a6dd-a57b299a0955`。此 ID 仅说明本地源码引擎测试组合，不是 P4/UGS 的发布 ID。

自动化启动参数（独立后台进程使用 `Start-Process -WindowStyle Hidden`）：

```text
"X:\Project\Dev01\DevKit.uproject" /Engine/Maps/Entry -unattended -nop4 -nosplash -NoSound -NoLiveCoding -ExecCmds="Automation RunTests DevKit.ActionRules+DevKit.StateConflict.Lifecycle+DevKit.ActionWorkbench" -TestExit="Automation Test Queue Empty" -ReportExportPath="X:\Project\Dev01\Saved\ActionRulesValidation\Automation-Final-20261002" -AbsLog="X:\Project\Dev01\Saved\ActionRulesValidation\Automation-Final-20261002.log" -ActionRuleSourcesCapture -windowed -ResX=1280 -ResY=720
```

截图测试必须使用真实 RHI，不使用 `-NullRHI` 或 `-RenderOffscreen`。复测请更换报告目录，保留已有验收记录。

本轮未创建或提交 P4 CL，未执行 Git commit/push、云端构建或 UGS 发布。现有旧动作资产未批量迁移；新规则配置仍默认关闭。Shipping 打包、联网预测、实际角色手工战斗与同事端冷同步仍属于后续验收，不能由上述局部自动化替代。

## 2026-10-05 发布合并验证

独立发布副本为 `X:\Dev-ProjectBuilds\Dev01-ActionRules-Release-20261005`，以 depot CL148 为基线，主 UGS 工作区的 opened 资产及未签出源码改动均不覆盖。发布范围为 94 个白名单文件；只合入 ActionRules、旧 StateConflict 贡献账本修复、Lua 插件、作者工具与工作台规则源页。

- 源码引擎完整 Development Editor 构建成功：236 个 action，461 秒；使用 `-NoEngineChanges -UsePrecompiled -NoUBA -MaxParallelActions=4`。
- UE 自动化 35/35 通过：33 无警告、2 含预期的旧 StateConflict 诊断，0 failed、0 notRun。报告：`Saved/ActionRulesRelease/Automation-Source-20261005/index.json`。
- Python 作者工具 35/35，Lua 独立 smoke 20/20；日志在 `Saved/ActionRulesRelease/`。
- 生成 6 张真实 Slate 截图；已检查默认工作台的小窗口及规则源面板的高 DPI 显示。截图在 `Saved/ActionRuleSourcesValidation/SlateCaptures/`。
- 主工作区默认规则配置、正式 StateConflict 资产、3 个现有存档，以及隔离副本的配置，6 项 SHA256 均保持不变。

以上是提交前验证，源码引擎 BuildId 为 `4b62cf96-fa6d-4dad-a6dd-a57b299a0955`，不是发布包 BuildId。正式发布必须另行通过云端 Installed Build 编译、PCB BuildId / hash 门禁、P4 archive verify 和 UGS 下载启动验证；发布 CL、PCB revision 和最终验收状态以发布日志为准。内容共享且预置 Engine 的隔离增量验收不可称为完整冷安装验收。
