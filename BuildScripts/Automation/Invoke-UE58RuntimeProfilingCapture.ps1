param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string[]]$Scenario = @("Low_VisualSafe_LumenOff_Isolated", "Low_LumenLite_VisualSafe"),
    [int]$CsvCaptureFrames = 180,
    [int]$TimeoutSec = 300,
    [switch]$NoQuit,
    [switch]$DisableCsvCapture,
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
    $candidates += "Z:\GZA_Software\RealityCapture\UE_5.8"
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
        Tier = "Mid"
        RequiresBatchProxy = $false
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 0",
            "r.ReflectionMethod 2",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 0"
        )
    }
    $definitions["LumenLite_NoBatch"] = [pscustomobject]@{
        Tier = "Mid"
        RequiresBatchProxy = $false
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 1",
            "r.ReflectionMethod 2",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.PostProcessQuality 2",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.IrradianceFieldGather.InterpolateDownsampleFactor 2",
            "r.LumenScene.SurfaceCache.AtlasSize 2048",
            "r.LumenScene.DirectLighting.UpdateFactor 128",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 0"
        )
    }
    $definitions["BatchProxy_LumenOff"] = [pscustomobject]@{
        Tier = "Mid"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 0",
            "r.ReflectionMethod 2",
            "sg.GlobalIlluminationQuality 0",
            "sg.ReflectionQuality 1",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "t.MaxFPS 0"
        )
    }
    $definitions["BatchProxy_LumenLite"] = [pscustomobject]@{
        Tier = "Mid"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 1",
            "r.ReflectionMethod 2",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 1",
            "sg.PostProcessQuality 2",
            "sg.ShadowQuality 1",
            "r.ScreenPercentage 70",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.IrradianceFieldGather.InterpolateDownsampleFactor 2",
            "r.LumenScene.SurfaceCache.AtlasSize 2048",
            "r.LumenScene.DirectLighting.UpdateFactor 128",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "t.MaxFPS 0"
        )
    }
    $definitions["Low_VisualSafe_LumenOff_Isolated"] = [pscustomobject]@{
        Tier = "Low"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 0",
            "r.ReflectionMethod 2",
            "sg.ViewDistanceQuality 0",
            "sg.ShadowQuality 0",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 0",
            "sg.PostProcessQuality 1",
            "sg.TextureQuality 1",
            "sg.EffectsQuality 0",
            "sg.FoliageQuality 0",
            "r.ScreenPercentage 55",
            "r.Lumen.DiffuseIndirect.Allow 0",
            "r.Lumen.Reflections.Allow 0",
            "r.Yog.MaterialLightQuality 1",
            "r.Yog.MaterialLight.MaxLightInfoCount 1",
            "t.MaxFPS 0"
        )
    }
    $definitions["Low_LumenLite_VisualSafe"] = [pscustomobject]@{
        Tier = "Low"
        RequiresBatchProxy = $true
        CVars = @(
            "r.SetRes 1280x720",
            "r.DynamicGlobalIlluminationMethod 1",
            "r.ReflectionMethod 2",
            "sg.ViewDistanceQuality 0",
            "sg.ShadowQuality 0",
            "sg.GlobalIlluminationQuality 1",
            "sg.ReflectionQuality 0",
            "sg.PostProcessQuality 1",
            "sg.TextureQuality 1",
            "sg.EffectsQuality 0",
            "sg.FoliageQuality 0",
            "r.ScreenPercentage 55",
            "r.Lumen.DiffuseIndirect.Allow 1",
            "r.Lumen.FinalGatherMethod 0",
            "r.Lumen.IrradianceFieldGather.InterpolateDownsampleFactor 2",
            "r.LumenScene.SurfaceCache.AtlasSize 2048",
            "r.LumenScene.DirectLighting.UpdateFactor 128",
            "r.Lumen.TraceMeshSDFs.Allow 0",
            "r.Lumen.HardwareRayTracing.HitLighting.Allowed 0",
            "r.Lumen.Reflections.Allow 0",
            "r.Yog.MaterialLightQuality 1",
            "r.Yog.MaterialLight.MaxLightInfoCount 1",
            "t.MaxFPS 0"
        )
    }
    $definitions["Epic_LumenHigh"] = [pscustomobject]@{
        Tier = "Epic"
        RequiresBatchProxy = $false
        CVars = @(
            "r.SetRes 1920x1080",
            "r.DynamicGlobalIlluminationMethod 1",
            "r.ReflectionMethod 1",
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
                    ($_.Name -match "ProfileGPU|profilegpu|GPU|\.csv$|\.ue4stats$|\.uestats$|\.utrace$")
                }
        }
    }

    return @($files | Sort-Object LastWriteTime -Descending -Unique)
}

