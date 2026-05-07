# Phase 370B -- sgfx_pack.json as the primary pack index.
#
# Goal: prove that when `<override>/sgfx_pack.json` is present, the
# pack's `text_overrides` / `asset_overrides` / `loose_files` paths
# drive the text/asset/loose loaders, and the legacy flat
# `sg_text_overrides.json` / `sg_asset_overrides.json` files in the
# same dir are NOT auto-loaded as a fallback. Pack-mode is
# authoritative.
#
# Two sub-tests run in sequence:
#
#   A) Positive: stage a pack with all three lanes pointing at
#      DIFFERENT files than the legacy flat names. Stage decoy flat
#      files in the same dir with content that should NEVER load.
#      Run UR through the auto-load -> Empire City flow. Verify the
#      pack-pointed manifests loaded, the decoys did not, and the
#      pack's loose_file substitution fired.
#
#   B) Path-traversal negative: re-stage the pack with text /
#      loose paths that use `..` to escape the override dir. Re-run
#      briefly. Verify Pack:Rejected events fire for each escaping
#      path AND the escape targets did NOT load.
#
# Acceptance gates (each fails with a distinct exit code):
#   2  build / deploy / launch failed
#   3  Pack:Loaded missing in test A
#   4  pack-pointed text manifest did not load (pack key not
#      observed in scoped-hit list; means the pack lane was bypassed)
#   5  pack-pointed asset manifest did not fire `Asset:PixelOverrideHit`
#   6  pack-pointed loose file did not fire `Asset:VisibleOverrideHit`
#   7  decoy flat file's content WAS loaded (text or asset
#      manifest's flat copy unexpectedly fell through)
#   8  path-traversal test did not emit Pack:Rejected events
#   9  no native BMP captured

