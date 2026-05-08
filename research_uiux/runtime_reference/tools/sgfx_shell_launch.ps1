# Phase 371A/B -- SGFX Shell launcher (one-click).
#
# Goal: turn UnleashedRecomp into a shell tool the operator can
# invoke without remembering env-var spelling, save backup, exe
# branding, or hot-reload toggles.
#
# Phase 371B additions:
#   - per-ticket save sandbox at <PackDir>/save/. UR's live save
#     dir (%APPDATA%\UnleashedRecomp\save\) is QUARANTINED to
#     %LOCALAPPDATA%\UnleashedRecomp\sgfx_real_save_quarantine\<ts>\
#     before launch. The sandbox is copied OVER the live save
#     dir; UR writes through normally. After exit, the live save
#     dir is captured back into the pack's sandbox, then the real
#     save is restored (SHA-256 verified). Quarantine is only
#     deleted after a verified restore -- if anything fails the
#     operator has the timestamped quarantine folder to recover.
#   - route preset (-Route or pack_meta.route): selects the
#     SG_PREFLIGHT_NO_AUTOLOAD value. `title` -> 1 (boot stays at
#     Title). All others -> 0 (UR resumes from the per-ticket
#     sandbox, which the operator captured by playing into the
#     desired state once).
#
# What it does:
#   1. Calls sgfx_pack_exporter.ps1 to produce a fresh pack at
#      -PackDir (default %LOCALAPPDATA%\UnleashedRecomp\sg_overrides_sgfx_shell).
#   2. Quarantines the real save (SHA-256 baseline) and overlays
#      the per-ticket sandbox (Phase 371B).
#   3. Sets SGFX-shell env vars (SG_PREFLIGHT_OVERRIDE_DIR,
#      _HOT_RELOAD, _NO_AUTOLOAD route-driven, _GAMEPLAY_SKIP,
#      _UI_ONLY_INPUT).
#   4. Copies UnleashedRecomp.exe -> SgfxShell.exe (taskbar/process
#      branding); deletes on exit.
#   5. Resets events.jsonl and launches.
#   6. Tails events.jsonl with a human-readable status line.
#   7. On exit (including Ctrl+C / kill): syncs the live save dir
#      back into the pack sandbox, restores the real save from
#      quarantine, verifies SHA-256.
#
# Usage:
#   .\sgfx_shell_launch.ps1 -SkipExport
#   .\sgfx_shell_launch.ps1 -Ticket "IDCEVODEV-960073" -Project "BMW SGFX"
#   .\sgfx_shell_launch.ps1 -PackDir "D:\packs\bmw_g65" -SkipExport
#   .\sgfx_shell_launch.ps1 -Ticket "IDCEVODEV-960073" -Route worldmap

[CmdletBinding()]
param(
    [string]$PackDir = '',

    # Forwarded to the exporter when -SkipExport is not set.
    [string]$Ticket = '',
    [string]$Project = '',
    [string]$Phase = '371B',
    [string]$ScopedRulesJson = '',
    [string]$LogoSonicteamReplacement = '',
    [string]$WindowTitle = '',
    [string]$BuildLabel = '',
    [string]$IconPath = '',
    [string]$LogoPath = '',

    # Phase 371B: route preset.
    #   title    -> NO_AUTOLOAD=1 (boot stays at Title screen)
    #   auto / worldmap / hud / results -> NO_AUTOLOAD=0
    #     (UR resumes from the per-ticket sandbox; the operator
    #      captures the desired state by playing into it once
    #      with `title` and exiting cleanly).
    # When -Route is left at '' and pack_meta.json declares a route,
    # the pack value is used. CLI -Route always wins when both set.
    [ValidateSet('','title','auto','worldmap','hud','results')]
    [string]$Route = '',

    [switch]$SkipExport,

    # Auto-kill UR after this many seconds. 0 = wait until UR exits
    # naturally (operator closes the window or Ctrl+C the runner).
    [int]$Lifetime = 0,

    # Disable real-save quarantine + restore. Only set this when
    # you have already isolated the save dir externally.
    [switch]$NoSaveBackup,

    # Disable per-ticket sandbox sync (overlay + capture). Useful
    # when you want the launcher to behave like 371A (real save
    # quarantine only, no per-ticket persistence). The sandbox dir
    # under <PackDir>/save/ is left untouched.
    [switch]$NoSandboxSync,

    # Phase 373: sg-preflight runtime bridge. When -SgPreflightRoot
    # points at the operator's sg-preflight checkout, the launcher
    # invokes sgfx_preflight_bridge_export.ps1 BEFORE launch to
    # write <PackDir>/sg_preflight_state.json, and exports
    # SG_PREFLIGHT_STATE_JSON pointing at that file so UR's state
    # loader can surface it in the QA panel. Empty string disables
    # the bridge step (no state file written, env var unset).
    # -PreflightProfile selects which sg-preflight profile the bridge
    # filters actions for; defaulted to "G65" if blank.
    # Named PreflightProfile (not Profile) because $Profile is a
    # PowerShell automatic variable.
    [string]$SgPreflightRoot = '',
    [string]$PreflightProfile = '',

    [string]$EvidenceDir = '',
    [string]$QuarantineRoot = '',
    [string]$RepoRoot = ''
)

