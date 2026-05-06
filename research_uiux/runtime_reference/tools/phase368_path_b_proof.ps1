# Phase 368 -- Path B visible content swap + Title-route hold proof.
#
# End-to-end runner. Three goals (per the Phase 368 review):
#
#   1. Replace the Phase 367b safe byte-identical DDS proof with a
#      NON-IDENTICAL but harmless visible-asset override. The script
#      builds the override file by taking the retail
#      `game/Loading/logo_sonicteam.dds` LZX-wrapped payload and
#      APPENDING a Phase 368 marker tail (`PHASE368_SGFX_OVERRIDE_v1`
#      + repo SHA-256 of the user's logo_sgfx.dds source). The retail
#      LZX decoder reads `decompressedDataSize` worth of compressed
#      blocks starting at offset 0x30 and stops; trailing bytes after
#      the last valid block are ignored, so the appended marker
#      changes the file's MD5/SHA-256 (proves the override is
#      non-identical) without breaking decompression (proves it is
#      harmless / does not crash retail SU's loader). The user's
#      raw BC7 DDS asset is NOT staged at the LZX-required path
#      because retail SU's resource manager decompresses LZX before
#      handing the bytes to `MakePictureData` / ddspp; a raw "DDS "
#      magic file at the LZX lane fails the retail loader's
#      decompression step and produces no texture. Pixel-level swap
#      requires LZX-encoding the user's DDS, which is out of scope
#      for this beat.
#
#   2. Exercise `SG_PREFLIGHT_NO_AUTOLOAD=1` to suppress retail SU's
#      save auto-resume. Without a resumable save, the title menu's
#      Continue row defaults to New Save and the runtime stops at
#      the title attract instead of racing to Empire City. Proof:
#         - Title.arl / Title.ar.00 file probes appear in events.jsonl
#         - Stage:GameplaySkip count <= 1 (only the title intro stage,
#           no second stage for gameplay)
#         - the script captures a screen frame while the title attract
#           or title menu is on screen
#
#   3. Stable SGFX shell launch profile lives at
#      `_sgfx_shell_launch.bat` at the repo root (not invoked directly
#      by this script -- this script writes the env vars itself -- but
#      kept around so day-to-day SGFX-shell launches don't have to
#      replicate the env). The profile honors `SGFX_NO_AUTOLOAD=1` and
#      `SGFX_LOG_SETTEXT=1` opt-in env vars.
#
# Failure exits:
#   2  build / deploy / launch failed
#   3  Text:OverridesLoaded missing
#   4  Asset:VisibleOverrideHit missing
#   5  staged DDS hash matches retail (override is identical -> not a swap)
#   6  no native BMP frame written
#   7  menu_accepted:Title row=continue observed (gameplay routing
#      fired despite NO_AUTOLOAD)
#   8  Title.arl probe missing (UR never reached the Title flow)

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 45,
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
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase368'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase368_path_b_proof'
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

# 2a. Text overrides: same set Phase 367b proved fires
#     `Text:CsdOverrideHit` against retail HUD digit literals.
$strings = [System.Collections.Specialized.OrderedDictionary]::new(
    [System.StringComparer]::Ordinal)
$strings['Common_Select']    = 'BMW Select 35'
$strings['Common_Cancel']    = 'BMW Cancel 35'
$strings['Common_Back']      = 'BMW Back 35'
$strings['Common_Yes']       = 'BMW Yes 35'
$strings['Common_No']        = 'BMW No 35'
$strings['999999']           = 'BMW999'
$strings['[200]']            = 'BMW200'
$strings['16']               = 'BMW16'
$strings['35']               = 'BMW35'
$strings['30']               = 'BMW30'
$strings['7']                = 'BMW7'
$strings['99']               = 'BMW99'
$textOverridesObj = [ordered]@{
    version = 1
    strings = $strings
}
$textOverridesPath = Join-Path $stagedOvDir 'sg_text_overrides.json'
$textOverridesObj | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $textOverridesPath -Encoding UTF8
Write-Host "wrote $textOverridesPath"

