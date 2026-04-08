@echo off
chcp 65001 >nul
title SyncBridge - 打包为独立 exe

echo ══════════════════════════════════════════
echo   将 SyncBridge 打包为独立 .exe 文件
echo   打包后无需 Python 环境即可运行
echo ══════════════════════════════════════════
echo.

:: 检查 Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 需要 Python 环境来打包！
    echo 请在有 Python 的电脑上运行此脚本。
    pause
    exit /b 1
)

:: 安装 PyInstaller
echo [1/2] 安装 PyInstaller...
python -m pip install pyinstaller -q
echo.

:: 打包（用 python -m 调用，避免 PATH 问题）
echo [2/2] 正在打包（可能需要 1-2 分钟）...
python -m PyInstaller --onefile --name SyncBridge --clean "%~dp0sync_bridge.py"
echo.

if exist "%~dp0dist\SyncBridge.exe" (
    echo ══════════════════════════════════════════
    echo   打包成功！
    echo.
    echo   文件位置: %~dp0dist\SyncBridge.exe
    echo.
    echo   部署到目标机器：
    echo   1. 复制 dist\SyncBridge.exe 到目标文件夹
    echo   2. 复制 config.example.json 到同一文件夹
    echo   3. 重命名为 config.json 并编辑配置
    echo   4. 双击 SyncBridge.exe 运行
    echo ══════════════════════════════════════════
) else (
    echo [错误] 打包失败，请检查上方错误信息。
)

:: 清理临时文件
rmdir /s /q "%~dp0build" 2>nul
del "%~dp0SyncBridge.spec" 2>nul

pause
