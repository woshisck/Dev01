param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$Map = "/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01",
    [string]$Cluster = "Prison_S_01_SourceProxy",
    [string]$Tier = "Mid",
    [string]$TextureBackend = "VTAtlas",
    [string]$SurfaceKind = "MixedStatic",
    [string]$BakePolicy = "StaticBake",
    [string]$RequireTag = "EnvBatch.",
    [int]$MaxActors = 2000
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\PilotCluster"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

function Convert-LongPackageNameToContentPath {
    param([string]$PackageName)

    if (-not $PackageName.StartsWith("/Game/")) {
        return ""
    }

    $relative = $PackageName.Substring("/Game/".Length).Replace("/", "\")
    return Join-Path $RepoRoot ("Content\" + $relative + ".umap")
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

function Get-MarkdownBulletValue {
    param(
        [string]$Text,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $pattern = "(?m)^- $([regex]::Escape($Label)): (.+)$"
    if ($Text -match $pattern) {
        return $Matches[1].Trim()
    }

    return ""
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58PilotCluster_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$mapContentPath = Convert-LongPackageNameToContentPath -PackageName $Map
$mapExists = -not [string]::IsNullOrWhiteSpace($mapContentPath) -and (Test-Path -LiteralPath $mapContentPath)
$targetMaterialPath = Join-Path $RepoRoot "Content\Art\Material\EnvMaterial\Main\M_Env_Building.uasset"
$targetMaterialExists = Test-Path -LiteralPath $targetMaterialPath
$dryRunScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58MaterialBatchDryRun.ps1"
$dryRunLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\MaterialBatchDryRun\LATEST.md"
$envBatchTagToolsLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\EnvBatchTagTools\LATEST.md"
$materialMcpAuditLatestPath = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\UE58MaterialMcpAudit_LATEST.md"

$dryRunScriptText = if (Test-Path -LiteralPath $dryRunScriptPath) { Get-Content -LiteralPath $dryRunScriptPath -Raw } else { "" }
$dryRunLatestText = if (Test-Path -LiteralPath $dryRunLatestPath) { Get-Content -LiteralPath $dryRunLatestPath -Raw } else { "" }
$envBatchTagToolsText = if (Test-Path -LiteralPath $envBatchTagToolsLatestPath) { Get-Content -LiteralPath $envBatchTagToolsLatestPath -Raw } else { "" }
$materialMcpAuditText = if (Test-Path -LiteralPath $materialMcpAuditLatestPath) { Get-Content -LiteralPath $materialMcpAuditLatestPath -Raw } else { "" }

$dryRunScriptAligned = (
    $dryRunScriptText -match [regex]::Escape($Map) -and
    $dryRunScriptText -match [regex]::Escape($Cluster) -and
    $dryRunScriptText -match "TextureBackend.*VTAtlas" -and
    $dryRunScriptText -match "SurfaceKind.*MixedStatic" -and
    $dryRunScriptText -match "BakePolicy.*StaticBake" -and
    $dryRunScriptText -match "RequireTag.*EnvBatch\."
)

$dryRunLatestAligned = (
    $dryRunLatestText -match [regex]::Escape("- Map: $Map") -and
    $dryRunLatestText -match [regex]::Escape("- Cluster: $Cluster") -and
    $dryRunLatestText -match [regex]::Escape("- Tier: $Tier") -and
    $dryRunLatestText -match [regex]::Escape("- Texture backend: $TextureBackend")
)

$envBatchToolReady = (
    $envBatchTagToolsText -match "(?m)^- Status: Passed\s*$" -and
    $envBatchTagToolsText -match "EnvBatch\.Baked\.Ground\.Mid" -and
    $envBatchTagToolsText -match "EnvBatch\.Baked\.Wall\.Low"
)

$materialMcpReady = (
    $materialMcpAuditText -match "(?m)^- Material: ``/Game/Art/Material/EnvMaterial/Main/M_Env_Building\.M_Env_Building``" -and
    $materialMcpAuditText -match "(?m)^- Required batch parameters present: True\s*$" -and
    $materialMcpAuditText -match "(?m)^- MaterialAttributes connected: True\s*$"
)

$dryRunStatus = Get-MarkdownBulletValue -Text $dryRunLatestText -Label "Status"
$actualLayerEvidence = Get-MarkdownBulletValue -Text $dryRunLatestText -Label "Actual layer evidence"
$sourceProxyAssetEvidence = Get-MarkdownBulletValue -Text $dryRunLatestText -Label "Source/Proxy asset evidence"
$residencyRiskEvidence = Get-MarkdownBulletValue -Text $dryRunLatestText -Label "Residency risk evidence"

$recommendedTags = @(
    "EnvBatch.Source.Prison_S_01",
    "EnvBatch.Proxy.Prison_S_01.Mid",
    "EnvBatch.Proxy.Prison_S_01.Low",
    "EnvBatch.Baked.Ground.Mid",
    "EnvBatch.Baked.Ground.Low",
    "EnvBatch.Baked.Wall.Mid",
    "EnvBatch.Baked.Wall.Low",
    "EnvBatch.BakeStaticDecal.Prison_S_01",
    "EnvBatch.RuntimeDecal",
    "EnvBatch.Exclude"
)

$readyForDryRunCommandlet = (
    $mapExists -and
    $targetMaterialExists -and
    $dryRunScriptAligned -and
    $dryRunLatestAligned -and
    $envBatchToolReady -and
    $materialMcpReady
)

$actorTagEvidenceCaptured = (
    $dryRunStatus -eq "DryRunCaptured" -and
    $actualLayerEvidence -eq "True" -and
    $sourceProxyAssetEvidence -eq "True" -and
    $residencyRiskEvidence -eq "True"
)

$status = if ($actorTagEvidenceCaptured) {
    "DryRunEvidenceCaptured"
}
elseif ($readyForDryRunCommandlet) {
    "ReadyForCommandlet"
}
else {
    "NeedsSetup"
}

$plannedBuildCommand = "BuildScripts\Automation\Invoke-UE58MaterialBatchDryRun.ps1 -RepoRoot `"$RepoRoot`" -Map=$Map -Cluster=$Cluster -Tier=$Tier -TextureBackend=$TextureBackend -SurfaceKind=$SurfaceKind -BakePolicy=$BakePolicy -RequireTag=$RequireTag -MaxActors=$MaxActors -Run"

$lines = @(
    "# UE5.8 Pilot Cluster Selection",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- Status: $status",
    "- Ready for commandlet: $readyForDryRunCommandlet",
    "- Actor tag evidence captured: $actorTagEvidenceCaptured",
    "- Map: $Map",
    "- Map file: $(ConvertTo-RelativePath -Path $mapContentPath)",
    "- Map file exists: $mapExists",
    "- Cluster: $Cluster",
    "- Tier: $Tier",
    "- Texture backend: $TextureBackend",
    "- Surface kind: $SurfaceKind",
    "- Bake policy: $BakePolicy",
    "- Require tag: $RequireTag",
    "- Max actors: $MaxActors",
    "- Target material: Content\Art\Material\EnvMaterial\Main\M_Env_Building.uasset",
    "- Target material exists: $targetMaterialExists",
    "- Material MCP ready: $materialMcpReady",
    "- EnvBatch tool ready: $envBatchToolReady",
    "- Dry-run script aligned: $dryRunScriptAligned",
    "- Dry-run latest aligned: $dryRunLatestAligned",
    "- Dry-run latest status: $dryRunStatus",
    "- Dry-run latest actual layer evidence: $actualLayerEvidence",
    "- Dry-run latest Source/Proxy asset evidence: $sourceProxyAssetEvidence",
    "- Dry-run latest residency evidence: $residencyRiskEvidence",
    "",
    "## Selection Rationale",
    "",
    "- This is the first pilot cluster for the UE5.8 Epic/High/Mid/Low model/material performance-tier workflow.",
    "- The target map is a small Prison room/section map under `GameLevel_L1`, suitable for validating Source/Proxy/Baked layers without taking on a full level.",
    "- The default tier is `Mid` because Mid is the recommended baseline tier and should exercise Proxy/Baked plus VT Atlas paths before Low forces more aggressive baked usage.",
    "- The surface kind is `MixedStatic` because this pilot must cover building, ground, wall, and static decal cases instead of a material-only probe.",
    "- The run requires `EnvBatch.` tags so characters, interactive objects, VFX, gameplay dynamic actors, and unreviewed dynamic material objects stay outside the first static batch.",
    "",
    "## Required EnvBatch Tags",
    "",
    "| Purpose | Tag | Notes |",
    "| --- | --- | --- |",
    "| Source display/edit fallback | `EnvBatch.Source.Prison_S_01` | Source defaults to LOD0 and remains the Epic/High comparison path. |",
    "| Generated proxy Mid | `EnvBatch.Proxy.Prison_S_01.Mid` | Proxy defaults to generated LOD1 fallback unless an optional explicit proxy is configured for a special model. |",
    "| Generated proxy Low | `EnvBatch.Proxy.Prison_S_01.Low` | Low should prefer proxy/baked usage after validation. |",
    "| Ground static bake Mid | `EnvBatch.Baked.Ground.Mid` | Ground mesh paint, height blend, and static decals bake before runtime RVT/overlay. |",
    "| Ground static bake Low | `EnvBatch.Baked.Ground.Low` | Low prioritizes baked ground and disables nonessential dynamic overlay. |",
    "| Wall static bake Mid | `EnvBatch.Baked.Wall.Mid` | Wall blend/decal results bake into wall atlas or baked replacement instead of default RVT. |",
    "| Wall static bake Low | `EnvBatch.Baked.Wall.Low` | Low prioritizes wall baked output. |",
    "| Static decal bake | `EnvBatch.BakeStaticDecal.Prison_S_01` | Static decals may enter bake outputs; runtime decals must remain separate. |",
    "| Runtime decal exclusion | `EnvBatch.RuntimeDecal` | Runtime decals are not baked into the static batch. |",
    "| Explicit exclusion | `EnvBatch.Exclude` | Characters, interactive objects, gameplay dynamic actors, and VFX stay out of this pilot. |",
    "",
    "## Recommended Tag Set",
    ""
)

foreach ($tag in $recommendedTags) {
    $lines += "- ``$tag``"
}

$lines += @(
    "",
    "## Planned Dry-Run Command",
    "",
    '```powershell',
    $plannedBuildCommand,
    '```',
    "",
    "## Evidence Contract",
    "",
    "| Evidence | Current | Required For Completion |",
    "| --- | --- | --- |",
    "| Map file exists | $mapExists | True |",
    "| Target material exists | $targetMaterialExists | True |",
    "| Material MCP ready | $materialMcpReady | True |",
    "| EnvBatch tool ready | $envBatchToolReady | True |",
    "| Dry-run script aligned | $dryRunScriptAligned | True |",
    "| Dry-run latest aligned | $dryRunLatestAligned | True |",
    "| Actor tag/StreamingLevel evidence captured | $actorTagEvidenceCaptured | True before WP12 completion |",
    "",
    "## Notes",
    "",
    "- This report is generated without launching Unreal and does not modify any asset.",
    "- `Status: ReadyForCommandlet` is not the same as `DryRunCaptured`; it only means the selected pilot cluster is ready for a commandlet run after a clean link.",
    "- `Status: DryRunEvidenceCaptured` requires the MaterialBatch dry-run latest report to show actual StreamingLevel layer, Source/Proxy asset, and residency evidence.",
    "- If the default pilot cluster changes, update this script and `Invoke-UE58MaterialBatchDryRun.ps1` together so WP1 and WP12 continue to target the same map and cluster."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote pilot cluster report: $reportPath"
Write-Output "Updated latest pilot cluster report: $latestPath"
Write-Output "Status: $status"
