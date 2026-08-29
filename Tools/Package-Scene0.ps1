param([string]$Label='20260828')
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Label 仅支持字母、数字、下划线和横线。'}
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$runtime=(Resolve-Path -LiteralPath (Join-Path $project 'Builds\Scene0-Windows')).Path
$archive=Join-Path $project ('Builds\LALAGAME-Scene0-Windows-'+$Label+'.zip')
if(Test-Path -LiteralPath $archive){throw '同名验证包已存在，请使用新的 -Label；不会覆盖此前包。'}
foreach($name in @('LastCall.exe','LastCall_Data','MonoBleedingEdge','Server\node.exe','Server\dist\server.js')){
    if(!(Test-Path -LiteralPath (Join-Path $runtime $name))){throw "运行包缺少 $name"}
}
Copy-Item -LiteralPath (Join-Path $project 'SCENE0_启动说明.md') -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $project 'Verification\SCENE0_测试报告.md') -Destination $runtime -Force
Copy-Item -LiteralPath (Join-Path $project 'Assets\LastCall\SceneZero\Audio\LICENSES.md') -Destination (Join-Path $runtime 'SCENE0_音频许可.md') -Force
$proof=Join-Path $runtime 'Verification'
New-Item -ItemType Directory -Path $proof -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $project 'Verification\SCENE0_测试报告.md') -Destination $proof -Force
if(Test-Path -LiteralPath (Join-Path $project 'Verification\DEEPSEEK_切换说明.md')){
    Copy-Item -LiteralPath (Join-Path $project 'Verification\DEEPSEEK_切换说明.md') -Destination $runtime -Force
    Copy-Item -LiteralPath (Join-Path $project 'Verification\deepseek-backend.xml') -Destination $proof -Force
    $deepseekProof=Join-Path $proof 'deepseek-online'
    New-Item -ItemType Directory -Path $deepseekProof -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project 'Verification\deepseek-online') -File |
        Where-Object {$_.Extension -in @('.png','.json')} | Copy-Item -Destination $deepseekProof -Force
}
if(Test-Path -LiteralPath (Join-Path $project 'Verification\CARDPLAY_修复说明.md')){
    Copy-Item -LiteralPath (Join-Path $project 'Verification\CARDPLAY_修复说明.md') -Destination $runtime -Force
    foreach($name in @('card-fix-backend.xml','card-fix-editmode.xml')){
        Copy-Item -LiteralPath (Join-Path $project ('Verification\'+$name)) -Destination $proof -Force
    }
    $cardProof=Join-Path $proof 'card-play'
    New-Item -ItemType Directory -Path $cardProof -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project 'Verification\card-play') -File |
        Where-Object {$_.Extension -in @('.png','.json')} | Copy-Item -Destination $cardProof -Force
}
foreach($name in @('scene0-backend.xml','scene0-final-editmode.xml','scene0-gateway.json')){
    Copy-Item -LiteralPath (Join-Path $project ('Verification\'+$name)) -Destination $proof -Force
}
foreach($case in @('scene0-delivery-720','scene0-online-800','scene0-final-1080')){
    $destination=Join-Path $proof $case
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $project ('Verification\'+$case)) -File |
        Where-Object {$_.Extension -in @('.png','.json')} | Copy-Item -Destination $destination -Force
}
& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-Scene0.mjs')
if($LASTEXITCODE -ne 0){throw '包隐私检查未通过。'}
Copy-Item -LiteralPath (Join-Path $project 'Verification\scene0-package-audit.json') -Destination $proof -Force
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[System.IO.Compression.ZipFile]::Open($archive,[System.IO.Compression.ZipArchiveMode]::Create)
try{
    Get-ChildItem -LiteralPath $runtime -File -Recurse | Where-Object {$_.FullName -notmatch 'DoNotShip'} | ForEach-Object {
        $entry=$_.FullName.Substring($runtime.Length+1).Replace('\','/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$_.FullName,('Scene0-Windows/'+$entry),[System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}finally{$zip.Dispose()}
Get-FileHash -LiteralPath $archive -Algorithm SHA256 | Select-Object Hash,Path