$ErrorActionPreference = 'Stop'

# --- 0. Locate paths -----------------------------------------------------

if (-not $RepoRoot -or $RepoRoot -eq '') {
    $scriptDir = Split-Path -Parent $PSCommandPath
    $RepoRoot  = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
}

$installDir   = Join-Path $RepoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$bridgeDir    = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$saveDir      = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$exporter     = Join-Path (Split-Path -Parent $PSCommandPath) 'sgfx_pack_exporter.ps1'
# Phase 371B: stable quarantine root (never under EvidenceDir, which
# the operator may delete between runs). Always under %LOCALAPPDATA%
# so the real save can be recovered manually even after a crash.
if (-not $QuarantineRoot -or $QuarantineRoot -eq '') {
    $quarantineRoot = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sgfx_real_save_quarantine'
} else {
    $quarantineRoot = $QuarantineRoot
}

if (-not $PackDir -or $PackDir -eq '') {
    $PackDir = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_sgfx_shell'
}

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $RepoRoot 'research_uiux\runtime_reference\out\sgfx_shell_launch'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

if (-not (Test-Path -LiteralPath $installDir)) {
    throw "Install dir not found: $installDir"
}
if (-not (Test-Path -LiteralPath (Join-Path $installDir 'UnleashedRecomp.exe'))) {
    throw "UnleashedRecomp.exe not found in $installDir. Run a build first."
}

function Write-Banner([string]$msg) {
    Write-Host ""
    Write-Host "==[ SGFX Shell ]== $msg" -ForegroundColor Cyan
}

function Get-SaveSnapshot {
    param([string]$Dir)
    $snap = [ordered]@{}
    if (-not (Test-Path -LiteralPath $Dir)) { return $snap }
    foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
        $p = Join-Path $Dir $name
        if (Test-Path -LiteralPath $p) {
            $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $p).Hash
            $snap[$name] = @{ Hash = $hash; Bytes = (Get-Item -LiteralPath $p).Length; Path = $p }
        }
    }
    return $snap
}

function Restore-Save {
    param(
        [string]$BackupDir,
        [string]$LiveDir
    )
    if (-not (Test-Path -LiteralPath $BackupDir)) { return }
    foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
        $src = Join-Path $BackupDir $name
        if (Test-Path -LiteralPath $src) {
            $dst = Join-Path $LiveDir $name
            Copy-Item -LiteralPath $src -Destination $dst -Force
        }
    }
}

# Phase 371B helpers ------------------------------------------------------

# Read pack_meta.json and return the route field, or '' if missing.
function Read-PackRoute {
    param([string]$PackDirPath)
    $meta = Join-Path $PackDirPath 'pack_meta.json'
    if (-not (Test-Path -LiteralPath $meta)) { return '' }
    try {
        $doc = Get-Content -LiteralPath $meta -Raw | ConvertFrom-Json
    } catch { return '' }
    if ($null -eq $doc -or -not $doc.route) { return '' }
    return [string]$doc.route
}

