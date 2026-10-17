$ErrorActionPreference='Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'dev01_code_sync_common.ps1')
. (Join-Path $PSScriptRoot 'dev01_sync_review.ps1')
. (Join-Path $PSScriptRoot 'dev01_mirror_preflight.ps1') -DefineOnly
$policy=Get-Dev01SyncPolicy (Join-Path $PSScriptRoot 'dev01_code_sync_policy.json')
$script:checks=0
function Assert-Equal($Actual,$Expected,[string]$Label){$script:checks++;if($Actual -cne $Expected){throw "$Label expected [$Expected], got [$Actual]"}}
function Assert-Throws([scriptblock]$Action,[string]$Label){$script:checks++;$threw=$false;try{& $Action}catch{$threw=$true};if(-not $threw){throw "$Label did not fail closed"}}

$cases=@{
 'Content/Script/Main.lua'='shared'
 'Content/Script/Maps/RoomDirector.lua'='shared'
 'Plugins/UnLua/Content/Script/UnLua/Input.lua'='shared'
 'Plugins/UnLuaExtensions/LuaSocket/Content/Script/socket/http.lua'='shared'
 'Plugins/UnLuaTestSuite/Content/UnLuaPerformanceTestProxy.lua'='shared'
 'Content/ActionRules/Examples/README.md'='shared'
 'Content/ActionRules/Examples/player_combat.example.json'='shared'
 'Content/ActionRules/Examples/conditions.lua'='shared'
 'Plugins/DevKitLuaRules/Tests/Standalone/DevKitLuaSandboxSmoke.c'='shared'
 'Plugins/DevKitLuaRules/Tests/Standalone/Run-Smoke.ps1'='shared'
 'Plugins/DevKitLuaRules/ThirdPartyNotices/Lua-LICENSE.txt'='shared'
 'Plugins/DevKitLuaRules/ThirdPartyNotices/Lua-PROVENANCE.md'='shared'
 'Plugins/DevKitLuaRules/README.md'='shared'
 'Build/Dev01WorkflowCatalog.json'='shared'
 'Tools/ActionRules/Templates/ActionRules_Template.xlsx'='shared'
 'Plugins/Any/Resources/Icon128.png'='shared'
 'Source/DevKit/Test.cpp'='shared'
 'Config/DefaultEngine.ini'='shared'
 'Content/Code/Enemy/B_Enemy.uasset'='forbidden'
 'Content/Maps/Stage.umap'='forbidden'
 'Content/Script/bad.dll'='forbidden'
 'Content/Script/Main.txt'='forbidden'
 'Plugins/Other/Content/Script/Main.lua'='forbidden'
 'Plugins/Test/Tests/BP.uasset'='forbidden'
 'Plugins/UnLua/Source/Collector/obj/project.assets.json'='forbidden'
 'Plugins/Test/Binaries/Win64/UnrealEditor.modules'='forbidden'
 'Engine/Shaders/Private/BasePass.usf'='forbidden'
 'Build/Dev01EngineRelease.txt'='forbidden'
 '2Handed Hammer Set.zip'='forbidden'
 'UI_Reference/download (2).png'='forbidden'
 'Tools/node_modules/dependency/index.js'='forbidden'
 'Source/UnknownPayload.blob'='unknown'
 'Plugins/Test/Source/Vendor/libLua.a'='forbidden'
 'Plugins/Test/Source/Vendor/libLua.dylib'='forbidden'
 'Docs/Interactive/test.cjs'='shared'
 'BuildScripts/Workflow/.gitignore'='shared'
 'Plugins/DevKitLuaRules/Source/DevKitLuaRules/Private/ThirdParty/Lua/README.upstream'='shared'
 'Plugins/YogArt_Material/README.md'='git-only'
 'not-reviewed-root.zip'='unknown'
 'Dev01_SaveSafety_Framework.md'='git-only'
 '.github/workflows/build.yml'='git-only'
}
foreach($path in $cases.Keys){Assert-Equal (Get-Dev01PathClass $path $policy) $cases[$path] $path}
foreach($path in @('../escape','Source/../escape','C:/outside','//server/share','Source/a:stream','Source//bad')){
 Assert-Throws {ConvertTo-Dev01RelativePath $path} "path $path"
}
$a='a'*64;$b='b'*64;$c='c'*64
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $a $a) 'Converged' 'unchanged'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $b $b) 'Converged' 'same changes'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $b $a) 'ReviewGitToP4' 'git changes'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $a $b) 'ReviewP4ToGit' 'p4 changes'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $b $c) 'NeedsThreeWayMerge' 'both changed'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a '' $a) 'DeletionRequiresReview' 'git deletion'
Assert-Equal (Get-Dev01ThreeWayDecision $a $a $a '') 'DeletionRequiresReview' 'p4 deletion'
Assert-Equal (Get-Dev01ThreeWayDecision $a $b $a $b) 'PreserveReviewedBaselineDifference' 'intentional paired checkpoint'
Assert-Throws {Get-Dev01ThreeWayDecision 'bad' $a $a $b} 'fingerprint validation'
$summary=Get-Dev01SyncChangeSummary -Policy $policy -GitChanges @(
 [pscustomobject]@{Relative='Content/Script/Main.lua';Action='M'},
 [pscustomobject]@{Relative='Content/Map.umap';Action='D'}) -P4Changes @(
 [pscustomobject]@{Relative='Content/Script/Main.lua';Action='edit'})
