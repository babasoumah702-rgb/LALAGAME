$ErrorActionPreference='Stop'
$project=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
$config=Join-Path $env:LOCALAPPDATA 'LALAGAME\private\model.env'
$keyLine=if(Test-Path -LiteralPath $config){Get-Content -LiteralPath $config | Where-Object { $_.StartsWith('LASTCALL_API_KEY=') } | Select-Object -First 1}else{$null}
$privateKey=if($keyLine){$keyLine.Substring('LASTCALL_API_KEY='.Length)}else{''}
$keyScanPerformed=-not [string]::IsNullOrEmpty($privateKey)
$roots=@('Assets','Server','Builds\LastCall-Windows')
$matchesFound=New-Object System.Collections.Generic.List[string]
$checked=0
foreach($root in $roots){
    $path=Join-Path $project $root
    foreach($file in Get-ChildItem -LiteralPath $path -File -Recurse){
        if($file.FullName -match '\\node_modules\\|\\MonoBleedingEdge\\'){continue}
        if($file.Extension -notin @('.cs','.ts','.js','.json','.md','.txt','.unity','.prefab','.asset','.cmd','.ps1','.env')){continue}
        $content=[IO.File]::ReadAllText($file.FullName)
        $checked++
        if($keyScanPerformed -and $content.Contains($privateKey)){$matchesFound.Add($file.FullName)}
    }
}
$privateKey=$null
$forbidden=Get-ChildItem -LiteralPath (Join-Path $project 'Builds\LastCall-Windows') -File -Recurse |
    Where-Object { $_.Name -match '^(model\.env|.*\.db(-wal|-shm)?|.*\.sqlite.*)$' }
$result=[ordered]@{
    checkedFiles=$checked
    keyScanPerformed=$keyScanPerformed
    keyMatches=@($matchesFound.ToArray())
    forbiddenPackageFiles=@($forbidden | ForEach-Object { $_.FullName })
    passed=($matchesFound.Count -eq 0 -and @($forbidden).Count -eq 0)
}
$result | ConvertTo-Json -Depth 4
if(-not $result.passed){throw 'Private-data audit failed'}
