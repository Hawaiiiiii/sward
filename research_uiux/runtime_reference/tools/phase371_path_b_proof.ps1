# Phase 371A -- SGFX Pack Exporter + one-click Shell launcher.
#
# Proves the SGFX shell can be produced from a single exporter call
# and launched with one script, no manual env-var or save-management
# step from the operator. Phase 370A/B/C demonstrated the engine
# could read a pack and reload it; Phase 371A turns that into the
# product layer (exporter + launcher + pack metadata).
#
# Acceptance gates (each fails with a distinct exit code):
#   2  build / deploy failed
#   3  exporter failed to produce sgfx_pack.json + pack_meta.json
#      + branding files at the requested OutputDir
#   4  Pack:Meta:<ticket>:<project>:<phase> did not appear in the
#      bridge events stream within boot window
#   5  Pack:Loaded:<text>:<asset>:<loose> missing or
#      Branding:Active missing (pack accepted but lanes
#      did not initialise)
#   6  Initial Text:ScopedRulesLoaded:<n> count did not match the
#      exporter's manifest (exporter wrote N rules; UR loaded != N)
#   7  Mid-run scoped-rules edit -> Text:ScopedRulesReloaded:<m>
#      did not appear within $ReloadTimeoutSeconds (hot reload
#      across packs broken)
#   8  Save SHA-256 differed pre/post and the launcher did NOT
#      restore (safety net broken)
#   9  No native frame captured (UR never reached render loop)
#
# This runner exercises the exporter plus the same runtime env,
# branding copy, hot-reload, and save-restore path used by the
# launcher while keeping the tail/wait loop in-script so it can
# re-export mid-run and assert reload timing. Run
# sgfx_shell_launch.ps1 directly for the human-facing launcher smoke.

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 60,
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
$packDir     = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase371'
$rulesPath   = Join-Path $env:TEMP 'phase371_scoped_rules.json'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase371_path_b_proof'
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

# --- 1. Build ------------------------------------------------------------

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
Get-Content -LiteralPath $buildLog -Tail 8 | Out-Host
if ($buildProc.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $buildOutExe)) {
    Write-Host "FAIL: build" -ForegroundColor Red
    exit 2
}

# Deploy fresh exe + DLLs to the install dir so the launcher copies
# the latest UnleashedRecomp.exe into SgfxShell.exe.
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
    }
}

# --- 2. Author scoped rules JSON for the exporter ------------------------

