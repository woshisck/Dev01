# UE5.8 Required Automation Tests

- Time: 2026-06-29T13:02:30.8129604+08:00
- Repo: D:\Self\GItGame\Dev01
- Engine root: D:\UE\UE_5.8
- Editor executable: D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
- Run requested: True
- Status: Passed
- Timeout per suite: 300 seconds

## Suites

| Suite | Filter | Passed | Test Count | ExitCode | TimedOut | Log |
| --- | --- | --- | ---: | ---: | --- | --- |
| DevKit.Performance.Settings | `DevKit.Performance.Settings` | True | 3 | 0 | False | `Saved\Logs\Codex_PerformanceSettings_Focus_Tests.log` |
| DevKitEditor.UI | `DevKitEditor.UI` | True | 4 | 0 | False | `Saved\Logs\Codex_UI_Focus_Tests.log` |
| DevKitEditor.MaterialBatch | `DevKitEditor.MaterialBatch` | True | 16 | 0 | False | `Saved\Logs\Codex_MaterialBatchTests.log` |
| DevKitEditor.UE58Performance | `DevKitEditor.UE58Performance` | True | 2 | 0 | False | `Saved\Logs\Codex_UE58Performance_Tests.log` |

## Commands

### DevKit.Performance.Settings

```text
D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "D:\Self\GItGame\Dev01\DevKit.uproject" -Unattended -NullRHI -NoSplash -NoSound -stdout -FullStdOutLogOutput -AbsLog="D:\Self\GItGame\Dev01\Saved\Logs\Codex_PerformanceSettings_Focus_Tests.log" -ExecCmds="Automation RunTests DevKit.Performance.Settings; Quit" -TestExit="Automation Test Queue Empty"
```

### DevKitEditor.UI

```text
D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "D:\Self\GItGame\Dev01\DevKit.uproject" -Unattended -NullRHI -NoSplash -NoSound -stdout -FullStdOutLogOutput -AbsLog="D:\Self\GItGame\Dev01\Saved\Logs\Codex_UI_Focus_Tests.log" -ExecCmds="Automation RunTests DevKitEditor.UI; Quit" -TestExit="Automation Test Queue Empty"
```

### DevKitEditor.MaterialBatch

```text
D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "D:\Self\GItGame\Dev01\DevKit.uproject" -Unattended -NullRHI -NoSplash -NoSound -stdout -FullStdOutLogOutput -AbsLog="D:\Self\GItGame\Dev01\Saved\Logs\Codex_MaterialBatchTests.log" -ExecCmds="Automation RunTests DevKitEditor.MaterialBatch; Quit" -TestExit="Automation Test Queue Empty"
```

### DevKitEditor.UE58Performance

```text
D:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "D:\Self\GItGame\Dev01\DevKit.uproject" -Unattended -NullRHI -NoSplash -NoSound -stdout -FullStdOutLogOutput -AbsLog="D:\Self\GItGame\Dev01\Saved\Logs\Codex_UE58Performance_Tests.log" -ExecCmds="Automation RunTests DevKitEditor.UE58Performance; Quit" -TestExit="Automation Test Queue Empty"
```

## Acceptance Notes

- Final upload automation must run this script with -Run after the reviewed scope is staged and before commit/push.
- Existing passing logs are useful for review, but Status: Passed requires a fresh run in this script.
