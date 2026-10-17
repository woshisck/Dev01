[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$pluginRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$privateRoot = Join-Path $pluginRoot 'Source\DevKitLuaRules\Private'
$outputRoot = Join-Path $pluginRoot 'Intermediate\Standalone'
$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'Visual Studio C++ x64 build tools not found.' }
$msvcRoot = Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory | Sort-Object Name -Descending | Select-Object -First 1
$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdkVersion = Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Include') -Directory | Where-Object { Test-Path (Join-Path $_.FullName 'ucrt\stdio.h') } | Sort-Object Name -Descending | Select-Object -First 1
if (!$msvcRoot -or !$sdkVersion) { throw 'MSVC or Windows SDK headers not found.' }
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$sources = @(Join-Path $privateRoot 'DevKitLuaSandbox.c') + @(Get-ChildItem -LiteralPath (Join-Path $privateRoot 'ThirdParty\Lua') -Filter '*.c' | Select-Object -ExpandProperty FullName) + @(Join-Path $PSScriptRoot 'DevKitLuaSandboxSmoke.c')
$compilerArgs = @('/nologo', '/O2', '/W3', '/D_CRT_SECURE_NO_WARNINGS', '/I', $privateRoot,
    '/I', (Join-Path $privateRoot 'ThirdParty\Lua'), '/I', (Join-Path $msvcRoot.FullName 'include'),
    '/I', (Join-Path $sdkVersion.FullName 'ucrt'), '/I', (Join-Path $sdkVersion.FullName 'shared'),
    '/I', (Join-Path $sdkVersion.FullName 'um'), ('/Fo' + $outputRoot + '\'),
    ('/Fe' + (Join-Path $outputRoot 'DevKitLuaSandboxSmoke.exe'))) + $sources + @('/link',
    ('/LIBPATH:' + (Join-Path $msvcRoot.FullName 'lib\x64')),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib\' + $sdkVersion.Name + '\ucrt\x64'))),
    ('/LIBPATH:' + (Join-Path $sdkRoot ('Lib\' + $sdkVersion.Name + '\um\x64'))))
& (Join-Path $msvcRoot.FullName 'bin\Hostx64\x64\cl.exe') @compilerArgs
if ($LASTEXITCODE -ne 0) { throw ('Lua standalone C build failed: ' + $LASTEXITCODE) }
& (Join-Path $outputRoot 'DevKitLuaSandboxSmoke.exe') | Tee-Object -FilePath (Join-Path $outputRoot 'smoke.log')
if ($LASTEXITCODE -ne 0) { throw ('Lua standalone smoke failed: ' + $LASTEXITCODE) }
