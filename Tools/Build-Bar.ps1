param(
    [ValidateSet('Prepare', 'Test', 'Build', 'Open', 'Smoke')]
    [string]$Action = 'Prepare',
    [string]$EditorPath = 'D:\unity cn\Editor\Tuanjie.exe'
)
$ErrorActionPreference = 'Stop'
$projectPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\BarPrototype'))
$verificationPath = Join-Path $projectPath 'Verification'
New-Item -ItemType Directory -Path $verificationPath -Force | Out-Null
if ($Action -eq 'Smoke') {
    $gamePath = Join-Path $projectPath 'Builds\Windows\AmberRoom.exe'
    if (!(Test-Path -LiteralPath $gamePath)) { throw 'Build the game first with -Action Build.' }
    # The game must be visible and focused for real keyboard and rendering verification.
    $gameArguments = @('-barSmokeTest', '-barArtifacts', "`"$verificationPath`"", '-logFile', "`"$(Join-Path $verificationPath 'player.log')`"")
    $gameProcess = Start-Process -FilePath $gamePath -ArgumentList $gameArguments -WindowStyle Normal -PassThru
    $gameProcess.WaitForExit()
    if ($gameProcess.ExitCode -ne 0) { throw 'Runtime checks failed. See Verification/smoke-report.json and player.log.' }
    $gameReport = Get-Content -LiteralPath (Join-Path $verificationPath 'smoke-report.json') -Raw | ConvertFrom-Json
    if (!$gameReport.passed) { throw 'Runtime checks did not pass.' }
    "Smoke checks passed: $($gameReport.checks.Count); average FPS: $($gameReport.averageFps)"
    return
}
if (!(Test-Path -LiteralPath $EditorPath)) {
    throw "Tuanjie 2022.3.62t14 is not installed at $EditorPath. Finish the editor installation, or pass -EditorPath with the actual Tuanjie.exe path."
}
$runningEditor = Get-CimInstance Win32_Process -Filter "Name='Unity.exe' OR Name='Tuanjie.exe'" | Where-Object { $_.CommandLine -like "*$projectPath*" }
if ($Action -ne 'Open' -and $runningEditor) { throw 'Close this project in Unity before running batch prepare/test/build.' }
if ($Action -eq 'Open') {
    & $EditorPath -projectPath $projectPath
    return
}
$logPath = Join-Path $verificationPath ($Action.ToLowerInvariant() + '.log')
$arguments = @('-batchmode', '-projectPath', "`"$projectPath`"", '-logFile', "`"$logPath`"")
switch ($Action) {
    'Prepare' { $arguments += @('-quit', '-executeMethod', 'BarPrototype.Editor.BarSceneBuilder.CreateScene') }
    'Test' { $arguments += @('-runTests', '-testPlatform', 'EditMode', '-testResults', "`"$(Join-Path $verificationPath 'editmode-results.xml')`"") }
    'Build' { $arguments += @('-quit', '-executeMethod', 'BarPrototype.Editor.BarSceneBuilder.BuildWindows') }
}
$buildProcess = Start-Process -FilePath $EditorPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
# Wait only for the editor, not persistent licensing services it may launch.
$buildProcess.WaitForExit()
Get-Content -LiteralPath $logPath | Where-Object {
    $_ -match 'AMBER_|error CS|Exception:|Error:|Build completed|Test Run|Tests completed|Exiting batchmode|return code' -and
    $_ -notmatch 'Licensing::|AuthorizationCheck|AccessToken|Machine Id:|Session Id:|Correlation Id:'
} | Select-Object -Last 20
if ($buildProcess.ExitCode -ne 0) { throw "Unity $Action failed with exit code $($buildProcess.ExitCode). See $logPath" }
Write-Output "Unity $Action completed. Log: $logPath"