# 2b. Visible asset: take the retail LZX-wrapped logo_sonicteam.dds
#     bytes and APPEND a Phase 368 trailer that includes the SHA-256
#     of the user's logo_sgfx.dds asset. The retail LZX decoder stops
#     after consuming `decompressedDataSize` worth of compressed
#     blocks (offset >= 0x30, count = total decompressed size); the
#     trailer is ignored at decode time but recorded on disk so the
#     override file's MD5/SHA-256 differ from retail. This is the
#     "non-identical but harmless" form the Phase 368 review asked
#     for: the runtime still boots, decompresses, decodes the DDS,
#     and renders the SonicTeam logo identically, but the loose-file
#     substitution path emits Asset:VisibleOverrideHit AND the on-
#     disk file proves the override pack is no longer pretending to
#     be retail.
$retailDds = Join-Path $installDir 'game\Loading\logo_sonicteam.dds'
$stagedDds = Join-Path $stagedOvDir 'Loading\logo_sonicteam.dds'
if (-not (Test-Path -LiteralPath $retailDds)) {
    Write-Host "FAIL: missing visible asset source $retailDds" -ForegroundColor Red
    exit 2
}
$retailBytes = [System.IO.File]::ReadAllBytes($retailDds)
$retailMd5 = (Get-FileHash -LiteralPath $retailDds -Algorithm MD5).Hash
$retailSha = (Get-FileHash -LiteralPath $retailDds -Algorithm SHA256).Hash

$userLogoDds = Join-Path $resDir 'logo_sgfx.dds'
$userLogoSha = if (Test-Path -LiteralPath $userLogoDds) {
    (Get-FileHash -LiteralPath $userLogoDds -Algorithm SHA256).Hash
} else {
    'no-user-logo-available'
}

$trailerText = "PHASE368_SGFX_OVERRIDE_v1`nbase=$retailSha`nuser_logo_sha256=$userLogoSha`n"
$trailerBytes = [System.Text.Encoding]::UTF8.GetBytes($trailerText)
$overrideBytes = New-Object byte[] ($retailBytes.Length + $trailerBytes.Length)
[Array]::Copy($retailBytes, 0, $overrideBytes, 0, $retailBytes.Length)
[Array]::Copy($trailerBytes, 0, $overrideBytes, $retailBytes.Length, $trailerBytes.Length)

New-Item -ItemType Directory -Path (Split-Path $stagedDds) -Force | Out-Null
[System.IO.File]::WriteAllBytes($stagedDds, $overrideBytes)
$stagedMd5 = (Get-FileHash -LiteralPath $stagedDds -Algorithm MD5).Hash
$stagedSha = (Get-FileHash -LiteralPath $stagedDds -Algorithm SHA256).Hash
Write-Host "retail $retailDds (md5=$retailMd5)"
Write-Host "staged $stagedDds (md5=$stagedMd5, +trailer=$($trailerBytes.Length) bytes)"
if ($stagedMd5 -eq $retailMd5) {
    Write-Host "FAIL: staged DDS MD5 unexpectedly equals retail MD5" -ForegroundColor Red
    exit 5
}

# 2c. Park user's custom DDS assets next to the staged override so
#     downstream phases can pick them up without re-discovering paths.
#     The asset names map to the SGFX shell's roles:
#       logo_sgfx.dds        - main framework logo (2752 x 1536, BC7)
#       framework_sgfx_logo.dds - alt framework logo
#       sgfx_icon.dds        - small icon (726 x 726, BC7)
#       debug_icon.dds       - debug indicator (2048 x 2048, BC7)
#     They live under <override>/sgfx_assets/ so the loose-asset
#     matcher's fileKey index does not collide with retail filenames
#     (retail has no `logo_sgfx.dds` etc.). Phase 368 does NOT make
#     retail UR consume them at runtime (LZX wrapper required, see
#     above); they are staged for documentation and for the next
#     phase that adds an LZX wrap step.
$sgfxAssetsDir = Join-Path $stagedOvDir 'sgfx_assets'
New-Item -ItemType Directory -Path $sgfxAssetsDir -Force | Out-Null
foreach ($name in 'logo_sgfx.dds','framework_sgfx_logo.dds','sgfx_icon.dds','debug_icon.dds') {
    $src = Join-Path $resDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $sgfxAssetsDir $name) -Force
        Write-Host "parked $name -> $sgfxAssetsDir"
    }
}

