# 🚀 Dev01 项目获取与 UGS 使用指南

> 适用对象：新加入的美术、策划、程序及外部协作成员
> 目标：拿到个人 P4 权限后，只配置一次；以后通过 UGS 同步并启动项目。

## 📌 先看结论

- 每位成员使用自己的 P4 账号，不共享 `Admin`。
- 每位成员每台电脑只建 1 个项目 workspace。
- 项目完整数据来自 P4；GitHub 主要用于代码协作，不分发引擎源码。
- 本地不需要 UE5.8 源码，也不需要自己编译编辑器。
- UGS 会同步项目、只读 Installed Build 引擎和匹配的预编译编辑器包。
- 建议准备至少 80 GB 可用空间，推荐 100 GB，给项目、引擎、DDC 和 Shader 缓存留余量。

## 🏗️ 当前架构

| 项目 | 地址或路径 |
| --- | --- |
| P4 服务器 | `ssl:124.223.187.156:1666` |
| 项目 Stream | `//Dev01/main` |
| 只读引擎 Stream | `//Dev01Engine/main` |
| UGS 预编译编辑器包 | `//Dev01Binaries/UGS/++Dev01+main-Editor.zip` |
| UGS 自动更新包 | `//Dev01Binaries/Tools/UnrealGameSync/Release.zip` |

引擎 Stream 只分发编译好的 UE5.8 Installed Build，不包含引擎源码。

截至 2026-07-21 的验证快照：项目 CL 83、引擎 CL 76、PCB 发布 CL 84。正常工作请以 UGS 中最新“已验证、可用”的版本为准，不要永久写死这些 CL。

## ✅ 开始前准备

1. 获取自己的 P4 账号与权限。
2. 安装 Helix Visual Client（P4V），安装时必须包含 `p4.exe` 命令行工具。
3. 准备一个非磁盘根目录，例如 `D:\Dev01` 或 `E:\Projects\Dev01`。
4. 不要把 workspace 直接设为 `D:\` 或 `E:\`。
5. 不要复制别人的 workspace；每台电脑创建自己的 workspace。

## ⚡ 推荐：一键初始化

使用文件：`SetupDev01Newcomer.bat`

当前维护位置：`X:\Project\Dev01\SetupDev01Newcomer.bat`

脚本会自动完成：

- 输入个人 P4 账号。
- 登录并信任 Dev01 SSL 服务器。
- 自选 UGS 安装目录。
- 自选项目 workspace 目录。
- 创建本机唯一 workspace。
- 绑定 `//Dev01/main`。
- 安装最新版 UGS。
- 同步最小启动文件。
- 启动 UGS。

双击运行即可。密码只由 P4 登录窗口临时输入，脚本不会保存密码。

也可以带参数运行：

```bat
SetupDev01Newcomer.bat --p4user 你的账号 --workspace "D:\Dev01" --ugs-path "D:\Tools\Dev01UGS"
```

生成的 workspace 名称：

```text
dev01_<P4账号>_<电脑名>_main
```

首次打开 UGS 时选择：

- Workspace：脚本创建的 `dev01_<账号>_<电脑名>_main`
- Path：`/DevKit.uproject`
- Generate P4Config：启用

然后点击 **Sync Now**。

## 🧭 手动配置方式

### P4V 连接

- Server：`ssl:124.223.187.156:1666`
- User：自己的个人账号
- Workspace：新建个人 workspace

### Workspace 推荐设置

- Name：`dev01_<账号>_<电脑名>_main`
- Root：`D:\Dev01` 或其他自选子目录
- Stream：`//Dev01/main`
- Host：当前电脑

一人一机一个 workspace 即可。

### UGS 打开项目

- Workspace：上面创建的 workspace
- Path：`/DevKit.uproject`
- Generate P4Config：勾选

### UGS 底部设置

- Generate Projects：由项目配置管理，保持默认。
- Build：不要勾选。
- Run：需要同步后自动启动时勾选。
- Open Solution：普通美术/策划不需要。

## 🎮 日常使用

### 每天开始工作

1. 打开 `StartDev01UGS.cmd` 或 `UnrealGameSync.exe`。
2. 查看最新可用/已验证 CL。
3. 点击 **Sync Now**。
4. 同步完成后点击 **Unreal Editor**。
5. 第一次启动会生成 DDC 和编译 Shader，时间较长属于正常现象。

### 美术 / 策划

- 所有资产和完整项目都在 P4 中工作。
- 修改前 Checkout，完成后提交 P4 changelist。
- 不需要 GitHub，不需要编译编辑器。

### 程序

- 日常代码可通过 GitHub 分支协作。
- 进入团队版本前必须同步回 P4，并由云端构建验证。
- 只有云端发布了匹配项目 CL 的 PCB 后，该 CL 才适合所有成员通过 UGS 使用。
- 不要把引擎源码上传 GitHub，也不要让普通成员拉取引擎源码。

## 🔍 同步后必须存在

```text
<Workspace>\DevKit.uproject
<Workspace>\Engine\Build\InstalledBuild.txt
<Workspace>\Engine\Binaries\Win64\UnrealEditor.exe
<Workspace>\Binaries\Win64\DevKitEditor.target
```

启动入口：UGS 顶部 **Unreal Editor**，或 `<Workspace>\StartDevKit.bat`。

## 🧯 常见问题

### 提示 Missing Modules / Editor out of date

- 选择 **No**，不要在个人电脑上重编译。
- 关闭编辑器。
- 回到 UGS 重新 **Sync Now**。
- 确认同步的是有匹配 PCB 的已验证 CL。

### UGS 安装目录为空

- 使用最新版 `SetupDev01Newcomer.bat`。
- 不要使用以前单独复制出去的旧 `SetupDev01UGS.bat`。
- 安装目录必须是磁盘下的子目录，不能直接选择盘符根目录。

### P4 login is required

- 使用自己的 P4 账号重新登录。
- 不要改成 Admin，也不要把密码写入脚本。
- 可在 P4V 中先完成登录，再重新运行初始化脚本。

### UGS 同步很慢

- 首次同步包含完整项目和编译好的引擎，属于正常现象。
- 后续增量同步通常会明显更快。
- 避免反复切换旧 CL，UGS 可能清理并重新下载大量二进制。

### Shader 第一次启动很慢

- 首次启动需要生成本机 DDC 和 Shader 缓存。
- 不要因为界面暂时无响应而强制结束。
- 后续启动会加快。

### Python could not be initialized

- 先确认 Installed Build 引擎完整同步。
- 若仍存在，记录 Output Log，并联系构建维护人员更新引擎发布包。

## 🛡️ 团队工作规则

- 禁止共享 Admin 账号。
- 禁止一个普通成员创建多个重复 workspace。
- 禁止普通成员点击“本地重建缺失模块”。
- 禁止把 UE 引擎源码提交到 GitHub。
- P4 是完整游戏项目与资产的主环境。
- GitHub 是代码协作入口，不是完整项目分发入口。
- UGS 是团队统一的同步、版本选择和编辑器启动入口。
- 引擎修改流程：本地源码机编译验证 → 上传 source-free Installed Build 到 P4 → 云端构建项目 PCB → UGS 分发。

## 维护说明

如果引擎、项目模块、UGS 或 PCB 有更新，构建维护人员应先完成云端验证，再把对应 CL 标记为团队可用版本。普通成员只需要在 UGS 中同步并启动。

最后更新：2026-07-21
