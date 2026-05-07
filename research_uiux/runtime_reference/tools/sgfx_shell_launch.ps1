# Phase 371A -- SGFX Shell launcher (one-click).
#
# Goal: turn UnleashedRecomp into a shell tool the operator can
# invoke without remembering env-var spelling, save backup, exe
# branding, or hot-reload toggles.
#
# What it does:
#   1. Calls sgfx_pack_exporter.ps1 to produce a fresh pack at
#      -PackDir (default %LOCALAPPDATA%\UnleashedRecomp\sg_overrides_sgfx_shell).
#   2. Backs up the SU save dir under the evidence dir (SHA-256
#      verified). Restores on exit, including Ctrl+C / kill.
#   3. Sets the SGFX-shell env vars (SG_PREFLIGHT_OVERRIDE_DIR,
#      _HOT_RELOAD, _NO_AUTOLOAD, _GAMEPLAY_SKIP, _UI_ONLY_INPUT).
#   4. Copies UnleashedRecomp.exe -> SgfxShell.exe so the taskbar
#      and process list show the SGFX brand.
#   5. Resets events.jsonl and launches.
#   6. Tails events.jsonl with a human-readable status line:
#        [boot] Pack:Meta:<ticket>:<project>
#        [boot] Branding:Active:<title>|<icon>|<label>
#        [pack] loaded -- text=sgfx_text.json asset=sgfx_pictures.json loose=0
#        [text] 5 scoped rules
#        [pix]  1 picture
#      and re-prints relevant Pack:Reloaded / Text:ScopedRulesReloaded
#      events as the operator iterates.
#
# Usage:
#   # Use whatever pack is already at PackDir (no re-export).
#   .\sgfx_shell_launch.ps1 -SkipExport
#
#   # Fresh pack from CLI args, then launch.
#   .\sgfx_shell_launch.ps1 -Ticket "IDCEVODEV-960073" -Project "BMW SGFX"
#
#   # Use a pre-built pack at a custom location.
#   .\sgfx_shell_launch.ps1 -PackDir "D:\packs\bmw_g65" -SkipExport

[CmdletBinding()]
param(
    [string]$PackDir = '',

    # Forwarded to the exporter when -SkipExport is not set.
    [string]$Ticket = '',
    [string]$Project = '',
    [string]$Phase = '371A',
    [string]$ScopedRulesJson = '',
    [string]$LogoSonicteamReplacement = '',
    [string]$WindowTitle = '',
    [string]$BuildLabel = '',
    [string]$IconPath = '',
    [string]$LogoPath = '',

    [switch]$SkipExport,

    # Auto-kill UR after this many seconds. 0 = wait until UR exits
    # naturally (operator closes the window or Ctrl+C the runner).
    [int]$Lifetime = 0,

    # Disable save backup/restore. Only set this when you have already
    # isolated the save dir externally; otherwise live runs that
    # advance the save will overwrite real progress.
    [switch]$NoSaveBackup,

    [string]$EvidenceDir = '',
    [string]$RepoRoot = ''
)

$ErrorActionPreference = 'Stop'

# --- 0. Locate paths -----------------------------------------------------

if (-not $RepoRoot -or $RepoRoot -eq '') {
    $scriptDir = Split-Path -Parent $PSCommandPath
    $RepoRoot  = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
}

$installDir = Join-Path $RepoRoot 'Unleashed Recomp - Windows (Complete Installation) 1.0.3'
$bridgeDir  = Join-Path $env:APPDATA 'UnleashedRecomp\sgfx_bridge'
$saveDir    = Join-Path $env:APPDATA 'UnleashedRecomp\save'
$exporter   = Join-Path (Split-Path -Parent $PSCommandPath) 'sgfx_pack_exporter.ps1'

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

# --- 2. Save backup (SHA-256 verified) -----------------------------------

$saveBackupDir = $null
$saveSnapshotPre = @{}
if (-not $NoSaveBackup) {
    Write-Banner 'Backing up save'
    $saveSnapshotPre = Get-SaveSnapshot -Dir $saveDir
    $saveBackupDir = Join-Path $EvidenceDir ("save_backup_" + (Get-Date -Format 'yyyyMMdd_HHmmss'))
    if ($saveSnapshotPre.Count -gt 0) {
        New-Item -ItemType Directory -Path $saveBackupDir -Force | Out-Null
        foreach ($name in $saveSnapshotPre.Keys) {
            Copy-Item -LiteralPath $saveSnapshotPre[$name].Path `
                      -Destination (Join-Path $saveBackupDir $name) -Force
        }
        Write-Host "  backed up $($saveSnapshotPre.Count) files to $saveBackupDir"
    } else {
        Write-Host "  no save files to back up"
    }
} else {
    Write-Banner 'NoSaveBackup: skipping save backup (operator-managed)'
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
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $PackDir
$env:SG_PREFLIGHT_HOT_RELOAD    = '1'
$env:SG_PREFLIGHT_NO_AUTOLOAD   = '1'
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
# Make sure stale flags from prior interactive runs do not leak.
Remove-Item Env:SG_PREFLIGHT_LOG_SETTEXT  -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_WINDOW_TITLE   -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_SHELL_BUILD_LABEL    -ErrorAction SilentlyContinue

Write-Host "  SG_PREFLIGHT_OVERRIDE_DIR  = $PackDir"
Write-Host "  SG_PREFLIGHT_HOT_RELOAD    = 1"
Write-Host "  SG_PREFLIGHT_NO_AUTOLOAD   = 1"
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
    return $null
}

function Pump-Events {
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
        Pump-Events
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
    Pump-Events
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
    Pump-Events
    if ($shellExe -and (Test-Path -LiteralPath $shellExe)) {
        Remove-Item -LiteralPath $shellExe -Force -ErrorAction SilentlyContinue
    }

    # Restore save if backup exists and post-run hashes differ.
    if (-not $NoSaveBackup -and $saveBackupDir -and (Test-Path -LiteralPath $saveBackupDir)) {
        $post = Get-SaveSnapshot -Dir $saveDir
        $changed = $false
        foreach ($name in @($saveSnapshotPre.Keys)) {
            if (-not $post.Contains($name) -or
                $post[$name].Hash -ne $saveSnapshotPre[$name].Hash) {
                $changed = $true
                break
            }
        }
        if ($changed) {
            Write-Host "[save] save changed during run; restoring backup" -ForegroundColor Yellow
            Restore-Save -BackupDir $saveBackupDir -LiveDir $saveDir
        } else {
            Write-Host "[save] save unchanged across run (good)"
        }
    }
}

Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_launch.jsonl') -Force
Write-Banner "Done. Evidence at $EvidenceDir"
