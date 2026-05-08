# Phase 372 -- BMW Pack Authoring from sg-preflight project data.
#
# Proves the end-to-end department workflow:
#   sg-preflight/config/bmw_qa_<id>.json
#       --[ sgfx_author_from_preflight.ps1 ]-->  sgfx pack on disk
#       --[ sgfx_shell_launch.ps1 ]-->          UR running with the
#                                                pack live
#       --[ in-game ]-->                          QAPanel:Active
#                                                emits with the
#                                                project's ticket+route
#       --[ edit BMW QA file mid-run ]-->        Text:ScopedRulesReloaded
#                                                with the new count
#
# Acceptance gates (distinct exit codes):
#   2  build / deploy failed
#   3  authoring tool failed to produce expected pack files
#   4  pack_meta.json fields diverged from the BMW QA project file
#      (ticket / project / route / phase round-trip broken)
#   5  Pack:Meta:<t>:<p>:372 not seen within boot window
#   6  Pack:Route:title not seen within boot window
#   7  Initial Text:ScopedRulesLoaded:4 mismatch (rules from project
#      file did not reach UR in the count the author tool wrote)
#   8  QAPanel:Active:<ticket>:title not seen
#   9  Mid-run re-author -> Text:ScopedRulesReloaded:5 not seen
#  10  Real save SHA mismatched pre/post run
#  11  Negative test: a BMW QA project file with an unscoped rule
#      was NOT rejected by the author tool (anti-regression for the
#      Phase 367b 'BMW999 everywhere' issue)
#  12  No native frame captured (UR never rendered)

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

$author      = Join-Path $scriptDir 'sgfx_author_from_preflight.ps1'

$bridgeDir   = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$packDir     = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase372'
$resDir      = Join-Path $repoRoot 'res'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase372_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

# Synthesise a BMW QA project file under EvidenceDir using absolute
# paths to repo res/ assets. The runtime sample at
# sg-preflight\config\bmw_qa_g65.json uses relative paths from its
# own dir; copying it elsewhere breaks those traversals, so the
# proof builds a self-contained file instead. The author tool
# accepts both relative-from-projectDir and absolute paths -- the
# sample exercises one branch; this proof exercises the other.
$bmwQaWork = Join-Path $EvidenceDir 'bmw_qa_phase372.json'
$iconAbs = (Join-Path $resDir 'sgfx_icon.png') -replace '\\','/'
$logoAbs = (Join-Path $resDir 'logo_sgfx.png') -replace '\\','/'
$ddsAbs  = (Join-Path $resDir 'logo_sgfx.dds') -replace '\\','/'
$initialBmwQa = @"
{
  "schema": "sgfx_bmw_qa_pack",
  "version": 1,
  "profile_id": "g65",
  "ticket":  "IDCEVODEV-960073",
  "project": "BMW G65 IDCevo SGFX QA Shell",
  "route":   "title",
  "branding": {
    "window_title_template": "`${project} (`${ticket})",
    "build_label_template":  "SGFX 0.6 (Phase 372 / `${profile_id})",
    "icon_relative":         "$iconAbs",
    "logo_relative":         "$logoAbs"
  },
  "scoped_text_rules": [
    { "literal": "99",     "csd_project_substring": "status", "replacement": "G65",        "rationale": "HUD digit -> car model" },
    { "literal": "999999", "csd_project_substring": "status", "replacement": "G65-IDCevo", "rationale": "HUD score -> profile tag" },
    { "literal": "16",     "csd_project_substring": "status", "replacement": "G65-16",     "rationale": "Cover second status digit" },
    { "literal": "30",     "csd_project_substring": "status", "replacement": "G65-30",     "rationale": "Cover third status digit"  }
  ],
  "picture_overrides": [
    { "guest_picture": "logo_sonicteam", "source_relative": "$ddsAbs", "rationale": "Replace SonicTeam logo with SGFX QA branding" }
  ]
}
"@
$initialBmwQa | Set-Content -LiteralPath $bmwQaWork -Encoding UTF8

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

# --- 2. Author pack from BMW QA project file --------------------------

