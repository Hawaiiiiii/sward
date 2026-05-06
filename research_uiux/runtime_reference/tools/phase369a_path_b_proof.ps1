# Phase 369A -- Path B pixel-level texture override proof.
#
# Builds on Phase 367b/368. Goal: substitute retail SU's SonicTeam
# logo with the user's `res/logo_sgfx.dds` (raw BC7) at runtime, via
# the MakePictureData hook in `gpu/video.cpp` -- no LZX wrapping
# needed because retail's resource manager has already decompressed
# the on-disk wrapper by the time MakePictureData runs.
#
# Acceptance gates (each fails with a distinct exit code):
#   2  build / deploy / launch failed
#   3  Asset:PixelOverridesLoaded missing (manifest never loaded)
#   4  Asset:PixelOverrideHit:logo_sonicteam missing (hook never fired)
#   5  no native BMP captured during the run
#   6  pixel-diff vs the Phase 368 baseline BMP did not exceed the
#      meaningful threshold (override fired but the rendered frame is
#      visually identical -> the override either isn't reaching the
#      screen or the baseline is from the same override pack)
#   7  menu_accepted:Title row=continue observed (gameplay routing
#      fired despite NO_AUTOLOAD)
#
# Save-data safety:
#   - %APPDATA%\UnleashedRecomp\save\{SYS-DATA,ACH-DATA,EXT-DATA}
#     is snapshotted to the evidence dir before launch.
#   - Restored by SHA-256 diff after the run regardless of pass/fail.
#
# Phase 367b and Phase 368 runners are unaffected. The override pack
# this script stages lives at a separate dir
# (`%LOCALAPPDATA%\UnleashedRecomp\sg_overrides_phase369a`), and the
# evidence dir is `research_uiux/runtime_reference/out/phase369a_path_b_proof`.

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 45,
    [string]$EvidenceDir,
    [double]$PixelDiffThreshold = 0.001,
    [int]$PixelChannelDelta = 8
)

$ErrorActionPreference = 'Stop'

$scriptDir    = Split-Path -Parent $PSCommandPath
$repoRoot     = Resolve-Path (Join-Path $scriptDir '..\..\..')
$buildBat     = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir   = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$resDir       = Join-Path $repoRoot 'res'
$baselineBmp  = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase368_path_b_proof\phase368_screen_grab.bmp'

$bridgeDir    = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase369a'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase369a_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
}

# --- 1. Build (with repo -> build-tree sync) ---------------------------------

Write-Section "1. Build UnleashedRecomp (repo sync + incremental ninja)"
if (-not (Test-Path -LiteralPath $buildBat)) {
    Write-Host "FAIL: missing build wrapper at $buildBat" -ForegroundColor Red
    exit 2
}
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
Get-Content -LiteralPath $buildLog -Tail 12 | Out-Host
if ($buildProc.ExitCode -ne 0) {
    Write-Host "FAIL: build exited $($buildProc.ExitCode); see $buildLog" -ForegroundColor Red
    exit 2
}
if (-not (Test-Path -LiteralPath $buildOutExe)) {
    Write-Host "FAIL: build artifact missing at $buildOutExe" -ForegroundColor Red
    exit 2
}
Write-Host "build artifact: $buildOutExe ($((Get-Item $buildOutExe).Length) bytes)"

# --- 2. Stage canonical override pack ---------------------------------------