# Map route -> NO_AUTOLOAD value. `title` keeps UR at the boot
# screen; everything else lets UR resume whatever state the
# per-ticket sandbox carries.
function Resolve-NoAutoload {
    param([string]$RouteValue)
    if ([string]::IsNullOrWhiteSpace($RouteValue)) { return '1' }
    if ($RouteValue -ieq 'title') { return '1' }
    return '0'
}


# --- 1. Export pack -------------------------------------------------------

if (-not $SkipExport) {
    Write-Banner "Exporting pack to $PackDir"
    $exporterArgs = @(
        '-OutputDir', $PackDir,
        '-Ticket',    $Ticket,
        '-Project',   $Project,
        '-Phase',     $Phase,
        '-Force'
    )
    # Pass -Route only when the operator gave one explicitly. The
    # exporter's default ('title') applies otherwise -- which is
    # what the launcher's NO_AUTOLOAD resolution would pick anyway,
    # so the resulting pack_meta.route stays consistent with the
    # launched env.
    if ($Route)                   { $exporterArgs += @('-Route', $Route) }
    if ($ScopedRulesJson)         { $exporterArgs += @('-ScopedRulesJson', $ScopedRulesJson) }
    if ($LogoSonicteamReplacement){ $exporterArgs += @('-LogoSonicteamReplacement', $LogoSonicteamReplacement) }
    if ($WindowTitle)             { $exporterArgs += @('-WindowTitle', $WindowTitle) }
    if ($BuildLabel)              { $exporterArgs += @('-BuildLabel', $BuildLabel) }
    if ($IconPath)                { $exporterArgs += @('-IconPath', $IconPath) }
    if ($LogoPath)                { $exporterArgs += @('-LogoPath', $LogoPath) }
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $exporter @exporterArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Pack exporter failed (exit $LASTEXITCODE)"
    }
} else {
    Write-Banner "SkipExport: using pack at $PackDir"
}

if (-not (Test-Path -LiteralPath (Join-Path $PackDir 'sgfx_pack.json'))) {
    throw "Pack at $PackDir is missing sgfx_pack.json. Re-run without -SkipExport."
}

# --- 1b. Phase 373: sg-preflight bridge state -------------------------

