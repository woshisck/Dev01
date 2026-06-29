param(
    [string]$RepoRoot = "",
    [string]$OutputRoot = "",
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [int]$TimeoutSec = 60
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}

$RepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $RepoRoot "Docs\GeneratedReports\UE58PerformanceAutomation\BatchVisualAudit"
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null

$mcpToolScriptPath = Join-Path $RepoRoot "BuildScripts\Automation\Invoke-UE58McpTool.ps1"
if (-not (Test-Path -LiteralPath $mcpToolScriptPath)) {
    throw "Invoke-UE58McpTool.ps1 was not found: $mcpToolScriptPath"
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportPath = Join-Path $OutputRoot "UE58BatchVisualMcpAudit_$timestamp.md"
$latestPath = Join-Path $OutputRoot "LATEST.md"

$assets = @(
    [pscustomobject]@{
        Label = "Source floor mesh"
        AssetPath = "/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/SM_FloorBrick_03a1.SM_FloorBrick_03a1"
        FileName = "source_floor_mesh.png"
    },
    [pscustomobject]@{
        Label = "Source floor material instance"
        AssetPath = "/Game/Art/EnvironmentAsset/L1/Prop/FloorBrick_03/MI_FloorBrick_03.MI_FloorBrick_03"
        FileName = "source_floor_material_instance.png"
    },
    [pscustomobject]@{
        Label = "Generated batch proxy mesh"
        AssetPath = "/Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/SM_BatchProxy_FloorBrick03_Probe.SM_BatchProxy_FloorBrick03_Probe"
        FileName = "generated_batch_proxy_mesh.png"
    },
    [pscustomobject]@{
        Label = "Generated batch material instance"
        AssetPath = "/Game/Generated/MaterialBatch/Medium/FloorBrick03_Probe/MI_Env_Batch_FloorBrick03_Probe.MI_Env_Batch_FloorBrick03_Probe"
        FileName = "generated_batch_material_instance.png"
    }
)

function ConvertFrom-McpTextResult {
    param([string]$Raw)

    $outer = $Raw | ConvertFrom-Json
    $text = $outer.result.content[0].text
    if ([string]::IsNullOrWhiteSpace($text)) {
        throw "MCP response did not contain text content."
    }

    return ($text | ConvertFrom-Json)
}

function Get-ImageStats {
    param([byte[]]$Bytes)

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $hash = [System.BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }

    return [pscustomobject]@{
        ByteCount = $Bytes.Length
        Sha256 = $hash
        NonEmpty = ($Bytes.Length -gt 512)
    }
}

$rows = @()
$failures = @()

foreach ($asset in $assets) {
    $argsJson = @{ assetPath = $asset.AssetPath } | ConvertTo-Json -Compress
    $argsJsonPath = Join-Path $OutputRoot "$($asset.FileName).args.json"
    $pngPath = Join-Path $OutputRoot $asset.FileName

    try {
        Set-Content -LiteralPath $argsJsonPath -Value $argsJson -Encoding UTF8
        $raw = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $mcpToolScriptPath `
            -ServerUrl $ServerUrl `
            -Mode CallTool `
            -ToolsetName "EditorToolset.EditorAppToolset" `
            -ToolName "CaptureAssetImage" `
            -ArgumentsJsonPath $argsJsonPath `
            -TimeoutSec $TimeoutSec

        $parsed = ConvertFrom-McpTextResult -Raw ($raw -join "`n")
        $image = $parsed.returnValue
        if ([string]::IsNullOrWhiteSpace($image.data)) {
            throw "MCP returned an empty image payload."
        }

        $bytes = [Convert]::FromBase64String($image.data)
        [IO.File]::WriteAllBytes($pngPath, $bytes)
        $stats = Get-ImageStats -Bytes $bytes

        $rows += [pscustomobject]@{
            Label = $asset.Label
            AssetPath = $asset.AssetPath
            PngPath = $pngPath
            MimeType = $image.mimeType
            ByteCount = $stats.ByteCount
            Sha256 = $stats.Sha256
            NonEmpty = $stats.NonEmpty
            Status = if ($stats.NonEmpty) { "Captured" } else { "SuspiciousEmpty" }
        }
    }
    catch {
        $failures += "$($asset.Label): $($_.Exception.Message)"
        $rows += [pscustomobject]@{
            Label = $asset.Label
            AssetPath = $asset.AssetPath
            PngPath = $pngPath
            MimeType = ""
            ByteCount = 0
            Sha256 = ""
            NonEmpty = $false
            Status = "Failed"
        }
    }
}

$allCaptured = ($rows | Where-Object { $_.Status -ne "Captured" } | Measure-Object).Count -eq 0

$lines = @(
    "# UE5.8 Batch Visual MCP Audit",
    "",
    "- Time: $(Get-Date -Format o)",
    "- Repo: $RepoRoot",
    "- MCP server: $ServerUrl",
    "- Status: $(if ($allCaptured) { "Captured" } else { "Incomplete" })",
    "- Scope: asset thumbnail capture for source and generated material-batch artifacts.",
    "- Limitation: this proves UE can load and render thumbnails through MCP; final visual parity still requires side-by-side review in the target scene.",
    "",
    "## Captures",
    "",
    "| Label | Asset | Status | PNG | Bytes | SHA256 |",
    "| --- | --- | --- | --- | ---: | --- |"
)

foreach ($row in $rows) {
    $relativePng = $row.PngPath
    if ($relativePng.StartsWith($RepoRoot)) {
        $relativePng = $relativePng.Substring($RepoRoot.Length).TrimStart("\", "/")
    }
    $label = $row.Label
    $assetPath = $row.AssetPath
    $status = $row.Status
    $byteCount = $row.ByteCount
    $sha256 = $row.Sha256
    $lines += "| $label | ``$assetPath`` | $status | ``$relativePng`` | $byteCount | ``$sha256`` |"
}

if ($failures.Count -gt 0) {
    $lines += @(
        "",
        "## Failures",
        ""
    )
    foreach ($failure in $failures) {
        $lines += "- $failure"
    }
}

$lines += @(
    "",
    "## Review Gate",
    "",
    "- Open the generated PNGs side by side before production replacement.",
    "- In-scene parity must still be checked with the proxy visible and the matching source actors hidden or isolated.",
    "- If the generated batch material thumbnail is blank, distorted, or untextured, review the `M_Env_Building_Batch` graph, `TexCoord7.x` material-index path, and `_PropTexture` slice rows before enabling map replacement."
)

Set-Content -LiteralPath $reportPath -Value $lines -Encoding UTF8
Set-Content -LiteralPath $latestPath -Value $lines -Encoding UTF8

Write-Output "Wrote visual MCP audit: $reportPath"
Write-Output "Updated latest visual MCP audit: $latestPath"
if (-not $allCaptured) {
    exit 1
}
