[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\Epic Games\UE_5.8',
    [string]$Output = '',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $projectRoot
$runtimeProjectRoot = $projectRoot
if ($projectRoot -match '[^\x00-\x7F]') {
    $runtimeProjectRoot = 'D:\LalalandUE'
    if (-not (Test-Path -LiteralPath $runtimeProjectRoot)) {
        New-Item -ItemType Junction -Path $runtimeProjectRoot -Target $projectRoot | Out-Null
    }
}
$project = Join-Path $runtimeProjectRoot 'LalalandUnreal.uproject'
$serverSource = Join-Path $repoRoot 'BarPrototype\Server'
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path -LiteralPath $uat)) { throw "Unreal Engine 5.8 is not installed at $EngineRoot" }
if (-not $Output) { $Output = 'D:\LalalandBuilds\Windows' }
New-Item -ItemType Directory -Force -Path 'D:\UnrealCache' | Out-Null
${env:UE-LocalDataCachePath} = 'D:\UnrealCache'

Push-Location $serverSource
try {
    npm run build
    if ($LASTEXITCODE -ne 0) { throw 'Server build failed' }
    if (-not $SkipTests) {
        npm test
        if ($LASTEXITCODE -ne 0) { throw 'Server tests failed' }
    }
} finally { Pop-Location }

& (Join-Path $PSScriptRoot 'Stage-LalalandServer.ps1') -ProjectRoot $projectRoot -RepoRoot $repoRoot
if ($LASTEXITCODE -ne 0) { throw "Server staging failed: $LASTEXITCODE" }

& $uat BuildCookRun -project="$project" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive -archivedirectory="$Output" -prereqs -utf8output
if ($LASTEXITCODE -ne 0) { throw "Unreal Windows build failed: $LASTEXITCODE" }
$bootstrap = Get-ChildItem -LiteralPath $Output -Recurse -Filter 'LalalandUnreal.exe' | Sort-Object FullName | Select-Object -First 1
if ($bootstrap) { Copy-Item -LiteralPath $bootstrap.FullName -Destination (Join-Path $bootstrap.DirectoryName 'Lalaland.exe') -Force }
Get-ChildItem -LiteralPath $Output -Recurse -File -Filter '*.pdb' | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README-Windows.txt') -Destination (Join-Path $Output 'README.txt') -Force
$hashLines = @()
foreach ($relative in @('Lalaland.exe', 'LalalandUnreal\Binaries\Win64\LalalandUnreal-Win64-Shipping.exe')) {
    $artifact = Join-Path $Output $relative
    if (Test-Path -LiteralPath $artifact) {
        $hash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash.ToLowerInvariant()
        $hashLines += "$hash *$relative"
    }
}
Set-Content -LiteralPath (Join-Path $Output 'SHA256SUMS.txt') -Value $hashLines -Encoding ascii
Write-Host "Windows build complete: $Output"