# When -SgPreflightRoot is set, run the bridge export to dump the
# real sg-preflight CLI output (profiles / actions / checkers /
# workflow) into <PackDir>/sg_preflight_state.json. UR's loader
# reads this via SG_PREFLIGHT_STATE_JSON and the QA panel surfaces
# the selected profile + action count + first warning.
$preflightStatePath = Join-Path $PackDir 'sg_preflight_state.json'
if ($SgPreflightRoot -and (Test-Path -LiteralPath $SgPreflightRoot)) {
    Write-Banner "Exporting sg-preflight bridge state"
    $bridgeScript = Join-Path (Split-Path -Parent $PSCommandPath) 'sgfx_preflight_bridge_export.ps1'
    if (-not (Test-Path -LiteralPath $bridgeScript)) {
        throw "Bridge export script not found: $bridgeScript"
    }
    $effectiveProfile = if ($PreflightProfile) { $PreflightProfile } else { 'G65' }
    & $bridgeScript `
        -SgPreflightRoot $SgPreflightRoot `
        -OutputPath      $preflightStatePath `
        -Profile         $effectiveProfile `
        -Ticket          $Ticket `
        -Project         $Project | Out-Host
    $env:SG_PREFLIGHT_STATE_JSON = $preflightStatePath
    Write-Host "  SG_PREFLIGHT_STATE_JSON = $preflightStatePath"
} else {
    if ($SgPreflightRoot) {
        Write-Host "[warn] -SgPreflightRoot set but path missing: $SgPreflightRoot" -ForegroundColor Yellow
    }
    # Make sure stale env or stale pack-local state from a previous
    # invocation does not leak into a bridge-disabled shell. UR has
    # a pack-relative fallback path, so clearing only the env var is
    # not enough once a pack has previously been launched with
    # -SgPreflightRoot.
    Remove-Item Env:SG_PREFLIGHT_STATE_JSON -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $preflightStatePath -Force -ErrorAction SilentlyContinue
}

# --- 2. Real-save quarantine + per-ticket sandbox overlay ---------------

# Phase 371B isolation flow:
#   real save  --quarantine-->  %LOCALAPPDATA%\..\sgfx_real_save_quarantine\<ts>\
#   <pack>/save/  --overlay-->  %APPDATA%\..\save\
#
# UR runs against the per-ticket overlay. After exit:
#   %APPDATA%\..\save\  --capture-->  <pack>/save/   (sandbox sync)
#   quarantine          --restore-->  %APPDATA%\..\save\
#
# `$saveSnapshotPre` keeps the SHA of the pre-quarantine real save
# so the post-run restore can verify it landed correctly.

$saveBackupDir   = $null   # legacy 371A name; reused for the quarantine path
$saveSnapshotPre = [ordered]@{}
$packSandboxDir  = Join-Path $PackDir 'save'

if (-not $NoSaveBackup) {
    Write-Banner 'Quarantining real save'
    $saveSnapshotPre = Get-SaveSnapshot -Dir $saveDir
    if (-not (Test-Path -LiteralPath $quarantineRoot)) {
        New-Item -ItemType Directory -Path $quarantineRoot -Force | Out-Null
    }
    $saveBackupDir = Join-Path $quarantineRoot ((Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8))
    if ($saveSnapshotPre.Count -gt 0) {
        New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
        foreach ($name in @($saveSnapshotPre.Keys)) {
            Copy-Item -LiteralPath $saveSnapshotPre[$name].Path `
                      -Destination (Join-Path $saveBackupDir $name) -Force
        }
        Write-Host "  quarantined $($saveSnapshotPre.Count) files to $saveBackupDir"
    } else {
        # Still create the dir so the restore step has a stable
        # marker even when the operator had no save data to start.
        New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
        Write-Host "  no real save to quarantine"
    }
} else {
    Write-Banner 'NoSaveBackup: skipping quarantine (operator-managed)'
}

if (-not $NoSandboxSync) {
    Write-Banner 'Overlaying per-ticket sandbox'
    if (-not (Test-Path -LiteralPath $saveDir)) {
        New-Item -ItemType Directory -Path $saveDir -Force | Out-Null
    }
    if (Test-Path -LiteralPath $packSandboxDir) {
        # Only sync the three known SU save artifacts. Anything
        # extra in the sandbox dir (sandbox_meta.json, etc.) stays
        # under <pack>/save/ untouched.
        $copied = 0
        foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
            $src = Join-Path $packSandboxDir $name
            if (Test-Path -LiteralPath $src) {
                Copy-Item -LiteralPath $src -Destination (Join-Path $saveDir $name) -Force
                $copied++
            } else {
                # Sandbox does not declare this file: leave the
                # quarantined real-save artifact NOT present in
                # the live dir. Otherwise an old SYS-DATA from a
                # different ticket would leak into this one.
                $live = Join-Path $saveDir $name
                if (Test-Path -LiteralPath $live) {
                    Remove-Item -LiteralPath $live -Force -ErrorAction SilentlyContinue
                }
            }
        }
        Write-Host "  overlaid $copied artifact(s) from $packSandboxDir"
    } else {
        # First time this ticket runs: clear the live dir so UR
        # boots without leaking the (already quarantined) real
        # save into this ticket's view.
        foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
            $live = Join-Path $saveDir $name
            if (Test-Path -LiteralPath $live) {
                Remove-Item -LiteralPath $live -Force -ErrorAction SilentlyContinue
            }
        }
        Write-Host "  sandbox empty for this ticket; live save cleared (will be captured on exit)"
    }
} else {
    Write-Banner 'NoSandboxSync: leaving live save dir untouched after quarantine'
}

