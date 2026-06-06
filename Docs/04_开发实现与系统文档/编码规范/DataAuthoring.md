# 数据编辑规范（DataAuthoring）

> P0 落地版本：2026-05-07  
> 配套实现：[`UDataEditorLibrary`](../../../Source/DevKitEditor/Public/Tools/DataEditorLibrary.h) / [`UDataValidator`](../../../Source/DevKitEditor/Public/Tools/DataValidator.h) / [`FRuneDataAssetDetails`](../../../Source/DevKitEditor/Private/Customization/RuneDataAssetDetails.h)

---

## 总原则

**DA 是主数据载体**，业务代码通过**统一访问器**读取，编辑体验靠**自定义 Detail Panel + EUW 批量编辑器**提升。

不要把数值搬到 DataTable / CSV，除非该数据天然是表格形态（比如角色基础属性表 `FYogBaseAttributeData`，已有 DT 的保留）。

---

## 业务代码读取规则

| ❌ 禁止 | ✅ 推荐 |
|--------|--------|
| `DA->RuneInfo.RuneConfig.GoldCost` | `DA->GetGoldCost()` |
| `DA->RuneInfo.RuneConfig.RuneName` | `DA->GetRuneName()` |
| `DA->RuneInfo.RuneConfig.RuneID` | `DA->GetRuneIdTag()`（迁移期可用 `GetLegacyRuneID()`） |
| `EffectDA->Duration` | `EffectDA->GetDuration()` |
| `data->GetBaseAttributeData()->MaxHealth` | `data->GetMaxHealth()` |

**唯一例外**：在单个函数内拿 `const FRuneConfig& Cfg = DA->RuneInfo.RuneConfig;` 引用后多次访问 `Cfg.X`，是 C++ 常见的"local alias"模式，不算绕过封装，可保留。

为什么要走访问器：
- 字段重命名/重构时只改一处
- 编译期可拦截下游错误
- 后续接 CurveTable / 缓存层时，业务代码无感

---

## RuneIdTag 命名空间

替代旧的 `int32 RuneID`，使用 `Rune.ID.*`：

```ini
GameplayTagList=(Tag="Rune.ID.BurningEdge",DevComment="燃烧之刃")
GameplayTagList=(Tag="Rune.ID.IronWill",DevComment="钢铁意志")
```

文件位置：[`Config/Tags/RuneIDs.ini`](../../../Config/Tags/RuneIDs.ini)

迁移工具分两步执行：

1. `UDataEditorLibrary::PrepareRuneIdTagIni()`：扫描所有 `RuneID > 0` 且 `RuneIdTag` 为空的 RuneDA，自动生成 `Rune.ID.Legacy_<N>` 临时 Tag 并写入 `Config/Tags/RuneIDs.ini`。
2. 重启编辑器或重新启动 headless 编辑器进程后，调用 `UDataEditorLibrary::ApplyRuneIdTagsAfterRestart()`：把已注册的 Tag 写回对应 RuneDA。

PowerShell 入口见 [`Tools/DataEditor/README.md`](../../../Tools/DataEditor/README.md)。后续策划可把 `Rune.ID.Legacy_<N>` 手动改成有意义的 Tag。

---

## EffectIdTag 命名空间

新建 `Effect.ID.*`，与既有的 `Buff.Effect.*` 并存：

- `Buff.Effect.*` —— 已被广泛使用，存量保留
- `Effect.ID.*` —— 新建 EffectDataAsset 推荐使用

文件位置：[`Config/Tags/EffectIDs.ini`](../../../Config/Tags/EffectIDs.ini)

---

## 编辑器工具速查

| 操作 | 调用方式 | 说明 |
|------|---------|------|
| 列出所有 RuneDA | `UDataEditorLibrary::GetAllRuneDAs()` | EUW 蓝图直接调用 |
| 批量改 GoldCost | `BatchSetRuneGoldCost(Targets, 50)` | 自带 Undo（FScopedTransaction），值会 clamp 到 >= 0 |
| 校验 Tag 唯一性等 | `UDataValidator::ValidateAll()` | 输出 FDataValidationReport，结果到 Output Log |
| 导出 CSV 快照 | `ExportRuneDAsToCSV("")` | 默认到 `Saved/Balance/RuneNumbers_<时间戳>.csv`，自动建目录 |
| 旧 ID 迁移到 Tag — 第 1 步 | `PrepareRuneIdTagIni()` | 写 Tag 到 `Config/Tags/RuneIDs.ini`（去重，不修改 DA） |
| 旧 ID 迁移到 Tag — 第 2 步 | `ApplyRuneIdTagsAfterRestart()` | **重启编辑器后**调用，把 Tag 写回 DA |
| 双源比对 | `VerifyAccessorParity()` | 检查访问器与字段读取结果是否一致 |

脚本入口：[`Tools/DataEditor`](../../../Tools/DataEditor/README.md)。默认不自动关闭正在运行的编辑器；需要自动关闭时显式传 `-CloseEditor`。

---

## 自定义 Detail Panel

打开任意 `URuneDataAsset` 时，顶部会出现 **Rune Tools** 分组：

- `Copy RuneIdTag` — 拷贝 Tag 到剪贴板（便于贴 Excel/Notion）
- `Validate This` — 单条校验
- 右侧实时显示当前 Rarity / GoldCost

注册位置：[`DevKitEditor.cpp`](../../../Source/DevKitEditor/DevKitEditor.cpp) `StartupModule()`

---

## 后续阶段（未实施）

- **P1.5**：`URuneEffectFragment` 内部数值通过 SetByCaller 注入（需扩展 `ApplyToGE` 接口）
- **P2**：`CT_RarityByDepth`、`CT_DifficultyByWave` CurveTable 引入
- **P3**：`UDamageCalculation` 伤害公式集中点

详见 [缺失引用记录](../../00_入口与规范/缺失引用记录.md)。
