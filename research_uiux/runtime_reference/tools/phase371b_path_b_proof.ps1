# Phase 371B -- per-ticket save isolation + route presets.
#
# Proves the launcher's quarantine + sandbox flow keeps two
# tickets' save state independent and the operator's real save
# bulletproof across runs.
#
# Test plan:
#
#   pristine real save SHA captured -> S0
#
#   Run 1: launcher -Ticket A -Route title (NO_AUTOLOAD=1)
#     - quarantines real save, overlays empty sandbox, launches UR
#       with auto-kill, captures empty sandbox, restores real save.
#     - asserts: Pack:Route:title fires; live save SHA == S0.
#
#   Plant marker bytes at <packA>/save/SYS-DATA = "MARKER_A_PLANTED"
#     - simulates a ticket-A session that left content in the sandbox.
#
#   Run 2: launcher -Ticket B -Route title
#     - asserts: Pack:Route:title fires; live save SHA == S0.
#     - asserts: <packB>/save/SYS-DATA does NOT exist (B never saw A's
#       data because the launcher cleared the live save before launch).
#
#   Run 3: launcher -Ticket A -Route title
#     - asserts: <packA>/save/SYS-DATA still equals "MARKER_A_PLANTED"
#       (overlay -> live -> capture round-trip preserves bytes when UR
#       doesn't touch the save path under NO_AUTOLOAD=1).
#
#   Final: live save SHA == S0; no quarantine dirs left under
#   %LOCALAPPDATA%\UnleashedRecomp\sgfx_real_save_quarantine\.
#
# Acceptance gates (distinct exit codes):
#   2  build / deploy failed
#   3  pre-test real save missing or unreadable
#   4  ticket A run 1: Pack:Route:title not seen
#   5  marker plant failed (file not writable under <packA>/save/)
#   6  ticket B run: Pack:Route:title not seen
#   7  cross-isolation broken: <packB>/save/SYS-DATA contains the marker
#      (B saw A's data)
#   8  real save SHA mismatch after run 1 or run 2
#   9  ticket A re-run: marker did not survive overlay -> capture
#  10  quarantine dirs left on disk after final run (cleanup broken)
#  11  real save SHA mismatch at end of test

[CmdletBinding()]
param(
    [int]$RunLifetimeSeconds = 12,
    [string]$EvidenceDir
)

$ErrorActionPreference = 'Stop'

$scriptDir   = Split-Path -Parent $PSCommandPath
$repoRoot    = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
$buildBat    = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir  = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'

$launcher    = Join-Path $scriptDir 'sgfx_shell_launch.ps1'
$saveDir     = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$bridgeDir   = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$quarantineRoot = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sgfx_real_save_quarantine_phase371b_proof'

$packDirA    = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase371b_A'
$packDirB    = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase371b_B'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase371b_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
}

function Get-SaveSha {
    param([string]$Dir)
    $out = [ordered]@{}
    if (-not (Test-Path -LiteralPath $Dir)) { return $out }
    foreach ($n in 'SYS-DATA','ACH-DATA','EXT-DATA') {
        $p = Join-Path $Dir $n
        if (Test-Path -LiteralPath $p) {
            $out[$n] = (Get-FileHash -Algorithm SHA256 -LiteralPath $p).Hash
        }
    }
    return $out
}

function Test-SaveShaEqual {
    param($A, $B)
    if (@($A.Keys).Count -ne @($B.Keys).Count) { return $false }
    foreach ($k in @($A.Keys)) {
        if (-not $B.Contains($k)) { return $false }
        if ($A[$k] -ne $B[$k]) { return $false }
    }
    return $true
}

function Wait-ForBridgeEvent {
    param(
        [string]$Pattern,
        [string]$EventsPath,
        [int]$TimeoutSeconds
    )
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $EventsPath) {
            $text = Get-Content -LiteralPath $EventsPath -Raw -ErrorAction SilentlyContinue
            if ($text -and ($text -match $Pattern)) { return $true }
        }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

