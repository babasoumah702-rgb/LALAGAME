[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\Epic Games\UE_5.8',
    [switch]$ImportCharacters
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$runtimeProjectRoot = $projectRoot
if ($projectRoot -match '[^\x00-\x7F]') {
    $runtimeProjectRoot = 'D:\LalalandUE'
    if (Test-Path -LiteralPath $runtimeProjectRoot) {
        $junction = Get-Item -LiteralPath $runtimeProjectRoot -Force
        if (-not ($junction.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "$runtimeProjectRoot exists and is not a directory junction."
        }
    } else {
        New-Item -ItemType Junction -Path $runtimeProjectRoot -Target $projectRoot | Out-Null
    }
}
$project = Join-Path $runtimeProjectRoot 'LalalandUnreal.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$dotnet = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$unrealBuildTool = Join-Path $EngineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
if (-not (Test-Path -LiteralPath $editor)) { throw "Unreal Engine 5.8 is not installed at $EngineRoot" }
New-Item -ItemType Directory -Force -Path 'D:\UnrealCache' | Out-Null
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', 'D:\UnrealCache', 'Process')
if (-not (Test-Path -LiteralPath $dotnet) -or -not (Test-Path -LiteralPath $unrealBuildTool)) {
    throw 'The Unreal Build Tool runtime is missing from the installed engine.'
}
& $dotnet $unrealBuildTool -ProjectFiles "-Project=$project" -Game -Engine -Progress
if ($LASTEXITCODE -ne 0) { throw "Unreal project generation failed: $LASTEXITCODE" }

& $build LalalandUnrealEditor Win64 Development "-Project=$project" -WaitMutex
if ($LASTEXITCODE -ne 0) { throw "Unreal editor build failed: $LASTEXITCODE" }

& (Join-Path $PSScriptRoot 'Stage-LalalandServer.ps1') -ProjectRoot $projectRoot -RepoRoot (Split-Path -Parent $projectRoot)
if ($LASTEXITCODE -ne 0) { throw "Server staging failed: $LASTEXITCODE" }

if ($ImportCharacters) {
    $script = Join-Path $runtimeProjectRoot 'Tools\Import-LalalandCharacters.py'
    $scriptArgument = "-script=$script"
    & $editorCmd $project -unattended -nop4 -nosplash -run=pythonscript $scriptArgument
    if ($LASTEXITCODE -ne 0) { throw "Character import failed: $LASTEXITCODE" }
}
Write-Host "Lalaland Unreal setup complete: $project"