function Get-CsvProfilerSummary {
    param([string[]]$ArtifactPaths)

    $summary = [ordered]@{
        CsvPath = ""
        CsvRows = ""
        CsvColumns = ""
        FrameTimeAvgMs = ""
        GameThreadAvgMs = ""
        RenderThreadAvgMs = ""
        GpuAvgMs = ""
    }

    $csvPath = @($ArtifactPaths | Where-Object { $_ -match "\.csv$" } | Select-Object -First 1)
    if (-not $csvPath) {
        return [pscustomobject]$summary
    }

    $summary.CsvPath = $csvPath
    try {
        $csvLines = @(Get-Content -LiteralPath $csvPath)
        $headerIndex = -1
        for ($lineIndex = 0; $lineIndex -lt $csvLines.Count; $lineIndex++) {
            if ($csvLines[$lineIndex] -match "(^|,)FrameTime(,|$)" -and $csvLines[$lineIndex] -match "(^|,)GPUTime(,|$)") {
                $headerIndex = $lineIndex
                break
            }
        }

        if ($headerIndex -lt 0) {
            $summary.CsvRows = "ParseFailed"
            return [pscustomobject]$summary
        }

        $headers = @($csvLines[$headerIndex].Split([char]","))
        $summary.CsvColumns = [string]$headers.Count
        $dataLines = @(
            $csvLines |
                Select-Object -Skip ($headerIndex + 1) |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        $summary.CsvRows = [string]$dataLines.Count

        $metricPatterns = [ordered]@{
            FrameTimeAvgMs = @("FrameTime", "MaxFrameTime")
            GameThreadAvgMs = @("GameThreadTime", "GameThreadTime_CriticalPath", "GameThread")
            RenderThreadAvgMs = @("RenderThreadTime", "RenderThreadTime_CriticalPath", "RenderThread")
            GpuAvgMs = @("GPUTime", "GPU")
        }

        foreach ($metric in $metricPatterns.Keys) {
            $columnIndex = -1
            foreach ($pattern in $metricPatterns[$metric]) {
                for ($headerIndexCandidate = 0; $headerIndexCandidate -lt $headers.Count; $headerIndexCandidate++) {
                    if ([string]::Equals($headers[$headerIndexCandidate], $pattern, [System.StringComparison]::OrdinalIgnoreCase)) {
                        $columnIndex = $headerIndexCandidate
                        break
                    }
                }
                if ($columnIndex -ge 0) {
                    break
                }
            }
            if ($columnIndex -lt 0) {
                foreach ($pattern in $metricPatterns[$metric]) {
                    for ($headerIndexCandidate = 0; $headerIndexCandidate -lt $headers.Count; $headerIndexCandidate++) {
                        if ($headers[$headerIndexCandidate].IndexOf($pattern, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
                            $columnIndex = $headerIndexCandidate
                            break
                        }
                    }
                    if ($columnIndex -ge 0) {
                        break
                    }
                }
            }
            if ($columnIndex -lt 0) {
                continue
            }

            $values = New-Object System.Collections.Generic.List[double]
            foreach ($dataLine in $dataLines) {
                $columns = $dataLine.Split([char]",")
                if ($columnIndex -ge $columns.Count) {
                    continue
                }

                $rawValue = [string]$columns[$columnIndex]
                $numericValue = 0.0
                if ([double]::TryParse($rawValue, [System.Globalization.NumberStyles]::Float, [System.Globalization.CultureInfo]::InvariantCulture, [ref]$numericValue)) {
                    $values.Add($numericValue)
                }
            }
            if ($values.Count -gt 0) {
                $summary[$metric] = ("{0:0.###}" -f (($values | Measure-Object -Average).Average))
            }
        }
    }
    catch {
        $summary.CsvRows = "ParseFailed"
    }

    return [pscustomobject]$summary
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

function Get-RuntimeMapSummary {
    param([string]$LogText)

    $summary = [ordered]@{
        ActualSampledMap = ""
        LoadedMapSequence = ""
    }

    if ([string]::IsNullOrWhiteSpace($LogText)) {
        return [pscustomobject]$summary
    }

    $loadedMaps = New-Object System.Collections.Generic.List[string]
    $lastLoadedMap = ""
    $sampledMap = ""

    foreach ($line in ($LogText -split "`r?`n")) {
        $mapPath = ""
        if ($line -match "LoadMap\(([^)]+)\)") {
            $mapPath = $Matches[1]
        }
        elseif ($line -match "LogWorld:\s+Bringing World\s+([^\s]+)\s+up") {
            $mapPath = ($Matches[1] -replace "\.[^./]+$", "")
        }
        elseif ($line -match "LogNet:\s+Browse:\s+([^?\s]+)") {
            $mapPath = $Matches[1]
        }

        if (-not [string]::IsNullOrWhiteSpace($mapPath)) {
            if ($loadedMaps.Count -eq 0 -or $loadedMaps[$loadedMaps.Count - 1] -ne $mapPath) {
                $loadedMaps.Add($mapPath)
            }
            $lastLoadedMap = $mapPath
        }

        if ($line -match "CsvProfile STARTFILE=" -and -not [string]::IsNullOrWhiteSpace($lastLoadedMap)) {
            $sampledMap = $lastLoadedMap
        }
    }

    if ([string]::IsNullOrWhiteSpace($sampledMap)) {
        $sampledMap = $lastLoadedMap
    }

    $summary.ActualSampledMap = $sampledMap
    $summary.LoadedMapSequence = ($loadedMaps -join " -> ")
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
    if (-not $DisableCsvCapture) {
        $csvFileStem = "UE58Runtime_${Name}_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
        $commands += @(
            "CsvProfile STARTFILE=$csvFileStem",
            "CsvProfile EXITONCOMPLETION",
            "CsvProfile FRAMES=$CsvCaptureFrames"
        )
    }
    if ($DisableCsvCapture -and -not $NoQuit) {
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
        "-csvGpuStats",
        "-csvMetadata=scenario=$Name,tier=$($Definition.Tier)",
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
    $mapSummary = Get-RuntimeMapSummary -LogText $logText
    $csvSummary = Get-CsvProfilerSummary -ArtifactPaths @($artifacts | ForEach-Object { $_.FullName })
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
        MapSummary = $mapSummary
        CsvSummary = $csvSummary
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
    "- Requested launch map: $Map",
    "- Run requested: $Run",
    "- NoQuit mode: $NoQuit",
    "- CSV capture enabled: $(-not $DisableCsvCapture)",
    "- CSV capture frames: $CsvCaptureFrames",
    "- Status: $status",
    "- Selected scenarios: $($Scenario -join ', ')",
    "- Timeout per scenario: $TimeoutSec seconds",
    "",
    "## Scenario Results",
    "",
    "| Scenario | Tier | Requires Batch Proxy | Actual Sampled Map | Status | ExitCode | TimedOut | CSV Rows | CSV Frame Avg ms | CSV GPU Avg ms | Frame Time ms | Root Incl ms | Root Draws | Root Dispatches | Root Prims | Root Verts | Top Incl Event | Top Incl ms | ProfileGPU Report | MeshDraw Mentioned | Log | Artifacts |",
    "| --- | --- | --- | --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | --- | --- | --- | ---: |"
)

if ($results.Count -gt 0) {
    foreach ($result in $results) {
        $relativeLog = ConvertTo-RelativePath -Path $result.LogPath
        $summary = $result.GpuSummary
        $mapSummary = $result.MapSummary
        $csv = $result.CsvSummary
        $topEvent = if ($summary.TopEvent) { $summary.TopEvent.Replace("|", "/") } else { "" }
        $actualSampledMap = if ($mapSummary.ActualSampledMap) { $mapSummary.ActualSampledMap } else { "" }
        $lines += "| ``$($result.Name)`` | $($result.Tier) | $($result.RequiresBatchProxy) | ``$actualSampledMap`` | $($result.Status) | $($result.ExitCode) | $($result.TimedOut) | $($csv.CsvRows) | $($csv.FrameTimeAvgMs) | $($csv.GpuAvgMs) | $($summary.FrameTimeMs) | $($summary.RootInclusiveTimeMs) | $($summary.RootInclusiveDraws) | $($summary.RootInclusiveDispatches) | $($summary.RootInclusivePrimitives) | $($summary.RootInclusiveVertices) | $topEvent | $($summary.TopEventInclusiveTimeMs) | $($result.ProfileGpuReportFound) | $($result.MeshDrawMentioned) | ``$relativeLog`` | $($result.ArtifactPaths.Count) |"
    }
}
else {
    foreach ($scenarioName in $Scenario) {
        $definition = $definitions[$scenarioName]
        $lines += "| ``$scenarioName`` | $($definition.Tier) | $($definition.RequiresBatchProxy) |  | Prepared |  | False |  |  |  |  |  |  |  |  |  |  |  | False | False | (not run) | 0 |"
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
        if ($result.MapSummary.ActualSampledMap -or $result.MapSummary.LoadedMapSequence) {
            $lines += ""
            $lines += "Map routing:"
            $lines += "- Requested launch map: ``$Map``"
            $lines += "- Actual sampled map: ``$($result.MapSummary.ActualSampledMap)``"
            if ($result.MapSummary.LoadedMapSequence) {
                $lines += "- Loaded map sequence: ``$($result.MapSummary.LoadedMapSequence)``"
            }
        }
        if ($result.CsvSummary.CsvPath) {
            $lines += ""
            $lines += "CSV summary:"
            $lines += "- File: ``$(ConvertTo-RelativePath -Path $result.CsvSummary.CsvPath)``"
            $lines += "- Rows: $($result.CsvSummary.CsvRows)"
            $lines += "- Columns: $($result.CsvSummary.CsvColumns)"
            $lines += "- Frame avg ms: $($result.CsvSummary.FrameTimeAvgMs)"
            $lines += "- GPU avg ms: $($result.CsvSummary.GpuAvgMs)"
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
        if (-not $DisableCsvCapture) {
            $commands += @("CsvProfile STARTFILE=UE58Runtime_$scenarioName", "CsvProfile EXITONCOMPLETION", "CsvProfile FRAMES=$CsvCaptureFrames")
        }
        if ($DisableCsvCapture -and -not $NoQuit) {
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
    "- ``Status: Captured`` requires at least one generated profiling artifact such as CSV profiler output in addition to a successful UE process exit.",
    "- The requested launch map can differ from the actual sampled map when game startup logic redirects to a hub or runtime entry room.",
    "- CSV capture is the default unattended artifact path; ProfileGPU remains a supplementary path because it may not emit a table or file in ``-game -unattended`` sessions.",
    "- Final tier decisions still require reviewing the captured logs/artifacts and comparing at least baseline versus Lumen Lite scenarios."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote runtime profiling capture report: $reportPath"
Write-Output "Updated latest runtime profiling capture: $latestPath"
Write-Output "Status: $status"

if ($Run -and $status -eq "Incomplete") {
    exit 1
}
