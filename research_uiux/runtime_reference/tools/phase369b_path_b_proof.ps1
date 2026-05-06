# Phase 369B -- Path B scoped text override proof.
#
# Builds on Phase 367b/368/369A. Goal: prove the v2
# `scoped_rules` lane in `sg_text_overrides.json` works AND the
# v1 `strings` global lane still works alongside it. The motivating
# bug: Phase 367b's "99 -> BMW99" global rule fired the override
# correctly but ALSO fired in any other CSD scene where the literal
# "99" appeared, producing visible artifacts (clipped digits, green
# rectangles). v2 narrows the rule to a CSD project substring so a
# rule scoped to `playscreen` only matches the gameplay HUD.
#
# Acceptance gates (each fails with a distinct exit code):
#   2  build / deploy / launch failed
#   3  Text:ScopedRulesLoaded missing (loader didn't see v2 schema)
#   4  Text:CsdScopedOverrideHit:</dependency>@status missing
#      (scoped rule didn't match a runtime-observed SetText literal
#      after the `ui_status` project was registered)
#   5  Text:CsdOverrideHit:99 OBSERVED (the scoped rule was
#      bypassed and the global lane fired -- this is the BMW999
#      regression we're trying to prevent)
#   6  Text:CsdScopedOverrideHit for the negative-control rule
#      OBSERVED (a rule with a scope that should never match did
#      match -- means the substring matcher is too loose)
#   7  no native BMP captured during the run
#
# Save-data safety: full snapshot+restore, same as Phase 368.
#
# This run intentionally allows the auto-load / title flow (NO_AUTOLOAD
# is OFF) and enables the bounded SetText sampler. The positive scoped
# control targets a runtime-observed literal that fires after `ui_status`
# is registered, so the proof is independent of whether the current save
# reaches the Empire City HUD digit route.

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 60,
    [string]$EvidenceDir
)

$ErrorActionPreference = 'Stop'

$scriptDir    = Split-Path -Parent $PSCommandPath
$repoRoot     = Resolve-Path (Join-Path $scriptDir '..\..\..')
$buildBat     = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir   = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'

$bridgeDir    = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase369b'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase369b_path_b_proof'
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

# --- 2. Stage v2 override pack ----------------------------------------------

Write-Section "2. Stage v2 override pack at $stagedOvDir"
if (Test-Path -LiteralPath $stagedOvDir) {
    Remove-Item -LiteralPath $stagedOvDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagedOvDir -Force | Out-Null

# v2 manifest: ONE positive scoped rule (`</dependency> -> SGFX_DEP`
# only when a CSD project containing "status" is active) AND ONE negative-
# control scoped rule (`35 -> BMW35` only when "ZZZNeverActive" is
# active -- a substring no real CSD project name contains, so the
# rule should never match). The global `strings` map is left
# empty so the global lane cannot mask the scoped path.
#
# Phase 369B prevents the BMW999 regression pattern: with the global
# lane empty, scoped rules are the only CSD override path. If the bridge
# ever emits `Text:CsdOverrideHit:99` during this
# run, the scoped lane was bypassed (gate 5).
$manifest = @"
{
  "version": 2,
  "strings": {},
  "scoped_rules": [
    { "literal": "</dependency>",
      "csd_project_substring": "status",
      "replacement": "SGFX_DEP" },
    { "literal": "35",
      "csd_project_substring": "ZZZNeverActive_phase369b_negative_control",
      "replacement": "BMW35_should_never_appear" }
  ]
}
"@
$manifestPath = Join-Path $stagedOvDir 'sg_text_overrides.json'
$manifest | Set-Content -LiteralPath $manifestPath -Encoding UTF8
Write-Host "wrote $manifestPath"

# --- 3. Deploy fresh exe ----------------------------------------------------

Write-Section "3. Deploy fresh exe to $installDir"
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
    }
}

# --- 4. Reset events.jsonl + back up save data ----------------------------

Write-Section "4. Reset bridge events.jsonl + back up save data"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase369b.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

# Save backup safety net (Phase 368 lesson).
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

# --- 5. Launch UnleashedRecomp ----------------------------------------------