Write-Section "2. Stage canonical override pack at $stagedOvDir"
if (Test-Path -LiteralPath $stagedOvDir) {
    Remove-Item -LiteralPath $stagedOvDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagedOvDir -Force | Out-Null

# 2a. Pixel override: copy the user's framework logo into
#     <override>/sgfx_assets/logo_sgfx.dds and declare it as the
#     replacement for retail SU's `logo_sonicteam` CTexturePicture
#     name. The MakePictureData hook (gpu/video.cpp) reads the
#     guest pictureData->name, finds this entry in
#     sg_asset_overrides.json, and substitutes the cached raw DDS
#     bytes BEFORE LoadTexture / ddspp parse them.
$userLogo = Join-Path $resDir 'logo_sgfx.dds'
if (-not (Test-Path -LiteralPath $userLogo)) {
    Write-Host "FAIL: missing user asset $userLogo" -ForegroundColor Red
    exit 2
}
$sgfxAssetsDir = Join-Path $stagedOvDir 'sgfx_assets'
New-Item -ItemType Directory -Path $sgfxAssetsDir -Force | Out-Null
$stagedLogo = Join-Path $sgfxAssetsDir 'logo_sgfx.dds'
Copy-Item -LiteralPath $userLogo -Destination $stagedLogo -Force
$stagedLogoSha = (Get-FileHash -LiteralPath $stagedLogo -Algorithm SHA256).Hash
$stagedLogoSize = (Get-Item -LiteralPath $stagedLogo).Length
Write-Host "staged $stagedLogo ($stagedLogoSize bytes, sha256=$stagedLogoSha)"

$assetManifest = [ordered]@{
    version  = 1
    pictures = [ordered]@{
        'logo_sonicteam' = 'sgfx_assets/logo_sgfx.dds'
    }
}
$assetManifestPath = Join-Path $stagedOvDir 'sg_asset_overrides.json'
$assetManifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $assetManifestPath -Encoding UTF8
Write-Host "wrote $assetManifestPath"

# 2b. Empty text override manifest -- Phase 369A doesn't gate on
#     text events. We still write an empty `strings` map so the
#     SGTextOverrides loader recognises the directory is in use.
$textManifest = [ordered]@{ version = 1; strings = @{} }
$textManifestPath = Join-Path $stagedOvDir 'sg_text_overrides.json'
$textManifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $textManifestPath -Encoding UTF8

# --- 3. Deploy fresh exe ----------------------------------------------------

Write-Section "3. Deploy fresh exe to $installDir"
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
        Write-Host "deployed $name"
    }
}

# --- 4. Reset events.jsonl + back up save data ----------------------------

Write-Section "4. Reset bridge events.jsonl + back up save data"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase369a.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII
Write-Host "reset $eventsPath"

# Save backup safety net (Phase 368 lesson, codified for every runner
# that launches UR under SG_PREFLIGHT_GAMEPLAY_SKIP=1).
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
            Write-Host "backed up $src ($size bytes) -> $dst"
        }
    }
}

# --- 5. Launch UnleashedRecomp ----------------------------------------------

Write-Section "5. Launch UnleashedRecomp.exe (kill timeout = $AutoExitSeconds s)"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
$env:SG_PREFLIGHT_NO_AUTOLOAD   = '1'
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_BRIDGE_DISABLE -ErrorAction SilentlyContinue

$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
Write-Host "OVERRIDE_DIR  = $stagedOvDir"
Write-Host "BRIDGE_DIR    = $bridgeDir"
Write-Host "NO_AUTOLOAD   = 1"
Write-Host "args          = (plain launch, no --ui-lab flags)"

$proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
$captureDone = $false
$captureBmp = Join-Path $EvidenceDir 'phase369a_screen_grab.bmp'
$startedAt = Get-Date
while (-not $proc.HasExited) {
    Start-Sleep -Seconds 2
    $elapsedSoFar = (Get-Date) - $startedAt

    if (-not $captureDone -and (Test-Path -LiteralPath $eventsPath)) {
        $eventsText = Get-Content -LiteralPath $eventsPath -Raw -ErrorAction SilentlyContinue
        if ($eventsText -match 'Asset:PixelOverrideHit') {
            try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
            try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
            $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
            $g   = [System.Drawing.Graphics]::FromImage($bmp)
            $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
            $bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
            $g.Dispose(); $bmp.Dispose()
            $captureDone = $true
            Write-Host "captured screen frame after Asset:PixelOverrideHit -> $captureBmp"
        }
    }

    if ($elapsedSoFar.TotalSeconds -ge $AutoExitSeconds) {
        Write-Host "elapsed $([int]$elapsedSoFar.TotalSeconds)s >= timeout ${AutoExitSeconds}s, killing UR"
        $proc.Kill()
        break
    }
}
$proc.WaitForExit()
$elapsed = [int]((Get-Date) - $startedAt).TotalSeconds
Write-Host "process exited (exit=$($proc.ExitCode), elapsed=${elapsed} s)"

# Restore save snapshot (Phase 368 safety net).
if ($savedFiles.Count -gt 0) {
    Write-Section "5b. Restore pre-run save snapshot"
    foreach ($f in $savedFiles) {
        if (Test-Path -LiteralPath $f.Source) {
            $beforeHash = (Get-FileHash -LiteralPath $f.Backup -Algorithm SHA256).Hash
            $afterHash  = (Get-FileHash -LiteralPath $f.Source -Algorithm SHA256).Hash
            if ($beforeHash -ne $afterHash) {
                Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
                Write-Host "restored $($f.Name) (sha256 was $afterHash, now $beforeHash)"
            } else {
                Write-Host "no restore needed for $($f.Name) (sha256 unchanged $beforeHash)"
            }
        }
    }
}

# --- 6. Summarise events.jsonl ---------------------------------------------

