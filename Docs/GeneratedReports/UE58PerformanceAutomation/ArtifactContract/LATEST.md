# UE5.8 Performance Artifact Contract

- Time: 2026-06-29T13:02:41.8330214+08:00
- Repo: D:\Self\GItGame\Dev01
- Status: Passed
- Checks: 19
- Failed checks: 0

## Checks

| Area | Check | Passed | Evidence |
| --- | --- | --- | --- |
| MCP | Codex MCP tool wrapper exists | True | BuildScripts/Automation/Invoke-UE58McpTool.ps1 initializes an MCP session. |
| MCP | MCP tool wrapper can list toolsets | True | Wrapper exposes the UE MCP list_toolsets flow. |
| Automation | Codex heartbeat/report automation exists | True | Heartbeat script records MCP status through the Codex automation path. |
| Automation | No Windows scheduled task automation is used | True | Automation scripts contain no Register-ScheduledTask/schtasks usage. |
| PlayerSettings | Graphics settings save model exists | True | FYogGraphicsSettings stores profile, target tier, dynamic/material light budgets, Lumen Lite, and batch-proxy preferences. |
| PlayerSettings | Runtime performance library applies profile CVars | True | Performance library applies GI, Nanite, VSM, resolution, and frame-rate settings. |
| PlayerSettings | Target platform tier API exists | True | Performance library exposes PC, Switch 2, Steam Deck 15W, Steam Deck 5W, and fallback target tiers. |
| PlayerSettings | Graphics settings UI base exists | True | Native settings widget exposes preset, custom apply, and Lumen Lite controls. |
| PlayerSettings | Graphics settings UI exposes target platform tiers | True | Native fallback UI, generated WBP designer tree, and test contract expose PC/Deck/Switch/fallback target buttons. |
| PlayerSettings | Graphics settings UI exposes detailed quality controls | True | Player graphics settings UI exposes model, shadow, reflection, texture, material, dynamic light, and material-light quality controls. |
| PlayerSettings | Main menu settings entry exists | True | Game instance can open the graphics settings menu from the frontend. |
| Scalability | Lumen Lite scalability tier exists | True | GI quality 1 defines the handheld Lumen Lite candidate path. |
| Scalability | Handheld and low device profiles exist | True | Device profiles define handheld/low tiers with Nanite and VSM disabled. |
| MaterialBatch | Material batch commandlets exist | True | Editor commandlets cover audit, build, and material audit flows. |
| MaterialBatch | Runtime mapping asset exists | True | Runtime data asset can store material-batch mapping metadata. |
| Tests | Automation tests cover settings and batch rules | True | C++ automation test sources exist for player settings, material batching, and scene audit. |
| Tests | Required test runner includes UE suites | True | Required test runner can run the focused UE automation suites. |
| UploadGate | Final upload fetches and audits remote first | True | Final upload automation fetches origin and reruns remote audit before commit/push. |
| UploadGate | Final upload compiles before push | True | Final upload path builds, runs tests, builds again, then commits and pushes. |

## Notes

- This is a static Codex-side contract check. It does not compile, open Unreal, run commandlets, stage files, commit, or push.
- Passing this check does not replace fresh UE automation tests and a pre-upload compile.