# --- 3. Reset bridge events.jsonl ----------------------------------------

if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_launch.jsonl') -Force
}
Set-Content -LiteralPath $eventsPath -Value '' -Encoding ASCII

# --- 4. Set env vars + branded exe copy ----------------------------------

Write-Banner "Configuring SGFX env"
# Route precedence: explicit -Route wins; otherwise pack_meta.json's
# `route` field; otherwise default to `title` (NO_AUTOLOAD=1).
$effectiveRoute = $Route
if ([string]::IsNullOrWhiteSpace($effectiveRoute)) {
    $effectiveRoute = Read-PackRoute -PackDirPath $PackDir
}
if ([string]::IsNullOrWhiteSpace($effectiveRoute)) {
    $effectiveRoute = 'title'
}
$noAutoloadValue = Resolve-NoAutoload -RouteValue $effectiveRoute

$env:SG_PREFLIGHT_OVERRIDE_DIR  = $PackDir
$env:SG_PREFLIGHT_HOT_RELOAD    = '1'
$env:SG_PREFLIGHT_NO_AUTOLOAD   = $noAutoloadValue
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
# Make sure stale flags from prior interactive runs do not leak.
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE   -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL    -ErrorAction SilentlyContinue

Write-Host "  SG_PREFLIGHT_OVERRIDE_DIR  = $PackDir"
Write-Host "  SG_PREFLIGHT_HOT_RELOAD    = 1"
Write-Host "  SG_PREFLIGHT_NO_AUTOLOAD   = $noAutoloadValue  (route: $effectiveRoute)"
Write-Host "  SG_PREFLIGHT_GAMEPLAY_SKIP = 1"
Write-Host "  SG_PREFLIGHT_UI_ONLY_INPUT = 1"

$origExe   = Join-Path $installDir 'UnleashedRecomp.exe'
$shellExe  = Join-Path $installDir 'SgfxShell.exe'
Copy-Item -LiteralPath $origExe -Destination $shellExe -Force

# --- 5. Launch + tail events --------------------------------------------

Write-Banner "Launching SgfxShell.exe"
$proc = Start-Process -FilePath $shellExe -WorkingDirectory $installDir -PassThru
$startedAt = Get-Date

try {
    [Console]::TreatControlCAsInput = $false
} catch {
    Write-Host "[warn] console Ctrl+C mode unavailable in this host; cleanup still runs through finally"
}

# The tail mode: print human-readable status lines as events arrive.
# We re-read the whole file each tick (it's append-only and small),
# diff against the last seen byte count, and pretty-print recognised
# event types. Anything we do not recognise prints raw.
$lastBytes = 0
$seenSet   = New-Object System.Collections.Generic.HashSet[string]