[CmdletBinding()]
param(
    [int]$AutoExitSecondsA = 60,
    [int]$AutoExitSecondsB = 12,
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
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase370b'
$negStagedDir = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase370b_negative'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase370b_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
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

# --- 2A. Stage pack-driven override (positive test) -----------------------

Write-Section "2A. Stage pack-driven override at $stagedOvDir"
if (Test-Path -LiteralPath $stagedOvDir) {
    Remove-Item -LiteralPath $stagedOvDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagedOvDir -Force | Out-Null

# 2A.i. Pack-pointed text manifest at pack-relative path. The pack
# uses a UNIQUE scoped rule whose replacement value (`PACK_16`) is
# different from any flat-file decoy. The Text:CsdScopedOverrideHit
# event encodes the scope name, not the replacement, so the proof
# distinguishes lanes by which event-namespace fires.
$textPackDir   = Join-Path $stagedOvDir 'text'
New-Item -ItemType Directory -Path $textPackDir -Force | Out-Null
$packTextPath  = Join-Path $textPackDir 'sgfx_text.json'
@"
{
  "version": 2,
  "strings": {},
  "scoped_rules": [
    { "literal": "99",     "csd_project_substring": "status", "replacement": "PACK_99"  },
    { "literal": "999999", "csd_project_substring": "status", "replacement": "PACK_999999" },
    { "literal": "16",     "csd_project_substring": "status", "replacement": "PACK_16"  },
    { "literal": "35",     "csd_project_substring": "status", "replacement": "PACK_35"  },
    { "literal": "30",     "csd_project_substring": "status", "replacement": "PACK_30"  },
    { "literal": "7",      "csd_project_substring": "status", "replacement": "PACK_7"   },
    { "literal": "[200]",  "csd_project_substring": "status", "replacement": "PACK_200" }
  ]
}
"@ | Set-Content -LiteralPath $packTextPath -Encoding UTF8
Write-Host "wrote pack text manifest: $packTextPath"

# 2A.ii. Pack-pointed asset manifest -> user's logo_sgfx.dds at the
# logo_sonicteam picture slot.
$picsPackDir   = Join-Path $stagedOvDir 'pictures'
New-Item -ItemType Directory -Path $picsPackDir -Force | Out-Null
$packPicsPath  = Join-Path $picsPackDir 'sgfx_pictures.json'
$userLogo      = Join-Path $resDir 'logo_sgfx.dds'
$packPicCopy   = Join-Path $picsPackDir 'logo_sgfx.dds'
Copy-Item -LiteralPath $userLogo -Destination $packPicCopy -Force
@"
{
  "version": 1,
  "pictures": {
    "logo_sonicteam": "pictures/logo_sgfx.dds"
  }
}
"@ | Set-Content -LiteralPath $packPicsPath -Encoding UTF8
Write-Host "wrote pack asset manifest: $packPicsPath"

# 2A.iii. Pack-pointed loose file at the canonical guest-relative
# path under the override dir. mod_loader::ResolvePath looks for
# `<override>/Loading/logo_sonicteam.dds` when retail SU's loader
# asks for `game:\Loading\logo_sonicteam.dds`, so the pack file
# must mirror that path. The pack's `loose_files` array still acts
# as the whitelist that mod_loader uses to SCOPE the loose-asset
# index for sub_82E0B500 (archive-entry lane); files outside the
# pack's loose_files list are not indexed there even if they sit
# in the override dir tree.
$looseDir = Join-Path $stagedOvDir 'Loading'
New-Item -ItemType Directory -Path $looseDir -Force | Out-Null
$looseDds = Join-Path $looseDir 'logo_sonicteam.dds'
$retailDds = Join-Path $installDir 'game\Loading\logo_sonicteam.dds'
$retailBytes = [System.IO.File]::ReadAllBytes($retailDds)
$trailer = [System.Text.Encoding]::UTF8.GetBytes("PHASE370B_PACK_LOOSE_v1`n")
$out = New-Object byte[] ($retailBytes.Length + $trailer.Length)
[Array]::Copy($retailBytes, 0, $out, 0, $retailBytes.Length)
[Array]::Copy($trailer, 0, $out, $retailBytes.Length, $trailer.Length)
[System.IO.File]::WriteAllBytes($looseDds, $out)
Write-Host "wrote pack loose file: $looseDds"

# 2A.iv. sgfx_pack.json that ties it all together.
$packPath = Join-Path $stagedOvDir 'sgfx_pack.json'
@"
{
  "version": 1,
  "name": "Phase 370B pack-driven proof",
  "description": "Pack supersedes flat-file loaders; flat files are decoys.",
  "branding": { "build_label": "SGFX Phase 370B pack-mode" },
  "text_overrides":  "text/sgfx_text.json",
  "asset_overrides": "pictures/sgfx_pictures.json",
  "loose_files":     ["Loading/logo_sonicteam.dds"]
}
"@ | Set-Content -LiteralPath $packPath -Encoding UTF8
Write-Host "wrote pack: $packPath"

# 2A.v. Decoy flat files. These MUST NOT be auto-loaded because the
# pack is present. The decoy text rule is a global v1 entry whose
# emit (`Text:CsdOverrideHit:99`) is distinct from the pack's
# scoped emit, so the proof can detect a wrong-lane fall-through.
$flatTextPath = Join-Path $stagedOvDir 'sg_text_overrides.json'
@"
{
  "version": 1,
  "strings": { "99": "DECOY_FLAT_99" }
}
"@ | Set-Content -LiteralPath $flatTextPath -Encoding UTF8
Write-Host "wrote decoy flat text: $flatTextPath"

$flatAssetPath = Join-Path $stagedOvDir 'sg_asset_overrides.json'
@"
{
  "version": 1,
  "pictures": { "logo_havok": "pictures/logo_sgfx.dds" }
}
"@ | Set-Content -LiteralPath $flatAssetPath -Encoding UTF8
Write-Host "wrote decoy flat asset: $flatAssetPath"

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
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase370b.jsonl') -Force
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

# --- 5A. Launch UR with pack-driven override (positive test) -------------

Write-Section "5A. Launch UR with pack-driven override"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
Remove-Item Env:SG_PREFLIGHT_NO_AUTOLOAD -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL   -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_EXE_NAME      -ErrorAction SilentlyContinue

$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
Write-Host "OVERRIDE_DIR = $stagedOvDir"
$proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
$captureBmp = Join-Path $EvidenceDir 'phase370b_screen_grab.bmp'
$captureDone = $false
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
            $bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
            $g.Dispose(); $bmp.Dispose()
            $captureDone = $true
            Write-Host "captured frame -> $captureBmp"
        }
    }

    if ($elapsedSoFar.TotalSeconds -ge $AutoExitSecondsA) {
        Write-Host "elapsed $([int]$elapsedSoFar.TotalSeconds)s >= timeout, killing UR"
        $proc.Kill()
        break
    }
}
$proc.WaitForExit()
$elapsedA = [int]((Get-Date) - $startedAt).TotalSeconds
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_5A.jsonl') -Force
Write-Host "5A elapsed = $elapsedA s"

