param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Label 仅支持字母、数字、下划线和横线。'}
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$source=(Resolve-Path -LiteralPath (Join-Path $project 'Builds\Scene0-Windows')).Path
$runtime=Join-Path $project ('Builds\Scene1-Windows-'+$Label)
$archive=Join-Path $project ('Builds\LALAGAME-Scene1-Windows-'+$Label+'.zip')
if((Test-Path -LiteralPath $runtime) -or (Test-Path -LiteralPath $archive)){throw '目标已存在；不会覆盖旧运行包或 ZIP，请换一个 Label。'}
$cases=@('scene1-final-720','scene1-final-1080','scene1-final-800','scene1-regression-scene0','scene1-regression-cards')
foreach($case in $cases){
    $result=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$case+'\report.json')) -Raw | ConvertFrom-Json
    if(!$result.passed){throw "实机验收未通过：$case"}
}
foreach($name in @('scene1-backend.xml','scene1-editmode.xml')){
    [xml]$xml=Get-Content -LiteralPath (Join-Path $project ('Verification\'+$name)) -Raw
    if($xml.SelectNodes('//failure').Count -gt 0){throw "测试未通过：$name"}
}
New-Item -ItemType Directory -Path $runtime | Out-Null
foreach($name in @('LastCall.exe','LastCall_Data','MonoBleedingEdge','Server','TuanjiePlayer.dll','TuanjieCrashHandler64.exe')){
    Copy-Item -LiteralPath (Join-Path $source $name) -Destination $runtime -Recurse
}
Copy-Item -LiteralPath (Join-Path $project 'SCENE1_启动说明.md') -Destination $runtime
Copy-Item -LiteralPath (Join-Path $project 'Verification\SCENE1_测试与来源说明.md') -Destination $runtime
Copy-Item -LiteralPath (Join-Path $project 'Assets\LastCall\SceneZero\Audio\LICENSES.md') -Destination (Join-Path $runtime '音频许可.md')
$proof=Join-Path $runtime 'Verification';New-Item -ItemType Directory -Path $proof | Out-Null
foreach($name in @('scene1-backend.xml','scene1-editmode.xml','scene1-live.json')){Copy-Item -LiteralPath (Join-Path $project ('Verification\'+$name)) -Destination $proof}
foreach($case in $cases){
    $destination=Join-Path $proof $case;New-Item -ItemType Directory -Path $destination | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project ('Verification\'+$case)) -File | Where-Object {$_.Extension -in @('.png','.json')} | Copy-Item -Destination $destination
}
& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-Scene1.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '密钥与存档排除检查失败。'}
Copy-Item -LiteralPath (Join-Path $project 'Verification\scene1-package-audit.json') -Destination $proof
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
$result=@{passed=$true;archive=$archive;sha256=$hash;verifiedFiles=$verified;bytes=(Get-Item -LiteralPath $archive).Length}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $project 'Verification\scene1-archive.json') -Encoding utf8
$result | ConvertTo-Json