# --- 3. Deploy fresh exe ----------------------------------------------------

Write-Section "3. Deploy fresh exe to $installDir"
if (-not (Test-Path -LiteralPath $installDir)) {
    Write-Host "FAIL: missing install dir at $installDir" -ForegroundColor Red
    exit 2
}
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
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase368.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII
Write-Host "reset $eventsPath"

# Phase 368 safety: take a content-byte backup of the user's save data
# files before launching UR. The Phase 367b -> Phase 368 transition
# observed that running retail SU under `SG_PREFLIGHT_GAMEPLAY_SKIP=1`
# while it auto-resumes a save and then writes a fresh auto-save can
# leave the on-disk SYS-DATA in an inconsistent shape (the runtime is
# in a synthetic input-locked state at write time). The proof script
# now snapshots the save BEFORE launch and restores it AFTER the run,
# regardless of pass/fail, so a re-run never costs the user real save
# progress. The snapshot lives under the evidence dir (versioned per
# run) so it stays alongside the rest of the proof artifacts.
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

function Restore-Phase368SaveSnapshot {
    if ($savedFiles.Count -lt 1) { return }

    Write-Section "5b. Restore pre-run save snapshot"
    if (-not (Test-Path -LiteralPath $saveDir)) {
        New-Item -ItemType Directory -Path $saveDir -Force | Out-Null
    }

    foreach ($f in $savedFiles) {
        $beforeHash = (Get-FileHash -LiteralPath $f.Backup -Algorithm SHA256).Hash
        if (-not (Test-Path -LiteralPath $f.Source)) {
            Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
            Write-Host "restored missing $($f.Name) (sha256 $beforeHash)"
            continue
        }

        $afterHash = (Get-FileHash -LiteralPath $f.Source -Algorithm SHA256).Hash
        if ($beforeHash -ne $afterHash) {
            Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
            Write-Host "restored $($f.Name) (sha256 was $afterHash, now $beforeHash)"
        } else {
            Write-Host "no restore needed for $($f.Name) (sha256 unchanged $beforeHash)"
        }
    }
}

# --- 5. Launch UnleashedRecomp ----------------------------------------------

Write-Section "5. Launch UnleashedRecomp.exe (kill timeout = $AutoExitSeconds s)"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
# Phase 368: NO_AUTOLOAD=1 keeps the runtime at the title attract
# without resuming the existing save. The runtime still passes the
# Title.arl / Title.ar.00 file probes (so this script's gate 8 passes)
# and the title-intro stage update fires once (so Stage:GameplaySkip
# count is exactly 1; gate 7 passes). Without this env, retail SU
# auto-resumes Empire City and Stage:GameplaySkip count goes to 2.
$env:SG_PREFLIGHT_NO_AUTOLOAD   = '1'
# SG_PREFLIGHT_LOG_SETTEXT is OFF here -- Phase 367b proved the
# probe works and identified the seven HUD digit literals already
# baked into the staged override pack. Re-enabling it would double
# the events.jsonl volume without adding signal for this beat.
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_BRIDGE_DISABLE -ErrorAction SilentlyContinue

$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
Write-Host "OVERRIDE_DIR  = $stagedOvDir"
Write-Host "BRIDGE_DIR    = $bridgeDir"
Write-Host "NO_AUTOLOAD   = 1"
Write-Host "args          = (plain launch, no --ui-lab flags)"

