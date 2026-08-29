param(
    [ValidateSet('Prepare','Test','Build')][string]$Action='Build',
    [string]$EditorPath='D:\unity cn\Editor\Tuanjie.exe'
)
$ErrorActionPreference='Stop'
$projectPath=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\BarPrototype')).Path
if(!(Test-Path -LiteralPath $EditorPath)){throw '找不到编辑器，请用 -EditorPath 指定团结引擎 2022.3.62t14。'}
Push-Location (Join-Path $projectPath 'Server')
try {
    $npmPath=(Get-Command npm.cmd -ErrorAction SilentlyContinue).Source
    if(!$npmPath -and (Test-Path -LiteralPath 'D:\npm.cmd')){$npmPath='D:\npm.cmd'}
    if(!$npmPath){throw '请安装 Node 24.12.x。'}
    & $npmPath ci --ignore-scripts
    if($LASTEXITCODE -ne 0){throw '依赖安装失败'}
    & $npmPath run build
    if($LASTEXITCODE -ne 0){throw '后端编译失败'}
} finally {Pop-Location}
$logPath=Join-Path $projectPath ("Verification\scene0-"+$Action.ToLower()+'.log')
$arguments=@('-batchmode','-projectPath',('"'+$projectPath+'"'),'-logFile',('"'+$logPath+'"'))
if($Action -eq 'Test'){
    $arguments+=@('-runTests','-testPlatform','EditMode','-testResults',('"'+(Join-Path $projectPath 'Verification\scene0-editmode.xml')+'"'))
}else{$arguments+=@('-quit','-executeMethod',("LastCall.Editor.SceneZeroBuilder.$Action"))}
$process=Start-Process -FilePath $EditorPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
$process.WaitForExit()
if($process.ExitCode -ne 0){throw "Scene0 $Action 失败，请查看 $logPath"}
Write-Output "Scene0 $Action 完成"
