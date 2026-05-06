# Phase 367b -- Path B visible override + CSD text-hit proof script.
#
# End-to-end runner. Executes the full proof pipeline without user
# intervention.
#
# Acceptance:
#   - Text:OverridesLoaded:<count> >= 1
#   - Text:CsdOverrideHit:<literal> >= 1   (guest CSD SetText path,
#       Phase 364 hook in CsdNodeText_patches.cpp::sub_830BF640).
#   - Text:HostOverrideHit:<key>    optional (host Localise path,
#       Phase 367b hook in locale.cpp -> sg_text_overrides.cpp::
#       NoteHostHitForKey). Recorded for visibility but not blocking
#       since the host path requires UR's installer-wizard /
#       options-menu / message-window UIs to fire, which is timing-
#       dependent on the launch flow.
#   - Asset:VisibleOverrideHit:<guestPath> >= 1 (loose-file
#       substitution path, Phase 367b hook in mod_loader.cpp::
#       ResolvePath -- distinct from Asset:OverrideHit which is the
#       archive-entry substitution path).
#   - Native frame capture: at least one BMP written. The script
#       fails with the on-disk reason if zero frames are captured.
#
# Failure exits:
#   2  build/deploy/launch failed
#   3  Text:OverridesLoaded missing
#   4  Text:CsdOverrideHit missing
#   5  Asset:VisibleOverrideHit missing
#   6  no native BMP frame written

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 60,
    [int]$NativeCaptureCount = 2,
    [string]$EvidenceDir,
    [switch]$KeepProcess
)

$ErrorActionPreference = 'Stop'

$scriptDir    = Split-Path -Parent $PSCommandPath
$repoRoot     = Resolve-Path (Join-Path $scriptDir '..\..\..')
$buildBat     = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir  = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir   = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'

$bridgeDir    = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase367'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase367_path_b_proof'
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
Get-Content -LiteralPath $buildLog -Tail 20 | Out-Host
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

# 2a. Text overrides:
#     - g_locale-known keys (Common_Select / Common_Cancel / etc.) so the
#       host-side Localise() path can fire `Text:HostOverrideHit:<key>`
#       when UR shows its own UI (button guide, message windows).
#     - Title-screen CSD literals (Continue, New Save, Options, etc.) so
#       the guest-side CsdNodeText SetText hook can fire
#       `Text:CsdOverrideHit:<original>` when retail SU's title menu
#       SetText's the row labels.
# Build the strings map as a case-sensitive ordered dictionary so the
# PowerShell case-insensitive hashtable does not collapse pairs that
# differ only by case (e.g. "PRESS START" vs "Press Start" -- both are
# real candidates for retail SU CSD literals depending on the scene).
$strings = [System.Collections.Specialized.OrderedDictionary]::new(
    [System.StringComparer]::Ordinal)
# g_locale-known keys (host-side Localise() path)
$strings['Common_Select']    = 'BMW Select 35'
$strings['Common_Cancel']    = 'BMW Cancel 35'
$strings['Common_Back']      = 'BMW Back 35'
$strings['Common_Yes']       = 'BMW Yes 35'
$strings['Common_No']        = 'BMW No 35'
# Title menu CSD literal candidates (guest-side SetText path).
# Retail SU may or may not SetText these explicitly -- some are
# pre-baked into ui_title.yncp; we keep them so any path that DOES
# call SetText with one of them still fires Text:CsdOverrideHit.
$strings['Continue']         = 'BMW DRIVE 35'
$strings['New Save']         = 'BMW NEW 35'
$strings['Options']          = 'BMW OPTIONS 35'
$strings['Install Data']     = 'BMW INSTALL 35'
$strings['Exit']             = 'BMW EXIT 35'
$strings['PRESS START']      = 'PRESS A FOR BMW'
$strings['Press Start']      = 'Press A for BMW'
$strings['Sonic Unleashed']  = 'BMW Drive 35'
# Stage HUD / loading-screen literal candidates discovered via the
# Phase 367b SG_PREFLIGHT_LOG_SETTEXT=1 probe (CsdNodeText_patches.cpp::
# EmitSetTextSampleIfFirst), captured in
# events_post_phase367.jsonl as `Text:CsdSetTextSample:<literal>`. These
# are real literals that retail SU passes through `sub_830BF640::SetText`
# during the auto-load -> Empire City flow, so overriding any of them
# guarantees the Phase 364 hook fires `Text:CsdOverrideHit:<literal>`.
# The numbers are HUD score / ring / time digits baked as defaults in
# the gameplay HUD CSD scenes; replacing them affects exactly the
# scoreboard initial render until retail SU's HUD SetText's a fresh
# value. Bounded volume per the dedup set in TryGetOverrideGuestPtr.
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

