# P4 CL162 technical-code integration

## Scope and provenance

- Source: committed `//Dev01/main/...@162`, not local P4 or Git working-copy edits.
- Git baseline: `291265d2eac41d3165cb4f56ae8526b02f857cee` (`main`, fetched 2026-10-14).
- Shared mirror checkpoint: P4 CL125 / Git `942b2ba04bc9214a25f2b41087e68aeb40d72bfd`. More recent shared file revisions were selected by normalized-content equality; CL140 was a nine-path import, not a repository-wide checkpoint.
- Destination: `codex/p4-cl162-technical-sync`, submitted for review rather than replacing `main`.
- The companion `Sync/Dev01_P4_CL162_Manifest.json` records per-file depot revision, hashes, merge base and disposition. Hashes named `p4_sha256` refer to exported bytes; merge comparisons normalize CRLF and a UTF-8 BOM.

The import contains Action Workbench, ActionRules and its sandboxed Lua adapter, authoring tools and template, workflow tools, LookDev's editor-module migration, and committed rendering/settings interfaces. It transfers existing P4 functionality; it does not design new lighting or combat behavior.

No Engine files, Unreal assets, DLL/PDB/LIB files, PCB archives, build output, tickets or credentials are included. Explicit authoring exceptions are `Content/ActionRules/Examples` text files and `Tools/ActionRules/Templates/ActionRules_Template.xlsx`. `Build/Dev01WorkflowCatalog.json` is a tool catalog, not an Engine release marker.

## Decisions preserving both development lines

1. Keep Git's UnLua and extension plugins, startup configuration, game Lua scripts, MobSpawner dispatch, GameMode/GameInstance refactors and montage-distance changes.
2. Add `DevKitLuaRules` beside UnLua. The former has a separate constrained Lua VM for action conditions; it does not replace UnLua's gameplay VM. Preserve the `Lua` and `UnLua` link dependencies and add the ActionRules dependencies/staging rule.
3. Preserve `AddTagReaction` and `RemoveTagReaction` while integrating ActionRules hooks, StateConflict contribution accounting and lifecycle cleanup.
4. Keep Git's `MontagePhaseTagDriver.cpp`. P4 CL144 reversed an earlier import, but current Git montage tasks still call its non-inline `Update` and `Clear` implementation. Mirroring that deletion would break the Git code line.
5. Keep Git's bubble-message and packaging settings; merge the P4 rendering/settings additions rather than replacing `DefaultGame.ini`.
6. Use the newer P4 CL159 CelesLight settings interface/UI. Git's last changes to the two conflicting files were older explanatory text from September 20. Do not re-expose controls deliberately retired by the P4 rendering-workbench migration.
7. Keep existing Git-only files and historical tracked assets unchanged. Generated files produced by validation are excluded from the commit, even where Git historically tracks them.
8. Defer the extra P4-only `Dev01_Menu.bat` launcher: it requires `--path/--no-launch` and `StartDev01UGS.cmd` support not implemented by Git's current installer. Keep the existing Git installer/user-authentication behavior unchanged; the independently usable `Dev01_Workflow.bat` and native editor workbenches are included. Installer reconciliation requires its own tested change.

## Validation

- ActionRules authoring unit tests: **35 passed**.
- Workflow core unit tests: **13 passed**.
- Workflow UI tests: **7 passed**.
- Standalone Lua sandbox: compiled and **20 checks passed**.
- UE automation (`DevKit.ActionRules.Core`, `DevKit.ActionRules.Integration`, `DevKit.StateConflict.Lifecycle`): **16/16 succeeded**, process exit 0; 14 without warnings and 2 with expected state/block diagnostic warnings. The reported test set exactly matches the expected set.
- BuildId verification: **18/18 records matched**, with no missing referenced DLLs (one target, one project manifest and 16 plugin manifests).
- First-party staged whitespace/conflict checks: passed. Pristine upstream Lua has existing EOF blank-line warnings; its source is not reformatted.
- Full merged `DevKitEditor Win64 Development` build: **passed**, 377 actions, `Result: Succeeded`, 823.29 seconds. Both UnLua and DevKitLuaRules modules compiled and linked in the same target. Existing deprecated-API/vendor warnings remain; this is not a zero-warning build.

Validation uses the isolated project checkout and the source-free Installed Build published as Engine CL160, with BuildId `521bf6ff-a998-43fe-962c-978d9a1e7d5e`, not the main UGS workspace's older Engine. UBT is invoked with `-NoEngineChanges -UsePrecompiled -NoHotReload -MaxParallelActions=2`.

The first build attempt could not configure Git's UnLua CMake dependency because the bundled .NET process resolved `VCTargetsPath` inside the SDK. The retry sets `VCTargetsPath` to the installed Visual Studio C++ targets directory and CMake's directory on the process-local PATH; no UnLua source or machine-wide settings were changed.

### Limits before merging/releasing

The headless editor's startup log is **not error-free**. The unchanged Git UnLuaTestSuite reports four uninitialized reflected fields (`FIssue517Struct::X/Y/Z`, `FHexHandle::Value`). The sparse code-validation checkout also lacks CountDownTime GameFeatureData and project Lua startup scripts/assets. These messages precede the selected tests; the 16 selected tests still succeed, but do not establish full-game startup or playtest acceptance. The UnLua test headers are byte-for-byte unchanged relative to the Git baseline.

Before merging and promoting this combined Git/P4 code line into a new P4/UGS release, review the retained UnLuaTestSuite initialization issues and validate the combined code with the complete approved assets and Lua scripts. No test plugin was silently disabled to hide these messages. The PR is intentionally a draft pending that review; no automatic merge or reverse-sync to P4 is performed.

This PR is a code integration, not a new P4/UGS release or a claim that teammate cold-sync acceptance passed. P4 CL162's cloud compilation and its PCB publication are separate checks. No cloud task, Engine stream pin, P4 opened file, or primary working-copy modification was changed by this integration.
