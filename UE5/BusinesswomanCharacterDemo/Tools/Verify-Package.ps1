param(
    [string]$EditorPath
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$projectFile = Join-Path $projectRoot 'BusinesswomanCharacterDemo.uproject'
$auditScript = Join-Path $PSScriptRoot 'verify_character_package.py'
$reportPath = Join-Path $projectRoot 'Saved\CharacterPackageAudit.json'

$required = @(
    'Content\orc_character\Women_Motified_Ultimate\tripo_convert_990a7ce5-2cf1-4992-8c37-2335c7420d46.uasset',
    'Content\orc_character\Women_Motified_Ultimate\A\ABP_Unarmed.uasset',
    'Content\orc_character\Women_Motified_Ultimate\A\BS_Idle_Walk_Run.uasset',
    'Content\orc_character\Women_Motified_Ultimate\FinalGameTest\BP_BusinesswomanPlayable.uasset',
    'Content\orc_character\Women_Motified_Ultimate\FinalGameTest\BP_BusinesswomanGameMode.uasset',
    'Content\orc_character\Women_Motified_Ultimate\FinalGameTest\L_BusinesswomanGameTest.umap'
)

$missing = @($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $projectRoot $_)) })
if ($missing.Count) {
    throw "Missing required package files: $($missing -join ', ')"
}

$largeFiles = @(Get-ChildItem -LiteralPath $projectRoot -Recurse -File | Where-Object Length -ge 100MB)
if ($largeFiles.Count) {
    throw "Files at or above GitHub's 100 MB limit: $($largeFiles.FullName -join ', ')"
}

if (-not $EditorPath) {
    $repoEditor = [IO.Path]::GetFullPath((Join-Path $projectRoot '..\..\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'))
    $epicEditor = Join-Path $env:ProgramFiles 'Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (Test-Path -LiteralPath $repoEditor) { $EditorPath = $repoEditor }
    elseif (Test-Path -LiteralPath $epicEditor) { $EditorPath = $epicEditor }
}

if (-not $EditorPath -or -not (Test-Path -LiteralPath $EditorPath)) {
    throw 'UnrealEditor-Cmd.exe 5.8 was not found. Pass -EditorPath with its absolute path.'
}

$generatedNames = @('Binaries', 'DerivedDataCache', 'Intermediate', 'Saved')
$preexisting = @($generatedNames | Where-Object { Test-Path -LiteralPath (Join-Path $projectRoot $_) })
if ($preexisting.Count) {
    throw "Generated delivery directories already exist: $($preexisting -join ', '). Remove them before verification."
}

try {
    & $EditorPath $projectFile "-ExecutePythonScript=$auditScript" -unattended -nop4 -nosplash -nullrhi -NoSound
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal audit process failed with exit code $LASTEXITCODE"
    }
    if (-not (Test-Path -LiteralPath $reportPath)) {
        throw 'Unreal audit did not produce CharacterPackageAudit.json'
    }
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    if (-not $report.success) {
        throw "Unreal audit failed: $($report.errors -join '; ')"
    }
    $mapCheckLines = @(
        Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved\Logs') -Filter '*.log' -File |
            Sort-Object LastWriteTime |
            ForEach-Object { Select-String -LiteralPath $_.FullName -Pattern 'MapCheck:' }
    )
    if (-not $mapCheckLines.Count) {
        throw 'The Unreal log contains no MapCheck result.'
    }
    $mapCheckSummary = $mapCheckLines[-1].Line
    if ($mapCheckSummary -notmatch '0.*(Error|错误)') {
        throw "Map Check did not report zero errors: $mapCheckSummary"
    }
    $report.map_check | Add-Member -NotePropertyName summary -NotePropertyValue $mapCheckSummary -Force
    $report | ConvertTo-Json -Depth 8
}
finally {
    foreach ($name in $generatedNames) {
        $target = [IO.Path]::GetFullPath((Join-Path $projectRoot $name))
        if ($target.StartsWith($projectRoot, [StringComparison]::OrdinalIgnoreCase) -and (Test-Path -LiteralPath $target)) {
            [IO.Directory]::Delete($target, $true)
        }
    }
}
