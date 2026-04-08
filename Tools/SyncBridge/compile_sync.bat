@echo off
chcp 65001 >nul
title SyncBridge - 编译并同步到 P4

echo ══════════════════════════════════════════
echo   Compile ^& Sync
echo   编译 UE5 项目，并将编译产物同步到 P4
echo   （不会全量同步，只同步本次编译的产物）
echo ══════════════════════════════════════════
echo.

python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 Python，请先安装 Python 3.8+
    pause
    exit /b 1
)

if not exist "%~dp0config.json" (
    echo [错误] 未找到 config.json，请先配置
    pause
    exit /b 1
)

echo 按任意键开始编译和同步（Ctrl+C 可取消）...
pause >nul
echo.

python "%~dp0compile_sync.py"

echo.
if errorlevel 1 (
    echo [失败] 编译或同步失败，请查看上方错误信息
) else (
    echo [完成] 编译并同步到 P4 成功
)
pause