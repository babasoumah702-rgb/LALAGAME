param([string[]]$Cases=@('fullnight-native-final','fullnight-scene1-720','fullnight-scene1-1080','fullnight-scene1-800','fullnight-scene23','fullnight-scene0','fullnight-cards'))
$ErrorActionPreference='Stop'
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../BarPrototype')).Path
$runtime=Join-Path $project 'Builds/Scene0-Windows'
foreach($case in $Cases){
 $width=1280;$height=720
 switch($case){
  'fullnight-native-final' {$flags=@('-fullNightVerify','-fullNightClock','3');$outputFlag='-fullNightOutput';$limit=1450000}
  'fullnight-scene1-720' {$flags=@('-sceneOneVerify');$outputFlag='-sceneOneOutput';$limit=480000}
  'fullnight-scene1-1080' {$width=1920;$height=1080;$flags=@('-sceneOneVerify','-sceneOneQuick');$outputFlag='-sceneOneOutput';$limit=300000}
  'fullnight-scene1-800' {$height=800;$flags=@('-sceneOneVerify','-sceneOneQuick');$outputFlag='-sceneOneOutput';$limit=300000}
  'fullnight-scene23' {$flags=@('-sceneTwoThreeVerify','-fullNightClock','4');$outputFlag='-sceneTwoThreeOutput';$limit=480000}
  'fullnight-scene0' {$flags=@('-scene0Verify');$outputFlag='-scene0Output';$limit=300000}
  'fullnight-cards' {$flags=@('-cardPlayVerify');$outputFlag='-cardPlayOutput';$limit=300000}
  default {throw "Unknown case: $case"}
 }
 $out=Join-Path $project ('Verification/'+$case)
 New-Item -ItemType Directory -Force -Path $out | Out-Null
 $arguments=@('-screen-fullscreen','0','-screen-width',$width,'-screen-height',$height)+$flags+@($outputFlag,('"'+$out+'"'),'-logFile',('"'+(Join-Path $project ('Verification/'+$case+'.log'))+'"'))
 Write-Output ('START '+$case+' '+(Get-Date -Format o))
 $process=Start-Process -FilePath (Join-Path $runtime 'LastCall.exe') -WorkingDirectory $runtime -ArgumentList $arguments -WindowStyle Normal -PassThru
 if(!$process.WaitForExit($limit)){Stop-Process -Id $process.Id;throw "Visible test timed out: $case"}
 $report=Get-Content -LiteralPath (Join-Path $out 'report.json') -Raw | ConvertFrom-Json
 Write-Output ('RESULT '+$case+' passed='+$report.passed+' checks='+$report.checks.Count+' errors='+$report.errors.Count)
 if(!$report.passed){throw "Visible test failed: $case; inspect its report and screenshots"}
}
