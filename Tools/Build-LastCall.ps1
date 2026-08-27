param([switch]$Prepare)
$ErrorActionPreference='Stop'
$project=Join-Path $PSScriptRoot '..\BarPrototype'
$project=(Resolve-Path -LiteralPath $project).Path
Push-Location (Join-Path $project 'Server')
try {
    & 'D:\npm.cmd' run build
    if($LASTEXITCODE -ne 0){throw 'Backend compilation failed'}
} finally { Pop-Location }
$method=if($Prepare){'Prepare'}else{'Build'}
$log=Join-Path $project "Verification\lastcall-$($method.ToLower()).log"
$process=Start-Process -FilePath 'D:\unity cn\Editor\Tuanjie.exe' -ArgumentList @(
    '-batchmode','-quit','-projectPath',('"{0}"' -f $project),
    '-executeMethod',("LastCall.Editor.LastCallSceneBuilder.$method"),
    '-logFile',('"{0}"' -f $log)
) -WindowStyle Hidden -PassThru
$process.WaitForExit()
if($process.ExitCode -ne 0){throw "Unity failed; inspect $log"}
Write-Output "Last Call $method succeeded"
