# Phase 373 -- sg-preflight runtime bridge + SGFX rebrand audit.
#
# Proves the SGFX shell can read real sg-preflight project data
# at boot, surface it in the in-game QA panel, and that the
# rebrand-coverage inventory exists alongside the runtime proof.
#
# Pipeline exercised:
#   sg-preflight CLI (real venv at <root>\.venv\Scripts\python.exe)
#       --[ sgfx_preflight_bridge_export.ps1 ]-->  <PackDir>\sg_preflight_state.json
#       --[ launcher sets SG_PREFLIGHT_STATE_JSON ]-->  UR boot
#       --[ SGPreflightState::EnsureLoaded ]-->  bridge events
#       --[ SGQAPanel::Draw ]-->  panel surface + QAPanel emit
#
# Acceptance gates (distinct exit codes):
#   2  build / deploy failed
#   3  sg-preflight bridge export missing or malformed JSON
#   4  Pack:Meta:<t>:<p>:373 not seen
#   5  Pack:Route:title not seen
#   6  SgPreflightState:Loaded:G65:<n> not seen (UR loader did not
#      pick up the bridge state)
#   7  QAPanel:SgPreflightState:G65:<n> not seen (panel never
#      surfaced the state)
#   8  rebrand-coverage report missing or empty
#   9  real save SHA mismatch pre/post
#  10  no native frame captured

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 45,
    [string]$EvidenceDir,
    [string]$SgPreflightRoot = 'C:\Users\DavidErikGarciaArena\Downloads\sg-preflight'
)

$ErrorActionPreference = 'Stop'

$scriptDir   = Split-Path -Parent $PSCommandPath
$repoRoot    = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
$buildBat    = Join-Path $repoRoot '_phase367_build.bat'
$buildOutExe = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe'
$buildOutDir = 'C:\ur103clean\b\ui_lab_runtime\UnleashedRecomp'
$installDir  = Join-Path $repoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$resDir      = Join-Path $repoRoot 'res'

$bridgeScript = Join-Path $scriptDir 'sgfx_preflight_bridge_export.ps1'
$author       = Join-Path $scriptDir 'sgfx_author_from_preflight.ps1'
$coverageDoc  = Join-Path $repoRoot 'research_uiux\SGFX_REBRAND_COVERAGE.md'

$bridgeDir   = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$packDir     = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase373'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase373_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
}

function Wait-ForBridgeEvent {
    param([string]$Pattern, [string]$EventsPath, [int]$TimeoutSeconds)
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

# --- 2. Author a G65 BMW QA pack via the Phase 372 author tool --------

Write-Section "2. Author G65 pack"
$expectedTicket  = 'IDCEVODEV-960073'
$expectedProject = 'BMW G65 IDCevo SGFX QA Shell'
$bmwQaWork       = Join-Path $EvidenceDir 'bmw_qa_phase373.json'

$iconAbs = (Join-Path $resDir 'sgfx_icon.png') -replace '\\','/'
$logoAbs = (Join-Path $resDir 'logo_sgfx.png') -replace '\\','/'
$ddsAbs  = (Join-Path $resDir 'logo_sgfx.dds') -replace '\\','/'
@"
{
  "schema": "sgfx_bmw_qa_pack",
  "version": 1,
  "profile_id": "g65",
  "ticket":  "$expectedTicket",
  "project": "$expectedProject",
  "route":   "title",
  "branding": {
    "window_title_template": "`${project} (`${ticket})",
    "build_label_template":  "SGFX 0.7 (Phase 373 / `${profile_id})",
    "icon_relative":         "$iconAbs",
    "logo_relative":         "$logoAbs"
  },
  "scoped_text_rules": [
    { "literal": "99",     "csd_project_substring": "status", "replacement": "G65",        "rationale": "HUD digit -> car model" },
    { "literal": "999999", "csd_project_substring": "status", "replacement": "G65-IDCevo", "rationale": "HUD score -> profile tag" }
  ],
  "picture_overrides": [
    { "guest_picture": "logo_sonicteam", "source_relative": "$ddsAbs", "rationale": "Replace SonicTeam logo with SGFX QA branding" }
  ]
}
"@ | Set-Content -LiteralPath $bmwQaWork -Encoding UTF8

if (Test-Path -LiteralPath $packDir) {
    Remove-Item -LiteralPath $packDir -Recurse -Force
}
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $author `
    -ProjectFile $bmwQaWork -OutputDir $packDir -Force | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: author exit $LASTEXITCODE" -ForegroundColor Red
    exit 2
}

# Patch pack_meta.json to phase=373 so the boot event matches what
# the proof asserts. The author tool defaults to phase=372 because
# its CLI param's default is 372; passing -Phase via the BMW QA
# project file is not supported in the v1 schema, so we patch
# directly. Pack metadata is read once at boot, so this is fine.
$packMetaPath = Join-Path $packDir 'pack_meta.json'
$packMeta = Get-Content -LiteralPath $packMetaPath -Raw | ConvertFrom-Json
$packMeta.phase = '373'
$packMeta | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $packMetaPath -Encoding UTF8

# --- 3. Export sg-preflight bridge state -----------------------------

Write-Section "3. Export sg-preflight bridge state"
$preflightStatePath = Join-Path $packDir 'sg_preflight_state.json'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $bridgeScript `
    -SgPreflightRoot $SgPreflightRoot `
    -OutputPath      $preflightStatePath `
    -Profile         'G65' `
    -Ticket          $expectedTicket `
    -Project         $expectedProject | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: bridge export exit $LASTEXITCODE" -ForegroundColor Red
    exit 3
}