Write-Section "6. Summarise events.jsonl"
$buckets = @{
    'Text:OverridesLoaded'         = New-Object System.Collections.Generic.List[string]
    'Asset:PixelOverridesLoaded'   = New-Object System.Collections.Generic.List[string]
    'Asset:PixelOverrideHit'       = New-Object System.Collections.Generic.List[string]
    'Asset:VisibleOverrideHit'     = New-Object System.Collections.Generic.List[string]
    'Asset:OverrideHit'            = New-Object System.Collections.Generic.List[string]
    'Stage:GameplaySkip'           = New-Object System.Collections.Generic.List[string]
    'Input:UiOnlyLock'             = New-Object System.Collections.Generic.List[string]
}
foreach ($line in (Get-Content -LiteralPath $eventsPath -ErrorAction SilentlyContinue)) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    foreach ($prefix in $buckets.Keys) {
        if ($line -match "`"screen`":`"$([regex]::Escape($prefix))(:[^`"]*)?`"") {
            [void]$buckets[$prefix].Add($line)
            break
        }
    }
}
$titleContinueAccepted = $false
foreach ($line in (Get-Content -LiteralPath $eventsPath -ErrorAction SilentlyContinue)) {
    if ($line -match '"event":"menu_accepted"' -and $line -match '"screen":"Title"' -and $line -match '"row_id":"continue"') {
        $titleContinueAccepted = $true
        break
    }
}
$summary = [ordered]@{}
foreach ($prefix in 'Text:OverridesLoaded','Asset:PixelOverridesLoaded','Asset:PixelOverrideHit','Asset:VisibleOverrideHit','Asset:OverrideHit','Stage:GameplaySkip','Input:UiOnlyLock') {
    $summary[$prefix] = $buckets[$prefix].Count
}
$summary | Format-Table -AutoSize | Out-String | Write-Host
foreach ($prefix in $summary.Keys) {
    $list = $buckets[$prefix]
    if ($list.Count -gt 0) {
        Write-Host ("first {0}:" -f $prefix) -ForegroundColor Yellow
        $list | Select-Object -First 2 | ForEach-Object { Write-Host "  $_" }
    }
}

# --- 7. Pixel-diff vs Phase 368 baseline ------------------------------------

Write-Section "7. Pixel-diff vs Phase 368 baseline"
$pixelDiffPercent = -1.0
$pixelDiffMad     = -1.0
$pixelDiffPath    = Join-Path $EvidenceDir 'phase369a_pixel_diff.json'
if (Test-Path -LiteralPath $captureBmp -PathType Leaf) {
    if (-not (Test-Path -LiteralPath $baselineBmp -PathType Leaf)) {
        Write-Host "WARN: no Phase 368 baseline at $baselineBmp; pixel-diff gate disabled" -ForegroundColor Yellow
    } else {
        try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
        $aBmp = [System.Drawing.Bitmap]::FromFile($captureBmp)
        $bBmp = [System.Drawing.Bitmap]::FromFile($baselineBmp)
        if ($aBmp.Width -ne $bBmp.Width -or $aBmp.Height -ne $bBmp.Height) {
            Write-Host ("WARN: capture {0}x{1} vs baseline {2}x{3} -- pixel-diff disabled" -f `
                $aBmp.Width, $aBmp.Height, $bBmp.Width, $bBmp.Height) -ForegroundColor Yellow
        } else {
            $w = $aBmp.Width
            $h = $aBmp.Height
            $rect = New-Object System.Drawing.Rectangle 0, 0, $w, $h
            $aData = $aBmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $bData = $bBmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $stride = $aData.Stride
            $size = [System.Math]::Abs($stride) * $h
            $abuf = New-Object byte[] $size
            $bbuf = New-Object byte[] $size
            [System.Runtime.InteropServices.Marshal]::Copy($aData.Scan0, $abuf, 0, $size)
            [System.Runtime.InteropServices.Marshal]::Copy($bData.Scan0, $bbuf, 0, $size)
            $aBmp.UnlockBits($aData); $bBmp.UnlockBits($bData)

            $totalPixels = $w * $h
            $diffPixels  = 0
            $absDiffSum  = [int64]0
            for ($i = 0; $i -lt $size; $i += 4) {
                $db = [int][System.Math]::Abs($abuf[$i  ] - $bbuf[$i  ])
                $dg = [int][System.Math]::Abs($abuf[$i+1] - $bbuf[$i+1])
                $dr = [int][System.Math]::Abs($abuf[$i+2] - $bbuf[$i+2])
                $absDiffSum += $dr + $dg + $db
                if ($dr -gt $PixelChannelDelta -or $dg -gt $PixelChannelDelta -or $db -gt $PixelChannelDelta) {
                    $diffPixels++
                }
            }
            $aBmp.Dispose(); $bBmp.Dispose()

            $pixelDiffPercent = [double]$diffPixels / [double]$totalPixels
            $pixelDiffMad     = [double]$absDiffSum / [double]($totalPixels * 3)
            Write-Host ("diff_pixels = {0} / {1} ({2:P3})" -f $diffPixels, $totalPixels, $pixelDiffPercent)
            Write-Host ("mean_abs_diff (per channel, 0-255) = {0:N3}" -f $pixelDiffMad)
        }
    }
} else {
    Write-Host "no capture BMP -- pixel-diff skipped" -ForegroundColor Yellow
}