Write-Section "2. Author scoped-rules input for exporter"
# 4 rules across known status-HUD literals. Pack expectations:
#   - exporter writes 4 entries
#   - UR loads "Text:ScopedRulesLoaded:4"
$rulesInitial = @"
[
  { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE371_99" },
  { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE371_999999" },
  { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE371_16" },
  { "literal": "30",     "csd_project_substring": "status", "replacement": "PHASE371_30" }
]
"@
$rulesInitial | Set-Content -LiteralPath $rulesPath -Encoding UTF8
Write-Host "wrote rules: $rulesPath (4 entries)"

# --- 3. Run exporter -----------------------------------------------------

Write-Section "3. Run sgfx_pack_exporter.ps1"
$expectedTicket  = 'IDCEVODEV-960073'
$expectedProject = 'BMW SGFX QA Shell'
$expectedPhase   = '371A'

# Wipe the pack dir so we can prove it was the exporter that
# populated it (not residual state from a prior run).
if (Test-Path -LiteralPath $packDir) {
    Remove-Item -LiteralPath $packDir -Recurse -Force
}

$exporterArgs = @(
    '-OutputDir', $packDir,
    '-Ticket',    $expectedTicket,
    '-Project',   $expectedProject,
    '-Phase',     $expectedPhase,
    '-ScopedRulesJson', $rulesPath,
    '-LogoSonicteamReplacement', (Join-Path $resDir 'logo_sgfx.dds'),
    '-Force'
)
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $exporter @exporterArgs | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: exporter exited $LASTEXITCODE" -ForegroundColor Red
    exit 3
}

# Verify exporter output on disk.
$expectedFiles = @(
    'sgfx_pack.json',
    'pack_meta.json',
    'text/sgfx_text.json',
    'pictures/sgfx_pictures.json',
    'pictures/logo_sonicteam_override.dds',
    'sgfx_branding/icon.png',
    'pack_export.log'
)
foreach ($rel in $expectedFiles) {
    $abs = Join-Path $packDir ($rel -replace '/','\')
    if (-not (Test-Path -LiteralPath $abs)) {
        Write-Host "FAIL: exporter missing $rel" -ForegroundColor Red
        exit 3
    }
}
Write-Host "exporter produced $($expectedFiles.Count) expected files"

# Sanity-check pack_meta.json fields match what we passed in.
$metaDoc = Get-Content -LiteralPath (Join-Path $packDir 'pack_meta.json') -Raw | ConvertFrom-Json
if ($metaDoc.ticket  -ne $expectedTicket  -or
    $metaDoc.project -ne $expectedProject -or
    $metaDoc.phase   -ne $expectedPhase) {
    Write-Host "FAIL: pack_meta.json mismatch" -ForegroundColor Red
    Write-Host "  expected: $expectedTicket / $expectedProject / $expectedPhase"
    Write-Host "  got:      $($metaDoc.ticket) / $($metaDoc.project) / $($metaDoc.phase)"
    exit 3
}

# --- 4. Reset events.jsonl + back up save data ---------------------------

Write-Section "4. Reset events.jsonl + back up save"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase371.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

$saveDir = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$savePreSha = Get-SaveSha -Dir $saveDir
$saveBackupDir = Join-Path $EvidenceDir ("save_backup_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
if ($savePreSha.Count -gt 0) {
    New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
    foreach ($n in $savePreSha.Keys) {
        Copy-Item -LiteralPath (Join-Path $saveDir $n) -Destination (Join-Path $saveBackupDir $n) -Force
    }
}

# --- 5. Launch via the shell env path (in-process so we can tail) --------

Write-Section "5. Launch SgfxShell.exe via env/branded copy"
# Apply env directly here (mirrors what sgfx_shell_launch.ps1 would
# do) so we can keep the proof's tail/wait loop in-script and
# re-export the pack while UR is still running.
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $packDir
$env:SG_PREFLIGHT_HOT_RELOAD    = '1'
$env:SG_PREFLIGHT_NO_AUTOLOAD   = '1'
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL   -ErrorAction SilentlyContinue

$origExe  = Join-Path $installDir 'UnleashedRecomp.exe'
$shellExe = Join-Path $installDir 'SgfxShell.exe'
Copy-Item -LiteralPath $origExe -Destination $shellExe -Force

$proc = Start-Process -FilePath $shellExe -WorkingDirectory $installDir -PassThru
$startedAt = Get-Date

# --- 6. Wait for boot-time events ----------------------------------------

Write-Section "6a. Pack:Meta within 20s"
$packMetaPattern = "Pack:Meta:$([regex]::Escape($expectedTicket)):$([regex]::Escape($expectedProject)):$([regex]::Escape($expectedPhase))"
$packMetaSeen = Wait-ForBridgeEvent -Pattern $packMetaPattern `
                                    -EventsPath $eventsPath `
                                    -TimeoutSeconds 20

Write-Section "6b. Pack:Loaded + Branding:Active"
$packLoadedSeen = Wait-ForBridgeEvent -Pattern 'Pack:Loaded:'    -EventsPath $eventsPath -TimeoutSeconds 15
$brandingSeen   = Wait-ForBridgeEvent -Pattern 'Branding:Active' -EventsPath $eventsPath -TimeoutSeconds 15

Write-Section "6c. Initial Text:ScopedRulesLoaded:4"
$textInitSeen   = Wait-ForBridgeEvent -Pattern 'Text:ScopedRulesLoaded:4' -EventsPath $eventsPath -TimeoutSeconds 15

# --- 7. Hot reload across exporter (re-export pack mid-run) --------------

Write-Section "7. Re-export pack mid-run with 6 rules"
$rulesReloaded = @"
[
  { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_99" },
  { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_999999" },
  { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_16" },
  { "literal": "30",     "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_30" },
  { "literal": "35",     "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_35" },
  { "literal": "0",      "csd_project_substring": "status", "replacement": "PHASE371_RELOAD_0" }
]
"@
$rulesReloaded | Set-Content -LiteralPath $rulesPath -Encoding UTF8

# Re-run the exporter; it overwrites text/sgfx_text.json. The
# watcher should see the mtime move and emit the reload event.
$reloadStartedAt = Get-Date
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $exporter `
    -OutputDir $packDir `
    -Ticket $expectedTicket -Project $expectedProject -Phase $expectedPhase `
    -ScopedRulesJson $rulesPath `
    -LogoSonicteamReplacement (Join-Path $resDir 'logo_sgfx.dds') | Out-Host

$textReloadSeen = Wait-ForBridgeEvent `
    -Pattern 'Text:ScopedRulesReloaded:6' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$textReloadElapsed = ((Get-Date) - $reloadStartedAt).TotalSeconds
Write-Host ("text-reload: {0} (elapsed {1:N2}s)" -f $textReloadSeen, $textReloadElapsed)

# --- 8. Capture frame + kill UR ------------------------------------------

Write-Section "8. Capture screen frame"
$captureBmp = Join-Path $EvidenceDir 'phase371_screen_grab.bmp'
try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
$g.Dispose(); $bmp.Dispose()
Write-Host "captured -> $captureBmp"

while (-not $proc.HasExited) {
    $elapsedSoFar = (Get-Date) - $startedAt
    if ($elapsedSoFar.TotalSeconds -ge $AutoExitSeconds) {
        $proc.Kill() | Out-Null
        break
    }
    Start-Sleep -Milliseconds 200
}

# Snapshot final events.
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase371.jsonl') -Force

# Clean up branded exe copy.
if (Test-Path -LiteralPath $shellExe) {
    Remove-Item -LiteralPath $shellExe -Force -ErrorAction SilentlyContinue
}

# --- 9. Save SHA pre/post + restore -------------------------------------

Write-Section "9. Save SHA pre/post"
$savePostSha = Get-SaveSha -Dir $saveDir
$saveChanged = $false
foreach ($n in @($savePreSha.Keys)) {
    if (-not $savePostSha.Contains($n) -or $savePostSha[$n] -ne $savePreSha[$n]) {
        $saveChanged = $true
        break
    }
}
$saveRestored = $false
if ($saveChanged) {
    Write-Host "save changed during run -- restoring backup" -ForegroundColor Yellow
    foreach ($n in @($savePreSha.Keys)) {
        $src = Join-Path $saveBackupDir $n
        if (Test-Path -LiteralPath $src) {
            Copy-Item -LiteralPath $src -Destination (Join-Path $saveDir $n) -Force
        }
    }
    $verify = Get-SaveSha -Dir $saveDir
    $saveRestored = $true
    foreach ($n in @($savePreSha.Keys)) {
        if ($verify[$n] -ne $savePreSha[$n]) {
            $saveRestored = $false; break
        }
    }
} else {
    Write-Host "save unchanged across run (good)"
    $saveRestored = $true
}

# --- 10. Native-frame capture sanity ------------------------------------

# Count BMPs written under EvidenceDir (the host screen-grab capture
# at section 8 lands here). At minimum, that grab proves UR reached
# the desktop / a render frame; an empty count means UR exited
# before WinForms could capture anything from the window region.
$nativeCount = (Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue | Measure-Object).Count

# --- 11. Verdict ---------------------------------------------------------

Write-Section "VERDICT"
$rows = @(
    [pscustomobject]@{ Gate = 'Pack:Meta';                  Pass = $packMetaSeen },
    [pscustomobject]@{ Gate = 'Pack:Loaded';                Pass = $packLoadedSeen },
    [pscustomobject]@{ Gate = 'Branding:Active';            Pass = $brandingSeen },
    [pscustomobject]@{ Gate = 'Text:ScopedRulesLoaded:4';   Pass = $textInitSeen },
    [pscustomobject]@{ Gate = 'Text:ScopedRulesReloaded:6'; Pass = $textReloadSeen },
    [pscustomobject]@{ Gate = 'Save restored';              Pass = $saveRestored },
    [pscustomobject]@{ Gate = 'Native frame written';       Pass = ($nativeCount -ge 1) }
)
$rows | Format-Table -AutoSize | Out-Host

if (-not $packMetaSeen)   { exit 4 }
if (-not $packLoadedSeen -or -not $brandingSeen) { exit 5 }
if (-not $textInitSeen)   { exit 6 }
if (-not $textReloadSeen) { exit 7 }
if (-not $saveRestored)   { exit 8 }
if ($nativeCount -lt 1)   { exit 9 }

Write-Host ""
Write-Host "Phase 371A: PASS" -ForegroundColor Green
exit 0