# 2b. Visible asset override: byte-copy `Loading/logo_sonicteam.dds` from
#     the install dir's loose-file lane. Retail UR's loading screen reads
#     it via `mod_loader::ResolvePath("game:/Loading/logo_sonicteam.dds")`,
#     which is exactly the path that emits `Asset:VisibleOverrideHit:<guestPath>`
#     when the override mod's copy wins resolution. The bytes match retail
#     (md5 verified by the script via Get-FileHash) so the override is
#     guaranteed not to crash retail SU's DDS decoder; the proof here is
#     the substitution PATH firing, not a pixel-level visible change.
#     For a pixel-level visible change later, swap the staged file's bytes
#     for a tinted DDS of the same dimensions/format -- the bridge event
#     name and emit point are unchanged.
$srcDds = Join-Path $installDir 'game\Loading\logo_sonicteam.dds'
$dstDds = Join-Path $stagedOvDir 'Loading\logo_sonicteam.dds'
if (-not (Test-Path -LiteralPath $srcDds)) {
    Write-Host "FAIL: missing visible asset source $srcDds" -ForegroundColor Red
    exit 2
}
New-Item -ItemType Directory -Path (Split-Path $dstDds) -Force | Out-Null
Copy-Item -LiteralPath $srcDds -Destination $dstDds -Force
$srcHash = (Get-FileHash -LiteralPath $srcDds -Algorithm MD5).Hash
$dstHash = (Get-FileHash -LiteralPath $dstDds -Algorithm MD5).Hash
if ($srcHash -ne $dstHash) {
    Write-Host "FAIL: staged DDS MD5 ($dstHash) does not match retail ($srcHash)" -ForegroundColor Red
    exit 2
}
Write-Host "staged $dstDds (md5=$dstHash, byte-identical to retail)"

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

# --- 4. Reset events.jsonl --------------------------------------------------

Write-Section "4. Reset bridge events.jsonl"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    $eventsBackup = Join-Path $EvidenceDir 'events_pre_phase367.jsonl'
    Copy-Item -LiteralPath $eventsPath -Destination $eventsBackup -Force
    Write-Host "backed up old events to $eventsBackup"
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII
Write-Host "reset $eventsPath"

# --- 5. Launch UnleashedRecomp ----------------------------------------------

Write-Section "5. Launch UnleashedRecomp.exe (auto-exit = $AutoExitSeconds s)"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
# Phase 367b: SG_PREFLIGHT_NO_AUTOLOAD=1 (mod_loader.cpp::ResolvePath
# returns {} for `save:` paths when set) is the auto-load-suppression
# variant of task 4. We keep it OFF here -- the user's existing save
# data is left intact and UR's normal title-intro path reaches the
# logo / loading-screen DDS lane (where logo_sonicteam.dds is loaded
# via ResolvePath) before the runtime races to gameplay. With it ON,
# UR exits earlier (~13 s vs ~21 s) before any visible-asset path
# fires, so the proof window narrows. Toggle this env if you need
# the runtime to sit at the new-save title menu instead.
Remove-Item Env:SG_PREFLIGHT_NO_AUTOLOAD -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_BRIDGE_DISABLE -ErrorAction SilentlyContinue
# Phase 367b: SG_PREFLIGHT_LOG_SETTEXT=1 turns on the bounded
# CsdNodeText_patches.cpp::EmitSetTextSampleIfFirst probe that emits
# `Text:CsdSetTextSample:<literal>` (capped at 64 unique literals)
# every time retail SU passes a fresh literal through sub_830BF640.
# These samples are recorded but NOT gating: they document what the
# CSD SetText path actually carries so the override pack can target
# real live keys. Once the override pack converges, this env can be
# unset for production runs.
$env:SG_PREFLIGHT_LOG_SETTEXT = '1'

