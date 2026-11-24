# Cloud synchronization implementation

This is the versioned source of the single Dev01 GitHub-main to Perforce code
worker. `../syncbridge.py` is its SSH user interface. Project assets, vendor
content, original image inputs, the Installed Engine and PCB stay in Perforce.
P4-authored code must be promoted through a reviewed Git change; this worker
does not push arbitrary P4 changes, assets or generated binaries to GitHub.

Production uses Node at `C:/Project/Dev01-GitToP4-Tools-20261023/node.exe` and
deploys this directory's files to `C:/Project/Dev01-GitToP4-Auto-20261024/`.
The only production state is `C:/BuildAgent/Dev01/state/git-to-p4-v3/`.
The Git checkout, isolated mirror client and build client are different roots.
The configured loopback HTTP proxy is supplied to Git commands individually;
Windows settings and global Git configuration are not changed.

Normal operations:

```powershell
node v3-worker/auto-sync.mjs --dry-run
node v3-worker/auto-sync.mjs --apply
```

The automatic task uses the same worker and state. A source submission remains
an active transaction until the ordinary cloud build publishes its matching
PCB and the worker verifies the submitted file identities, receipt, EngineCL,
BuildId and archive. Only then does the checkpoint advance. A task returning
zero or a PCB file existing is not sufficient evidence of completion.

## Reviewed reconciliation after an external release

The automatic worker intentionally refuses an external P4/PCB advancement.
`v3-worker/reconcile-reviewed.mjs` provides a separate, explicit recovery
transaction, not a force option or checkpoint reset.

1. Stop the scheduled mirror from racing a deployment; preserve its original
   task definition and all current scripts, policy, checkpoint and receipts.
   Never remove an existing active transaction or unexplained lock.
2. Pin the current advertised Git commit, raw checkpoint SHA256, old and new
   policy hashes, project head, EngineCL/BuildId, external PCB and build receipt.
3. Generate a new plan with `--mode plan --target SHA --checkpoint-sha SHA256
   --old-policy-sha SHA256 --out NEW_ABSOLUTE_PATH`. The output deliberately
   leaves `approvedDeletes` and `approvedOpenedOverlaps` empty.
4. Review every action and its preimage. Document source-control history for
   deletions. Back up any overlapping unsubmitted local work and record its
   exact owner/client/change/revision and server preimage. Only nonexclusive
   text edits can be acknowledged; an open file scheduled for deletion also
   requires that exact deletion approval. No other client's files, pending
   changes, shelves or have-list are modified by this process.
5. Fill only the explicit approved lists, hash the complete review bytes and
   apply with `--mode apply --review ABSOLUTE_PATH --review-sha SHA256`.
   The tool reacquires the shared worker lock, recollects the live plan,
   revalidates the pins, saves the review and old checkpoint, and verifies both
   old and external PCB revisions. It then uses the normal stage/shelf/submit
   implementation to create a real binary-build transaction.
6. Let the existing project build/PCB pipeline finish, then run the normal
   worker `--apply` to verify and complete the transaction. Policy migration
   is accepted only with the matching immutable recovery review and real
   active journal. The new policy hash is stored only upon completion.
7. Compare both managed trees, archive verification and local WIP fingerprints;
   restore the scheduled mirror's prior enabled state and confirm a no-op run.

An interrupted staging or submission has a real journal. Inspect its precise
state and changelist before resuming. Never generate a fake `active.json`,
delete the journal, force-sync, or create another CL to disguise an uncertain
outcome. The separate `baseline-adoption.mjs` is only for an already identical
managed Git/P4 tree and verifies fresh NO_OP and release evidence; it cannot
approve divergent code or perform a reconciliation.

The exact YogVision text path additions do not admit PNGs, caches or vendor
assets. New unreviewed paths still stop the planner. A future HLSL material
source change is held for material/asset validation; it is not silently treated
as a documentation change. The maintenance tool sources themselves are
explicit Git-only paths and are deployed through reviewed maintenance.

Tests (no source-control or cloud writes):

```powershell
node --test cloud/v2-readonly/path-migration.test.mjs cloud/v3-worker/*.test.mjs
python -B -m unittest discover -s tests -v
```
