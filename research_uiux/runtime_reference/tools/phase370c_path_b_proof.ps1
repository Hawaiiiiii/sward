# Phase 370C -- hot reload via mtime polling + immutable snapshots.
#
# Proves the SGFX shell can re-read its override manifests at
# runtime without restarting UR. The Phase 370C refactor moves
# pack / text / asset state behind immutable shared_ptr snapshots
# so a watcher-thread reload that swaps the global pointer cannot
# free bytes that an in-flight MakePictureData / Localise /
# TryGetOverride caller is still reading.
#
# Acceptance gates (each fails with a distinct exit code):
#   2  build / deploy / launch failed
#   3  HotReload:WatcherStarted missing -- watcher never began
#      polling (env not picked up, or override dir empty)
#   4  initial Pack:Loaded / Text:ScopedRulesLoaded missing
#      (boot-time loaders did not install a snapshot)
#   5  Text:ScopedRulesReloaded did not appear within
#      $ReloadTimeoutSeconds after the runner edited the text
#      manifest -- the watcher did not detect the mtime change
#      (or the dispatch path is broken)
#   6  Reloaded count did not match the new manifest's rule count
#      (a reload fired but with a stale snapshot)
#   7  Pack:Reloaded did not fire after the runner edited
#      sgfx_pack.json
#   8  Asset:PixelOverridesReloaded did not fire after the runner
#      edited the asset manifest
#   9  no native BMP captured (UR never reached a render frame)
#
# Save-data safety: full snapshot+restore, same as Phase 368/369/370A/B.

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 60,
    [int]$ReloadTimeoutSeconds = 5,
    [string]$EvidenceDir
)

$ErrorActionPreference = 'Stop'

$scriptDir    = Split-Path -Parent $PSCommandPath
$repoRoot     = Resolve-Path (Join-Path $scriptDir '..\..\..')
$buildBat     = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir   = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$resDir       = Join-Path $repoRoot 'res'

$bridgeDir    = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase370c'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase370c_path_b_proof'
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

# --- 1. Build --------------------------------------------------------------

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

# --- 2. Stage initial pack ------------------------------------------------

