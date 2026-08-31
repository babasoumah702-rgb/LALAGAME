param(
    [Parameter(Mandatory=$true)][string]$RuntimeLabel,
    [string]$ArchiveLabel=$RuntimeLabel
)
$ErrorActionPreference='Stop'
[Console]::OutputEncoding=[System.Text.Encoding]::UTF8
$OutputEncoding=[System.Text.Encoding]::UTF8
foreach($value in @($RuntimeLabel,$ArchiveLabel)){if($value -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Label 仅支持字母、数字、下划线和横线。'}}
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$runtime=(Resolve-Path -LiteralPath (Join-Path $project ('Builds\FullNight-Windows-'+$RuntimeLabel))).Path
$archive=Join-Path $project ('Builds\LALAGAME-Windows-'+$ArchiveLabel+'.zip')
if(Test-Path -LiteralPath $archive){throw 'ZIP 已存在；不会覆盖，请换一个 ArchiveLabel。'}

& (Join-Path $runtime 'Server\node.exe') (Join-Path $PSScriptRoot 'Audit-FullNight.mjs') $runtime
if($LASTEXITCODE -ne 0){throw '运行目录密钥、存档与调试产物排除检查失败。'}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($runtime,$archive,[System.IO.Compression.CompressionLevel]::Optimal,$true)
$runtimeName=Split-Path -Leaf $runtime
$zip=[System.IO.Compression.ZipFile]::OpenRead($archive)
$verified=0
try {
    foreach($entry in $zip.Entries){
        if(!$entry.Name){continue}
        $prefix=$runtimeName+'/'
        if(!$entry.FullName.StartsWith($prefix,[System.StringComparison]::Ordinal)){throw 'ZIP 根目录结构异常：'+$entry.FullName}
        $relative=$entry.FullName.Substring($prefix.Length).Replace('/','\')
        $file=Join-Path $runtime $relative
        if(!(Test-Path -LiteralPath $file)){throw 'ZIP 中出现运行目录外文件：'+$entry.FullName}
        $stream=$entry.Open();$sha=[System.Security.Cryptography.SHA256]::Create()
        try {$entryHash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
        if($entryHash -ne (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash){throw 'ZIP 文件校验失败：'+$relative}
        $verified++
    }
} finally {$zip.Dispose()}
$expected=(Get-ChildItem -LiteralPath $runtime -File -Recurse).Count
if($verified -ne $expected){throw "ZIP 文件数量不一致：$verified / $expected"}
$hash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower()
Set-Content -LiteralPath ($archive+'.sha256') -Value ($hash+'  '+[System.IO.Path]::GetFileName($archive)) -Encoding utf8
$result=[ordered]@{passed=$true;runtime=$runtime;archive=$archive;sha256=$hash;verifiedFiles=$verified;bytes=(Get-Item -LiteralPath $archive).Length;rootFolder=$runtimeName}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $project 'Verification\player-zip-delivery.json') -Encoding utf8
$result | ConvertTo-Json
