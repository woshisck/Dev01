# Rune Value Table Tabs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a center-panel `数值表 / 流程图` split so designers can configure rune numbers in one table while keeping flow logic in the graph.

**Architecture:** Extend rune tuning rows into a source-aware value model, keep basic rune info in the right details panel, and add a Slate table in the center panel. Flow nodes can keep reading values through existing `GetRuneTuningValue` while the editor exposes source mode, formula/context/MMC metadata, and designer-friendly row controls.

**Tech Stack:** Unreal Engine 5.4, Slate, DevKit `URuneDataAsset`, BuffFlow/Yog Flow editor.

---

### Task 1: Smoke Coverage

**Files:**
- Modify: `Tools/RuneEditor/rune_editor_smoke.py`

- [ ] Add assertions for `ERuneTuningValueSource`, `URuneValueCalculation`, center tabs, value table UI builders, and tuning row edit handlers.
- [ ] Run `Tools\RuneEditor\run_smoke_test.ps1` and verify it fails because the feature is not implemented yet.

### Task 2: Data Model

**Files:**
- Modify: `Source/DevKit/Public/Data/RuneDataAsset.h`
- Modify: `Source/DevKit/Private/Data/RuneDataAsset.cpp`

- [ ] Add value source enum: literal, formula, MMC, context.
- [ ] Add tuning metadata fields: category, value source, formula, context key, MMC class, unit, description.
- [ ] Add a minimal `URuneValueCalculation` base class so MMC rows have a typed extension point.
- [ ] Keep existing `Value` fallback so old assets and existing nodes continue to work.

### Task 3: Center Tabs And Value Table

**Files:**
- Modify: `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.h`
- Modify: `Source/DevKitEditor/Private/RuneEditor/SRuneEditorWidget.cpp`

- [ ] Add center panel tab state for `数值表` and `流程图`.
- [ ] Add a value table list view sourced from the selected rune's tuning rows.
- [ ] Add row controls for key, display name, category, source mode, value, formula, context key, unit, description, and delete.
- [ ] Add an "新增数值" action and keep graph editing under the `流程图` tab.

### Task 4: Verification

**Files:**
- Test: `Tools/RuneEditor/run_smoke_test.ps1`
- Build: `D:\UE\UE_5.4\Engine\Build\BatchFiles\Build.bat`

- [ ] Run smoke test and confirm PASS.
- [ ] Build `DevKitEditor Win64 Development` and confirm exit code 0.
