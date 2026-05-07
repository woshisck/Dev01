# DataEditor 数值编辑器使用与自动化测试

> 配套实现：[UDataEditorLibrary](../../Source/DevKitEditor/Public/Tools/DataEditorLibrary.h) / [UDataValidator](../../Source/DevKitEditor/Public/Tools/DataValidator.h) / [FRuneDataAssetDetails](../../Source/DevKitEditor/Private/Customization/RuneDataAssetDetails.h)

日常调数值优先在 UE 编辑器顶部菜单打开 `Tools > DataEditor`。这个入口是 C++ Slate 面板，不依赖 `EUW_RuneBalanceEditor.uasset` 蓝图资产。

本目录下的 PowerShell + UE Python 脚本继续保留，用于自动化 smoke test、headless 迁移和批量运维。

---

## 0. 日常打开方式（推荐）

1. 编译 `DevKitEditor`。
2. 启动 `DevKit.uproject`。
3. 在 UE 顶部菜单点击 `Tools > DataEditor`。

面板能力：

- Rune 主表显示 `Asset`、`RuneIdTag`、`LegacyRuneID`、`RuneName`、`Rarity`、`TriggerType`、`GoldCost`、`SellPrice`、`ChainRole`。
- 顶部按钮：`Refresh`、`Validate All`、`Export CSV`、`Migration Prepare`、`Migration Apply`。
- 行按钮：`Open` 打开对应 RuneDA，`Copy Tag` 复制当前 `RuneIdTag`。
- 选中多行后可以批量设置 `GoldCost` 或 `Rarity`。
- 编辑器面板内的批量修改只标记资产 dirty，不自动保存；可用 `Ctrl+Z` 撤销。

当前 `EffectDA count = 0` 是资产现状，面板只显示统计和空状态；后续有 EffectDA 后再扩展 Effect 表。

---

## 脚本前置依赖

- UE5.4，工程已启用 `PythonScriptPlugin` 与 `EditorScriptingUtilities`。
- PowerShell 5.1 或 7+。
- 默认编辑器路径：`D:/UE/UE_5.4/Engine/Binaries/Win64/UnrealEditor-Cmd.exe`。如路径不同，修改 [_common.ps1](_common.ps1) 的 `$UEEditorCmd`。

默认情况下，脚本发现本机已有 `UnrealEditor*` 进程时会直接失败，避免误关你正在编辑的窗口或覆盖未保存资产。确认可以关闭编辑器时，给 wrapper 加 `-CloseEditor`。

下文示例使用 `pwsh`。如果本机未安装 PowerShell 7，可用 Windows PowerShell：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\DataEditor\run_smoke_test.ps1
```

---

## 1. 自动化冒烟测试（只读）

```powershell
pwsh Tools/DataEditor/run_smoke_test.ps1
```

如果你确认可以自动关闭当前编辑器：

```powershell
pwsh Tools/DataEditor/run_smoke_test.ps1 -CloseEditor
```

执行内容：

- 收集所有 `RuneDA` / `EffectDA`。
- 调 `UDataValidator::ValidateAll()`。
- 调 `UDataEditorLibrary::VerifyAccessorParity()`。
- 导出 `RuneNumbers_*.csv` 和 `EffectNumbers_*.csv` 到 `Saved/Balance/`。
- 生成 `Saved/Balance/SmokeTest_<时间戳>/report.txt`。

通过标准：

- `Errors: 0`。
- `Accessor/field diff count: 0`。
- CSV 文件路径非空。
- PowerShell 退出码为 `0`。

当前基线：2026-05-07 的 smoke test 结果为 `RuneDA count: 73`、`EffectDA count: 0`、`Errors: 0`、`Warnings: 115`。`EffectDA count = 0` 是当前资产现状，不作为失败。

---

## 2. RuneID -> RuneIdTag 迁移（会修改并保存资产）

迁移分两步。第 1 步只写 `Config/Tags/RuneIDs.ini`，不改 DA：

```powershell
pwsh Tools/DataEditor/run_migration.ps1 -Phase prepare
```

第 1 步会把 `RuneID > 0` 且 `RuneIdTag` 为空的 RuneDA 生成临时 Tag：

```ini
GameplayTagList=(Tag="Rune.ID.Legacy_1001",DevComment="Auto-migrated from legacy RuneID=1001 ...")
```

然后必须重启编辑器或重新跑一个 headless 编辑器进程，让 `GameplayTagsManager` 在启动时加载新 Tag。

第 2 步会把已注册的 Tag 写回 RuneDA，并在 headless 进程退出前保存修改过的 RuneDA：

```powershell
pwsh Tools/DataEditor/run_migration.ps1 -Phase apply
```

回滚方式：因为 headless 模式会保存资产，不能依赖编辑器内 `Ctrl+Z`。如迁移结果不对，用 source control 或 `git restore <uasset路径>` 回退。

验证迁移：

```powershell
pwsh Tools/DataEditor/run_smoke_test.ps1
```

期望 `RuneIdTag 未配置` 类 Warning 数下降；如 `apply` 显示 skipped，通常是没有重启导致新 Tag 未加载。

---

## 3. Headless 批量操作（会修改并保存资产）

```powershell
# 所有 RuneDA 的 GoldCost 设为 99（C++ clamp 到 >= 0）
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_gold -Value 99