Assert-Equal @($summary.GitShared).Count 1 'lua selected'
Assert-Equal @($summary.GitExcluded).Count 1 'asset excluded, not fatal'
Assert-Equal @($summary.GitInvalid).Count 0 'no false asset error'
Assert-Equal @($summary.Overlap).Count 1 'shared overlap'
Assert-Equal @((Get-Dev01SyncChangeSummary -Policy $policy).Overlap).Count 0 'empty sides'
$state=[pscustomobject]@{PolicySha256=$policy.previousPolicySha256}
Assert-Equal (Test-Dev01SyncPolicyMigration $policy $state ('f'*64)) 'ClassificationOnlyReviewRequired' 'old hash allows diagnostics only'
Assert-Equal $state.PolicySha256 $policy.previousPolicySha256 'ledger not modified'
Assert-Throws {Test-Dev01SyncPolicyMigration $policy ([pscustomobject]@{PolicySha256='unrecognized'}) ('f'*64)} 'unknown policy stays blocked'
$candidate=New-Dev01SyncCheckpointCandidate ('a'*40) ('b'*40) 165 ('c'*64)
Assert-Equal $candidate.ReadyToActivate $false 'candidate is not active'
Assert-Equal $candidate.RequiresPerPathByteVerification $true 'byte proof still required'
Assert-Equal (Get-Dev01MirrorDirection 1 1) 'Conflict' 'no automatic two-way overwrite'
Assert-Equal (Get-Dev01MirrorDirection 1 0) 'GitToP4Review' 'one-sided still review gated'

$fixture=Join-Path ([IO.Path]::GetTempPath()) ('dev01-sync-test-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture|Out-Null
$target=Join-Path $fixture 'Keep.txt';[IO.File]::WriteAllText($target,'user-owned')
$comparison=[pscustomobject]@{TargetOnly=@([pscustomobject]@{Relative='Keep.txt'});SourceOnly=@();Different=@()}
Assert-Throws {Apply-Dev01SharedTreeComparison $comparison $fixture} 'implicit deletion blocked'
Assert-Equal ([IO.File]::ReadAllText($target)) 'user-owned' 'target-only preserved'
Assert-Throws {Resolve-Dev01SafeChildPath $fixture '../outside.txt'} 'escape target'
$outside=Join-Path $fixture 'external';New-Item -ItemType Directory -Path $outside|Out-Null
$junction=Join-Path $fixture 'linked';New-Item -ItemType Junction -Path $junction -Target $outside|Out-Null
Assert-Throws {Resolve-Dev01SafeChildPath $fixture 'linked/file.txt'} 'junction target'

# P4 emits CP936 path bytes; Git emits UTF-8. Test decoding independently of console settings.
$unicodeText=[string][char]0x4e2d+[char]0x6587
foreach($codePage in @(936,65001)){
 $command="[Console]::OutputEncoding=[Text.Encoding]::GetEncoding($codePage);[Console]::Write([string][char]0x4e2d+[char]0x6587)"
 $encoded=[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($command))
 $read=Invoke-Dev01EncodedRead -Executable (Get-Command powershell.exe).Source -Arguments "-NoLogo -NoProfile -NonInteractive -EncodedCommand $encoded" -CodePage $codePage
 Assert-Equal $read.ExitCode 0 "native codepage $codePage exit"
 Assert-Equal $read.Lines[0] $unicodeText "native codepage $codePage path"
}

# Native-command fixture verifies newest P4 action wins, without contacting a server.
function Invoke-Dev01P4Read {
 param([string]$Arguments)
 $lines=if($Arguments -like 'changes -m*'){'Change 212 on 2026/10/18 by test@client'}
 elseif($Arguments -like 'changes*'){@('Change 212 on 2026/10/18 by test@client','Change 211 on 2026/10/17 by test@client')}
 elseif($Arguments -eq 'describe -s 212'){'... //Dev01/main/Source/Test.cpp#2 delete'}
 elseif($Arguments -like 'describe*'){'... //Dev01/main/Source/Test.cpp#1 add'}
 else{throw 'Unexpected native fixture call'}
 return [pscustomobject]@{ExitCode=0;Lines=@($lines);ErrorPresent=$false}
}
$global:LASTEXITCODE=0
$records=@(Get-Dev01P4ChangedPaths 210)
Assert-Equal $records.Count 1 'one changed path'
Assert-Equal $records[0].Change 212 'latest revision'
Assert-Equal $records[0].Action 'delete' 'newest action retained'
Write-Output "SYNC_REPAIR_TESTS_PASSED=$script:checks"