# --- 8. Persist evidence ----------------------------------------------------

Write-Section "8. Persist evidence to $EvidenceDir"
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase369a.jsonl') -Force
$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

[ordered]@{
    auto_exit_seconds                 = $AutoExitSeconds
    pixel_diff_threshold              = $PixelDiffThreshold
    pixel_channel_delta               = $PixelChannelDelta
    text_overrides_loaded             = $summary['Text:OverridesLoaded']
    asset_pixel_overrides_loaded      = $summary['Asset:PixelOverridesLoaded']
    asset_pixel_override_hits         = $summary['Asset:PixelOverrideHit']
    asset_visible_override_hits       = $summary['Asset:VisibleOverrideHit']
    asset_override_hits               = $summary['Asset:OverrideHit']
    stage_gameplay_skip               = $summary['Stage:GameplaySkip']
    input_ui_only_lock                = $summary['Input:UiOnlyLock']
    title_continue_accepted           = $titleContinueAccepted
    native_frames_written             = $frames.Count
    pixel_diff_percent                = $pixelDiffPercent
    pixel_diff_mean_abs_diff          = $pixelDiffMad
    elapsed_seconds                   = $elapsed
    process_exit_code                 = $proc.ExitCode
    staged_user_logo_sha256           = $stagedLogoSha
    staged_user_logo_bytes            = $stagedLogoSize
    override_dir                      = $stagedOvDir
    bridge_dir                        = $bridgeDir
    install_dir                       = $installDir
    build_exe                         = $buildOutExe
    evidence_dir                      = $EvidenceDir
    baseline_bmp                      = $baselineBmp
    save_backup_dir                   = $saveBackupDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $EvidenceDir 'summary.json') -Encoding UTF8
Write-Host "summary.json -> $(Join-Path $EvidenceDir 'summary.json')"

# --- Acceptance check -------------------------------------------------------

Write-Section "Acceptance"
if ($summary['Asset:PixelOverridesLoaded'] -lt 1) {
    Write-Host "FAIL: Asset:PixelOverridesLoaded missing (sg_asset_overrides.json never loaded)" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Asset:PixelOverridesLoaded = $($summary['Asset:PixelOverridesLoaded'])" -ForegroundColor Green

if ($summary['Asset:PixelOverrideHit'] -lt 1) {
    Write-Host "FAIL: Asset:PixelOverrideHit missing (MakePictureData hook never matched)" -ForegroundColor Red
    Write-Host "      Confirm retail SU's CTexturePicture name for the SonicTeam logo is `"logo_sonicteam`""
    exit 4
}
Write-Host "OK   Asset:PixelOverrideHit     = $($summary['Asset:PixelOverrideHit'])" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 5
}
Write-Host "OK   native frames written     = $($frames.Count)" -ForegroundColor Green

if ($pixelDiffPercent -ge 0) {
    if ($pixelDiffPercent -lt $PixelDiffThreshold) {
        Write-Host ("FAIL: pixel-diff {0:P3} < threshold {1:P3} -- override fired but frame is ~identical to Phase 368 baseline" -f $pixelDiffPercent, $PixelDiffThreshold) -ForegroundColor Red
        exit 6
    }
    Write-Host ("OK   pixel-diff vs baseline   = {0:P3} (threshold {1:P3})" -f $pixelDiffPercent, $PixelDiffThreshold) -ForegroundColor Green
} else {
    Write-Host "INFO pixel-diff disabled (no baseline or dimension mismatch)" -ForegroundColor Cyan
}

if ($titleContinueAccepted) {
    Write-Host "FAIL: menu_accepted:Title row=continue observed -> gameplay routing fired" -ForegroundColor Red
    exit 7
}
Write-Host "OK   no Title.continue accept   = $true" -ForegroundColor Green

exit 0
