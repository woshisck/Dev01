@echo off
setlocal EnableExtensions

set "PROJECT_DIR=%~dp0"
set "PROJECT_FILE=%PROJECT_DIR%DevKit.uproject"
set "PACKAGE_BAT=%PROJECT_DIR%PackageDevelopmentWin64.bat"
set "ENGINE_DIR="
set "VS_INSTALL="
set "RUN_PREREQS=1"
set "INSTALL_BUILD_TOOLS=0"
set FORWARDED_ARGS=

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="/?" goto help
if /I "%~1"=="-?" goto help
if /I "%~1"=="--help" goto help
if /I "%~1"=="/SkipPrereqs" (
    set "RUN_PREREQS=0"
    shift
    goto parse_args
)
if /I "%~1"=="-SkipPrereqs" (
    set "RUN_PREREQS=0"
    shift
    goto parse_args
)
if /I "%~1"=="/InstallBuildTools" (
    set "INSTALL_BUILD_TOOLS=1"
    shift
    goto parse_args
)
if /I "%~1"=="-InstallBuildTools" (
    set "INSTALL_BUILD_TOOLS=1"
    shift
    goto parse_args
)
set "ARG=%~1"
set FORWARDED_ARGS=%FORWARDED_ARGS% "%ARG%"
shift
goto parse_args

:args_done
if not exist "%PROJECT_FILE%" (
    echo ERROR: Could not find "%PROJECT_FILE%".
    echo Run this batch file from the DevKit project folder.
    pause
    exit /b 1
)

if not exist "%PACKAGE_BAT%" (
    echo ERROR: Could not find "%PACKAGE_BAT%".
    pause
    exit /b 1
)

call :find_engine
if not defined ENGINE_DIR (
    echo ERROR: Could not find Unreal Engine 5.4.
    echo Set UE_ENGINE_DIR to your Unreal Engine root and run again.
    echo Example:
    echo   setx UE_ENGINE_DIR "D:\Code\UE_Engine\UE_5.4"
    pause
    exit /b 1
)

echo.
echo DevKit Development package setup
echo   Project: "%PROJECT_FILE%"
echo   Engine:  "%ENGINE_DIR%"
echo   Config:  Development
echo.

call :find_visual_studio
if not defined VS_INSTALL (
    if "%INSTALL_BUILD_TOOLS%"=="1" (
        call :install_build_tools
        if errorlevel 1 exit /b 1
        call :find_visual_studio
    )
)

if defined VS_INSTALL (
    echo Initializing Visual Studio C++ environment...
    set "VS_DEV_CMD=%VS_INSTALL%\Common7\Tools\VsDevCmd.bat"
    if exist "%VS_DEV_CMD%" (
        call "%VS_DEV_CMD%" -arch=x64 -host_arch=x64 >nul
    ) else (
        echo WARNING: VsDevCmd.bat not found under "%VS_INSTALL%".
    )
) else (
    echo WARNING: Visual Studio C++ Build Tools were not found.
    echo          Install Visual Studio 2022 with "Desktop development with C++",
    echo          or run this script with /InstallBuildTools.
)

if "%RUN_PREREQS%"=="1" (
    call :install_unreal_prereqs
) else (
    echo Skipping Unreal prerequisites.
)

set "UE_ENGINE_DIR=%ENGINE_DIR%"

echo.
echo Packaging DevKit Win64 Development...
call "%PACKAGE_BAT%" %FORWARDED_ARGS%
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo Setup/package failed with exit code %EXIT_CODE%.
    pause
    exit /b %EXIT_CODE%
)

echo.
echo Development package completed successfully.
exit /b 0

:find_engine
if defined UE_ENGINE_DIR call :try_engine "%UE_ENGINE_DIR%"
if defined ENGINE_DIR exit /b 0
call :try_engine "D:\Code\UE_Engine\UE_5.4"
if defined ENGINE_DIR exit /b 0
call :try_engine "D:\UE_src\5.4\UnrealEngine-5.4"
if defined ENGINE_DIR exit /b 0
call :try_engine "D:\Epic Library\UE_5.4"
if defined ENGINE_DIR exit /b 0
call :try_engine "C:\Program Files\Epic Games\UE_5.4"
if defined ENGINE_DIR exit /b 0
call :try_engine "D:\Program Files\Epic Games\UE_5.4"
exit /b 0

:try_engine
set "CANDIDATE=%~1"
if not defined CANDIDATE exit /b 0
if exist "%CANDIDATE%\Engine\Build\BatchFiles\RunUAT.bat" (
    set "ENGINE_DIR=%CANDIDATE%"
    exit /b 0
)
if exist "%CANDIDATE%\Build\BatchFiles\RunUAT.bat" (
    for %%I in ("%CANDIDATE%\..") do set "ENGINE_DIR=%%~fI"
    exit /b 0
)
exit /b 0

:find_visual_studio
set "VS_INSTALL="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%I"
)
exit /b 0

:install_build_tools
echo.
echo Installing Visual Studio 2022 Build Tools with C++ workload...
where winget >nul 2>nul
if errorlevel 1 (
    echo ERROR: winget is not installed. Install Visual Studio 2022 Build Tools manually.
    exit /b 1
)

winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --accept-source-agreements --accept-package-agreements --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
if errorlevel 1 (
    echo ERROR: Visual Studio Build Tools installation failed.
    exit /b 1
)
exit /b 0

:install_unreal_prereqs
set "UE_PREREQ=%ENGINE_DIR%\Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe"
if not exist "%UE_PREREQ%" (
    echo WARNING: Unreal prerequisite installer was not found:
    echo          "%UE_PREREQ%"
    exit /b 0
)

echo Installing Unreal prerequisites...
start /wait "" "%UE_PREREQ%" /quiet /norestart
set "PREREQ_EXIT=%ERRORLEVEL%"
if "%PREREQ_EXIT%"=="0" exit /b 0
if "%PREREQ_EXIT%"=="3010" (
    echo Unreal prerequisites installed; Windows restart is recommended.
    exit /b 0
)

echo WARNING: Unreal prerequisites exited with code %PREREQ_EXIT%.
echo          Packaging will continue, but runtime prerequisites may need manual install.
exit /b 0

:help
echo Usage:
echo   SetupAndPackageDevelopmentWin64.bat [options] [PackageDevelopmentWin64 options]
echo.
echo Options:
echo   /SkipPrereqs        Do not run UEPrereqSetup_x64.exe.
echo   /InstallBuildTools  If C++ Build Tools are missing, install them through winget.
echo   -? or --help        Show this help.
echo.
echo Forwarded examples:
echo   SetupAndPackageDevelopmentWin64.bat -Clean
echo   SetupAndPackageDevelopmentWin64.bat -SkipBuild
echo   SetupAndPackageDevelopmentWin64.bat -ArchiveRoot="D:\DevBuilds"
exit /b 0
