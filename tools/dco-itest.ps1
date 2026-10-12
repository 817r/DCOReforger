param(
    [Parameter(Mandatory = $true)][string]$World,
    [int]$TimeoutSec = 900,
    [int]$Retries = 2,
    [int]$MaxSec = 0,
    [switch]$Diag
)

$ErrorActionPreference = 'Stop'
$proj = 'D:\Git\DCOReforger\DCO_AI'
$gameDir = 'F:\SteamLibrary\steamapps\common\Arma Reforger'
$exe = Join-Path $gameDir $(if ($Diag) { 'ArmaReforgerSteamDiag.exe' } else { 'ArmaReforgerSteam.exe' })
$profileRoot = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'My Games\ArmaReforgerWorkbench'
$dcoLogs = Join-Path $profileRoot 'profile\DCO\Logs'
$runLogs = Join-Path $profileRoot 'itest_runs'
$testAi = Join-Path $profileRoot 'addons\TestAI'

$worldRel = $World.Replace('\', '/').TrimStart('/')
$meta = Get-Content -Raw (Join-Path $proj ($worldRel + '.meta'))
if ($meta -notmatch 'Name "(\{[0-9A-F]{16}\}[^"]+)"') { throw "no GUID in $worldRel.meta" }
$worldRes = $Matches[1]

function Find-Session($since) {
    Get-ChildItem $dcoLogs -Directory -Recurse -Filter 'session_*' -ErrorAction SilentlyContinue |
        Where-Object { $_.CreationTime -ge $since } | Sort-Object CreationTime |
        Where-Object {
            $st = Join-Path $_.FullName 'session.txt'
            (Test-Path $st) -and ((Get-Content -Raw $st -ErrorAction SilentlyContinue) -like "*$worldRel*")
        } | Select-Object -First 1
}

for ($attempt = 1; $attempt -le $Retries + 1; $attempt++) {
    $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
    $logDir = Join-Path $runLogs ("{0}_{1}" -f [IO.Path]::GetFileNameWithoutExtension($worldRel), $stamp)
    New-Item -ItemType Directory -Force $logDir | Out-Null
    $start = Get-Date
    $argList = @('-gproj', "`"$proj\addon.gproj`"", '-addonsDir', "`"$gameDir\addons`"", '-world', "`"$worldRes`"",
        '-profile', "`"$profileRoot`"", '-logsDir', "`"$logDir`"", '-window', '-screenWidth', '960', '-screenHeight', '540',
        '-nosplash', '-forceupdate', '-noPause', '-nofocus', '-noCrashDialog', '-dcoTest', '-dcoTestExit')
    if (Test-Path $testAi) {
        $argList[3] = "`"$gameDir\addons,$profileRoot\addons`""
        $argList += @('-addons', 'TestAI', '-taiDevCmd')
    }
    if ($MaxSec -gt 0) { $argList += @('-dcoTestMax', "$MaxSec") }
    $p = Start-Process $exe -ArgumentList $argList -WindowStyle Minimized -PassThru
    Write-Output "RUN attempt=$attempt pid=$($p.Id) world=$worldRes logs=$logDir"

    $result = 'TIMEOUT'
    $session = $null
    $selfExit = $false
    while ((Get-Date) -lt $start.AddSeconds($TimeoutSec)) {
        Start-Sleep -Seconds 5
        $console = Get-ChildItem $logDir -Recurse -Filter console.log -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($console -and (Select-String -Path $console.FullName -Pattern "Can't compile" -Quiet)) {
            $result = 'COMPILE_ERROR'
            break
        }
        if (-not $session) { $session = Find-Session $start }
        if ($session -and (Test-Path (Join-Path $session.FullName 'dco.log')) -and (Select-String -Path (Join-Path $session.FullName 'dco.log') -Pattern 'itest_done' -Quiet -ErrorAction SilentlyContinue)) {
            $result = 'DONE'
            $selfExit = $p.WaitForExit(30000)
            break
        }
        if ($p.HasExited) {
            $result = 'CRASH'
            break
        }
    }

    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    if ($result -eq 'CRASH') {
        Get-Process CrashReporter -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$gameDir*" -and $_.StartTime -ge $start } | Stop-Process -Force -ErrorAction SilentlyContinue
    }
    $exitCode = $(if ($p.HasExited) { $p.ExitCode } else { 'killed' })
    Write-Output "END attempt=$attempt result=$result self_exit=$selfExit exit=$exitCode secs=$([int]((Get-Date) - $start).TotalSeconds) session=$($session.FullName)"

    if ($result -eq 'COMPILE_ERROR') {
        Select-String -Path $console.FullName -Pattern "SCRIPT\s+\(E\): @|Can't compile" | Select-Object -First 25 | ForEach-Object { $_.Line }
        exit 2
    }
    if ($result -eq 'DONE') {
        Select-String -Path (Join-Path $session.FullName 'dco.log') -Pattern 'ITEST |ASSERT |itest_done' | ForEach-Object { $_.Line }
        exit 0
    }
}
exit 1
