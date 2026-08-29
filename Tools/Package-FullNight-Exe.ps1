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
if(Test-Path -LiteralPath $runtime){throw '目标已存在；不会覆盖旧运行目录，请换一个 Label。'}

$cases=@('fullnight-native-final','fullnight-scene1-720','fullnight-scene1-1080','fullnight-scene1-800','fullnight-scene23','fullnight-scene0','fullnight-cards')
foreach($case in $cases){
    $result=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$case+'\report.json')) -Raw | ConvertFrom-Json
    if(!$result.passed){throw "实机验收未通过：$case"}
}
$backend=Get-Content -LiteralPath (Join-Path $project 'Verification\interaction-backend-final.txt') -Raw
if($backend -notmatch 'tests 153' -or $backend -notmatch 'pass 153' -or $backend -notmatch 'fail 0'){throw '服务端 153 项测试未通过'}
foreach($name in @('interaction-editmode-final.xml','humanoid-all-editmode.xml')){
    [xml]$xml=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$name)) -Raw
    if($xml.SelectNodes('//failure').Count -gt 0){throw "测试未通过：$name"}
}

New-Item -ItemType Directory -Path $runtime | Out-Null
foreach($name in @('LastCall.exe','LastCall_Data','MonoBleedingEdge','TuanjiePlayer.dll','TuanjieCrashHandler64.exe')){
    Copy-Item -LiteralPath (Join-Path $source $name) -Destination $runtime -Recurse
}

# Runtime whitelist: production modules only; no tests, private config, saves or diagnostics.
$server=Join-Path $runtime 'Server'
New-Item -ItemType Directory -Path $server | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'Server/node.exe') -Destination $server
Copy-Item -LiteralPath (Join-Path $project 'Server/package.json') -Destination $server
Copy-Item -LiteralPath (Join-Path $project 'Server/NODE-LICENSE.txt') -Destination $server

$dist=Join-Path $server 'dist'
New-Item -ItemType Directory -Path $dist | Out-Null
Get-ChildItem -LiteralPath (Join-Path $project 'Server/dist') -File |
    Where-Object {$_.Extension -eq '.js' -and $_.Name -notmatch '^(live-|intro-live)'} |
    Copy-Item -Destination $dist

$scenarios=Join-Path $server 'scenarios'
New-Item -ItemType Directory -Path $scenarios | Out-Null
foreach($name in @('last_call.json','navigation.json')){
    Copy-Item -LiteralPath (Join-Path $project ('Server/scenarios/'+$name)) -Destination $scenarios
}

function Copy-ProductionPackage([string]$From,[string]$To){
    New-Item -ItemType Directory -Force -Path $To | Out-Null
    foreach($item in Get-ChildItem -LiteralPath $From -Force){
        if($item.PSIsContainer){
            if($item.Name -notin @('test','tests','benchmark','benchmarks','examples','.github','docs','.git','node_modules')){
                Copy-ProductionPackage $item.FullName (Join-Path $To $item.Name)
            }
        } elseif(($item.Extension -in @('.js','.cjs','.mjs','.json','.node','.wasm','.md','.txt') -or $item.Name -match '^(LICENSE|NOTICE|COPYING)$') -and $item.Name -notmatch '^\.env'){
            Copy-Item -LiteralPath $item.FullName -Destination $To
        }
    }
}

$production=& D:\npm.cmd ls --omit=dev --parseable --all --prefix (Join-Path $project 'Server')
if($LASTEXITCODE -ne 0){throw '无法解析生产依赖'}
foreach($package in $production){
    # Locate deps by the ASCII '\node_modules\' marker, not the Chinese project path:
    # native-command output and Resolve-Path can disagree on non-ASCII prefix encoding.
    $idx=$package.IndexOf('\node_modules\')
    if($idx -ge 0){Copy-ProductionPackage $package (Join-Path $server $package.Substring($idx+1))}
}

Copy-Item -LiteralPath (Join-Path $project 'FULLNIGHT_启动说明.md') -Destination $runtime
Copy-Item -LiteralPath (Join-Path $project 'Assets\LastCall\SceneZero\Audio\LICENSES.md') -Destination (Join-Path $runtime '音频许可.md')

& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-FullNight.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '密钥、存档与调试产物排除检查失败。'}

# Smoke-test the exact whitelisted directory in a visible window.
$smoke=Join-Path $project ('Verification\fullnight-exe-smoke-'+$Label)
New-Item -ItemType Directory -Path $smoke | Out-Null
$smokeLog=Join-Path $project ('Verification\fullnight-exe-smoke-'+$Label+'.log')
$p=Start-Process -FilePath (Join-Path $runtime 'LastCall.exe') -WorkingDirectory $runtime -ArgumentList @(
    '-screen-fullscreen','0','-screen-width','1280','-screen-height','720',
    '-scene0Verify','-scene0Output',('"'+$smoke+'"'),'-logFile',('"'+$smokeLog+'"')
) -WindowStyle Normal -PassThru
$p.WaitForExit()
if($p.ExitCode -ne 0){throw '白名单 exe 目录启动验收失败'}
$smokeResult=Get-Content -LiteralPath (Join-Path $smoke 'report.json') -Raw | ConvertFrom-Json
if(!$smokeResult.passed){throw '白名单 exe 目录报告未通过'}

& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-FullNight.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '最终目录密钥、存档与调试产物排除检查失败。'}

$exe=Join-Path $runtime 'LastCall.exe'
$hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLower()
$files=(Get-ChildItem -LiteralPath $runtime -File -Recurse).Count
$bytes=(Get-ChildItem -LiteralPath $runtime -File -Recurse | Measure-Object -Property Length -Sum).Sum
$result=[ordered]@{
    passed=$true
    runtime=$runtime
    executable=$exe
    exeSha256=$hash
    files=$files
    bytes=$bytes
    smokeReport=(Join-Path $smoke 'report.json')
    archiveCreated=$false
}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $project 'Verification\fullnight-exe-delivery.json') -Encoding utf8
$result | ConvertTo-Json
