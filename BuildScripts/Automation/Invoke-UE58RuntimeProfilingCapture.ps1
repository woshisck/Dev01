param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string[]]$Scenario = @("Baseline_LumenOff_NoBatch", "LumenLite_NoBatch"),
    [int]$TimeoutSec = 300,
    [switch]$NoQuit,
    [switch]$Run
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$uprojectPath = Join-Path $RepoRoot "DevKit.uproject"

$Scenario = @(
    foreach ($entry in $Scenario) {
        foreach ($part in ($entry -split ",")) {
            $trimmed = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                $trimmed
            }
        }
    }
)

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\RuntimeProfilingCapture"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Resolve-UE58EngineRoot {
    param([string]$RequestedRoot)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $candidates += $RequestedRoot
    }
    if (-not [string]::IsNullOrWhiteSpace($env:UE58_ENGINE_ROOT)) {
        $candidates += $env:UE58_ENGINE_ROOT
    }
    $candidates += "D:\UE\UE_5.8"
    $candidates += "D:\Epic Library\UE_5.8"

    foreach ($candidate in $candidates) {
        $buildPath = Join-Path $candidate "Engine\Binaries\Win64\UnrealEditor.exe"
        if (Test-Path -LiteralPath $buildPath) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "UE5.8 engine root was not found. Set -EngineRoot or UE58_ENGINE_ROOT."
}

function New-ScenarioDefinitions {
    $definitions = @{}
    $definitions["Baseline_LumenOff_NoBatch"] = [pscustomobject]@{
        Tier = "Medium"
        RequiresBatchProxy = $false
        CVars = @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 60"
        )
    }
    $definitions["LumenLite_NoBatch"] = [pscustomobject]@{
        Tier = "Handheld15W"
        RequiresBatchProxy = $false
        CVars = @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 60"
        )
    }
    $definitions["BatchProxy_LumenOff"] = [pscustomobject]@{
        Tier = "Medium"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 60"
        )
    }
    $definitions["BatchProxy_LumenLite"] = [pscustomobject]@{
        Tier = "Handheld15W"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 60"
        )
    }
    $definitions["Handheld5W_LumenOff_Aggressive"] = [pscustomobject]@{
        Tier = "Handheld5W"
        RequiresBatchProxy = $true
        CVars = @(
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
            "t.MaxFPS 30"
        )
    }
    $definitions["PCUltra_LumenHigh"] = [pscustomobject]@{
        Tier = "PCUltra"
        RequiresBatchProxy = $false
        CVars = @(
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
    }

    return $definitions
}

function ConvertTo-RelativePath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }
    if ($Path.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Path.Substring($RepoRoot.Length).TrimStart("\", "/")
    }
    return $Path
}

function Get-NewProfilingArtifacts {
    param([datetime]$Since)

    $roots = @(
        (Join-Path $RepoRoot "Saved\Profiling"),
        (Join-Path $RepoRoot "Saved\Logs")
    )

    $files = @()
    foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            $files += Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.LastWriteTime -ge $Since -and
                    ($_.Name -match "ProfileGPU|profilegpu|GPU|\.csv$|\.ue4stats$|\.utrace$")
                }
        }
    }

    return @($files | Sort-Object LastWriteTime -Descending -Unique)
}