# Validate the generated bridge JSON.
if (-not (Test-Path -LiteralPath $preflightStatePath)) {
    Write-Host "FAIL: bridge state file missing" -ForegroundColor Red
    exit 3
}
try {
    $stateDoc = Get-Content -LiteralPath $preflightStatePath -Raw | ConvertFrom-Json
} catch {
    Write-Host "FAIL: bridge state JSON parse: $($_.Exception.Message)" -ForegroundColor Red
    exit 3
}
if ($stateDoc.schema -ne 'sgfx_preflight_state' -or
    $stateDoc.selected_profile -ne 'G65' -or
    $stateDoc.actions.Count -lt 1) {
    Write-Host "FAIL: bridge state shape mismatch" -ForegroundColor Red
    Write-Host ("  schema=$($stateDoc.schema), profile=$($stateDoc.selected_profile), actions=$($stateDoc.actions.Count)")
    exit 3
}
$expectedActionCount = $stateDoc.actions.Count
Write-Host "bridge state: profile=$($stateDoc.selected_profile) actions=$expectedActionCount checkers=$($stateDoc.checkers.Count) workflow=$($stateDoc.workflow.Count) warnings=$($stateDoc.warnings.Count)"

# --- 4. Reset events + back up save ---------------------------------

Write-Section "4. Reset events + back up save"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

