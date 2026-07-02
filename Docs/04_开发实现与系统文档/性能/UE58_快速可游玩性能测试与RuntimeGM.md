# UE5.8 快速可游玩性能测试与 Runtime GM

更新时间：2026-07-02

这套功能用于快速打出一个 Development Win64 测试包。测试包从正常主菜单进入可游玩关卡，保留真实移动、战斗、暂停、刷敌和性能检测流程，方便在接近正式运行环境下做性能对比。

## 编辑器入口

打开方式：

```text
Tools -> DevKit 工具 -> 性能工具 -> 运行时 GM 配置
```

也可以从：

```text
Tools -> DevKit 工具 -> 性能工具 -> 性能工具快捷入口
```

进入后点击 `运行时 GM 配置`。

## Runtime GM 配置项

面板直接编辑 `Yog Runtime GM` 项目设置：

| 配置 | 用途 |
| --- | --- |
| `bEnableRuntimeGM` | 是否启用 F12 Runtime GM。Development/PIE 测试建议开启。 |
| `RuntimeGMWidgetClass` | 可选的自定义 GM 面板 Widget。为空时使用 C++ fallback 面板。 |
| `WeaponDefinition` | F12 面板中“给予配置武器”使用的玩家武器。 |
| `EnemyData` | F12 面板中“生成配置敌人”使用的敌人数据。 |
| `SpawnCount` | 默认刷敌数量。运行时面板里也能临时调整。 |
| `SpawnRadius` | 玩家附近刷敌半径，默认 1200。 |
| `SpawnMinDistance` | 距离玩家的最小刷敌距离，避免贴脸生成。 |
| `SpawnZOffset` | 生成位置 Z 偏移。 |
| `bSpawnedEnemiesCountForLevelClear` | GM 刷出的敌人是否计入清房。默认关闭，避免测试敌人影响关卡流程。 |
| `bApplyRoomBuffsToSpawnedEnemies` | 是否给 GM 刷出的敌人附加当前房间 Buff。默认关闭，便于做干净性能测试。 |

修改后点击 `保存 GM 配置`。配置会写入：

```ini
[/Script/DevKit.YogRuntimeGMSettings]
```

## 游戏内使用

在 PIE 或 Development 包中：

1. 从主菜单进入游戏。
2. 进入可游玩关卡，例如 `InitialRoom`。
3. 按 `F12` 打开 Runtime GM 面板，或在控制台输入 `Yog.GM` / `Yog.GM.Toggle` 打开。
4. 面板可以执行：
   - `给予配置武器`
   - `生成配置敌人`
   - `重置玩家 / 敌人`
   - 临时调整刷敌数量和刷敌半径

注意：Shipping 包默认不启用此调试入口。UE 控制台命令不需要输入 `/` 前缀；如果后续做游戏内聊天式命令框，再把 `/Yog.GM` 映射到同一个 Runtime GM 开关即可。

## 快速打包

命令行或双击运行：

```bat
PackagePlayablePerfTestWin64.bat
```

脚本位置：

```text
D:\Self\GItGame\Dev01\PackagePlayablePerfTestWin64.bat
D:\Self\GItGame\Dev01\BuildScripts\PackagePlayablePerfTestWin64.ps1
```

默认输出：

```text
D:\Self\GItGame\Dev01\Build\Packages\PlayablePerfTest
```

默认打包地图：

```text
/Game/Maps/L_EntryMenu
/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom
```

脚本使用 UE5.8 的 Development Win64，保留 F12 Runtime GM，适合性能测试，不作为正式发布包。

## 打包产物

每次打包会在输出目录生成：

```text
Play DevKit Playable Perf Test.bat
Run Runtime GM Smoke Test.bat
README_PLAYABLE_PERF_TEST.txt
```

`Play DevKit Playable Perf Test.bat` 用于正常游玩测试，从主菜单开始。

`Run Runtime GM Smoke Test.bat` 用于自动烟测：直接打开 `InitialRoom`，执行“给予配置武器 -> 生成 1 个配置敌人 -> 重置玩家/敌人”，然后退出。日志里出现下面内容代表 Runtime GM 主链路通过：

```text
[RuntimeGM][Smoke] PASSED
```

## 可选参数

```bat
PackagePlayablePerfTestWin64.bat -Clean
PackagePlayablePerfTestWin64.bat -SkipBuild
PackagePlayablePerfTestWin64.bat -EngineDir="D:\UE\UE_5.8"
PackagePlayablePerfTestWin64.bat -ArchiveRoot="D:\DevBuilds\PlayablePerfTest"
```

## 验证重点

- 主菜单是否能进入游戏。
- `InitialRoom` 是否能正常移动、战斗、暂停。
- `F12` 或控制台命令 `Yog.GM` 是否能打开 Runtime GM 面板。
- 给武器后攻击/技能链路是否正常。
- 刷敌是否在玩家附近 NavMesh 上生成。
- 重置是否恢复玩家和存活敌人的生命、护甲、护盾、动作状态。
- 自动烟测日志是否出现 `[RuntimeGM][Smoke] PASSED`。
- 使用 `stat unit`、`stat gpu`、Graphics Pipeline 等工具记录性能数据。
