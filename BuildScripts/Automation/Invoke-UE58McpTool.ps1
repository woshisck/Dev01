param(
    [string]$ServerUrl = "http://127.0.0.1:8765/mcp",
    [ValidateSet("ListToolsets", "DescribeToolset", "CallTool")]
    [string]$Mode = "ListToolsets",
    [string]$ToolsetName = "",
    [string]$ToolName = "",
    [string]$ArgumentsJson = "{}",
    [string]$ArgumentsJsonPath = "",
    [int]$TimeoutSec = 30
)

$ErrorActionPreference = "Stop"

function ConvertFrom-SseJson {
    param([string]$Content)

    $trimmed = $Content.Trim()
    if ($trimmed.StartsWith("event:")) {
        $dataLines = @()
        foreach ($line in ($trimmed -split "`r?`n")) {
            if ($line.StartsWith("data:")) {
                $dataLines += $line.Substring(5).Trim()
            }
        }
        return ($dataLines -join "`n")
    }

    return $Content
}

function Invoke-McpRequest {
    param(
        [hashtable]$Headers,
        [hashtable]$Payload
    )

    $body = $Payload | ConvertTo-Json -Depth 32 -Compress
    $response = Invoke-WebRequest -Uri $ServerUrl -Method Post -Headers $Headers -Body $body -UseBasicParsing -TimeoutSec $TimeoutSec
    return ConvertFrom-SseJson -Content $response.Content
}

$commonHeaders = @{
    "Content-Type" = "application/json"
    "Accept" = "application/json, text/event-stream"
}

$initializePayload = @{
    jsonrpc = "2.0"
    id = 1
    method = "initialize"
    params = @{
        protocolVersion = "2025-06-18"
        capabilities = @{}
        clientInfo = @{
            name = "devkit-ue58-automation"
            version = "1.0"
        }
    }
}

$initializeBody = $initializePayload | ConvertTo-Json -Depth 32 -Compress
$initializeResponse = Invoke-WebRequest -Uri $ServerUrl -Method Post -Headers $commonHeaders -Body $initializeBody -UseBasicParsing -TimeoutSec $TimeoutSec
$sessionId = $initializeResponse.Headers["Mcp-Session-Id"]
if ([string]::IsNullOrWhiteSpace($sessionId)) {
    throw "MCP initialize response did not include Mcp-Session-Id."
}

$sessionHeaders = @{
    "Content-Type" = "application/json"
    "Accept" = "application/json, text/event-stream"
    "Mcp-Session-Id" = $sessionId
    "Mcp-Protocol-Version" = "2025-06-18"
}

$initializedPayload = @{
    jsonrpc = "2.0"
    method = "notifications/initialized"
    params = @{}
}

try {
    Invoke-McpRequest -Headers $sessionHeaders -Payload $initializedPayload | Out-Null
}
catch {
    # Some server builds do not reply to notifications. Continue with the session.
}

switch ($Mode) {
    "ListToolsets" {
        $payload = @{
            jsonrpc = "2.0"
            id = 2
            method = "tools/call"
            params = @{
                name = "list_toolsets"
                arguments = @{}
            }
        }
        Invoke-McpRequest -Headers $sessionHeaders -Payload $payload
    }
    "DescribeToolset" {
        if ([string]::IsNullOrWhiteSpace($ToolsetName)) {
            throw "-ToolsetName is required for DescribeToolset."
        }
        $payload = @{
            jsonrpc = "2.0"
            id = 2
            method = "tools/call"
            params = @{
                name = "describe_toolset"
                arguments = @{
                    toolset_name = $ToolsetName
                }
            }
        }
        Invoke-McpRequest -Headers $sessionHeaders -Payload $payload
    }
    "CallTool" {
        if ([string]::IsNullOrWhiteSpace($ToolName)) {
            throw "-ToolName is required for CallTool."
        }

        $arguments = @{}
        $resolvedArgumentsJson = $ArgumentsJson
        if (-not [string]::IsNullOrWhiteSpace($ArgumentsJsonPath)) {
            $resolvedArgumentsJson = Get-Content -LiteralPath $ArgumentsJsonPath -Raw
        }
        if (-not [string]::IsNullOrWhiteSpace($resolvedArgumentsJson)) {
            $argumentsObject = $resolvedArgumentsJson | ConvertFrom-Json
            if ($argumentsObject) {
                $arguments = $argumentsObject
            }
        }

        $callArguments = @{
            tool_name = $ToolName
            arguments = $arguments
        }
        if (-not [string]::IsNullOrWhiteSpace($ToolsetName)) {
            $callArguments.toolset_name = $ToolsetName
        }

        $payload = @{
            jsonrpc = "2.0"
            id = 2
            method = "tools/call"
            params = @{
                name = "call_tool"
                arguments = $callArguments
            }
        }
        Invoke-McpRequest -Headers $sessionHeaders -Payload $payload
    }
}