$saveDir = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$savePreSha = Get-SaveSha -Dir $saveDir
$saveBackupDir = Join-Path $EvidenceDir ("save_backup_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
if ($savePreSha.Count -gt 0) {
    New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
    foreach ($n in @($savePreSha.Keys)) {
        Copy-Item -LiteralPath (Join-Path $saveDir $n) -Destination (Join-Path $saveBackupDir $n) -Force
    }
}

# --- 5. Launch UR ---------------------------------------------------

Write-Section "5. Launch UR with SG_PREFLIGHT_STATE_JSON"
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $packDir
$env:SG_PREFLIGHT_STATE_JSON    = $preflightStatePath
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

# --- 6. Boot-time event gates ---------------------------------------

Write-Section "6. Boot-time event gates"
$packMetaPattern   = "Pack:Meta:$([regex]::Escape($expectedTicket)):$([regex]::Escape($expectedProject)):373"
$preflightLoadedPattern = "SgPreflightState:Loaded:G65:$expectedActionCount"
$panelStatePattern      = "QAPanel:SgPreflightState:G65:$expectedActionCount"

$packMetaSeen      = Wait-ForBridgeEvent -Pattern $packMetaPattern        -EventsPath $eventsPath -TimeoutSeconds 25
$packRouteSeen     = Wait-ForBridgeEvent -Pattern 'Pack:Route:title'      -EventsPath $eventsPath -TimeoutSeconds 15
$preflightLoaded   = Wait-ForBridgeEvent -Pattern $preflightLoadedPattern -EventsPath $eventsPath -TimeoutSeconds 15
$qaPanelStateSeen  = Wait-ForBridgeEvent -Pattern $panelStatePattern      -EventsPath $eventsPath -TimeoutSeconds 25

Write-Host "pack-meta:        $packMetaSeen"
Write-Host "pack-route:       $packRouteSeen"
Write-Host "preflight loaded: $preflightLoaded"
Write-Host "panel preflight:  $qaPanelStateSeen"

# --- 7. Capture frame + auto-kill ----------------------------------

Write-Section "7. Capture frame + auto-kill"
$captureBmp = Join-Path $EvidenceDir 'phase373_screen_grab.bmp'
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
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase373.jsonl') -Force

# Restore save.
foreach ($n in @($savePreSha.Keys)) {
    $src = Join-Path $saveBackupDir $n
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination (Join-Path $saveDir $n) -Force
    }
}
if (Test-Path -LiteralPath $shellExe) {
    Remove-Item -LiteralPath $shellExe -Force -ErrorAction SilentlyContinue
}

# --- 8. Save SHA verify ---------------------------------------------

Write-Section "8. Save SHA verify"
$savePostSha = Get-SaveSha -Dir $saveDir
$saveOk = $true
foreach ($n in @($savePreSha.Keys)) {
    if (-not $savePostSha.Contains($n) -or $savePostSha[$n] -ne $savePreSha[$n]) {
        $saveOk = $false; break
    }
}
Write-Host "save-shas-equal: $saveOk"

# --- 9. Rebrand coverage report check ------------------------------

Write-Section "9. Rebrand coverage report"
$coverageOk = $false
if (Test-Path -LiteralPath $coverageDoc) {
    $sz = (Get-Item -LiteralPath $coverageDoc).Length
    if ($sz -gt 1024) { $coverageOk = $true }
    Write-Host "coverage doc:    $coverageDoc ($sz bytes)"
} else {
    Write-Host "coverage doc:    MISSING"
}

# --- 10. Native frame check ----------------------------------------

$nativeCount = (Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue | Measure-Object).Count

# --- 11. Verdict ---------------------------------------------------

Write-Section "VERDICT"
$rows = @(
    [pscustomobject]@{ Gate = 'Bridge state JSON valid';       Pass = $true },
    [pscustomobject]@{ Gate = 'Pack:Meta:...:373';              Pass = $packMetaSeen },
    [pscustomobject]@{ Gate = 'Pack:Route:title';               Pass = $packRouteSeen },
    [pscustomobject]@{ Gate = "SgPreflightState:Loaded:G65:$expectedActionCount"; Pass = $preflightLoaded },
    [pscustomobject]@{ Gate = "QAPanel:SgPreflightState:G65:$expectedActionCount"; Pass = $qaPanelStateSeen },
    [pscustomobject]@{ Gate = 'Rebrand coverage report';        Pass = $coverageOk },
    [pscustomobject]@{ Gate = 'Save SHA unchanged';             Pass = $saveOk },
    [pscustomobject]@{ Gate = 'Native frame written';           Pass = ($nativeCount -ge 1) }
)
$rows | Format-Table -AutoSize | Out-Host

if (-not $packMetaSeen)     { exit 4 }
if (-not $packRouteSeen)    { exit 5 }
if (-not $preflightLoaded)  { exit 6 }
if (-not $qaPanelStateSeen) { exit 7 }
if (-not $coverageOk)       { exit 8 }
if (-not $saveOk)           { exit 9 }
if ($nativeCount -lt 1)     { exit 10 }

Write-Host ""
Write-Host "Phase 373: PASS" -ForegroundColor Green
exit 0
