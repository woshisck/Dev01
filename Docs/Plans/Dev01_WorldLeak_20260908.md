# 2026-09-08 RVT 地图 World Memory Leaks 崩溃复盘

## 根因

这是本任务上一轮检查操作造成的 Python 会话引用残留，不是显存不足，也没有证据表明地图文件损坏。

22:15:01，本任务通过 Python 控制台把非当前地图 `L1_Corridor_01a_RVT_Test` 用 `unreal.load_asset` 载入，赋给持久控制台变量 `p`。随后读取不存在的 `persistent_level` 属性报错，但 `p` 仍然保留 World。

22:58:42，用户通过编辑器重新打开这张 RVT 地图。日志引用链明确为：`FPyReferenceCollector → RVT World → Package`。UE 无法回收这个旧 World 和其 Package，于 EditorServer.cpp:2544 主动终止。这就是 “2 leaks”，并非两个材质或 2 GB 内存。

原图与 LightingStudy 切换时没有命中这个后台 World 的目标地图检查，所以之前的验证未暴露问题；上一轮覆盖不足，不能据此宣称全部地图切换已安全。

## 证据

- 崩溃日志：[DevKit.log:4336](X:/Project/YogProject/Dev01/Saved/Crashes/UECC-Windows-47A3C2154F15A0F7BA05BD9EEE21EF06_0000/DevKit.log:4336)：Python 后台加载及随后属性异常。
- [DevKit.log:5264](X:/Project/YogProject/Dev01/Saved/Crashes/UECC-Windows-47A3C2154F15A0F7BA05BD9EEE21EF06_0000/DevKit.log:5264)：FPyReferenceCollector 保活该 World。
- [DevKit.log:5333](X:/Project/YogProject/Dev01/Saved/Crashes/UECC-Windows-47A3C2154F15A0F7BA05BD9EEE21EF06_0000/DevKit.log:5333)：World 引用关联 Package。
- 本任务工具调用记录确认是控制台变量 `p`。故障操作是一次性控制台命令，不是 `dev01_study_preview` 回调保存了 World。
- CrashContext BaseDir 是 `X:/Project/YogProject/Dev01/Engine/Binaries/Win64/`，IsSourceDistribution=False。错误中的 `X:/Dev-BuildEngine/...cpp` 是二进制编译时的源码路径，不代表本次启动了源码引擎。

## 修复与防复发

1. 已崩溃的进程退出后，持久控制台变量自然释放；不需要删除地图、缓存或 BuiltData。没有自动重放那条后台地图加载命令。
2. 不再用 `load_asset(World)` 后台查看非当前地图。地图元数据走 AssetRegistry，实际 Actor 查看须正常打开目标地图。
3. `dev01_inspect_reference_character.py` 现在只检查当前 RVT 地图，全部 Actor/Component 引用为函数临时变量；不匹配时明确跳过，不扫描后台 World。
4. `open_dev01_lighting_study.py` 在切图前只保留当前地图路径字符串，不持有旧 World wrapper。
5. 新增 `test_dev01_map_lifetime.py`，覆盖 RVT → Study → 原图 → RVT 三轮，以及检查脚本的临时异常退出、Profile/CVar 还原和无脏资产检查。测试不调用任何地图保存操作。

不要使用 `globals().clear()`、全项目 Python 引用清除或禁用 World 泄漏断言来掩盖问题。仅增加 `gc.collect()` 也不能回收仍被持久变量引用的 World。

`py "file.py"` 默认 Private 独立作用域；不能把所有文件脚本的顶层变量都说成永久控制台变量。直接 `py` 字符串命令使用持久控制台字典，本次正是后者。

## 验证范围

修复脚本已通过语法检查。独立 Installed Editor 的 NullRHI 无渲染切图回归结果记录在：

- [map_lifetime_regression.json](X:/Project/YogProject/Dev01/Saved/LightingStudy/20260908/map_lifetime_regression.json)
- [Dev01_MapLifetime_Regression.log](X:/Project/YogProject/Dev01/Saved/Logs/Dev01_MapLifetime_Regression.log)

23:10 回归完成：passed=true，三轮全部通过，进程退出码 0；Profile/CVar 还原通过，没有脏地图或脏内容，测试未保存地图。原走廊 7 个备份文件 SHA256 仍一致，P4 opened 未增加。

NullRHI 检查的是地图与 Python 引用生命周期，不代替灯光外观、GPU 或游戏内性能验收。本轮没有修改或重新编译引擎 C++/Shader，没有 P4/GitHub 提交。