function Format-Event {
    param([string]$Line)
    if (-not $Line) { return $null }
    try {
        $obj = $Line | ConvertFrom-Json -ErrorAction Stop
    } catch {
        return $null
    }
    $screen = $obj.screen
    if (-not $screen) { return $null }

    if ($screen -like 'Pack:Meta:*') {
        $rest = $screen.Substring('Pack:Meta:'.Length)
        $parts = $rest -split ':', 3
        $ticket  = if ($parts.Count -gt 0) { $parts[0] } else { '_' }
        $project = if ($parts.Count -gt 1) { $parts[1] } else { '_' }
        $phase   = if ($parts.Count -gt 2) { $parts[2] } else { '_' }
        return "[meta] ticket=$ticket project=$project phase=$phase"
    }
    if ($screen -like 'Pack:Route:*') {
        $route = $screen.Substring('Pack:Route:'.Length)
        return "[route] $route"
    }
    if ($screen -like 'Pack:Loaded:*') {
        return "[pack] $screen"
    }
    if ($screen -like 'Pack:Reloaded:*') {
        return "[pack] reloaded -- $screen"
    }
    if ($screen -like 'Pack:Rejected:*') {
        return "[pack] REJECTED -- $screen"
    }
    if ($screen -like 'Branding:Active:*') {
        return "[brand] $screen"
    }
    if ($screen -like 'Text:ScopedRulesLoaded:*') {
        $count = $screen.Split(':')[-1]
        return "[text] $count scoped rules loaded"
    }
    if ($screen -like 'Text:OverridesLoaded:*') {
        $count = $screen.Split(':')[-1]
        return "[text] $count locale overrides loaded"
    }
    if ($screen -like 'Text:ScopedRulesReloaded:*') {
        $count = $screen.Split(':')[-1]
        return "[text] reloaded -- $count scoped rules"
    }
    if ($screen -like 'Text:OverridesReloaded:*') {
        $count = $screen.Split(':')[-1]
        return "[text] reloaded -- $count locale overrides"
    }
    if ($screen -like 'Asset:PixelOverridesLoaded:*') {
        $count = $screen.Split(':')[-1]
        return "[pix]  $count pictures loaded"
    }
    if ($screen -like 'Asset:PixelOverridesReloaded:*') {
        $count = $screen.Split(':')[-1]
        return "[pix]  reloaded -- $count pictures"
    }
    if ($screen -like 'HotReload:WatcherStarted*') {
        return "[hot]  watcher started (mtime+debounce)"
    }
    if ($screen -like 'SgPreflightState:Loaded:*') {
        $rest = $screen.Substring('SgPreflightState:Loaded:'.Length)
        return "[pre]  state loaded -- $rest"
    }
    if ($screen -like 'SgPreflightState:Missing:*') {
        $reason = $screen.Substring('SgPreflightState:Missing:'.Length)
        return "[pre]  state MISSING ($reason)"
    }
    if ($screen -like 'QAPanel:SgPreflightState:*') {
        $rest = $screen.Substring('QAPanel:SgPreflightState:'.Length)
        return "[panel] sg-preflight: $rest"
    }
    return $null
}

function Read-PendingEvents {
    if (-not (Test-Path -LiteralPath $eventsPath)) { return }
    $size = (Get-Item -LiteralPath $eventsPath).Length
    if ($size -le $lastBytes) { return }
    $stream = [System.IO.File]::Open($eventsPath, 'Open', 'Read', 'ReadWrite')
    try {
        $stream.Seek($lastBytes, 'Begin') | Out-Null
        $buf = New-Object byte[] ($size - $lastBytes)
        $stream.Read($buf, 0, $buf.Length) | Out-Null
        $chunk = [System.Text.Encoding]::UTF8.GetString($buf)
        foreach ($line in $chunk -split "`r?`n") {
            if (-not $line) { continue }
            $key = $line
            if ($seenSet.Contains($key)) { continue }
            [void]$seenSet.Add($key)
            $msg = Format-Event -Line $line
            if ($msg) { Write-Host $msg }
        }
        $script:lastBytes = $size
    } finally {
        $stream.Close()
    }
}

Write-Banner "Tailing $eventsPath"
Write-Host "(Ctrl+C to stop; UR will be killed and save will be restored.)"

