# Dev01 云同步入口

`git_http_proxy` 默认是 `http://127.0.0.1:7890`，这里的 loopback 指云服务器上的现有代理，不是本机代理。只允许 HTTP 的 `127.0.0.1` 或 `localhost` 及明确端口，不接受外部地址、用户信息、路径或查询参数。设为 `null` 时，探针为该次 Git 调用显式传入空的 `http.proxy`。该选项仅用于 `status` 的 Git 命令，通过逐次 `-c http.proxy=...` 传递，不改全局或仓库 Git 配置，也不改变 worker 的代理设置。

此工具通过 OpenSSH 调用唯一的云端同步 worker。Git/P4 的文件计划、锁、checkpoint、提交和 PCB 验证由云端负责。本机不复制或删除项目文件，不运行旧 `force_sync.py`，不提交本机 Binaries，也不发送飞书消息。

需要 Python 3.9+、PATH 中的 `ssh`、已配置的 SSH 私钥，以及本机 known_hosts 中已核实的服务器主机密钥。工具启用 `BatchMode`、`IdentitiesOnly` 和严格主机密钥校验；不会读取或打印私钥内容，也不会自动接受新主机密钥。

可以直接使用内置的现有云主机和路径配置。需要覆盖时，将 `config.example.json` 复制为同目录的 `config.json`，或使用 `-c` 指定外部配置文件。配置只允许非敏感的连接信息和路径，不接受密码、token、远端命令或任意脚本文本。不要将 `config.json`、私钥、日志、旧 sync_state、build/dist 或缓存提交到 Git。

```powershell
python Tools/SyncBridge/syncbridge.py --help
python Tools/SyncBridge/syncbridge.py check
python Tools/SyncBridge/syncbridge.py status
python Tools/SyncBridge/syncbridge.py preview
python Tools/SyncBridge/syncbridge.py once
python Tools/SyncBridge/syncbridge.py run --interval 30 --count 4
python Tools/SyncBridge/syncbridge.py -c C:/LocalConfig/dev01-sync.json status
```

不传子命令时显示菜单。`check` 检查 SSH 身份和远端路径；`status` 读取云 Git HEAD、GitHub advertised main、工作区脏文件数量、checkpoint、worker 状态和两项既有计划任务。`run` 只是重复读取这些状态；不会启动计划任务或建立另一套本地写循环。默认观察至 Ctrl+C，也可以使用 `--count` 限定次数。

`preview` 调用正式 `run-scheduled.ps1 -DryRun`。云端预检可能维护自身状态文件、锁和 Git fetch 缓存，但不应提交项目文件。`once` 直接调用既有 Node worker 的 `--apply`，绕过通知包装器，保留全部同步、来源、并发和发布 guard。它可能按照已通过的云端计划修改 Git/P4、提交代码并等待或验证 PCB；请先检查预检计划。工具没有 force 模式，没有自动更改规则、清 journal 或重设 checkpoint 的入口。

远端退出码原样返回，包括 guard 阻塞和 SSH 的 255。只有 `check/status` 在 SSH 明确报告认证前的 banner/kex reset、连接拒绝或连接超时，且没有标准输出时，才最多尝试三次、间隔两秒。`once/preview` 不自动重试；无法判断发生阶段的进程超时也不重试，返回 124。超时后应先读取 `status`，避免在写操作结果未知时盲目重试。任务的 `LastResult=0` 只代表该任务最近一次退出状态，不能单独证明当前 Git/P4、构建、PCB 和 UGS 都已完成。

源码、Shader、配置和批准的工具文本按云端归属规则收敛；场景、材质/纹理资产、FluidNinja 内容及发布产物仍由 P4 管理。此入口不会改变这些规则。

本机测试（不连接云端）：

```powershell
python -B -m unittest discover -s Tools/SyncBridge/tests -v
```
