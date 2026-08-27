$ErrorActionPreference='Stop'
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$build=Join-Path $project 'Builds\LastCall-Windows'
& (Join-Path $PSScriptRoot 'Audit-LastCall.ps1')
Copy-Item -LiteralPath (Join-Path $project 'LASTCALL_启动说明.md') -Destination (Join-Path $build '启动说明.md')
$reports=Join-Path $build 'Verification'
New-Item -Path $reports -ItemType Directory -Force | Out-Null
foreach($name in @('LASTCALL_测试报告.md','lastcall-backend.xml','lastcall-editmode.xml','lastcall-online-night.json','lastcall-offline-night.json','lastcall-degraded-night.json')){
    Copy-Item -LiteralPath (Join-Path $project "Verification\$name") -Destination $reports
}
foreach($name in @('lastcall-offline','lastcall-online-16x10','lastcall-online-final','lastcall-full-night')){
    $target=Join-Path $reports $name
    New-Item -Path $target -ItemType Directory -Force | Out-Null
    foreach($file in @('report.json','entry.png','scene.png','reflection.png')){
        $source=Join-Path $project "Verification\$name\$file"
        if(Test-Path -LiteralPath $source){Copy-Item -LiteralPath $source -Destination $target}
    }
}
$files=Get-ChildItem -LiteralPath $build | Where-Object {
    $_.Name -notmatch 'DoNotShip|\.log$|\.db$|\.env$'
}
$destination=Join-Path $project 'Builds\LALAGAME-LastCall-v0.2-Windows.zip'
Compress-Archive -LiteralPath $files.FullName -DestinationPath $destination -CompressionLevel Optimal -Force
Get-FileHash -LiteralPath $destination -Algorithm SHA256