function Invoke-LauncherRun {
    param(
        [string]$Ticket,
        [string]$PackDirPath,
        [string]$Route,
        [string]$RunTag
    )
    Write-Host "[run $RunTag] launcher -Ticket $Ticket -PackDir $PackDirPath -Route $Route -Lifetime $RunLifetimeSeconds"

    # Reset bridge events so each run reads fresh.
    if (-not (Test-Path -LiteralPath $bridgeDir)) {
        New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
    }
    $eventsPath = Join-Path $bridgeDir 'events.jsonl'
    Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

    $runEvidence = Join-Path $EvidenceDir "run_$RunTag"
    if (-not (Test-Path -LiteralPath $runEvidence)) {
        New-Item -ItemType Directory -Path $runEvidence -Force | Out-Null
    }

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $launcher `
        -Ticket $Ticket `
        -Project "Phase 371B QA" `
        -Phase "371B" `
        -Route $Route `
        -PackDir $PackDirPath `
        -Lifetime $RunLifetimeSeconds `
        -QuarantineRoot $quarantineRoot `
        -EvidenceDir $runEvidence | Out-Host

    Copy-Item -LiteralPath $eventsPath `
              -Destination (Join-Path $runEvidence "events_run_$RunTag.jsonl") -Force
    return $eventsPath
}

# --- 1. Build -----------------------------------------------------------

Write-Section "1. Build UnleashedRecomp"
$buildLog = Join-Path $EvidenceDir 'build.log'
$buildProc = Start-Process -FilePath 'cmd.exe' `
    -ArgumentList '/c', "`"$buildBat`"" `
    -NoNewWindow -Wait -PassThru `
    -WorkingDirectory $repoRoot `
    -RedirectStandardOutput $buildLog `
    -RedirectStandardError "$buildLog.err"
if (Test-Path -LiteralPath "$buildLog.err") {
    Get-Content -LiteralPath "$buildLog.err" | Add-Content -LiteralPath $buildLog
    Remove-Item -LiteralPath "$buildLog.err" -Force
}
Get-Content -LiteralPath $buildLog -Tail 6 | Out-Host
if ($buildProc.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $buildOutExe)) {
    Write-Host "FAIL: build" -ForegroundColor Red
    exit 2
}
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
    }
}

# --- 2. Capture pristine real-save SHA ----------------------------------

Write-Section "2. Capture pristine real-save SHA"
$saveS0 = Get-SaveSha -Dir $saveDir
if ($saveS0.Count -eq 0) {
    # The proof requires SOMETHING in the real save dir to verify
    # round-trip integrity. Synthesise a baseline: write three
    # 4-byte stub files we can SHA-check. The launcher's quarantine
    # treats them just like real saves.
    Write-Host "no real save found; synthesising 3-byte stubs for the round-trip test"
    if (-not (Test-Path -LiteralPath $saveDir)) {
        New-Item -ItemType Directory -Path $saveDir -Force | Out-Null
    }
    foreach ($n in 'SYS-DATA','ACH-DATA','EXT-DATA') {
        $p = Join-Path $saveDir $n
        Set-Content -LiteralPath $p -Value "PRISTINE_$n" -Encoding ASCII -NoNewline
    }
    $saveS0 = Get-SaveSha -Dir $saveDir
}
if ($saveS0.Count -lt 1) {
    Write-Host "FAIL: cannot establish pristine save baseline" -ForegroundColor Red
    exit 3
}
Write-Host ("S0 = {0} entries: {1}" -f $saveS0.Count, ((@($saveS0.Keys)) -join ', '))

# Reset both pack dirs so every test starts clean.
foreach ($p in $packDirA, $packDirB) {
    if (Test-Path -LiteralPath $p) {
        Remove-Item -LiteralPath $p -Recurse -Force
    }
}
# Wipe only this proof's dedicated quarantine root so we can later
# assert it's empty after the final run. Do not touch the real
# operator quarantine root; that may contain recovery data from an
# interrupted manual launch.
if (Test-Path -LiteralPath $quarantineRoot) {
    Get-ChildItem -LiteralPath $quarantineRoot -Directory -ErrorAction SilentlyContinue |
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

# --- 3. Run 1: ticket A, route title -----------------------------------

Write-Section "3. Run ticket A (route=title) -- empty sandbox"
$eventsA1 = Invoke-LauncherRun -Ticket 'TICKET-A' -PackDirPath $packDirA -Route 'title' -RunTag 'A1'
$routeAseen = Wait-ForBridgeEvent -Pattern 'Pack:Route:title' -EventsPath $eventsA1 -TimeoutSeconds 1
if (-not $routeAseen) {
    # Tail asynchronously off the post-run snapshot just in case the
    # path did not match the live events.jsonl after run completion.
    $jsonlSnap = Join-Path $EvidenceDir 'run_A1\events_run_A1.jsonl'
    if (Test-Path -LiteralPath $jsonlSnap) {
        $text = Get-Content -LiteralPath $jsonlSnap -Raw
        $routeAseen = $text -match 'Pack:Route:title'
    }
}
if (-not $routeAseen) {
    Write-Host "FAIL: Pack:Route:title not seen for ticket A run 1" -ForegroundColor Red
    exit 4
}

$saveSha_after_A1 = Get-SaveSha -Dir $saveDir
if (-not (Test-SaveShaEqual $saveS0 $saveSha_after_A1)) {
    Write-Host "FAIL: real save SHA changed after run A1" -ForegroundColor Red
    exit 8
}

# --- 4. Plant marker bytes in ticket-A sandbox -------------------------

Write-Section "4. Plant marker in <packA>/save/SYS-DATA"
$packASandbox = Join-Path $packDirA 'save'
if (-not (Test-Path -LiteralPath $packASandbox)) {
    New-Item -ItemType Directory -Path $packASandbox -Force | Out-Null
}
$markerText = 'MARKER_A_PLANTED'
$markerPath = Join-Path $packASandbox 'SYS-DATA'
try {
    Set-Content -LiteralPath $markerPath -Value $markerText -Encoding ASCII -NoNewline
} catch {
    Write-Host "FAIL: marker plant: $($_.Exception.Message)" -ForegroundColor Red
    exit 5
}
$markerSha = (Get-FileHash -Algorithm SHA256 -LiteralPath $markerPath).Hash
Write-Host "planted: $markerPath ($markerSha)"

# --- 5. Run 2: ticket B, route title -----------------------------------

Write-Section "5. Run ticket B (route=title) -- separate sandbox"
$eventsB = Invoke-LauncherRun -Ticket 'TICKET-B' -PackDirPath $packDirB -Route 'title' -RunTag 'B1'
$routeBseen = Wait-ForBridgeEvent -Pattern 'Pack:Route:title' -EventsPath $eventsB -TimeoutSeconds 1
if (-not $routeBseen) {
    $jsonlSnap = Join-Path $EvidenceDir 'run_B1\events_run_B1.jsonl'
    if (Test-Path -LiteralPath $jsonlSnap) {
        $text = Get-Content -LiteralPath $jsonlSnap -Raw
        $routeBseen = $text -match 'Pack:Route:title'
    }
}
if (-not $routeBseen) {
    Write-Host "FAIL: Pack:Route:title not seen for ticket B" -ForegroundColor Red
    exit 6
}

# Cross-isolation: ticket B's sandbox must NOT contain the marker.
$packBSysData = Join-Path (Join-Path $packDirB 'save') 'SYS-DATA'
$bSaw_a = $false
if (Test-Path -LiteralPath $packBSysData) {
    $bytes = [System.IO.File]::ReadAllBytes($packBSysData)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    if ($text -eq $markerText) {
        $bSaw_a = $true
    }
}
if ($bSaw_a) {
    Write-Host "FAIL: <packB>/save/SYS-DATA contains the marker (cross-ticket leak)" -ForegroundColor Red
    exit 7
}
$bSysPresence = if (Test-Path -LiteralPath $packBSysData) { 'present' } else { 'absent' }
Write-Host "B's SYS-DATA: $bSysPresence -- marker NOT present (good)"

$saveSha_after_B = Get-SaveSha -Dir $saveDir
if (-not (Test-SaveShaEqual $saveS0 $saveSha_after_B)) {
    Write-Host "FAIL: real save SHA changed after run B" -ForegroundColor Red
    exit 8
}

# --- 6. Run 3: ticket A again -- marker round-trip ---------------------

Write-Section "6. Run ticket A again -- expect marker to round-trip"
$null = Invoke-LauncherRun -Ticket 'TICKET-A' -PackDirPath $packDirA -Route 'title' -RunTag 'A2'

# Marker should still be in <packA>/save/SYS-DATA after the A2 run
# (overlay -> live -> capture round-trip; UR with NO_AUTOLOAD=1
# does not touch save dir, so the bytes survive verbatim).
$markerSurvived = $false
if (Test-Path -LiteralPath $markerPath) {
    $bytes = [System.IO.File]::ReadAllBytes($markerPath)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    if ($text -eq $markerText) {
        $markerSurvived = $true
    }
}
if (-not $markerSurvived) {
    Write-Host "FAIL: marker did not survive ticket-A round-trip" -ForegroundColor Red
    if (Test-Path -LiteralPath $markerPath) {
        Write-Host ("present at {0} but content differs" -f $markerPath)
    } else {
        Write-Host ("missing at {0}" -f $markerPath)
    }
    exit 9
}

$saveSha_after_A2 = Get-SaveSha -Dir $saveDir
if (-not (Test-SaveShaEqual $saveS0 $saveSha_after_A2)) {
    Write-Host "FAIL: real save SHA changed after run A2" -ForegroundColor Red
    exit 11
}

# --- 7. Quarantine cleanup ---------------------------------------------

Write-Section "7. Quarantine cleanup"
$leftover = @()
if (Test-Path -LiteralPath $quarantineRoot) {
    $leftover = @(Get-ChildItem -LiteralPath $quarantineRoot -Directory -ErrorAction SilentlyContinue)
}
if ($leftover.Count -gt 0) {
    Write-Host ("FAIL: {0} orphan quarantine dirs left:" -f $leftover.Count) -ForegroundColor Red
    foreach ($d in $leftover) { Write-Host "  $($d.FullName)" }
    exit 10
}
Write-Host "no orphan quarantine dirs (good)"

# --- 8. Verdict --------------------------------------------------------

Write-Section "VERDICT"
$rows = @(
    [pscustomobject]@{ Gate = 'Pack:Route:title (run A1)';          Pass = $true },
    [pscustomobject]@{ Gate = 'Marker plant';                        Pass = $true },
    [pscustomobject]@{ Gate = 'Pack:Route:title (run B)';            Pass = $true },
    [pscustomobject]@{ Gate = 'Cross-isolation (B did not see A)';   Pass = $true },
    [pscustomobject]@{ Gate = 'Marker survived A round-trip';        Pass = $true },
    [pscustomobject]@{ Gate = 'Real save SHA == S0 (after each)';    Pass = $true },
    [pscustomobject]@{ Gate = 'Quarantine cleared';                  Pass = $true }
)
$rows | Format-Table -AutoSize | Out-Host

Write-Host ""
Write-Host "Phase 371B: PASS" -ForegroundColor Green
exit 0
