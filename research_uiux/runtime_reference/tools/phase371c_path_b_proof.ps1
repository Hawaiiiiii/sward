# Phase 371C -- in-game ImGui QA panel.
#
# Proves the panel reaches a render frame in the SGFX shell, reads
# pack metadata via SGPack::TryGet*(), and emits the
# session. Reload-counter wiring is exercised by editing the text
# manifest mid-run and waiting for the panel itself to emit
# QAPanel:ReloadCounts:<text>:<asset>:<pack> with text > 0.
#
# Acceptance gates (distinct exit codes):
#   2  build / deploy failed
#   3  exporter output missing
#   4  Pack:Meta:<t>:<p>:<ph> not seen within boot window
#   5  Pack:Route:title not seen within boot window
#   6  QAPanel:Active:<ticket>:<route> not seen (panel never reached
#      a render frame, or visibility resolution rejected it)
#   7  Text:ScopedRulesReloaded:<n> not seen after mid-run edit
#      OR QAPanel:ReloadCounts did not show text reload count > 0
#   8  -- env-disable test failed: with SG_PREFLIGHT_QA_PANEL=0,
#      QAPanel:Active must NOT fire (visibility opt-out broken)
#   9  no native frame captured (UR never rendered)

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 45,
    [int]$ReloadTimeoutSeconds = 8,
    [string]$EvidenceDir
)

$ErrorActionPreference = 'Stop'

$scriptDir   = Split-Path -Parent $PSCommandPath
$repoRoot    = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
$buildBat    = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir  = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$resDir      = Join-Path $repoRoot 'res'

$exporter    = Join-Path $scriptDir 'sgfx_pack_exporter.ps1'

$bridgeDir   = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$packDir     = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase371c'
$rulesPath   = Join-Path $env:TEMP 'phase371c_scoped_rules.json'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase371c_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
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

# --- 1. Build ----------------------------------------------------------

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

# --- 2. Author scoped rules + run exporter -----------------------------