try {
    while (-not $proc.HasExited) {
        Read-PendingEvents
        if ($Lifetime -gt 0) {
            $elapsed = ((Get-Date) - $startedAt).TotalSeconds
            if ($elapsed -ge $Lifetime) {
                Write-Host ("[life] elapsed {0:N0}s >= {1}s, killing" -f $elapsed, $Lifetime)
                $proc.Kill() | Out-Null
                $proc.WaitForExit(5000) | Out-Null
                break
            }
        }
        Start-Sleep -Milliseconds 500
    }
    Read-PendingEvents
}
finally {
    if ($proc -and -not $proc.HasExited) {
        try {
            $proc.Kill() | Out-Null
            $proc.WaitForExit(5000) | Out-Null
        } catch {}
    } elseif ($proc) {
        try { $proc.WaitForExit(1000) | Out-Null } catch {}
    }
    Read-PendingEvents
    if ($shellExe -and (Test-Path -LiteralPath $shellExe)) {
        Remove-Item -LiteralPath $shellExe -Force -ErrorAction SilentlyContinue
    }

    # --- Phase 371B post-run flow ----------------------------------------
    #
    #   1. Capture the live save dir back into the per-ticket
    #      sandbox at <pack>/save/. (Skipped under -NoSandboxSync.)
    #   2. Restore the real save from quarantine, then verify the
    #      live save dir's SHA matches the pre-quarantine snapshot.
    #   3. Only delete the quarantine after a verified restore.
    #      A failed verify keeps the timestamped quarantine on disk
    #      so the operator can recover manually.

    if (-not $NoSandboxSync) {
        try {
            if (-not (Test-Path -LiteralPath $packSandboxDir)) {
                New-Item -ItemType Directory -Path $packSandboxDir -Force | Out-Null
            }
            $captured = 0
            foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
                $live = Join-Path $saveDir $name
                $dst  = Join-Path $packSandboxDir $name
                if (Test-Path -LiteralPath $live) {
                    Copy-Item -LiteralPath $live -Destination $dst -Force
                    $captured++
                } elseif (Test-Path -LiteralPath $dst) {
                    # Live file vanished during the run while the
                    # sandbox had a copy: drop the stale sandbox
                    # entry so two tickets do not see each other's
                    # ghost files.
                    Remove-Item -LiteralPath $dst -Force -ErrorAction SilentlyContinue
                }
            }
            $sandboxMeta = [ordered]@{
                version          = 1
                ticket           = $Ticket
                project          = $Project
                phase            = $Phase
                route            = $effectiveRoute
                captured_at      = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
                captured_files   = $captured
            }
            $sandboxMetaPath = Join-Path $packSandboxDir 'sandbox_meta.json'
            $sandboxMeta | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $sandboxMetaPath -Encoding UTF8
            Write-Host "[sand] captured $captured artifact(s) to $packSandboxDir"
        } catch {
            Write-Host "[sand] WARN: sandbox capture failed: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }

    if (-not $NoSaveBackup -and $saveBackupDir -and (Test-Path -LiteralPath $saveBackupDir)) {
        # First, clear the live save dir of whatever the run left
        # behind. We will re-stamp it from the quarantine; any
        # artifact the quarantine doesn't contain belongs to the
        # sandbox and must NOT bleed into the real save.
        foreach ($name in 'SYS-DATA','ACH-DATA','EXT-DATA') {
            $live = Join-Path $saveDir $name
            if (Test-Path -LiteralPath $live) {
                Remove-Item -LiteralPath $live -Force -ErrorAction SilentlyContinue
            }
        }
        Restore-Save -BackupDir $saveBackupDir -LiveDir $saveDir

        # Verify SHA-256 against the pre-quarantine snapshot.
        $verified = $true
        $verifyPost = Get-SaveSnapshot -Dir $saveDir
        foreach ($name in @($saveSnapshotPre.Keys)) {
            if (-not $verifyPost.Contains($name) -or
                $verifyPost[$name].Hash -ne $saveSnapshotPre[$name].Hash) {
                $verified = $false
                Write-Host "[save] FAIL verify: $name SHA mismatch after restore" -ForegroundColor Red
                break
            }
        }
        # Also catch the case where the restore left an extra file
        # behind that the quarantine did not have (e.g. sandbox
        # leaked through). Keys-equal check covers both directions.
        foreach ($name in @($verifyPost.Keys)) {
            if (-not $saveSnapshotPre.Contains($name)) {
                $verified = $false
                Write-Host "[save] FAIL verify: extra file $name present after restore" -ForegroundColor Red
                break
            }
        }

        if ($verified) {
            Write-Host "[save] real save restored + verified; clearing quarantine"
            Remove-Item -LiteralPath $saveBackupDir -Recurse -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "[save] verification failed; quarantine kept at $saveBackupDir" -ForegroundColor Yellow
        }
    }
}

Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_launch.jsonl') -Force
Write-Banner "Done. Evidence at $EvidenceDir"
