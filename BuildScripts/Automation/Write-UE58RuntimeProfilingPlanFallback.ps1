param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string]$Cluster = "Prison_S_01_SourceProxy",
    [string]$Camera = "RepresentativeCamera"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\CommandletReports"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$automationOutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingPlanFallback"
New-Item -ItemType Directory -Force -Path $automationOutputRoot | Out-Null

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$sharedReportPath = Join-Path $OutputRoot "UE58RuntimeProfilingPlanReport.md"
$automationReportPath = Join-Path $automationOutputRoot "UE58RuntimeProfilingPlanFallback_$timestamp.md"
$automationLatestPath = Join-Path $automationOutputRoot "LATEST.md"

function New-Scenario {
    param(
        [string]$Name,
        [string]$Tier,
        [string]$Description,
        [bool]$RequiresBatchProxy,
        [string[]]$CVars
    )

    [pscustomobject]@{
        Name = $Name
        Tier = $Tier
        Description = $Description
        RequiresBatchProxy = $RequiresBatchProxy
        CVars = $CVars
        CaptureCommands = @(
            "stat unit",
            "stat rhi",
            "stat scenerendering",
            "stat gpu",
            "r.MeshDrawCommands.LogDynamicInstancingStats 1",
            "profilegpu"
        )
    }
}

$scenarios = @(
    New-Scenario `
        -Name "Baseline_LumenOff_NoBatch" `
        -Tier "Mid" `
        -Description "Explicit Lumen-off comparison baseline, no generated batch proxy." `
        -RequiresBatchProxy $false `
        -CVars @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 0"
        )
    New-Scenario `
        -Name "LumenLite_NoBatch" `
        -Tier "Mid" `
        -Description "Mid candidate with Lumen Lite enabled before proxy batching." `
        -RequiresBatchProxy $false `
        -CVars @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 0"
        )
    New-Scenario `
        -Name "BatchProxy_LumenOff" `
        -Tier "Mid" `
        -Description "Generated geometry-merge proxy path with Lumen off." `
        -RequiresBatchProxy $true `
        -CVars @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 0"
        )
    New-Scenario `
        -Name "BatchProxy_LumenLite" `
        -Tier "Mid" `
        -Description "Combined Mid candidate: generated proxy plus Lumen Lite." `
        -RequiresBatchProxy $true `
        -CVars @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 0"
        )
    New-Scenario `
        -Name "Low_LumenOff_Aggressive" `
        -Tier "Low" `
        -Description "Low-power profile; Lumen remains off and the batch/proxy path should be preferred." `
        -RequiresBatchProxy $true `
        -CVars @(
            "r.SetRes 1280x720",
            "sg.ViewDistanceQuality 0",
            "sg.ShadowQuality 0",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 0",
            "sg.PostProcessQuality 0",
            "sg.TextureQuality 1",
            "sg.EffectsQuality 0",
            "sg.FoliageQuality 0",
            "r.ScreenPercentage 55",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 0"
        )
    New-Scenario `
        -Name "Epic_LumenHigh" `
        -Tier "Epic" `
        -Description "Epic upper-bound quality profile for comparison against lower tiers." `
        -RequiresBatchProxy $false `
        -CVars @(
            "r.SetRes 1920x1080",
            "sg.ViewDistanceQuality 3",
            "sg.ShadowQuality 3",
            "sg.GlobalIlluminationQuality 3",
            "sg.ReflectionQuality 3",
            "sg.PostProcessQuality 3",
            "sg.TextureQuality 3",
            "sg.EffectsQuality 3",
            "sg.FoliageQuality 3",
            "r.ScreenPercentage 100",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "t.MaxFPS 0"
        )
)

