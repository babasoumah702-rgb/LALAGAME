param([string]$ArchiveDirectory='D:/LaLaLandRelease/ue-lounge-demo-v002-20261006')
$ErrorActionPreference='Stop'
$releaseRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$releaseProject=Join-Path $releaseRoot 'UE5/LoungeGameplayIntegration_v001/LoungeGameplayIntegration_v001.uproject'
$releaseLog=Join-Path $releaseRoot 'Deliverables/Release_UE_v002_20261006'
New-Item -ItemType Directory -Path $releaseLog -Force | Out-Null
$releaseArgs=@('BuildCookRun',('-project="'+$releaseProject+'"'),'-target=LoungeGameplayIntegration_v001','-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-map=/Game/Diagnostics/GameplayIntegration/L_RecordableLounge_v002','-stage','-pak','-iostore','-compressed','-prereqs','-archive',('-archivedirectory="'+$ArchiveDirectory+'"'),'-unattended','-utf8output','-nocompileeditor','-ddc=InstalledNoZenLocalFallback','-MaxParallelActions=2')
& 'D:/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat' @releaseArgs 2>&1 | Tee-Object -FilePath (Join-Path $releaseLog 'package.log')
if($LASTEXITCODE -ne 0){throw "BuildCookRun failed: $LASTEXITCODE"}
Write-Output "PACKAGE_COMPLETE $ArchiveDirectory; NO PLAYTEST PER USER REQUEST"
