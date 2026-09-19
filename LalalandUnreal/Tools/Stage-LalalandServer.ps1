[CmdletBinding()]
param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$RepoRoot = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
)

$ErrorActionPreference = 'Stop'
$serverSource = Join-Path $RepoRoot 'BarPrototype\Server'
$serverStage = Join-Path $ProjectRoot 'Content\Server'
$scriptPath = Join-Path $serverSource 'dist\server.js'
if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Built Lalaland server was not found: $scriptPath"
}

$resolvedProject = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\')
$resolvedStage = [IO.Path]::GetFullPath($serverStage).TrimEnd('\')
if (-not $resolvedStage.StartsWith($resolvedProject + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe server staging path: $resolvedStage"
}

if (Test-Path -LiteralPath $resolvedStage) {
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $resolvedStage | Out-Null
foreach ($name in @('dist', 'scenarios', 'node_modules', 'package.json', 'package-lock.json', 'NODE-LICENSE.txt')) {
    $source = Join-Path $serverSource $name
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination $resolvedStage -Recurse -Force
    }
}

$node = Get-Command node.exe -ErrorAction Stop
Copy-Item -LiteralPath $node.Source -Destination (Join-Path $resolvedStage 'node.exe') -Force
Write-Host "Lalaland local server staged: $resolvedStage"