# Restore save (engaged when retail SU's auto-resume writes back).
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

# --- 6A. Parse 5A events --------------------------------------------------

Write-Section "6A. Parse 5A events"
$packLoadedLine = $null
$scopedHits     = [System.Collections.Generic.HashSet[string]]::new()
$globalHits     = [System.Collections.Generic.HashSet[string]]::new()
$pixelHits      = [System.Collections.Generic.HashSet[string]]::new()
$visibleHits    = [System.Collections.Generic.HashSet[string]]::new()
$pixelLoadedCnt = -1
$textLoadedCnt  = -1
$scopedRulesCnt = -1
foreach ($line in (Get-Content -LiteralPath (Join-Path $EvidenceDir 'events_post_5A.jsonl'))) {
    if ($line -match '"screen":"Pack:Loaded:([^"]+)"') {
        $packLoadedLine = $Matches[1]
    }
    elseif ($line -match '"screen":"Text:OverridesLoaded:(\d+)"') {
        $textLoadedCnt = [int]$Matches[1]
    }
    elseif ($line -match '"screen":"Text:ScopedRulesLoaded:(\d+)"') {
        $scopedRulesCnt = [int]$Matches[1]
    }
    elseif ($line -match '"screen":"Asset:PixelOverridesLoaded:(\d+)"') {
        $pixelLoadedCnt = [int]$Matches[1]
    }
    elseif ($line -match '"screen":"Text:CsdScopedOverrideHit:([^"]+)"') {
        [void]$scopedHits.Add($Matches[1])
    }
    elseif ($line -match '"screen":"Text:CsdOverrideHit:([^"]+)"') {
        [void]$globalHits.Add($Matches[1])
    }
    elseif ($line -match '"screen":"Asset:PixelOverrideHit:([^"]+)"') {
        [void]$pixelHits.Add($Matches[1])
    }
    elseif ($line -match '"screen":"Asset:VisibleOverrideHit:([^"]+)"') {
        [void]$visibleHits.Add($Matches[1])
    }
}
Write-Host ("Pack:Loaded               = `"{0}`"" -f ($packLoadedLine -as [string]))
Write-Host ("Text:OverridesLoaded      = {0}" -f $textLoadedCnt)
Write-Host ("Text:ScopedRulesLoaded    = {0}" -f $scopedRulesCnt)
Write-Host ("Asset:PixelOverridesLoaded= {0}" -f $pixelLoadedCnt)
Write-Host ("Text:CsdScopedOverrideHit = {0} unique" -f $scopedHits.Count)
foreach ($h in $scopedHits) { Write-Host "  $h" }
Write-Host ("Text:CsdOverrideHit       = {0} unique" -f $globalHits.Count)
foreach ($h in $globalHits) { Write-Host "  $h" }
Write-Host ("Asset:PixelOverrideHit    = {0} unique" -f $pixelHits.Count)
foreach ($h in $pixelHits) { Write-Host "  $h" }
Write-Host ("Asset:VisibleOverrideHit  = {0} unique" -f $visibleHits.Count)
foreach ($h in $visibleHits) { Write-Host "  $h" }

# --- 7. Path-traversal negative test -------------------------------------

Write-Section "7. Path-traversal negative test"
if (Test-Path -LiteralPath $negStagedDir) {
    Remove-Item -LiteralPath $negStagedDir -Recurse -Force
}
New-Item -ItemType Directory -Path $negStagedDir -Force | Out-Null

# Stage a "decoy" file in the parent dir of the override pack and
# point the pack at it via `..`. SGPack::ResolveRelativeUnderBase
# must reject these and emit Pack:Rejected events. The escape
# target file actually exists so the proof verifies the rejection
# is geometric (".." in the path), not just "file not found".
$escapeText = Join-Path (Split-Path $negStagedDir -Parent) 'escape_text.json'
$escapeDds  = Join-Path (Split-Path $negStagedDir -Parent) 'escape_loose.dds'
'{"version":1,"strings":{"99":"ESCAPE_TEXT_LEAKED"}}' | Set-Content -LiteralPath $escapeText -Encoding UTF8
[System.IO.File]::WriteAllBytes($escapeDds, [byte[]](0x44,0x44,0x53,0x20))  # raw "DDS " magic, 4 bytes

# Also stage a valid physical loose-file override at a path retail
# definitely requests, but DO NOT list it in the negative pack's
# `loose_files`. Pack mode must keep this file invisible to the
# direct ResolvePath lane too, not only to the archive-entry loose
# index. If `Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds`
# fires in this sub-test, the pack whitelist is leaking.
$negLooseDir = Join-Path $negStagedDir 'Loading'
New-Item -ItemType Directory -Path $negLooseDir -Force | Out-Null
$negUnlistedLoose = Join-Path $negLooseDir 'logo_sonicteam.dds'
Copy-Item -LiteralPath $retailDds -Destination $negUnlistedLoose -Force
$negPack = Join-Path $negStagedDir 'sgfx_pack.json'
@"
{
  "version": 1,
  "name": "Phase 370B path-traversal negative",
  "text_overrides":  "../escape_text.json",
  "asset_overrides": "../escape_text.json",
  "loose_files":     ["../escape_loose.dds"]
}
"@ | Set-Content -LiteralPath $negPack -Encoding UTF8

Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII
$env:SG_PREFLIGHT_OVERRIDE_DIR = $negStagedDir
$proc2 = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
Start-Sleep -Seconds $AutoExitSecondsB
if (-not $proc2.HasExited) {
    Write-Host "killing negative-test UR"
    $proc2.Kill()
}
$proc2.WaitForExit()
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_7.jsonl') -Force

# Save restore for negative test too.
foreach ($f in $savedFiles) {
    if (Test-Path -LiteralPath $f.Source) {
        $beforeHash = (Get-FileHash -LiteralPath $f.Backup -Algorithm SHA256).Hash
        $afterHash  = (Get-FileHash -LiteralPath $f.Source -Algorithm SHA256).Hash
        if ($beforeHash -ne $afterHash) {
            Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
            Write-Host "restored $($f.Name) (negative test)"
        }
    }
}

$rejectEvents = [System.Collections.Generic.List[string]]::new()
$leaked = $false
foreach ($line in (Get-Content -LiteralPath (Join-Path $EvidenceDir 'events_post_7.jsonl'))) {
    if ($line -match '"screen":"Pack:Rejected:([^"]+)"') {
        $rejectEvents.Add($Matches[1])
    }
    if ($line -match 'ESCAPE_TEXT_LEAKED') {
        $leaked = $true
    }
    if ($line -match '"screen":"Text:CsdOverrideHit:99"' -or
        $line -match '"screen":"Asset:OverrideHit:.*escape_loose' -or
        $line -match '"screen":"Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds"') {
        $leaked = $true
    }
}
Write-Host ("Pack:Rejected events = {0}" -f $rejectEvents.Count)
foreach ($r in $rejectEvents) { Write-Host "  $r" }
Write-Host ("escape leaked        = {0}" -f $leaked)

# Cleanup the escape decoys so they don't sit at user disk.
Remove-Item -LiteralPath $escapeText -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $escapeDds  -Force -ErrorAction SilentlyContinue

# --- 8. Persist evidence + summary ---------------------------------------

Write-Section "8. Persist evidence"
$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)
[ordered]@{
    auto_exit_seconds_a            = $AutoExitSecondsA
    auto_exit_seconds_b            = $AutoExitSecondsB
    pack_loaded_payload            = $packLoadedLine
    text_overrides_loaded          = $textLoadedCnt
    text_scoped_rules_loaded       = $scopedRulesCnt
    asset_pixel_overrides_loaded   = $pixelLoadedCnt
    scoped_hits                    = @($scopedHits)
    global_hits                    = @($globalHits)
    pixel_hits                     = @($pixelHits)
    visible_hits                   = @($visibleHits)
    pack_rejected_events           = @($rejectEvents)
    escape_leaked                  = $leaked
    native_frames_written          = $frames.Count
    elapsed_seconds_a              = $elapsedA
    override_dir_a                 = $stagedOvDir
    override_dir_b                 = $negStagedDir
    bridge_dir                     = $bridgeDir
    install_dir                    = $installDir
    build_exe                      = $buildOutExe
    evidence_dir                   = $EvidenceDir
    save_backup_dir                = $saveBackupDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $EvidenceDir 'summary.json') -Encoding UTF8

# --- Acceptance check ----------------------------------------------------

Write-Section "Acceptance"

if (-not $packLoadedLine) {
    Write-Host "FAIL: Pack:Loaded missing in 5A" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Pack:Loaded = $packLoadedLine" -ForegroundColor Green

# Pack text manifest loaded => the pack-declared rule count is on
# the bridge AND at least one scoped hit fired during the window.
# The exact set of HUD literals that hit `sub_830BF640::SetText` in
# any given gameplay window varies (Phase 367b captured 7 of them
# across a 60 s window; a single rerun typically hits a subset),
# so the gate is "any scoped hit observed" rather than a specific
# literal. The pack stages all seven Phase 367b-observed literals
# scoped to `status` to maximise the chance one fires.
if ($scopedRulesCnt -lt 1) {
    Write-Host "FAIL: pack-pointed text manifest didn't load (Text:ScopedRulesLoaded = $scopedRulesCnt)" -ForegroundColor Red
    exit 4
}
if ($scopedHits.Count -lt 1) {
    Write-Host "FAIL: no Text:CsdScopedOverrideHit observed; pack text manifest loaded but no rule matched a runtime SetText literal in the gameplay window" -ForegroundColor Red
    exit 4
}
$packScopedHits = @($scopedHits | Where-Object { $_ -like '*@status' })
if ($packScopedHits.Count -lt 1) {
    Write-Host "FAIL: scoped hits observed but none with @status scope (pack rules use @status)" -ForegroundColor Red
    foreach ($k in $scopedHits) { Write-Host "  observed: $k" }
    exit 4
}
Write-Host "OK   Text:CsdScopedOverrideHit @status fired ($($packScopedHits -join ', ')) -- pack text manifest active" -ForegroundColor Green

if ($pixelLoadedCnt -lt 1) {
    Write-Host "FAIL: pack-pointed asset manifest didn't load (Asset:PixelOverridesLoaded = $pixelLoadedCnt)" -ForegroundColor Red
    exit 5
}
if (-not $pixelHits.Contains('logo_sonicteam')) {
    Write-Host "FAIL: Asset:PixelOverrideHit:logo_sonicteam missing; pack-driven asset manifest didn't bind" -ForegroundColor Red
    exit 5
}
Write-Host "OK   Asset:PixelOverrideHit:logo_sonicteam fired (pack asset manifest active)" -ForegroundColor Green

$visiblePackHit = $false
foreach ($v in $visibleHits) {
    if ($v -match 'Loading/logo_sonicteam.dds') { $visiblePackHit = $true; break }
}
if (-not $visiblePackHit) {
    Write-Host "FAIL: Asset:VisibleOverrideHit for the pack's loose_files entry was not observed" -ForegroundColor Red
    exit 6
}
Write-Host "OK   Asset:VisibleOverrideHit fired for pack loose_files entry" -ForegroundColor Green

# Decoy guard: no global Text:CsdOverrideHit:99 (decoy flat key)
# AND no Asset:PixelOverrideHit for `logo_havok` (decoy flat key).
if ($globalHits.Contains('99')) {
    Write-Host "FAIL: decoy flat text fell through -- Text:CsdOverrideHit:99 observed (would mean flat sg_text_overrides.json was auto-loaded)" -ForegroundColor Red
    exit 7
}
if ($pixelHits.Contains('logo_havok')) {
    Write-Host "FAIL: decoy flat asset fell through -- Asset:PixelOverrideHit:logo_havok observed" -ForegroundColor Red
    exit 7
}
Write-Host "OK   no decoy flat-file fall-through (pack-mode authoritative)" -ForegroundColor Green

# Path traversal: at least one Pack:Rejected event AND no escape
# evidence in the negative-test events.
if ($rejectEvents.Count -lt 1) {
    Write-Host "FAIL: path-traversal test did not emit any Pack:Rejected events" -ForegroundColor Red
    exit 8
}
if ($leaked) {
    Write-Host "FAIL: traversal escape target leaked into runtime (decoy keys / replacements observed in events)" -ForegroundColor Red
    exit 8
}
Write-Host "OK   Pack:Rejected fired for traversal paths and no escape leaked" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 9
}
Write-Host "OK   native frames written = $($frames.Count)" -ForegroundColor Green

exit 0