Write-Section "2. Run sgfx_author_from_preflight.ps1"
if (Test-Path -LiteralPath $packDir) {
    Remove-Item -LiteralPath $packDir -Recurse -Force
}
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $author `
    -ProjectFile $bmwQaWork `
    -OutputDir   $packDir `
    -Force | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: author exited $LASTEXITCODE" -ForegroundColor Red
    exit 3
}

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
        Write-Host "FAIL: author missing $rel" -ForegroundColor Red
        exit 3
    }
}

# Verify pack_meta round-trip from project file.
$bmwDoc  = Get-Content -LiteralPath $bmwQaWork -Raw | ConvertFrom-Json
$packMeta = Get-Content -LiteralPath (Join-Path $packDir 'pack_meta.json') -Raw | ConvertFrom-Json
$expectedTicket  = [string]$bmwDoc.ticket
$expectedProject = [string]$bmwDoc.project
$expectedRoute   = [string]$bmwDoc.route
if ($packMeta.ticket  -ne $expectedTicket  -or
    $packMeta.project -ne $expectedProject -or
    $packMeta.route   -ne $expectedRoute   -or
    $packMeta.phase   -ne '372') {
    Write-Host "FAIL: pack_meta.json round-trip mismatch" -ForegroundColor Red
    Write-Host "  expected: $expectedTicket / $expectedProject / $expectedRoute / 372"
    Write-Host "  got:      $($packMeta.ticket) / $($packMeta.project) / $($packMeta.route) / $($packMeta.phase)"
    exit 4
}

# Verify the rationale appendix landed in pack_export.log.
$logText = Get-Content -LiteralPath (Join-Path $packDir 'pack_export.log') -Raw
if ($logText -notmatch 'author rationale \(Phase 372\)') {
    Write-Host "FAIL: pack_export.log missing rationale appendix" -ForegroundColor Red
    exit 3
}

# --- 3. Reset events + back up save -----------------------------------

Write-Section "3. Reset events + back up save"
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

# --- 4. Launch UR with the authored pack ------------------------------

Write-Section "4. Launch UR (authored pack at $packDir)"
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

# --- 5. Boot-time event gates ----------------------------------------

Write-Section "5. Boot-time event gates"
$packMetaPattern = "Pack:Meta:$([regex]::Escape($expectedTicket)):$([regex]::Escape($expectedProject)):372"
$qaActivePattern = "QAPanel:Active:$([regex]::Escape($expectedTicket)):title"

$packMetaSeen   = Wait-ForBridgeEvent -Pattern $packMetaPattern  -EventsPath $eventsPath -TimeoutSeconds 25
$packRouteSeen  = Wait-ForBridgeEvent -Pattern 'Pack:Route:title' -EventsPath $eventsPath -TimeoutSeconds 15
$initialRules   = Wait-ForBridgeEvent -Pattern 'Text:ScopedRulesLoaded:4' -EventsPath $eventsPath -TimeoutSeconds 15
$qaActiveSeen   = Wait-ForBridgeEvent -Pattern $qaActivePattern   -EventsPath $eventsPath -TimeoutSeconds 25

Write-Host "pack-meta:  $packMetaSeen"
Write-Host "pack-route: $packRouteSeen"
Write-Host "initial 4:  $initialRules"
Write-Host "qa-active:  $qaActiveSeen"

# --- 6. Mid-run: edit BMW QA file, re-author, expect reload to 5 ------

Write-Section "6. Mid-run: edit BMW QA file (4 -> 5 rules)"
$mutated = $bmwDoc | ConvertTo-Json -Depth 8 | ConvertFrom-Json
$newRule = [pscustomobject]@{
    literal               = '0'
    csd_project_substring = 'status'
    replacement           = 'G65-0'
    rationale             = 'Cover the zero-state status digit (added mid-run for Phase 372)'
}
$mutated.scoped_text_rules = @($mutated.scoped_text_rules) + @($newRule)
$mutated | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $bmwQaWork -Encoding UTF8

$reloadStartedAt = Get-Date
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $author `
    -ProjectFile $bmwQaWork -OutputDir $packDir | Out-Host