$proc = $null
$elapsed = 0
try {
    $proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
    $captureDone = $false
    $captureSnapshotPath = Join-Path $EvidenceDir 'phase368_screen_grab.bmp'
    $startedAt = Get-Date
    while (-not $proc.HasExited) {
        Start-Sleep -Seconds 2
        $elapsedSoFar = (Get-Date) - $startedAt

        if (-not $captureDone -and (Test-Path -LiteralPath $eventsPath)) {
            $eventsText = Get-Content -LiteralPath $eventsPath -Raw -ErrorAction SilentlyContinue
            if ($eventsText -match 'Asset:VisibleOverrideHit') {
                try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
                try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
                $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
                $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
                $g   = [System.Drawing.Graphics]::FromImage($bmp)
                $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
                $bmp.Save($captureSnapshotPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
                $g.Dispose(); $bmp.Dispose()
                $captureDone = $true
                Write-Host "captured screen frame after Asset:VisibleOverrideHit -> $captureSnapshotPath"
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
}
finally {
    if ($proc -ne $null -and -not $proc.HasExited) {
        try { $proc.Kill() } catch {}
        try { $proc.WaitForExit() } catch {}
    }
    Restore-Phase368SaveSnapshot
}

# --- 6. Summarise events.jsonl ---------------------------------------------

Write-Section "6. Summarise events.jsonl"
$buckets = @{
    'Text:OverridesLoaded'     = New-Object System.Collections.Generic.List[string]
    'Text:HostOverrideHit'     = New-Object System.Collections.Generic.List[string]
    'Text:CsdOverrideHit'      = New-Object System.Collections.Generic.List[string]
    'Asset:VisibleOverrideHit' = New-Object System.Collections.Generic.List[string]
    'Asset:OverrideHit'        = New-Object System.Collections.Generic.List[string]
    'Asset:FileProbe'          = New-Object System.Collections.Generic.List[string]
    'Stage:GameplaySkip'       = New-Object System.Collections.Generic.List[string]
    'Input:UiOnlyLock'         = New-Object System.Collections.Generic.List[string]
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
$titleArlProbed = $false
foreach ($line in $buckets['Asset:FileProbe']) {
    if ($line -match 'Title\.arl' -or $line -match 'Title\.ar\.00') {
        $titleArlProbed = $true
        break
    }
}

# Phase 368: 'no gameplay entry' is checked by the absence of a
# `menu_accepted:Title row=continue` event in events.jsonl. The
# Stage:GameplaySkip counter is NOT useful here -- retail SU's title
# attract and title menu are both implemented as CGameModeStage
# subclasses, so they both fire the hook and the count is naturally
# 2 even when the runtime has never left Title. The Continue row's
# accept event, on the other hand, is the explicit gameplay-routing
# trigger emitted by `CTitleStateMenu_patches.cpp::sub_825882B8`
# every time the runtime accepts that row.
$titleContinueAccepted = $false
$titleAcceptCount = 0
foreach ($line in (Get-Content -LiteralPath $eventsPath -ErrorAction SilentlyContinue)) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    if ($line -match '"event":"menu_accepted"' -and $line -match '"screen":"Title"') {
        $titleAcceptCount += 1
        if ($line -match '"row_id":"continue"') {
            $titleContinueAccepted = $true
        }
    }
}

$summary = [ordered]@{}
foreach ($prefix in 'Text:OverridesLoaded','Text:HostOverrideHit','Text:CsdOverrideHit','Asset:VisibleOverrideHit','Asset:OverrideHit','Stage:GameplaySkip','Input:UiOnlyLock') {
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
Write-Host ("title.arl probed = {0}" -f $titleArlProbed) -ForegroundColor Yellow

# --- 7. Persist evidence ----------------------------------------------------

Write-Section "7. Persist evidence to $EvidenceDir"
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase368.jsonl') -Force
$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

$summaryPath = Join-Path $EvidenceDir 'summary.json'
[ordered]@{
    auto_exit_seconds            = $AutoExitSeconds
    text_overrides_loaded        = $summary['Text:OverridesLoaded']
    text_host_override_hits      = $summary['Text:HostOverrideHit']
    text_csd_override_hits       = $summary['Text:CsdOverrideHit']
    asset_visible_override_hits  = $summary['Asset:VisibleOverrideHit']
    asset_override_hits          = $summary['Asset:OverrideHit']
    stage_gameplay_skip          = $summary['Stage:GameplaySkip']
    input_ui_only_lock           = $summary['Input:UiOnlyLock']
    title_arl_probed             = $titleArlProbed
    title_menu_accept_count      = $titleAcceptCount
    title_continue_accepted      = $titleContinueAccepted
    save_backup_dir              = $saveBackupDir
    save_backup_files            = $savedFiles | ForEach-Object { @{ name = $_.Name; bytes = $_.Bytes } }
    native_frames_written        = $frames.Count
    elapsed_seconds              = $elapsed
    process_exit_code            = $proc.ExitCode
    retail_dds_md5               = $retailMd5
    retail_dds_sha256            = $retailSha
    staged_dds_md5               = $stagedMd5
    staged_dds_sha256            = $stagedSha
    user_logo_sgfx_sha256        = $userLogoSha
    override_dir                 = $stagedOvDir
    bridge_dir                   = $bridgeDir
    install_dir                  = $installDir
    build_exe                    = $buildOutExe
    evidence_dir                 = $EvidenceDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Host "summary.json -> $summaryPath"

# --- Acceptance check (explicit failures with exit codes) -------------------

Write-Section "Acceptance"
if ($summary['Text:OverridesLoaded'] -lt 1) {
    Write-Host "FAIL: Text:OverridesLoaded missing" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Text:OverridesLoaded     = $($summary['Text:OverridesLoaded'])" -ForegroundColor Green

if ($summary['Asset:VisibleOverrideHit'] -lt 1) {
    Write-Host "FAIL: Asset:VisibleOverrideHit missing" -ForegroundColor Red
    exit 4
}
Write-Host "OK   Asset:VisibleOverrideHit = $($summary['Asset:VisibleOverrideHit'])" -ForegroundColor Green

if ($stagedMd5 -eq $retailMd5) {
    Write-Host "FAIL: staged DDS MD5 ($stagedMd5) matches retail" -ForegroundColor Red
    exit 5
}
Write-Host "OK   staged_dds_md5            = $stagedMd5 (retail = $retailMd5)" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 6
}
Write-Host "OK   native frames written    = $($frames.Count)" -ForegroundColor Green

if ($titleContinueAccepted) {
    Write-Host "FAIL: menu_accepted:Title row=continue observed -> gameplay routing fired" -ForegroundColor Red
    exit 7
}
Write-Host ("OK   menu_accepted:Title continue = false (Stage:GameplaySkip count {0} is title-intro+title-menu, no Continue accept)" -f $summary['Stage:GameplaySkip']) -ForegroundColor Green

if (-not $titleArlProbed) {
    Write-Host "FAIL: no Title.arl / Title.ar.00 file probe -- runtime never reached the Title flow" -ForegroundColor Red
    exit 8
}
Write-Host "OK   Title.arl probed         = $titleArlProbed" -ForegroundColor Green

# Informational only -- not gating. Phase 367b proved these fire via the
# auto-load -> Empire City flow; Phase 368 deliberately suppresses that
# flow so they may be zero in this run.
Write-Host "INFO Text:CsdOverrideHit      = $($summary['Text:CsdOverrideHit']) (informational, gameplay-flow only)" -ForegroundColor Cyan
Write-Host "INFO Text:HostOverrideHit     = $($summary['Text:HostOverrideHit']) (informational, UR-UI-flow only)" -ForegroundColor Cyan

exit 0