function Get-GpuProfileSummary {
    param([string]$LogText)

    $summary = [ordered]@{
        FrameTimeMs = ""
        RootInclusiveTimeMs = ""
        RootInclusiveDraws = ""
        RootInclusiveDispatches = ""
        RootInclusivePrimitives = ""
        RootInclusiveVertices = ""
        TopEvent = ""
        TopEventInclusiveTimeMs = ""
        TopEventInclusivePercent = ""
    }

    if ([string]::IsNullOrWhiteSpace($LogText) -or $LogText -notmatch "GPU Profile for Frame") {
        return [pscustomobject]$summary
    }

    $profileStart = $LogText.LastIndexOf("GPU Profile for Frame", [System.StringComparison]::OrdinalIgnoreCase)
    $profileText = $LogText.Substring($profileStart)

    if ($profileText -match "Frame Time\s*:\s*([0-9]+(?:\.[0-9]+)?)ms") {
        $summary.FrameTimeMs = $Matches[1]
    }

    $rows = @()
    foreach ($line in ($profileText -split "`r?`n")) {
        if ($line -notmatch "LogRHI: Display:\s+(\|.*\|)") {
            continue
        }

        $rowText = $Matches[1]
        $parts = @($rowText -split "\|" | ForEach-Object { $_.Trim() })
        if ($parts.Count -lt 14) {
            continue
        }

        $inclusiveTime = ""
        if ($parts[12] -match "([0-9]+(?:\.[0-9]+)?)\s*ms") {
            $inclusiveTime = $Matches[1]
        }
        else {
            continue
        }

        $eventName = $parts[13].Trim()
        if ([string]::IsNullOrWhiteSpace($eventName) -or $eventName -eq "Events") {
            continue
        }

        $rows += [pscustomobject]@{
            ExclusiveDraws = $parts[1]
            InclusiveDraws = $parts[7]
            InclusiveDispatches = $parts[8]
            InclusivePrimitives = $parts[9]
            InclusiveVertices = $parts[10]
            InclusivePercent = $parts[11]
            InclusiveTimeMs = [double]$inclusiveTime
            Event = ($eventName -replace '^\s+', '')
        }
    }

    $root = $rows | Where-Object { $_.Event -eq "<root>" } | Select-Object -First 1
    if ($root) {
        $summary.RootInclusiveTimeMs = ("{0:0.###}" -f $root.InclusiveTimeMs)
        $summary.RootInclusiveDraws = $root.InclusiveDraws
        $summary.RootInclusiveDispatches = $root.InclusiveDispatches
        $summary.RootInclusivePrimitives = $root.InclusivePrimitives
        $summary.RootInclusiveVertices = $root.InclusiveVertices
    }

    $top = $rows |
        Where-Object { $_.Event -ne "<root>" } |
        Sort-Object InclusiveTimeMs -Descending |
        Select-Object -First 1
    if ($top) {
        $summary.TopEvent = $top.Event
        $summary.TopEventInclusiveTimeMs = ("{0:0.###}" -f $top.InclusiveTimeMs)
        $summary.TopEventInclusivePercent = $top.InclusivePercent
    }

    return [pscustomobject]$summary
}

