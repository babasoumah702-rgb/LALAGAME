param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
# npm writes UTF-8; PowerShell 5.1 decodes native stdout with the OEM codepage (GBK),
# which garbles the Chinese project path and breaks Get-ChildItem -LiteralPath below.
[Console]::OutputEncoding=[System.Text.Encoding]::UTF8
$OutputEncoding=[System.Text.Encoding]::UTF8
if($Label -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Label 仅支持字母、数字、下划线和横线。'}
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$source=(Resolve-Path -LiteralPath (Join-Path $project 'Builds\Scene0-Windows')).Path
$runtime=Join-Path $project ('Builds\FullNight-Windows-'+$Label)
$archive=Join-Path $project ('Builds\LALAGAME-FullNight-Windows-'+$Label+'.zip')
if((Test-Path -LiteralPath $runtime) -or (Test-Path -LiteralPath $archive)){throw '目标已存在；不会覆盖旧运行包或 ZIP，请换一个 Label。'}
$cases=@('fullnight-native-final','fullnight-scene1-720','fullnight-scene1-1080','fullnight-scene1-800','fullnight-scene0','fullnight-cards')
foreach($case in $cases){
    $result=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$case+'\report.json')) -Raw | ConvertFrom-Json
    if(!$result.passed){throw "实机验收未通过：$case"}
}
foreach($name in @('fullnight-backend.xml','fullnight-editor.xml')){
    [xml]$xml=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$name)) -Raw
    if($xml.SelectNodes('//failure').Count -gt 0){throw "测试未通过：$name"}
}
New-Item -ItemType Directory -Path $runtime | Out-Null
foreach($name in @('LastCall.exe','LastCall_Data','MonoBleedingEdge','TuanjiePlayer.dll','TuanjieCrashHandler64.exe')){
    Copy-Item -LiteralPath (Join-Path $source $name) -Destination $runtime -Recurse
}
# Runtime whitelist: production modules only; no tests, private config, saves or diagnostics.
$server=Join-Path $runtime 'Server';New-Item -ItemType Directory -Path $server | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'Server/node.exe') -Destination $server
Copy-Item -LiteralPath (Join-Path $project 'Server/package.json') -Destination $server
Copy-Item -LiteralPath (Join-Path $project 'Server/NODE-LICENSE.txt') -Destination $server
$dist=Join-Path $server 'dist';New-Item -ItemType Directory -Path $dist | Out-Null
Get-ChildItem -LiteralPath (Join-Path $project 'Server/dist') -File | Where-Object {$_.Extension -eq '.js' -and $_.Name -notmatch '^(live-|intro-live)'} | Copy-Item -Destination $dist
$scenarios=Join-Path $server 'scenarios';New-Item -ItemType Directory -Path $scenarios | Out-Null
foreach($name in @('last_call.json','navigation.json')){Copy-Item -LiteralPath (Join-Path $project ('Server/scenarios/'+$name)) -Destination $scenarios}
function Copy-ProductionPackage([string]$From,[string]$To){
 New-Item -ItemType Directory -Force -Path $To | Out-Null
 foreach($item in Get-ChildItem -LiteralPath $From -Force){
  if($item.PSIsContainer){if($item.Name -notin @('test','tests','benchmark','benchmarks','examples','.github','docs','.git','node_modules')){Copy-ProductionPackage $item.FullName (Join-Path $To $item.Name)}}
  elseif(($item.Extension -in @('.js','.cjs','.mjs','.json','.node','.wasm','.md','.txt') -or $item.Name -match '^(LICENSE|NOTICE|COPYING)$') -and $item.Name -notmatch '^\.env'){Copy-Item -LiteralPath $item.FullName -Destination $To}
 }
}
$production=& D:\npm.cmd ls --omit=dev --parseable --all --prefix (Join-Path $project 'Server')
if($LASTEXITCODE -ne 0){throw '无法解析生产依赖'}
foreach($package in $production){$idx=$package.IndexOf('\node_modules\');if($idx -ge 0){Copy-ProductionPackage $package (Join-Path $server $package.Substring($idx+1))}}
Copy-Item -LiteralPath (Join-Path $project 'FULLNIGHT_启动说明.md') -Destination $runtime
Copy-Item -LiteralPath (Join-Path $project 'Verification\FULLNIGHT_测试与来源说明.md') -Destination $runtime
Copy-Item -LiteralPath (Join-Path $project 'Assets\LastCall\SceneZero\Audio\LICENSES.md') -Destination (Join-Path $runtime '音频许可.md')
$proof=Join-Path $runtime 'Verification';New-Item -ItemType Directory -Path $proof | Out-Null
foreach($name in @('fullnight-backend.xml','fullnight-editor.xml','fullnight-live.json','fullnight-live-attempt1.json','fullnight-story-originals.json')){Copy-Item -LiteralPath (Join-Path $project ('Verification\'+$name)) -Destination $proof}
foreach($case in $cases){
    $destination=Join-Path $proof $case;New-Item -ItemType Directory -Path $destination | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project ('Verification\'+$case)) -File | Where-Object {$_.Extension -in @('.png','.json')} | Copy-Item -Destination $destination
}
& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-FullNight.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '密钥与存档排除检查失败。'}
Copy-Item -LiteralPath (Join-Path $project 'Verification\fullnight-package-audit.json') -Destination $proof
# Smoke-test the exact whitelisted directory in a visible window before archiving.
$smoke=Join-Path $project 'Verification/fullnight-package-smoke';New-Item -ItemType Directory -Force -Path $smoke | Out-Null
$p=Start-Process -FilePath (Join-Path $runtime 'LastCall.exe') -WorkingDirectory $runtime -ArgumentList @('-screen-fullscreen','0','-screen-width','1280','-screen-height','720','-scene0Verify','-scene0Output',('"'+$smoke+'"'),'-logFile',('"'+(Join-Path $project 'Verification/fullnight-package-smoke.log')+'"')) -WindowStyle Normal -PassThru
$p.WaitForExit();if($p.ExitCode -ne 0){throw '白名单运行包启动验收失败'}
$smokeResult=Get-Content -LiteralPath (Join-Path $smoke 'report.json') -Raw | ConvertFrom-Json
if(!$smokeResult.passed){throw '白名单运行包报告未通过'}
Copy-Item -LiteralPath (Join-Path $smoke 'report.json') -Destination (Join-Path $proof 'package-smoke.json')
& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-FullNight.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '最终目录密钥与存档排除检查失败。'}
Copy-Item -LiteralPath (Join-Path $project 'Verification\fullnight-package-audit.json') -Destination $proof -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($runtime,$archive,[System.IO.Compression.CompressionLevel]::Optimal,$true)
$hash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower()
Set-Content -LiteralPath ($archive+'.sha256') -Value ($hash+'  '+[System.IO.Path]::GetFileName($archive)) -Encoding utf8
# Verify every archived file against the staging tree, not merely that ZIP creation returned success.
$zip=[System.IO.Compression.ZipFile]::OpenRead($archive);$verified=0
try {
    foreach($entry in $zip.Entries){
        if(!$entry.Name){continue}
        $relative=$entry.FullName.Substring($entry.FullName.IndexOf('/')+1).Replace('/','\')
        $file=Join-Path $runtime $relative
        $stream=$entry.Open();$sha=[System.Security.Cryptography.SHA256]::Create()
        try {$entryHash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
        if($entryHash -ne (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash){throw "归档校验失败：$relative"}
        $verified++
    }
}finally{$zip.Dispose()}
if($verified -ne (Get-ChildItem -LiteralPath $runtime -File -Recurse).Count){throw '归档文件数量与运行目录不一致'}
$result=@{passed=$true;archive=$archive;sha256=$hash;verifiedFiles=$verified;bytes=(Get-Item -LiteralPath $archive).Length}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $project 'Verification\fullnight-archive.json') -Encoding utf8
$result | ConvertTo-Json