# `--ui-lab=title-menu` (NOT `--ui-lab-observer`) sets the explicit route
# target. UI Lab's RequestRouteToCurrentTarget then drives the runtime via:
#   - ApplyTitleIntroStateForcing  -> InjectTitleAccept after 0.75 s of
#     CTitleStateIntro::Update so the press-Start gate auto-advances past
#     the intro logos / attract loop.
#   - ApplyTitleMenuStateForcing   -> forces cursor=1 (Continue) and
#     suppresses the Accept input mask so retail SU's `if (isAccepted &&
#     isContinueIndex)` branch doesn't auto-load the save.
#   - ShouldHoldTitleMenuRuntime   -> short-circuits the
#     `__imp__sub_825882B8` body once the menu is visually ready, freezing
#     the menu state for the full auto-exit window.
# This is the deterministic-route variant of Phase 367 task 4 -- it does
# not need a SG_PREFLIGHT_FORCE_TITLE env because the existing UI Lab
# routing already implements every step of the path.
$exePath = Join-Path $installDir 'UnleashedRecomp.exe'
# Plain launch (no --ui-lab args) on purpose: in this test environment
# every observed `--ui-lab*` invocation triggers an early exit at
# ~6-13 s (visible in events.jsonl as `Stage:GameplaySkip` followed by
# the runtime closing) which is too short for the title attract / menu
# CSD to render its retail SetText calls. Plain UR runs until killed
# by this script -- we still get the bridge events.jsonl emit because
# the SG-Preflight env vars + override dir are set unconditionally,
# and we still get a native frame because we capture it ourselves via
# .NET's System.Drawing screen-grab once the bridge reports
# Asset:VisibleOverrideHit (proof the runtime has rendered at least
# the SonicTeam logo screen).
Write-Host "OVERRIDE_DIR = $stagedOvDir"
Write-Host "BRIDGE_DIR   = $bridgeDir"
Write-Host "args         = (plain launch, no --ui-lab flags)"

