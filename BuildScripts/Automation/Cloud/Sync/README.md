# Reviewed GitHub / P4 synchronization repair

This directory is a candidate repair, not permission to merge, submit or publish.
Production remains on the protected read-only preflight until the combined code
and its asset compatibility have been accepted. Do not deploy the parent directory's
historical build/Feishu entrypoints over newer live cloud scripts.

## Fixed inputs (2026-10-18)

- Live GitHub main: `7c3c3da714abaf497d3139fdae7ce6d1e3fb08a9`.
- Existing Draft PR #1: `4d12013c9a9da45ca036e2b16e0b0f78c5acfb85`.
- Git merge base of these two lines: `291265d2eac41d3165cb4f56ae8526b02f857cee`.
- Prior P4/Git per-file integration checkpoint: CL125 / `942b2ba04bc9214a25f2b41087e68aeb40d72bfd`.
- P4 project head: CL165. The 30 paths after CL162 are excluded assets; the CL162
  reviewed technical import therefore remains the relevant P4 code snapshot.
- The active old mirror ledger still points to CL121 / `6fe2ffb...`; do not pretend
  this is the current reconciled checkpoint or replace it with an unverified tip.

## Repair behavior

- Git and P4 path decoding is explicit: UTF-8 for Git; CP936 for the current
  non-Unicode Windows P4 depot. Do not apply the latter to a Unicode P4 server.
- Only named Lua source locations and approved authoring examples are exceptions
  to Content exclusion. Engine, Unreal assets, binary libraries and generated
  paths remain excluded. Existing Git-only assets are preserved, not copied or
  deleted, and no longer make all unrelated code classification fail.
- Unknown paths still stop the preflight. Approved plugin test/license/readme
  sources are recognized; Git-only documents remain Git-only.
- A target-only file cannot be silently deleted. Review and explicit deletion
  authorization are required. Reparse-point targets/sources are rejected.
- P4 change enumeration is fixed to the observed changelist range and retains the
  newest action instead of allowing an old add to replace a recent deletion.
- The preflight emits invalid/excluded/overlap counts and live remote identity.
  A recognized old policy hash permits classification-only diagnostics, not a
  ledger advance or write to Git/P4. Both-sided changes remain review-gated.
- Three-way decisions use separate per-side baseline hashes, preserving previously
  reviewed Git/P4 differences rather than flattening the two development lines.
- The merged build exposed an actual API mismatch: imported RoomConfig controls
  still referenced the per-room portal model removed by main on October 15.
  Only that widget and its contract test are adapted: point users to placed
  `APortal.DestinationLevel`, remove dead controls and assert the removed fields
  stay absent. Runtime portal/gameplay files are unchanged. The integration audit
  pins the two reviewed replacement blobs rather than bypassing whole paths.

## Tests

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\test_dev01_sync_repair.ps1
python .\audit_sync_integration.py --repo X:\Dev-ProjectBuilds\Dev01-SyncRepair-20261018
```

The PowerShell tests use fixtures and do not contact GitHub/P4. The Python audit
reads Git objects/index only; it does not stage, commit or push. Real cloud preview
is under `C:\BuildAgent\Dev01\repairs\sync-20261018`, with a separate status file.

### Recorded validation of the combined tree

- Installed Build CL160: merged `DevKitEditor Win64 Development` compiled and
  linked successfully after adapting the two obsolete portal editor/test files.
  Log: `Saved/SyncRepairValidation/Build-PortalCompatibility.log`.
- Sync policy/transaction fixtures: 79 assertions passed locally and on the cloud.
- UE automation: 17 selected tests succeeded, zero failed (two retain expected
  StateConflict diagnostic warnings). Includes the updated RoomConfig native-card
  contract. Report: `Saved/SyncRepairValidation/UEAutomation/index.json`.
- ActionRules Python 35, Workflow Python 20, ActionWorkbench JavaScript 42
  assertions passed. Integration index audit passed: 156 main-only and 198
  feature-only changed paths preserved, with the two exact portal adaptations;
  five both-sided paths compiled together. No generated binaries/assets staged.
- The sparse checkout is not full-game acceptance: CountDownTime GameFeatureData
  is absent and startup still reports retained upstream UnLuaTestSuite issues.
  Do not disable the plugin or mistake selected-test success for asset/cook/UGS QA.
- Cloud preview has zero invalid whitelist paths. It correctly remains blocked
  at `LocalGitHeadDiffersFromRemote`, with both-sided changes under the old ledger.
  Production Sync files and ledger hashes remained unchanged during preview.

## Activation gates

The user subsequently authorized direct integration into GitHub main and
code/text synchronization on 2026-10-18. Updating a Draft PR is no longer a
required publication route. That authorization does not waive byte verification,
asset compatibility, clean workspace checks, or the Engine/asset exclusions.

1. Confirm remote main, Draft PR and P4 snapshot have not changed since this packet.
2. Pass merged-project compile, focused tests and required full asset/runtime QA.
3. Publish through the user-approved route. Do not force-push main.
4. Record the immutable integrated Git commit and its validation limits.
5. Reconcile only the approved code/text allowlist into an isolated clean P4
   workspace. Review pending/resolve and per-file diffs; compile with the pinned
   Installed Build. No Engine or asset changes are authorized by this repair.
6. Submit only with explicit authorization, then prove managed per-file bytes
   match the fixed Git commit/P4 CL (with recorded intentional exclusions).
7. Activate a new verified ledger *after* that proof, with recoverable old state.
   `New-Dev01SyncCheckpointCandidate` deliberately returns ReadyToActivate=false.
8. Deploy only the reviewed Sync files with hash/backup checks. Keep the old
   destructive Build And Mirror task disabled. Any future write cycle must use
   the same immutable snapshots, clean guards and reviewed three-way transaction;
   do not turn on the old all-tree mirroring implementation.

Until these gates pass, an updated preview reporting LocalGitHeadDiffersFromRemote
or reviewed reconciliation required is correct. Clearing the old ledger or simply
allowing every path would conceal the divergence, not repair it.