Write-Section "2. Stage initial pack at $stagedOvDir"
if (Test-Path -LiteralPath $stagedOvDir) {
    Remove-Item -LiteralPath $stagedOvDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagedOvDir -Force | Out-Null

$textPackDir   = Join-Path $stagedOvDir 'text'
New-Item -ItemType Directory -Path $textPackDir -Force | Out-Null
$packTextPath  = Join-Path $textPackDir 'sgfx_text.json'

# Initial text manifest: 3 scoped rules. After UR boots, the runner
# rewrites this file with 5 scoped rules; the watcher should see
# the mtime change, debounce, and emit Text:ScopedRulesReloaded:5.
$initialText = @"
{
  "version": 2,
  "strings": {},
  "scoped_rules": [
    { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE370C_INITIAL_99" },
    { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE370C_INITIAL_999999" },
    { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE370C_INITIAL_16" }
  ]
}
"@
$initialText | Set-Content -LiteralPath $packTextPath -Encoding UTF8

$picsPackDir   = Join-Path $stagedOvDir 'pictures'
New-Item -ItemType Directory -Path $picsPackDir -Force | Out-Null
$packPicsPath  = Join-Path $picsPackDir 'sgfx_pictures.json'
$userLogo      = Join-Path $resDir 'logo_sgfx.dds'
$packPicCopy   = Join-Path $picsPackDir 'logo_sgfx.dds'
Copy-Item -LiteralPath $userLogo -Destination $packPicCopy -Force
$initialPics = @"
{
  "version": 1,
  "pictures": {
    "logo_sonicteam": "pictures/logo_sgfx.dds"
  }
}
"@
$initialPics | Set-Content -LiteralPath $packPicsPath -Encoding UTF8

$packPath = Join-Path $stagedOvDir 'sgfx_pack.json'
$initialPack = @"
{
  "version": 1,
  "name": "Phase 370C hot-reload proof",
  "description": "Initial manifest; watcher rewrites text + pack mid-run",
  "text_overrides":  "text/sgfx_text.json",
  "asset_overrides": "pictures/sgfx_pictures.json"
}
"@
$initialPack | Set-Content -LiteralPath $packPath -Encoding UTF8
Write-Host "wrote initial pack + text + asset manifests"

# --- 3. Deploy fresh exe ---------------------------------------------------

Write-Section "3. Deploy fresh exe"
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
    }
}

# --- 4. Reset events.jsonl + back up save data ----------------------------

Write-Section "4. Reset events.jsonl + back up save data"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase370c.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

$saveDir = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$saveBackupDir = Join-Path $EvidenceDir ("save_backup_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
$savedFiles = @()
if (Test-Path -LiteralPath $saveDir) {
    New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
    foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
        $src = Join-Path $saveDir $name
        if (Test-Path -LiteralPath $src) {
            $dst = Join-Path $saveBackupDir $name
            Copy-Item -LiteralPath $src -Destination $dst -Force
            $size = (Get-Item -LiteralPath $src).Length
            $savedFiles += [pscustomobject]@{ Name = $name; Source = $src; Backup = $dst; Bytes = $size }
        }
    }
}

# --- 5. Launch UR with hot reload enabled ---------------------------------

Write-Section "5. Launch UR with SG_PREFLIGHT_HOT_RELOAD=1"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
$env:SG_PREFLIGHT_HOT_RELOAD    = '1'
Remove-Item Env:SG_PREFLIGHT_NO_AUTOLOAD -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL   -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_EXE_NAME      -ErrorAction SilentlyContinue

$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
$proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
$startedAt = Get-Date

# Wait for the watcher boot marker so we know it has begun polling.
Write-Section "5a. Wait for HotReload:WatcherStarted (boot marker)"
$watcherStarted = Wait-ForBridgeEvent -Pattern 'HotReload:WatcherStarted' -EventsPath $eventsPath -TimeoutSeconds 20
Write-Host "watcher started: $watcherStarted"

# Wait for the initial loader markers so we know snapshots exist.
Write-Section "5b. Wait for initial Pack:Loaded + Text:ScopedRulesLoaded"
$packLoadedSeen     = Wait-ForBridgeEvent -Pattern 'Pack:Loaded:' -EventsPath $eventsPath -TimeoutSeconds 15
$textLoadedSeen     = Wait-ForBridgeEvent -Pattern 'Text:ScopedRulesLoaded:3' -EventsPath $eventsPath -TimeoutSeconds 15
$pixelLoadedSeen    = Wait-ForBridgeEvent -Pattern 'Asset:PixelOverridesLoaded:1' -EventsPath $eventsPath -TimeoutSeconds 15
Write-Host "pack-loaded:  $packLoadedSeen"
Write-Host "text-loaded:  $textLoadedSeen"
Write-Host "pixel-loaded: $pixelLoadedSeen"

# --- 6. Edit text manifest -> watch for Text:ScopedRulesReloaded ---------

Write-Section "6. Edit text manifest mid-run"
$reloadedText = @"
{
  "version": 2,
  "strings": {},
  "scoped_rules": [
    { "literal": "99",     "csd_project_substring": "status", "replacement": "PHASE370C_RELOADED_99" },
    { "literal": "999999", "csd_project_substring": "status", "replacement": "PHASE370C_RELOADED_999999" },
    { "literal": "16",     "csd_project_substring": "status", "replacement": "PHASE370C_RELOADED_16" },
    { "literal": "35",     "csd_project_substring": "status", "replacement": "PHASE370C_RELOADED_35" },
    { "literal": "30",     "csd_project_substring": "status", "replacement": "PHASE370C_RELOADED_30" }
  ]
}
"@
$reloadedText | Set-Content -LiteralPath $packTextPath -Encoding UTF8
Write-Host "rewrote $packTextPath with 5 rules"
$textReloadStartedAt = Get-Date

$textReloadedSeen = Wait-ForBridgeEvent `
    -Pattern 'Text:ScopedRulesReloaded:5' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$textReloadElapsed = ((Get-Date) - $textReloadStartedAt).TotalSeconds
Write-Host ("text-reloaded:  {0} (elapsed {1:N2}s)" -f $textReloadedSeen, $textReloadElapsed)

# --- 7. Edit asset manifest -> watch for Asset:PixelOverridesReloaded ----

Write-Section "7. Edit asset manifest mid-run"
$reloadedPics = @"
{
  "version": 1,
  "pictures": {
    "logo_sonicteam": "pictures/logo_sgfx.dds",
    "logo_havok":     "pictures/logo_sgfx.dds"
  }
}
"@
$reloadedPics | Set-Content -LiteralPath $packPicsPath -Encoding UTF8
Write-Host "rewrote $packPicsPath with 2 pictures"
$assetReloadStartedAt = Get-Date

$assetReloadedSeen = Wait-ForBridgeEvent `
    -Pattern 'Asset:PixelOverridesReloaded:2' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$assetReloadElapsed = ((Get-Date) - $assetReloadStartedAt).TotalSeconds
Write-Host ("asset-reloaded: {0} (elapsed {1:N2}s)" -f $assetReloadedSeen, $assetReloadElapsed)

# --- 8. Edit pack manifest -> watch for Pack:Reloaded --------------------

Write-Section "8. Edit pack manifest mid-run"
$reloadedPack = @"
{
  "version": 1,
  "name": "Phase 370C hot-reload proof (post-reload)",
  "description": "Pack manifest rewritten mid-run",
  "text_overrides":  "text/sgfx_text.json",
  "asset_overrides": "pictures/sgfx_pictures.json"
}
"@
$reloadedPack | Set-Content -LiteralPath $packPath -Encoding UTF8
Write-Host "rewrote $packPath"
$packReloadStartedAt = Get-Date

$packReloadedSeen = Wait-ForBridgeEvent `
    -Pattern 'Pack:Reloaded:' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$packReloadElapsed = ((Get-Date) - $packReloadStartedAt).TotalSeconds
Write-Host ("pack-reloaded:  {0} (elapsed {1:N2}s)" -f $packReloadedSeen, $packReloadElapsed)

# --- 9. Capture screen frame + kill UR -----------------------------------

Write-Section "9. Capture screen frame"
$captureBmp = Join-Path $EvidenceDir 'phase370c_screen_grab.bmp'
try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
$g.Dispose(); $bmp.Dispose()
Write-Host "captured -> $captureBmp"

# Wait until total elapsed reaches AutoExitSeconds, then kill.
while (-not $proc.HasExited) {
    $elapsedSoFar = (Get-Date) - $startedAt
    if ($elapsedSoFar.TotalSeconds -ge $AutoExitSeconds) {
        Write-Host "elapsed $([int]$elapsedSoFar.TotalSeconds)s >= timeout, killing UR"
        $proc.Kill()
        break
    }
    Start-Sleep -Seconds 1
}
$proc.WaitForExit()
$elapsed = [int]((Get-Date) - $startedAt).TotalSeconds

# Restore save snapshot.
foreach ($f in $savedFiles) {
    if (Test-Path -LiteralPath $f.Source) {
        $beforeHash = (Get-FileHash -LiteralPath $f.Backup -Algorithm SHA256).Hash
        $afterHash  = (Get-FileHash -LiteralPath $f.Source -Algorithm SHA256).Hash
        if ($beforeHash -ne $afterHash) {
            Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
            Write-Host "restored $($f.Name)"
        }
    }
}

# --- 10. Persist evidence ------------------------------------------------

Write-Section "10. Persist evidence"
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase370c.jsonl') -Force
$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

[ordered]@{
    auto_exit_seconds              = $AutoExitSeconds
    reload_timeout_seconds         = $ReloadTimeoutSeconds
    watcher_started                = $watcherStarted
    initial_pack_loaded            = $packLoadedSeen
    initial_text_scoped_loaded_3   = $textLoadedSeen
    initial_pixel_loaded_1         = $pixelLoadedSeen
    text_scoped_reloaded_5         = $textReloadedSeen
    text_reload_elapsed_seconds    = $textReloadElapsed
    asset_pixel_reloaded_2         = $assetReloadedSeen
    asset_reload_elapsed_seconds   = $assetReloadElapsed
    pack_reloaded                  = $packReloadedSeen
    pack_reload_elapsed_seconds    = $packReloadElapsed
    native_frames_written          = $frames.Count
    elapsed_seconds                = $elapsed
    process_exit_code              = $proc.ExitCode
    override_dir                   = $stagedOvDir
    bridge_dir                     = $bridgeDir
    install_dir                    = $installDir
    build_exe                      = $buildOutExe
    evidence_dir                   = $EvidenceDir
    save_backup_dir                = $saveBackupDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $EvidenceDir 'summary.json') -Encoding UTF8

# --- Acceptance check -----------------------------------------------------

Write-Section "Acceptance"
if (-not $watcherStarted) {
    Write-Host "FAIL: HotReload:WatcherStarted missing -- watcher did not begin polling" -ForegroundColor Red
    exit 3
}
Write-Host "OK   HotReload:WatcherStarted observed" -ForegroundColor Green

if (-not $packLoadedSeen -or -not $textLoadedSeen -or -not $pixelLoadedSeen) {
    Write-Host "FAIL: initial loader markers missing (pack=$packLoadedSeen text3=$textLoadedSeen pixel1=$pixelLoadedSeen)" -ForegroundColor Red
    exit 4
}
Write-Host "OK   initial Pack:Loaded + Text:ScopedRulesLoaded:3 + Asset:PixelOverridesLoaded:1" -ForegroundColor Green

if (-not $textReloadedSeen) {
    Write-Host "FAIL: Text:ScopedRulesReloaded:5 not observed within ${ReloadTimeoutSeconds}s after manifest edit" -ForegroundColor Red
    exit 5
}
if ($textReloadElapsed -gt $ReloadTimeoutSeconds) {
    Write-Host ("FAIL: Text:ScopedRulesReloaded:5 took {0:N2}s, exceeds budget" -f $textReloadElapsed) -ForegroundColor Red
    exit 6
}
Write-Host ("OK   Text:ScopedRulesReloaded:5 fired in {0:N2}s" -f $textReloadElapsed) -ForegroundColor Green

if (-not $assetReloadedSeen) {
    Write-Host "FAIL: Asset:PixelOverridesReloaded:2 not observed within ${ReloadTimeoutSeconds}s after asset manifest edit" -ForegroundColor Red
    exit 8
}
Write-Host ("OK   Asset:PixelOverridesReloaded:2 fired in {0:N2}s" -f $assetReloadElapsed) -ForegroundColor Green

if (-not $packReloadedSeen) {
    Write-Host "FAIL: Pack:Reloaded not observed within ${ReloadTimeoutSeconds}s after pack edit" -ForegroundColor Red
    exit 7
}
Write-Host ("OK   Pack:Reloaded fired in {0:N2}s" -f $packReloadElapsed) -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 9
}
Write-Host "OK   native frames written = $($frames.Count)" -ForegroundColor Green

exit 0