$reloadSeen = Wait-ForBridgeEvent `
    -Pattern 'Text:ScopedRulesReloaded:5' `
    -EventsPath $eventsPath `
    -TimeoutSeconds $ReloadTimeoutSeconds
$reloadElapsed = ((Get-Date) - $reloadStartedAt).TotalSeconds
Write-Host ("text-reload: {0} (elapsed {1:N2}s)" -f $reloadSeen, $reloadElapsed)

# --- 7. Capture frame + auto-kill ------------------------------------

Write-Section "7. Capture frame + auto-kill"
$captureBmp = Join-Path $EvidenceDir 'phase372_screen_grab.bmp'
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
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase372.jsonl') -Force

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

# --- 8. Save SHA verify -----------------------------------------------

Write-Section "8. Save SHA verify"
$savePostSha = Get-SaveSha -Dir $saveDir
$saveOk = $true
foreach ($n in @($savePreSha.Keys)) {
    if (-not $savePostSha.Contains($n) -or $savePostSha[$n] -ne $savePreSha[$n]) {
        $saveOk = $false; break
    }
}
Write-Host "save-shas-equal: $saveOk"

# --- 9. Negative test: unscoped rule must be rejected -----------------

Write-Section "9. Negative test: unscoped rule rejection"
$negativeFile = Join-Path $EvidenceDir 'bmw_qa_negative_unscoped.json'
@'
{
  "schema": "sgfx_bmw_qa_pack",
  "version": 1,
  "ticket":  "NEG-001",
  "project": "Negative test (unscoped rule)",
  "route":   "title",
  "scoped_text_rules": [
    { "literal": "BMW999", "replacement": "EVERYWHERE" }
  ]
}
'@ | Set-Content -LiteralPath $negativeFile -Encoding UTF8

$negativeOut = Join-Path $EvidenceDir 'negative_pack'
$negativeOutLog = Join-Path $EvidenceDir 'negative_test_output.log'
$negativeOutErr = Join-Path $EvidenceDir 'negative_test_output.err.log'
$negativeRejected = $false

# File-based capture is more reliable than `2>&1 | Out-String` for
# the child powershell.exe's stderr stream: Start-Process keeps the
# streams cleanly separated and the error text lands verbatim in
# the .err file regardless of console-formatter wrapping.
#
# Start-Process -ArgumentList does NOT shell-escape elements with
# spaces, so paths under "UI-UX Sonic World Adventure..." get
# truncated at the first space. Wrap path-bearing args in
# double-quotes so cmd.exe / powershell.exe see them as one token.
$negProc = Start-Process -FilePath 'powershell.exe' `
    -ArgumentList @(
        '-NoProfile',
        '-ExecutionPolicy','Bypass',
        '-File',          ('"' + $author + '"'),
        '-ProjectFile',   ('"' + $negativeFile + '"'),
        '-OutputDir',     ('"' + $negativeOut + '"'),
        '-Force'
    ) `
    -NoNewWindow -Wait -PassThru `
    -RedirectStandardOutput $negativeOutLog `
    -RedirectStandardError  $negativeOutErr
if ($negProc.ExitCode -ne 0) { $negativeRejected = $true }

$negCombined = ''
foreach ($p in $negativeOutLog, $negativeOutErr) {
    if (Test-Path -LiteralPath $p) {
        $negCombined += (Get-Content -LiteralPath $p -Raw -ErrorAction SilentlyContinue)
    }
}
$negativeMessageOk = ($negCombined -match 'csd_project_substring')
Write-Host "negative-rejected: $negativeRejected"
Write-Host "error-mentions-scope-field: $negativeMessageOk"
if (-not $negativeMessageOk) {
    Write-Host "--- captured negative-test output ---"
    Write-Host $negCombined
    Write-Host "--- end ---"
}

# --- 10. Native frame check ------------------------------------------

$nativeCount = (Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue | Measure-Object).Count

# --- 11. Verdict ------------------------------------------------------

Write-Section "VERDICT"
$rows = @(
    [pscustomobject]@{ Gate = 'Pack files produced';            Pass = $true },
    [pscustomobject]@{ Gate = 'pack_meta round-trip';           Pass = $true },
    [pscustomobject]@{ Gate = 'Pack:Meta';                      Pass = $packMetaSeen },
    [pscustomobject]@{ Gate = 'Pack:Route:title';                Pass = $packRouteSeen },
    [pscustomobject]@{ Gate = 'Text:ScopedRulesLoaded:4';        Pass = $initialRules },
    [pscustomobject]@{ Gate = 'QAPanel:Active';                  Pass = $qaActiveSeen },
    [pscustomobject]@{ Gate = 'Text:ScopedRulesReloaded:5';      Pass = $reloadSeen },
    [pscustomobject]@{ Gate = 'Save SHA unchanged';              Pass = $saveOk },
    [pscustomobject]@{ Gate = 'Unscoped rule rejected';          Pass = ($negativeRejected -and $negativeMessageOk) },
    [pscustomobject]@{ Gate = 'Native frame written';            Pass = ($nativeCount -ge 1) }
)
$rows | Format-Table -AutoSize | Out-Host

if (-not $packMetaSeen)   { exit 5 }
if (-not $packRouteSeen)  { exit 6 }
if (-not $initialRules)   { exit 7 }
if (-not $qaActiveSeen)   { exit 8 }
if (-not $reloadSeen)     { exit 9 }
if (-not $saveOk)         { exit 10 }
if (-not $negativeRejected -or -not $negativeMessageOk) { exit 11 }
if ($nativeCount -lt 1)   { exit 12 }

Write-Host ""
Write-Host "Phase 372: PASS" -ForegroundColor Green
exit 0
