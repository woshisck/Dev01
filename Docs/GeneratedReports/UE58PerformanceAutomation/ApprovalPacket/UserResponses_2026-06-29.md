# UE5.8 Approval User Responses

- Time: 2026-06-29
- Source: user answers in current Codex thread.
- Status: scoped approval recorded; commit/push is still blocked until remote delta review, tests, compile, and final explicit upload approval pass.

## Answers Recorded

| Question | Answer | Operational meaning |
| --- | --- | --- |
| Q1 Remote commit strategy | C | Do not process remote commits for now; continue local UE5.8 work. Final upload must still fetch origin and review remote additions before commit/push. |
| Q2 .slnx policy | A | Accept remote deletion of `Automation_DevKit.slnx` and `DevKit.slnx`. |
| Q3 Commit scope | Custom | Include `Phase1Candidate` plus the 6 `NeedsManualReview` assets, exclude `EvidenceOnly`. |
| Q4 Manual-review assets | A | The 6 binary/map/UI assets may be included in the reviewed commit scope. |
| Q5 EvidenceOnly reports | B | Do not include EvidenceOnly reports in the commit; keep them locally. |
| Q6 Rerun tests | A | After remote sync, required tests may be rerun. |
| Q7 Pre-upload compile | A | After tests pass, compile before upload. |
| Q8 Commit and push | A | Commit and push are allowed only after all upload gates, tests, compile, and remote review pass. This is not immediate authorization while the branch is behind origin/main. |
| Q9 Heartbeat | Do not restore | Do not recreate the heartbeat automation. |
| Q10 Extra limits | None | No additional restrictions. |

## Q3 Gate Explanation

`Phase1Candidate` means files that appear to belong to the UE5.8 Phase 1 implementation scope. This includes automation scripts, runtime/editor C++ code, performance config, generated material-batch assets, and design docs that define the feature. This is the minimum useful implementation commit.

`EvidenceOnly` means generated proof and audit artifacts, such as commandlet reports, latest gate reports, screenshots, and profiling evidence. These are useful for traceability, but they make the commit larger and noisier. The user selected Q5=B, so EvidenceOnly should be left out of the commit unless explicitly changed later.

`NeedsManualReview` means binary or broad asset changes that cannot be reliably reviewed by text diff alone. The current list includes one map and five UI/HUD assets. The user selected Q4=A, so these may enter the commit scope after normal review.

## Q3 Selected Scope

Given Q4=A and Q5=B, the coherent Q3 choice is:

- Q3 custom scope: include `Phase1Candidate` plus the 6 `NeedsManualReview` assets, but exclude `EvidenceOnly`.

Use `Invoke-UE58ReviewedStaging.ps1` / `Invoke-UE58FinalUpload.ps1` with `-IncludeManualReview` and without `-IncludeEvidence` for this scope. In approval text, this is `APPROVE_REMOTE_DELETE_SLNX_AND_STAGE_PHASE1_PLUS_MANUAL_REVIEW_NO_EVIDENCE`.

## Current Blocking Facts

- Local `main` is still behind `origin/main` by 4 commits.
- Remote delta audit is `NeedsManualResolution`.
- Q1=C says not to process remote commits yet.
- Therefore commit/push/upload remains blocked until the remote delta is reviewed or Q1 changes.
