@echo off
chcp 65001 >nul
title SyncBridge - Git 与 P4 同步

echo ══════════════════════════════════════════
echo   SyncBridge - Git 与 P4 双向同步工具
echo ══════════════════════════════════════════
echo.

:: 检查 Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 Python！
    echo.
    echo 请先安装 Python 3.8+:
    echo   https://www.python.org/downloads/
    echo.
    echo 安装时务必勾选 "Add Python to PATH"
    echo.
    pause
    exit /b 1
)

:: 检查配置文件
if not exist "%~dp0config.json" (
    echo [提示] 未找到 config.json
    echo 正在从模板创建，请编辑后重新运行...
    echo.
    copy "%~dp0config.example.json" "%~dp0config.json" >nul
    notepad "%~dp0config.json"
    pause
    exit /b 0
)

echo 配置文件: %~dp0config.json
echo 日志文件: %~dp0sync_bridge.log
echo.
echo 按 Ctrl+C 可随时停止
echo ──────────────────────────────────────────
echo.

python "%~dp0sync_bridge.py"
pause