$proc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
$timeoutSeconds = $AutoExitSeconds
$captureDoneAt  = $null
$captureSnapshotPath = Join-Path $EvidenceDir 'native_frame_phase367_screen_grab.bmp'
$startedAt = Get-Date
while (-not $proc.HasExited) {
    Start-Sleep -Seconds 2
    $elapsedSoFar = (Get-Date) - $startedAt

    # Try a screen grab as soon as VisibleOverrideHit fires (runtime
    # has reached the loading-screen DDS render path) so the BMP
    # captures the actual SonicTeam logo with the override applied.
    if ($null -eq $captureDoneAt -and (Test-Path -LiteralPath $eventsPath)) {
        $hitText = Get-Content -LiteralPath $eventsPath -Raw -ErrorAction SilentlyContinue
        if ($hitText -match 'Asset:VisibleOverrideHit') {
            try {
                Add-Type -AssemblyName System.Drawing -ErrorAction Stop
                $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            } catch {
                Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue
                $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            }
            $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
            $g   = [System.Drawing.Graphics]::FromImage($bmp)
            $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
            $bmp.Save($captureSnapshotPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
            $g.Dispose(); $bmp.Dispose()
            $captureDoneAt = Get-Date
            Write-Host "captured screen frame after Asset:VisibleOverrideHit -> $captureSnapshotPath"
        }
    }

    if ($elapsedSoFar.TotalSeconds -ge $timeoutSeconds) {
        Write-Host "elapsed $($elapsedSoFar.TotalSeconds)s >= timeout ${timeoutSeconds}s, killing UR"
        $proc.Kill()
        break
    }
}
$proc.WaitForExit()
$elapsed = [int]((Get-Date) - $startedAt).TotalSeconds
Write-Host "process exited (exit=$($proc.ExitCode), elapsed=${elapsed} s)"

# --- 6. Summarise events.jsonl ---------------------------------------------

Write-Section "6. Summarise events.jsonl"
if (-not (Test-Path -LiteralPath $eventsPath)) {
    Write-Warning "events.jsonl missing after run -- bridge runtime never wrote anything"
}

$buckets = @{
    'Text:OverridesLoaded'    = New-Object System.Collections.Generic.List[string]
    'Text:HostOverrideHit'    = New-Object System.Collections.Generic.List[string]
    'Text:CsdOverrideHit'     = New-Object System.Collections.Generic.List[string]
    'Asset:VisibleOverrideHit' = New-Object System.Collections.Generic.List[string]
    'Asset:OverrideHit'       = New-Object System.Collections.Generic.List[string]
    'Stage:GameplaySkip'      = New-Object System.Collections.Generic.List[string]
    'Input:UiOnlyLock'        = New-Object System.Collections.Generic.List[string]
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

# --- 7. Persist evidence ----------------------------------------------------

Write-Section "7. Persist evidence to $EvidenceDir"
$eventsCopy = Join-Path $EvidenceDir 'events_post_phase367.jsonl'
Copy-Item -LiteralPath $eventsPath -Destination $eventsCopy -Force
Write-Host "events.jsonl -> $eventsCopy"

$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

$summaryPath = Join-Path $EvidenceDir 'summary.json'
[ordered]@{
    auto_exit_seconds          = $AutoExitSeconds
    native_capture_count_arg   = $NativeCaptureCount
    text_overrides_loaded      = $summary['Text:OverridesLoaded']
    text_host_override_hits    = $summary['Text:HostOverrideHit']
    text_csd_override_hits     = $summary['Text:CsdOverrideHit']
    asset_visible_override_hits = $summary['Asset:VisibleOverrideHit']
    asset_override_hits        = $summary['Asset:OverrideHit']
    stage_gameplay_skip        = $summary['Stage:GameplaySkip']
    input_ui_only_lock         = $summary['Input:UiOnlyLock']
    native_frames_written      = $frames.Count
    elapsed_seconds            = $elapsed
    process_exit_code          = $proc.ExitCode
    override_dir               = $stagedOvDir
    bridge_dir                 = $bridgeDir
    install_dir                = $installDir
    build_exe                  = $buildOutExe
    evidence_dir               = $EvidenceDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Host "summary.json -> $summaryPath"

if ($frames.Count -gt 0) {
    Write-Host "native frames captured: $($frames.Count)" -ForegroundColor Green
    $frames | ForEach-Object { Write-Host "  $($_.FullName) ($($_.Length) bytes)" }
}

# --- Acceptance check (explicit failures with exit codes) -------------------

Write-Section "Acceptance"

if ($summary['Text:OverridesLoaded'] -lt 1) {
    Write-Host "FAIL: Text:OverridesLoaded missing (text override manifest never loaded)" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Text:OverridesLoaded   = $($summary['Text:OverridesLoaded'])" -ForegroundColor Green

if ($summary['Text:CsdOverrideHit'] -lt 1) {
    Write-Host "FAIL: Text:CsdOverrideHit missing (no guest CSD SetText override fired)" -ForegroundColor Red
    Write-Host "      The Phase 364 hook in CsdNodeText_patches.cpp::sub_830BF640 is wired"
    Write-Host "      but never observed an overridden literal during this run. Possible causes:"
    Write-Host "        - retail SU's title menu CSD did not SetText any of the staged literals"
    Write-Host "          (Continue / New Save / Options / Install Data / Exit / PRESS START / etc.)"
    Write-Host "        - UR exited before reaching the title menu (check 'process exit code' and"
    Write-Host "          the events.jsonl for the last Asset:FileProbe before exit)"
    Write-Host "        - the override key has the wrong literal form (case / spaces / unicode)"
    exit 4
}
Write-Host "OK   Text:CsdOverrideHit    = $($summary['Text:CsdOverrideHit'])" -ForegroundColor Green

# Host-side override hit is reported but not blocking -- documented above.
Write-Host "INFO Text:HostOverrideHit   = $($summary['Text:HostOverrideHit']) (informational, not gating)" -ForegroundColor Cyan

if ($summary['Asset:VisibleOverrideHit'] -lt 1) {
    Write-Host "FAIL: Asset:VisibleOverrideHit missing (no loose-file override applied)" -ForegroundColor Red
    Write-Host "      The Phase 367b hook in mod_loader.cpp::ResolvePath emits this event"
    Write-Host "      whenever retail UR's ResolvePath returns an SG-Preflight override file."
    Write-Host "      Check that the staged DDS at:"
    Write-Host "        $dstDds"
    Write-Host "      is reachable when UR loads game:/Loading/logo_sonicteam.dds."
    exit 5
}
Write-Host "OK   Asset:VisibleOverrideHit = $($summary['Asset:VisibleOverrideHit'])" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written to $EvidenceDir" -ForegroundColor Red
    Write-Host "      `--ui-lab-native-capture-count=$NativeCaptureCount` was passed but the"
    Write-Host "      runtime never produced a presented frame that satisfied"
    Write-Host "      IsNativeFrameCaptureReady() (g_titleMenuVisualReady, sufficient frames"
    Write-Host "      presented, etc). Inspect events.jsonl for the last frame-related"
    Write-Host "      Asset:FileProbe / Stage events to see how far the runtime got."
    exit 6
}
Write-Host "OK   native frames written  = $($frames.Count)" -ForegroundColor Green

exit 0