function Invoke-ProfilingScenario {
    param(
        [string]$Name,
        [object]$Definition,
        [string]$EditorExe,
        [string]$ScenarioOutputRoot
    )

    New-Item -ItemType Directory -Force -Path $ScenarioOutputRoot | Out-Null

    $logPath = Join-Path $ScenarioOutputRoot "$Name.log"
    $commands = @()
    $commands += $Definition.CVars
    $commands += @(
        "r.ProfileGPU.ShowUI 0",
        "r.ProfileGPU.UnicodeOutput 0",
        "r.ProfileGPU.TableFormatting 1",
        "stat unit",
        "stat rhi",
        "stat scenerendering",
        "stat gpu",
        "r.MeshDrawCommands.LogDynamicInstancingStats 1",
        "profilegpu"
    )
    if (-not $NoQuit) {
        $commands += "quit"
    }
    $execCmds = $commands -join ", "
    $startTime = Get-Date

    $arguments = @(
        "`"$uprojectPath`"",
        $Map,
        "-game",
        "-windowed",
        "-ResX=1280",
        "-ResY=720",
        "-nosplash",
        "-nop4",
        "-NoSound",
        "-unattended",
        "-log",
        "-AbsLog=`"$logPath`"",
        "-ExecCmds=`"$execCmds`""
    )

    $process = Start-Process -FilePath $EditorExe -ArgumentList $arguments -PassThru -WindowStyle Minimized
    $completed = $process.WaitForExit($TimeoutSec * 1000)
    $timedOut = -not $completed
    if ($timedOut) {
        try {
            $process.Kill()
            $process.WaitForExit(30000) | Out-Null
        }
        catch {
        }
    }

    $artifacts = Get-NewProfilingArtifacts -Since $startTime
    $logText = if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath -Raw } else { "" }
    $profileGpuMentioned = $logText -match "profilegpu|ProfileGPU|GPU profile"
    $profileGpuReportFound = $logText -match "GPU Profile for Frame"
    $meshDrawMentioned = $logText -match "MeshDrawCommands|DynamicInstancing"
    $gpuSummary = Get-GpuProfileSummary -LogText $logText
    $exitCode = if ($completed) { $process.ExitCode } else { $null }
    $status = if ($completed -and $exitCode -eq 0 -and $artifacts.Count -gt 0) {
        "Captured"
    }
    elseif ($artifacts.Count -gt 0) {
        "CapturedWithForcedExit"
    }
    elseif ($profileGpuReportFound) {
        "ParsedLogCaptured"
    }
    elseif ($completed -and $exitCode -eq 0 -and $profileGpuMentioned) {
        "LogCaptured"
    }
    elseif ($timedOut) {
        "TimedOut"
    }
    else {
        "Incomplete"
    }

    return [pscustomobject]@{
        Name = $Name
        Tier = $Definition.Tier
        RequiresBatchProxy = $Definition.RequiresBatchProxy
        Status = $status
        ExitCode = $exitCode
        TimedOut = $timedOut
        LogPath = $logPath
        ProfileGpuMentioned = $profileGpuMentioned
        ProfileGpuReportFound = $profileGpuReportFound
        MeshDrawMentioned = $meshDrawMentioned
        GpuSummary = $gpuSummary
        ArtifactPaths = @($artifacts | ForEach-Object { $_.FullName })
        ExecCmds = $execCmds
    }
}

$engineRootResolved = Resolve-UE58EngineRoot -RequestedRoot $EngineRoot
$editorExe = Join-Path $engineRootResolved "Engine\Binaries\Win64\UnrealEditor.exe"
$definitions = New-ScenarioDefinitions
$unknownScenarios = @($Scenario | Where-Object { -not $definitions.ContainsKey($_) })
if ($unknownScenarios.Count -gt 0) {
    throw "Unknown scenario(s): $($unknownScenarios -join ', ')"
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$runRoot = Join-Path $OutputRoot $timestamp
$reportPath = Join-Path $OutputRoot "UE58RuntimeProfilingCapture_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"
$results = @()

if ($Run) {
    New-Item -ItemType Directory -Force -Path $runRoot | Out-Null
    foreach ($scenarioName in $Scenario) {
        $results += Invoke-ProfilingScenario -Name $scenarioName -Definition $definitions[$scenarioName] -EditorExe $editorExe -ScenarioOutputRoot (Join-Path $runRoot $scenarioName)
    }
}

$capturedCount = @($results | Where-Object { $_.Status -in @("Captured", "CapturedWithForcedExit") }).Count
$parsedLogCapturedCount = @($results | Where-Object { $_.Status -eq "ParsedLogCaptured" }).Count
$logCapturedCount = @($results | Where-Object { $_.Status -eq "LogCaptured" }).Count
$status = if (-not $Run) {
    "Prepared"
}
elseif ($capturedCount -eq $Scenario.Count) {
    "Captured"
}
elseif (($capturedCount + $parsedLogCapturedCount) -eq $Scenario.Count) {
    "ParsedLogCaptured"
}
elseif (($capturedCount + $logCapturedCount) -eq $Scenario.Count) {
    "LogCaptured"
}
elseif ($capturedCount -gt 0) {
    "Partial"
}
else {
    "Incomplete"
}

$lines = @(
    "# UE5.8 Runtime Profiling Capture",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Engine root: $engineRootResolved",
    "- Editor executable: $editorExe",
    "- Map: $Map",
    "- Run requested: $Run",
    "- NoQuit mode: $NoQuit",
    "- Status: $status",
    "- Selected scenarios: $($Scenario -join ', ')",
    "- Timeout per scenario: $TimeoutSec seconds",
    "",
    "## Scenario Results",
    "",
    "| Scenario | Tier | Requires Batch Proxy | Status | ExitCode | TimedOut | Frame Time ms | Root Incl ms | Root Draws | Root Dispatches | Root Prims | Root Verts | Top Incl Event | Top Incl ms | ProfileGPU Report | MeshDraw Mentioned | Log | Artifacts |",
    "| --- | --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | --- | --- | --- | ---: |"
)

if ($results.Count -gt 0) {
    foreach ($result in $results) {
        $relativeLog = ConvertTo-RelativePath -Path $result.LogPath
        $summary = $result.GpuSummary
        $topEvent = if ($summary.TopEvent) { $summary.TopEvent.Replace("|", "/") } else { "" }
        $lines += "| ``$($result.Name)`` | $($result.Tier) | $($result.RequiresBatchProxy) | $($result.Status) | $($result.ExitCode) | $($result.TimedOut) | $($summary.FrameTimeMs) | $($summary.RootInclusiveTimeMs) | $($summary.RootInclusiveDraws) | $($summary.RootInclusiveDispatches) | $($summary.RootInclusivePrimitives) | $($summary.RootInclusiveVertices) | $topEvent | $($summary.TopEventInclusiveTimeMs) | $($result.ProfileGpuReportFound) | $($result.MeshDrawMentioned) | ``$relativeLog`` | $($result.ArtifactPaths.Count) |"
    }
}
else {
    foreach ($scenarioName in $Scenario) {
        $definition = $definitions[$scenarioName]
        $lines += "| ``$scenarioName`` | $($definition.Tier) | $($definition.RequiresBatchProxy) | Prepared |  | False |  |  |  |  |  |  |  |  | False | False | (not run) | 0 |"
    }
}

$lines += @(
    "",
    "## Executed Commands",
    ""
)

if ($results.Count -gt 0) {
    foreach ($result in $results) {
        $lines += "### $($result.Name)"
        $lines += ""
        $lines += '```text'
        $lines += $result.ExecCmds
        $lines += '```'
        if ($result.ArtifactPaths.Count -gt 0) {
            $lines += ""
            $lines += "Artifacts:"
            foreach ($artifact in $result.ArtifactPaths) {
                $lines += ("- ``{0}``" -f (ConvertTo-RelativePath -Path $artifact))
            }
        }
        $lines += ""
    }
}
else {
    foreach ($scenarioName in $Scenario) {
        $definition = $definitions[$scenarioName]
        $commands = @()
        $commands += $definition.CVars
        $commands += @("r.ProfileGPU.ShowUI 0", "r.ProfileGPU.UnicodeOutput 0", "r.ProfileGPU.TableFormatting 1", "stat unit", "stat rhi", "stat scenerendering", "stat gpu", "r.MeshDrawCommands.LogDynamicInstancingStats 1", "profilegpu")
        if (-not $NoQuit) {
            $commands += "quit"
        }
        $lines += "### $scenarioName"
        $lines += ""
        $lines += '```text'
        $lines += ($commands -join ", ")
        $lines += '```'
        $lines += ""
    }
}

$lines += @(
    "## Acceptance Notes",
    "",
    "- This script is the Codex automation entry point for runtime profiling capture.",
    "- ``Status: Prepared`` only proves the command harness is ready; it is not performance evidence.",
    "- ``Status: LogCaptured`` means the UE process exited and logs show the profiling commands were invoked, but no ProfileGPU artifact was found.",
    "- ``Status: ParsedLogCaptured`` means the UE process produced a ``GPU Profile for Frame`` log table and this report parsed frame/root/top-pass metrics, but no separate artifact file.",
    "- ``Status: Captured`` requires at least one generated profiling artifact in addition to a successful UE process exit.",
    "- Final handheld decisions still require reviewing the captured logs/artifacts and comparing at least baseline versus Lumen Lite scenarios."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote runtime profiling capture report: $reportPath"
Write-Output "Updated latest runtime profiling capture: $latestPath"
Write-Output "Status: $status"

if ($Run -and $status -eq "Incomplete") {
    exit 1
}
