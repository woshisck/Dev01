# Dev01 GitHub-to-P4 automatic mirror

The cloud `Dev01 Code Mirror` scheduled task polls GitHub `main` every five
minutes as SYSTEM. The worker is deployed at
`C:\Project\Dev01-GitToP4-Auto-20261024\v3-worker\auto-sync.mjs`; its private
checkpoint and transaction journal are under
`C:\BuildAgent\Dev01\state\git-to-p4-v3`. It uses the dedicated Perforce
client `mirror_v3_10_0_0_10_Dev01_main`. Do not reuse that client for manual
edits.

GitHub is authoritative only for the reviewed project code/text whitelist.
The mirror compares the complete eligible Git tree with P4, so a file deleted
from Git is deleted from P4 only when it is in that whitelist. P4-only Engine,
Content assets, PCB archives, distribution Build metadata, and generated files
remain outside the mirror. Content assets in a mixed Git commit are skipped;
eligible code and Docs still proceed. GitHub `Docs/` is strictly mirrored.

Before any P4 write, the worker requires a fast-forward Git history, clean Git
checkout, unchanged Engine pin and BuildId, matching prior PCB, an isolated
client with no opened/pending/resolve/local drift, full eligible-tree parity at
the checkpoint, and no global opened overlap on planned paths. It stages a
numbered change, shelves it, and compares shelved actions, file types, and raw
bytes with Git blobs. Source byte drift pauses the transaction; Docs text
normalization may be retried as binary and must then match exactly. It checks
the same guards before submit and verifies the submitted server bytes again.
An unknown submit result is never blindly retried.

Source or plugin binary-affecting changes are submitted to P4 and then built
and published by the separate `Dev01 UGS PCB Build` task. The mirror checkpoint
advances only after the new PCB revision, UGS CodeCL, EngineCL, BuildId, cloud
build receipt, and `p4 verify -q` all match. Docs/Lua/config-only changes reuse
the existing matching PCB. Unknown code-impact paths pause for review.

For routine inspection on the cloud server:

```powershell
Get-ScheduledTask -TaskName 'Dev01 Code Mirror','Dev01 UGS PCB Build'
Get-Content C:\BuildAgent\Dev01\state\git-to-p4-v3\status.json
p4 changes -m1 //Dev01/main/...
p4 fstat -T depotFile,headRev,headChange //Dev01Binaries/UGS/++Dev01+main-Editor.zip
```

`Status=Blocked` means no automatic recovery should be assumed. Inspect the
protected `last-error.json`, `active.json` (if present), P4 opened/pending
state, GitHub head, and the Feishu alert before touching the checkpoint.
Never delete an active journal or force a new CL to make the task green. The
initial audited baseline was Git `3e1c2e58821eb59aba446645d7d9894c93377d61`,
project P4 CL170, Engine CL160, and PCB #36 (CL171).