$lines = @(
    "# UE5.8 Runtime Profiling Plan",
    "",
    "- Map: ``$Map``",
    "- Cluster: ``$Cluster``",
    "- Camera label: ``$Camera``",
    "- Evidence status: NotMeasured",
    "- Generated by: BuildScripts/Automation/Write-UE58RuntimeProfilingPlanFallback.ps1",
    "- Commandlet fallback: True",
    "- Commandlet class pending clean link: True",
    "",
    "## Rules",
    "",
    "- Run every scenario from the same camera position and scene state.",
    "- Record at least a five-second stable sample for ``stat unit``, ``stat rhi``, and ``stat scenerendering``.",
    "- Run ``profilegpu`` once per scenario and store the generated CSV path.",
    "- Batch proxy scenarios require the generated proxy to be visible and matching source actors to be hidden or isolated in a test layer.",
    "- This report is a measurement checklist; it does not contain measured GPU values.",
    "- This fallback report mirrors the source-controlled C++ plan while the current editor binaries have not linked the commandlet class yet.",
    "",
    "## Scenario Matrix",
    "",
    "| Scenario | Tier | Requires Batch Proxy | Description |",
    "| --- | --- | --- | --- |"
)

foreach ($scenario in $scenarios) {
    $requires = if ($scenario.RequiresBatchProxy) { "Yes" } else { "No" }
    $lines += "| ``$($scenario.Name)`` | $($scenario.Tier) | $requires | $($scenario.Description) |"
}

$lines += ""

foreach ($scenario in $scenarios) {
    $requires = if ($scenario.RequiresBatchProxy) { "Yes" } else { "No" }
    $lines += "## Scenario: $($scenario.Name)"
    $lines += ""
    $lines += "- Tier: ``$($scenario.Tier)``"
    $lines += "- Requires batch proxy: $requires"
    $lines += ""
    $lines += "### Setup CVars"
    $lines += ""
    $lines += '```text'
    $lines += $scenario.CVars
    $lines += '```'
    $lines += ""
    $lines += "### Capture Commands"
    $lines += ""
    $lines += '```text'
    $lines += $scenario.CaptureCommands
    $lines += '```'
    $lines += ""
    $lines += "### Result Template"
    $lines += ""
    $lines += "| Metric | Value |"
    $lines += "| --- | --- |"
    $lines += "| FPS | NotMeasured |"
    $lines += "| Game ms | NotMeasured |"
    $lines += "| Draw ms | NotMeasured |"
    $lines += "| GPU ms | NotMeasured |"
    $lines += "| RHI ms | NotMeasured |"
    $lines += "| Mesh draw calls | NotMeasured |"
    $lines += "| Total draw calls | NotMeasured |"
    $lines += "| Visible dynamic primitives | NotMeasured |"
    $lines += "| Highest GPU pass | NotMeasured |"
    $lines += "| profilegpu CSV | NotMeasured |"
    $lines += "| Visual notes | NotMeasured |"
    $lines += ""
}

$lines += @(
    "## Acceptance Checks",
    "",
    "- Mid passes only if ``BatchProxy_LumenLite`` is stable at the target frame budget or has a documented fallback to ``BatchProxy_LumenOff``.",
    "- Low passes only if ``Low_LumenOff_Aggressive`` is stable at 30 FPS with acceptable visual loss.",
    "- Batch proxy is considered useful only if mesh draw calls drop enough to offset any extra material shader cost.",
    "- Lumen Lite is optional for Mid if Lumen passes consume more frame time than the visual gain justifies."
)

$existingSharedReportText = ""
if (Test-Path -LiteralPath $sharedReportPath) {
    $existingSharedReportText = Get-Content -LiteralPath $sharedReportPath -Raw
}

$preserveCommandletSharedReport = -not [string]::IsNullOrWhiteSpace($existingSharedReportText) -and
    $existingSharedReportText -notmatch "(?m)^- Commandlet fallback: True\s*$"

if (-not $preserveCommandletSharedReport) {
    Set-Content -LiteralPath $sharedReportPath -Value $lines -Encoding UTF8
}
Set-Content -LiteralPath $automationReportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $automationLatestPath -Value $lines -Encoding UTF8

if ($preserveCommandletSharedReport) {
    Write-Output "Preserved existing commandlet runtime profiling shared report: $sharedReportPath"
}
else {
    Write-Output "Wrote runtime profiling fallback shared report: $sharedReportPath"
}
Write-Output "Wrote runtime profiling fallback report: $automationReportPath"
Write-Output "Updated runtime profiling fallback latest: $automationLatestPath"
