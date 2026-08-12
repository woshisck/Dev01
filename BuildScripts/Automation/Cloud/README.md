# Dev01 Cloud UGS Automation

These scripts are the version-controlled source for the production automation deployed to:

```text
C:\BuildAgent\Dev01
```

## Deployment contract

- Deploy the five scripts in this directory directly under `C:\BuildAgent\Dev01`.
- Run Perforce as the dedicated `Dev01BuildAgent` service identity.
- Keep the P4 ticket at `C:\BuildAgent\Dev01\secrets\Dev01BuildAgent.ticket`.
- Keep the P4 trust file at `C:\BuildAgent\Dev01\secrets\p4trust.txt`.
- Never copy credentials, ticket contents, webhook URLs, or other secrets into this directory.

## Scheduled task

The `Dev01 UGS PCB Build` scheduled task should invoke:

```powershell
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File C:\BuildAgent\Dev01\dev01_auto_ugs_build_with_feishu.ps1
```

The wrapper runs `dev01_auto_ugs_build.ps1` in a child PowerShell process, preserves its build/publish exit code, and queues the completion event through `C:\CodexBridge\scripts\Receive-DesktopNotification.ps1`.

A Feishu message is queued only when a real build/publish attempt occurred. Routine scheduler runs that report `skipped` because there is no unpublished code do not notify. Build or publication failures do notify, and a notification failure never masks the original build exit code.

## Local validation

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test_dev01_auto_ugs_build_with_feishu.ps1
```

The self-test uses temporary fake build/receiver scripts. It does not access P4, publish a PCB, send a Feishu message, or read production credentials.