Write-Section "5. Launch UnleashedRecomp.exe (kill timeout = $AutoExitSeconds s)"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
# Phase 369B re-lights the auto-load -> Empire City flow that Phase
# 367b proved fires HUD literals. NO_AUTOLOAD must be OFF for this
# run -- if we suppress save-resume, the HUD CSD project never
# loads and the scoped rule never has a chance to fire.
Remove-Item Env:SG_PREFLIGHT_NO_AUTOLOAD -ErrorAction SilentlyContinue
# Keep the bounded SetText sampler enabled for this proof. It makes
# route drift visible in events.jsonl if the current save no longer
# produces the positive-control literal.
$env:SG_PREFLIGHT_LOG_SETTEXT = '1'
Remove-Item Env:SG_PREFLIGHT_BRIDGE_DISABLE -ErrorAction SilentlyContinue

$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
Write-Host "OVERRIDE_DIR  = $stagedOvDir"
Write-Host "BRIDGE_DIR    = $bridgeDir"
Write-Host "NO_AUTOLOAD   = (off, gameplay flow expected)"
Write-Host "args          = (plain launch, no --ui-lab flags)"

$proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
$captureBmp = Join-Path $EvidenceDir 'phase369b_screen_grab.bmp'
$captureDone = $false
$startedAt = Get-Date
while (-not $proc.HasExited) {
    Start-Sleep -Seconds 2
    $elapsedSoFar = (Get-Date) - $startedAt

    if (-not $captureDone -and (Test-Path -LiteralPath $eventsPath)) {
        $eventsText = Get-Content -LiteralPath $eventsPath -Raw -ErrorAction SilentlyContinue
        if ($eventsText -match 'Text:CsdScopedOverrideHit') {
            try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
            try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
            $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
            $g   = [System.Drawing.Graphics]::FromImage($bmp)
            $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
            $bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
            $g.Dispose(); $bmp.Dispose()
            $captureDone = $true
            Write-Host "captured screen frame after Text:CsdScopedOverrideHit -> $captureBmp"
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

# Restore save snapshot.
if ($savedFiles.Count -gt 0) {
    Write-Section "5b. Restore pre-run save snapshot"
    foreach ($f in $savedFiles) {
        if (Test-Path -LiteralPath $f.Source) {
            $beforeHash = (Get-FileHash -LiteralPath $f.Backup -Algorithm SHA256).Hash
            $afterHash  = (Get-FileHash -LiteralPath $f.Source -Algorithm SHA256).Hash
            if ($beforeHash -ne $afterHash) {
                Copy-Item -LiteralPath $f.Backup -Destination $f.Source -Force
                Write-Host "restored $($f.Name)"
            } else {
                Write-Host "no restore needed for $($f.Name)"
            }
        }
    }
}

# --- 6. Summarise events.jsonl ---------------------------------------------

Write-Section "6. Summarise events.jsonl"
$events = @()
if (Test-Path -LiteralPath $eventsPath) {
    $events = Get-Content -LiteralPath $eventsPath -ErrorAction SilentlyContinue
}
$scopedRulesLoaded = 0
$scopedHitsByKey   = [System.Collections.Generic.Dictionary[string,int]]::new()
$globalCsdHits     = [System.Collections.Generic.Dictionary[string,int]]::new()
$activeProjects    = [System.Collections.Generic.HashSet[string]]::new()
foreach ($line in $events) {
    if ($line -match '"screen":"Text:ScopedRulesLoaded:(\d+)"') {
        $scopedRulesLoaded = [int]$Matches[1]
    }
    if ($line -match '"screen":"Text:CsdScopedOverrideHit:([^"]+)"') {
        $key = $Matches[1]
        if ($scopedHitsByKey.ContainsKey($key)) {
            $scopedHitsByKey[$key] = $scopedHitsByKey[$key] + 1
        } else {
            $scopedHitsByKey[$key] = 1
        }
    }
    if ($line -match '"screen":"Text:CsdOverrideHit:([^"]+)"') {
        $key = $Matches[1]
        if ($globalCsdHits.ContainsKey($key)) {
            $globalCsdHits[$key] = $globalCsdHits[$key] + 1
        } else {
            $globalCsdHits[$key] = 1
        }
    }
    if ($line -match '"screen":"Text:CsdProjectActive:([^"]+)"') {
        [void]$activeProjects.Add($Matches[1])
    }
}
$activeProjectsArray = [string[]]::new($activeProjects.Count)
$activeProjects.CopyTo($activeProjectsArray)
$scopedHitKeysArray  = [string[]]::new($scopedHitsByKey.Count)
$scopedHitsByKey.Keys.CopyTo($scopedHitKeysArray, 0)
$globalHitKeysArray  = [string[]]::new($globalCsdHits.Count)
$globalCsdHits.Keys.CopyTo($globalHitKeysArray, 0)

Write-Host ("Text:ScopedRulesLoaded     = {0}" -f $scopedRulesLoaded)
Write-Host ("Text:CsdProjectActive      = {0} unique projects" -f $activeProjects.Count)
foreach ($projName in $activeProjectsArray) {
    Write-Host ("  {0}" -f $projName)
}
Write-Host ("Text:CsdScopedOverrideHit  = {0} unique keys" -f $scopedHitsByKey.Count)
foreach ($scopedKey in $scopedHitKeysArray) {
    Write-Host ("  {0} (x{1})" -f $scopedKey, $scopedHitsByKey[$scopedKey])
}
Write-Host ("Text:CsdOverrideHit (global) = {0} unique keys" -f $globalCsdHits.Count)
foreach ($globalKey in $globalHitKeysArray) {
    Write-Host ("  {0} (x{1})" -f $globalKey, $globalCsdHits[$globalKey])
}

$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

# --- 7. Persist evidence ----------------------------------------------------

Write-Section "7. Persist evidence to $EvidenceDir"
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase369b.jsonl') -Force

[ordered]@{
    auto_exit_seconds            = $AutoExitSeconds
    scoped_rules_loaded          = $scopedRulesLoaded
    scoped_hit_keys              = $scopedHitKeysArray
    global_hit_keys              = $globalHitKeysArray
    active_csd_projects          = $activeProjectsArray
    native_frames_written        = $frames.Count
    elapsed_seconds              = $elapsed
    process_exit_code            = $proc.ExitCode
    override_dir                 = $stagedOvDir
    bridge_dir                   = $bridgeDir
    install_dir                  = $installDir
    build_exe                    = $buildOutExe
    evidence_dir                 = $EvidenceDir
    save_backup_dir              = $saveBackupDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $EvidenceDir 'summary.json') -Encoding UTF8

# --- Acceptance check -------------------------------------------------------

Write-Section "Acceptance"
if ($scopedRulesLoaded -lt 1) {
    Write-Host "FAIL: Text:ScopedRulesLoaded missing or zero -- v2 schema not parsed" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Text:ScopedRulesLoaded   = $scopedRulesLoaded" -ForegroundColor Green

$positiveKey = '</dependency>@status'
if (-not $scopedHitsByKey.ContainsKey($positiveKey)) {
    Write-Host "FAIL: Text:CsdScopedOverrideHit:$positiveKey missing -- positive scoped rule did not match the runtime-observed status project literal" -ForegroundColor Red
    Write-Host "      events captured:"
    foreach ($k in $scopedHitsByKey.Keys) { Write-Host "        $k" }
    exit 4
}
Write-Host "OK   Text:CsdScopedOverrideHit:$positiveKey fired" -ForegroundColor Green

if ($globalCsdHits.ContainsKey('99')) {
    Write-Host "FAIL: Text:CsdOverrideHit:99 OBSERVED -- global lane fired despite scoped rule existing" -ForegroundColor Red
    exit 5
}
Write-Host "OK   Text:CsdOverrideHit:99 NOT observed (scoped lane took precedence)" -ForegroundColor Green

$negativeKey = '35@ZZZNeverActive_phase369b_negative_control'
if ($scopedHitsByKey.ContainsKey($negativeKey)) {
    Write-Host "FAIL: Text:CsdScopedOverrideHit:$negativeKey OBSERVED -- the negative-control scope substring matched a real project (substring match is too loose)" -ForegroundColor Red
    exit 6
}
Write-Host "OK   negative-control rule did NOT fire (scope substring matcher is precise)" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 7
}
Write-Host "OK   native frames written    = $($frames.Count)" -ForegroundColor Green

exit 0