# 所有 RuneDA 的 Rarity 设为 Rare（Common / Rare / Epic / Legendary）
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_rune_rarity -Value Rare

# 所有 EffectDA 的 Duration 设为 5.0（C++ clamp 到 >= 0.01）
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_effect_duration -Value 5.0

# 所有 EffectDA 的 MaxStack 设为 3（C++ clamp 到 >= 1）
pwsh Tools/DataEditor/batch_ops.ps1 -Op set_effect_maxstack -Value 3
```

注意：

- 当前基线里 `EffectDA count = 0`，Effect 批量操作可能没有目标资产。
- 策划日常只改选中 Rune 时，优先用 `Tools > DataEditor` 面板；下面这些脚本是全量 headless 运维入口。
- Headless 批量操作会保存修改过的资产。
- 编辑器面板内可用 `Ctrl+Z` 撤销；headless 保存后请用 source control 回滚。

---

## 编辑器内最少回归

| # | 验证 | 步骤 | 期望 |
|---|------|------|------|
| 1 | 编辑器启动 | 双击 `DevKit.uproject` | 无 GameplayTag 解析错误 |
| 2 | DataEditor 菜单 | 点击 `Tools > DataEditor` | 打开 `DataEditor` 窗口，Rune 列表约 73 条 |
| 3 | 批量修改撤销 | 选中 1-2 条 Rune，设置 `GoldCost` 或 `Rarity`，再 `Ctrl+Z` | Detail 面板能看到 dirty 变化，撤销后恢复 |
| 4 | 行操作 | 点 `Open` / `Copy Tag` | 能打开 RuneDA，剪贴板得到当前 `RuneIdTag` |
| 5 | 自定义 Detail Panel | 双击任意 `DA_Rune_*` | 顶部出现 **Rune Tools**，含 `Copy RuneIdTag` / `Validate This` |
| 6 | Smoke report | 打开最新 `Saved/Balance/SmokeTest_*/report.txt` | `Overall` 为 `PASS` |
| 7 | 游戏回归 | PIE 玩 1 关，买符文、看背包、触发符文、看传送门浮窗 | 与修改前一致 |
| 8 | 迁移结果 | 如跑过 migration，打开被迁移的 RuneDA | `RuneIdTag` 有 `Rune.ID.Legacy_*` 或正式 Tag |

---

## 常见问题

| 问题 | 处理 |
|------|------|
| `Editor not found: D:/UE/UE_5.4/...` | 修改 `_common.ps1` 的 `$UEEditorCmd` |
| `UnrealEditor is already running` | 手动关闭编辑器，或确认安全后加 `-CloseEditor` |
| `unreal.DataEditorLibrary` AttributeError | 先重新编译 `DevKitEditor`，确保最新 DLL 已加载 |
| smoke test 报 `Errors > 0` | 打开 `report.txt` 和 `Saved/Logs/DevKit.log`，按 `[Rune]` / `[Effect]` 行修 DA |
| `apply` 后仍 skipped | 先跑 `prepare`，再重启编辑器/headless 进程，再跑 `apply` |
| 批量操作改错 | 用 source control 或 `git restore` 回滚保存后的 `.uasset` |

---

## 维护入口

- 新增批量操作：改 [batch_ops.py](batch_ops.py) 的 `OPS` 字典，并在 [batch_ops.ps1](batch_ops.ps1) 加 `ValidateSet`。
- 新增校验规则：改 [DataValidator.cpp](../../Source/DevKitEditor/Private/Tools/DataValidator.cpp)，重新编译后跑 smoke test。
- 调整 UE 路径或编辑器进程策略：改 [_common.ps1](_common.ps1)。
# DataEditor Multi-Panel Update (2026-05)

Daily tuning should now start from the UE top menu:

- `Tools > DevKit Data > Character Balance`: character base attributes and movement values.
- `Tools > DevKit Data > Action Balance`: melee Montage/AttackData windows and musket Light/Heavy/Sprint values.
- `Tools > DevKit Data > Rune Balance`: rune economy, rarity, trigger type, FlowAsset, and Tuning Scalars.

The old `Tools > DataEditor` entry is kept and opens `Rune Balance`. Script wrappers remain for smoke tests, headless migration, and batch maintenance. Editor-panel edits mark assets or DataTables dirty, do not auto-save, and support `Ctrl+Z`.