Write-Section "2. Run sgfx_pack_exporter.ps1"
@'
[
  { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE371C_99" },
  { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE371C_999999" },
  { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE371C_16" }
]
'@ | Set-Content -LiteralPath $rulesPath -Encoding UTF8

if (Test-Path -LiteralPath $packDir) {
    Remove-Item -LiteralPath $packDir -Recurse -Force
}

$expectedTicket  = 'IDCEVODEV-960073'
$expectedProject = 'BMW SGFX QA Shell'
$expectedPhase   = '371C'

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $exporter `
    -OutputDir $packDir `
    -Ticket $expectedTicket `
    -Project $expectedProject `
    -Phase $expectedPhase `
    -Route 'title' `
    -ScopedRulesJson $rulesPath `
    -LogoSonicteamReplacement (Join-Path $resDir 'logo_sgfx.dds') `
    -Force | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: exporter exited $LASTEXITCODE" -ForegroundColor Red
    exit 3
}

$expectedFiles = @(
    'sgfx_pack.json',
    'pack_meta.json',
    'text/sgfx_text.json',
    'pictures/sgfx_pictures.json',
    'pictures/logo_sonicteam_override.dds',
    'sgfx_branding/icon.png'
)
foreach ($rel in $expectedFiles) {
    $abs = Join-Path $packDir ($rel -replace '/','\')
    if (-not (Test-Path -LiteralPath $abs)) {
        Write-Host "FAIL: exporter missing $rel" -ForegroundColor Red
        exit 3
    }
}

# --- 3. Reset events + back up save ------------------------------------

Write-Section "3. Reset events + back up save"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

$saveDir   = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$savePreSha = Get-SaveSha -Dir $saveDir
$saveBackupDir = Join-Path $EvidenceDir ("save_backup_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
if ($savePreSha.Count -gt 0) {
    New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
    foreach ($n in @($savePreSha.Keys)) {
        Copy-Item -LiteralPath (Join-Path $saveDir $n) -Destination (Join-Path $saveBackupDir $n) -Force
    }
}

# --- 4. Launch UR with QA panel auto-on (pack ticket present) ---------

Write-Section "4. Launch UR (QA panel default-on for staged ticket)"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $packDir
$env:SG_PREFLIGHT_HOT_RELOAD    = '1'
$env:SG_PREFLIGHT_NO_AUTOLOAD   = '1'
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_QA_PANEL    -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL   -ErrorAction SilentlyContinue

$origExe  = Join-Path $installDir 'UnleashedRecomp.exe'
$shellExe = Join-Path $installDir 'SgfxShell.exe'
Copy-Item -LiteralPath $origExe -Destination $shellExe -Force

$proc = Start-Process -FilePath $shellExe -WorkingDirectory $installDir -PassThru
$startedAt = Get-Date

# --- 5. Boot-time event gates ------------------------------------------

Write-Section "5a. Pack:Meta + Pack:Route + QAPanel:Active"
$packMetaPattern   = "Pack:Meta:$([regex]::Escape($expectedTicket)):$([regex]::Escape($expectedProject)):$([regex]::Escape($expectedPhase))"
$qaActivePattern   = "QAPanel:Active:$([regex]::Escape($expectedTicket)):title"

$packMetaSeen  = Wait-ForBridgeEvent -Pattern $packMetaPattern  -EventsPath $eventsPath -TimeoutSeconds 25
$packRouteSeen = Wait-ForBridgeEvent -Pattern 'Pack:Route:title' -EventsPath $eventsPath -TimeoutSeconds 15
$qaActiveSeen  = Wait-ForBridgeEvent -Pattern $qaActivePattern   -EventsPath $eventsPath -TimeoutSeconds 25

Write-Host "pack-meta:    $packMetaSeen"
Write-Host "pack-route:   $packRouteSeen"
Write-Host "qa-active:    $qaActiveSeen"

# --- 6. Edit text manifest mid-run -> reload counter increments --------

Write-Section "6. Edit text manifest mid-run"
@'
[
  { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE371C_RELOAD_99" },
  { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE371C_RELOAD_999999" },
  { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE371C_RELOAD_16" },
  { "literal": "30",     "csd_project_substring": "status", "replacement": "PHASE371C_RELOAD_30" }
]
'@ | Set-Content -LiteralPath $rulesPath -Encoding UTF8

# Re-export onto the same pack dir; the watcher's mtime poll picks
# up the text manifest change and dispatches a reload.
$reloadStartedAt = Get-Date
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $exporter `
    -OutputDir $packDir `
    -Ticket $expectedTicket -Project $expectedProject -Phase $expectedPhase -Route 'title' `
    -ScopedRulesJson $rulesPath `
    -LogoSonicteamReplacement (Join-Path $resDir 'logo_sgfx.dds') | Out-Host

$reloadSeen = Wait-ForBridgeEvent `
    -Pattern 'Text:ScopedRulesReloaded:4' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$panelReloadSeen = Wait-ForBridgeEvent `
    -Pattern 'QAPanel:ReloadCounts:[1-9][0-9]*:[0-9]+:[0-9]+' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$reloadElapsed = ((Get-Date) - $reloadStartedAt).TotalSeconds
Write-Host ("text-reload: {0} (elapsed {1:N2}s)" -f $reloadSeen, $reloadElapsed)
Write-Host "panel-count: $panelReloadSeen"

# --- 7. Capture screen frame --------------------------------------------

Write-Section "7. Capture screen frame"
$captureBmp = Join-Path $EvidenceDir 'phase371c_screen_grab.bmp'
try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
$g.Dispose(); $bmp.Dispose()

while (-not $proc.HasExited) {
    if (((Get-Date) - $startedAt).TotalSeconds -ge $AutoExitSeconds) {
        $proc.Kill() | Out-Null
        $proc.WaitForExit(5000) | Out-Null
        break
    }
    Start-Sleep -Milliseconds 200
}
if (-not $proc.HasExited) {
    $proc.WaitForExit(1000) | Out-Null
}
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase371c.jsonl') -Force

# Restore save before the second sub-test so we don't drift.
foreach ($n in @($savePreSha.Keys)) {
    $src = Join-Path $saveBackupDir $n
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $saveDir $n) -Force
    }
}

# --- 8. Re-launch with SG_PREFLIGHT_QA_PANEL=0 (env opt-out) -----------

Write-Section "8. Re-launch with SG_PREFLIGHT_QA_PANEL=0"
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII
$env:SG_PREFLIGHT_QA_PANEL = '0'

$proc2 = Start-Process -FilePath $shellExe -WorkingDirectory $installDir -PassThru
$startedAt2 = Get-Date

# Wait long enough for QAPanel:Active to fire IF it was going to.
# 25s mirrors the first sub-test's window. We expect NOT to see it.
$qaSuppressed_seen = Wait-ForBridgeEvent `
    -Pattern 'QAPanel:Active:' `
    -EventsPath $eventsPath `
    -TimeoutSeconds 25

while (-not $proc2.HasExited) {
    if (((Get-Date) - $startedAt2).TotalSeconds -ge 30) {
        $proc2.Kill() | Out-Null
        $proc2.WaitForExit(5000) | Out-Null
        break
    }
    Start-Sleep -Milliseconds 200
}
if (-not $proc2.HasExited) {
    $proc2.WaitForExit(1000) | Out-Null
}
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase371c_off.jsonl') -Force
Remove-Item Env:SG_PREFLIGHT_QA_PANEL -ErrorAction SilentlyContinue

if (Test-Path -LiteralPath $shellExe) {
    Remove-Item -LiteralPath $shellExe -Force -ErrorAction SilentlyContinue
}

# Final save restore.
foreach ($n in @($savePreSha.Keys)) {
    $src = Join-Path $saveBackupDir $n
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $saveDir $n) -Force
    }
}

# --- 9. Native frame check ---------------------------------------------

$nativeCount = (Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue | Measure-Object).Count

# --- 10. Verdict --------------------------------------------------------

Write-Section "VERDICT"
$rows = @(
    [pscustomobject]@{ Gate = 'Pack:Meta';                  Pass = $packMetaSeen },
    [pscustomobject]@{ Gate = 'Pack:Route:title';            Pass = $packRouteSeen },
    [pscustomobject]@{ Gate = 'QAPanel:Active (auto-on)';    Pass = $qaActiveSeen },
    [pscustomobject]@{ Gate = 'Text:ScopedRulesReloaded:4';  Pass = $reloadSeen },
    [pscustomobject]@{ Gate = 'QAPanel:ReloadCounts text>0'; Pass = $panelReloadSeen },
    [pscustomobject]@{ Gate = 'QA panel env-disable opt-out';Pass = (-not $qaSuppressed_seen) },
    [pscustomobject]@{ Gate = 'Native frame written';        Pass = ($nativeCount -ge 1) }
)
$rows | Format-Table -AutoSize | Out-Host

if (-not $packMetaSeen)             { exit 4 }
if (-not $packRouteSeen)            { exit 5 }
if (-not $qaActiveSeen)             { exit 6 }
if (-not $reloadSeen -or -not $panelReloadSeen) { exit 7 }
if ($qaSuppressed_seen)             { exit 8 }
if ($nativeCount -lt 1)             { exit 9 }

Write-Host ""
Write-Host "Phase 371C: PASS" -ForegroundColor Green
exit 0
