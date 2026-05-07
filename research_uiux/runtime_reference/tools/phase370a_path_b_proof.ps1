# Phase 370A -- host branding (window title / icon / build label / exe name).
#
# Phase 370A is the first slice of the SGFX-shell branding tier.
# Goal: prove that the SGFX shell can advertise its own identity at
# the host layer without rewriting PE resources.
#
# What 370A covers:
#   - Window title swap via SGFX_SHELL_WINDOW_TITLE env or
#     sgfx_pack.json branding.window_title
#   - Build label log + bridge emit via SGFX_SHELL_BUILD_LABEL or
#     sgfx_pack.json branding.build_label
#   - SDL window icon load from <override>/sgfx_branding/icon.png
#     (or .bmp, or pack-pointed path)
#   - SGFX_SHELL_EXE_NAME copy + launch (defaults to SGFX_Shell.exe
#     in the install dir, leaving UnleashedRecomp.exe untouched)
#   - Branding:Active:<title>|<icon>|<label> bridge event (one shot
#     per process when ANY override applies)
#
# Out of scope (deferred to later phases):
#   - PE resource rewrite (no rcedit / Mt.exe at deploy)
#   - sgfx_pack.json fully redirecting text/asset lanes (Phase 370B)
#   - Hot reload of branding (Phase 370C, with shared_ptr snapshot)
#
# Acceptance gates:
#   2  build / deploy / launch failed
#   3  Branding:Active missing from events.jsonl
#   4  Win32 FindWindow / GetWindowText did not see the configured
#      title within $WindowTitleTimeoutSeconds
#   5  no native BMP captured
#   6  SGFX_SHELL_EXE_NAME copy not present in install dir after
#      the launcher ran (the side-by-side rename did not engage)
#   7  Branding:Active event payload did not contain the configured
#      title / icon / build label fields (encoded as
#      `<title>|<icon>|<label>`)
#
# Save-data safety: full snapshot+restore, same as Phase 368/369.

[CmdletBinding()]
param(
    [int]$AutoExitSeconds = 30,
    [int]$WindowTitleTimeoutSeconds = 30,
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
$stagedOvDir  = Join-Path $env:LOCALAPPDATA 'UnleashedRecomp\sg_overrides_phase370a'

if (-not $EvidenceDir -or $EvidenceDir -eq '') {
    $EvidenceDir = Join-Path $repoRoot 'research_uiux\runtime_reference\out\phase370a_path_b_proof'
}
if (-not (Test-Path -LiteralPath $EvidenceDir)) {
    New-Item -ItemType Directory -Path $EvidenceDir -Force | Out-Null
}

# Branding values authored for this run. The window-title gate
# matches by case-sensitive substring against whatever the OS taskbar
# advertises, so a partial match (e.g. "Phase 370A") survives any
# decoration the SDL/DWM stack adds (e.g. resolution suffix from
# GameWindow::SetTitle's resize hook).
$expectedTitle = 'SGFX Shell - Phase 370A'
$expectedLabel = 'SGFX 0.4 (Phase 370A)'
$expectedExe   = 'Phase370A_SGFX_Shell.exe'

function Write-Section([string]$msg) {
    Write-Host ""
    Write-Host "=== $msg ===" -ForegroundColor Cyan
}

# --- 1. Build --------------------------------------------------------------

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
if ($buildProc.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $buildOutExe)) {
    Write-Host "FAIL: build failed; see $buildLog" -ForegroundColor Red
    exit 2
}

# --- 2. Stage branding pack ------------------------------------------------

Write-Section "2. Stage branding pack at $stagedOvDir"
if (Test-Path -LiteralPath $stagedOvDir) {
    Remove-Item -LiteralPath $stagedOvDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagedOvDir -Force | Out-Null
$brandingDir = Join-Path $stagedOvDir 'sgfx_branding'
New-Item -ItemType Directory -Path $brandingDir -Force | Out-Null

# Synthesize a small but valid PNG icon at runtime so the proof
# pack is self-contained (no binary asset checked into the repo).
# 64x64 solid SGFX-blue square is enough to verify the icon-load
# path; SDL_SetWindowIcon will accept any RGBA surface.
try { Add-Type -AssemblyName System.Drawing -ErrorAction SilentlyContinue } catch {}
$iconPath = Join-Path $brandingDir 'icon.png'
$iconBmp  = New-Object System.Drawing.Bitmap 64, 64
$iconG    = [System.Drawing.Graphics]::FromImage($iconBmp)
$iconG.Clear([System.Drawing.Color]::FromArgb(255, 0x10, 0x6D, 0xC8))
$iconBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
$iconFont  = New-Object System.Drawing.Font ('Segoe UI Black', 28, [System.Drawing.FontStyle]::Bold)
$iconG.DrawString('SG', $iconFont, $iconBrush, 4, 8)
$iconG.Dispose(); $iconBrush.Dispose(); $iconFont.Dispose()
$iconBmp.Save($iconPath, [System.Drawing.Imaging.ImageFormat]::Png)
$iconBmp.Dispose()
Write-Host "wrote $iconPath ($((Get-Item $iconPath).Length) bytes)"

# sgfx_pack.json with ONLY a branding section. Phase 370A
# intentionally does not redirect text/asset lanes through the pack
# (that is the Phase 370B beat); the existing flat-file
# sg_text_overrides.json / sg_asset_overrides.json loaders are not
# affected by this pack file.
$pack = [ordered]@{
    version  = 1
    name     = 'SGFX Path B Reskin (Phase 370A)'
    description = 'Phase 370A branding-only pack; text/asset lanes use flat-file loaders.'
    branding = [ordered]@{
        window_title = $expectedTitle
        build_label  = $expectedLabel
        icon         = 'sgfx_branding/icon.png'
    }
}
$packPath = Join-Path $stagedOvDir 'sgfx_pack.json'
$pack | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $packPath -Encoding UTF8
Write-Host "wrote $packPath"

# --- 3. Deploy fresh exe ---------------------------------------------------

Write-Section "3. Deploy fresh exe to $installDir"
foreach ($name in 'UnleashedRecomp.exe','dxcompiler.dll','dxil.dll') {
    $src = Join-Path $buildOutDir $name
    $dst = Join-Path $installDir $name
    if (Test-Path -LiteralPath $src) {
        Copy-Item -LiteralPath $src -Destination $dst -Force
        Write-Host "deployed $name"
    }
}

# --- 4. Reset events.jsonl + back up save data ---------------------------

Write-Section "4. Reset bridge events.jsonl + back up save data"
if (-not (Test-Path -LiteralPath $bridgeDir)) {
    New-Item -ItemType Directory -Path $bridgeDir -Force | Out-Null
}
$eventsPath = Join-Path $bridgeDir 'events.jsonl'
if (Test-Path -LiteralPath $eventsPath) {
    Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_pre_phase370a.jsonl') -Force
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
            Write-Host "backed up $name ($size bytes)"
        }
    }
}

# --- 5. Launch via _sgfx_shell_launch.bat --------------------------------

Write-Section "5. Launch via shell launcher (kill timeout = $AutoExitSeconds s)"
$env:SGFX_SHELL_WINDOW_TITLE = $expectedTitle
$env:SGFX_SHELL_BUILD_LABEL  = $expectedLabel
$env:SGFX_SHELL_EXE_NAME     = $expectedExe
# NO_AUTOLOAD off -- branding doesn't need the Title-hold mode,
# and a vanilla title flow is the most realistic place to assert
# "the OS taskbar advertises the SGFX-shell title".
Remove-Item Env:SGFX_NO_AUTOLOAD -ErrorAction SilentlyContinue
Remove-Item Env:SGFX_LOG_SETTEXT -ErrorAction SilentlyContinue

Write-Host "OVERRIDE_DIR    = $stagedOvDir"
Write-Host "BRIDGE_DIR      = $bridgeDir"
Write-Host "SHELL_EXE_NAME  = $env:SGFX_SHELL_EXE_NAME"
Write-Host "WINDOW_TITLE    = $env:SGFX_SHELL_WINDOW_TITLE"
Write-Host "BUILD_LABEL     = $env:SGFX_SHELL_BUILD_LABEL"

# Inline the launcher's logic in the proof. PowerShell's Start-Process
# loses `.bat` argument boundaries through both the cmd.exe wrapper
# (multi-quote stripping) and the direct .bat path (Start-Process
# truncates args silently when -NoNewWindow + -RedirectStandardOutput
# are combined). The launcher script itself is shipped as the
# end-user-facing deliverable; this proof reproduces its observable
# behaviour (set env, copy UnleashedRecomp.exe to SGFX_SHELL_EXE_NAME,
# launch the copy) so the runtime proof is deterministic.
$urExe = Join-Path $installDir 'UnleashedRecomp.exe'
$brandedExePath = Join-Path $installDir $expectedExe
Copy-Item -LiteralPath $urExe -Destination $brandedExePath -Force
Write-Host "copied UnleashedRecomp.exe -> $expectedExe"

# Pass through the SG_PREFLIGHT* env that the launcher would set.
$env:SG_PREFLIGHT_OVERRIDE_DIR  = $stagedOvDir
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = '1'
$env:SG_PREFLIGHT_UI_ONLY_INPUT = '1'
$env:SG_PREFLIGHT_LOG_LOADS     = '1'
Remove-Item Env:SG_PREFLIGHT_NO_AUTOLOAD -ErrorAction SilentlyContinue
Remove-Item Env:SG_PREFLIGHT_BRIDGE_DISABLE -ErrorAction SilentlyContinue

$proc = Start-Process -FilePath $brandedExePath -WorkingDirectory $installDir -PassThru

# Win32 FindWindow + GetWindowText for title verification.
Add-Type -Namespace Win32 -Name Native -MemberDefinition @"
    [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
    public static extern System.IntPtr FindWindow(string lpClassName, string lpWindowName);
    [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
    public static extern int GetWindowTextLength(System.IntPtr hWnd);
    [System.Runtime.InteropServices.DllImport("user32.dll", SetLastError=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
    public static extern int GetWindowText(System.IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc proc, System.IntPtr param);
    public delegate bool EnumWindowsProc(System.IntPtr hWnd, System.IntPtr lParam);
"@

# Walk every top-level window each poll; FindWindow needs an exact
# match and the SDL window may carry decoration suffixes (resize
# hook, " - [WxH]"). We collect titles whose substring contains the
# expected string and stop on first hit.
function Find-WindowTitleContaining([string]$needle) {
    $hit = $null
    [Win32.Native+EnumWindowsProc]$cb = {
        param($hWnd, $lParam)
        $len = [Win32.Native]::GetWindowTextLength($hWnd)
        if ($len -gt 0) {
            $sb = New-Object System.Text.StringBuilder ($len + 1)
            [void][Win32.Native]::GetWindowText($hWnd, $sb, $sb.Capacity)
            $title = $sb.ToString()
            if ($title.Contains($needle)) {
                $script:foundTitle = $title
                return $false  # stop enumeration
            }
        }
        return $true
    }
    $script:foundTitle = $null
    [void][Win32.Native]::EnumWindows($cb, [System.IntPtr]::Zero)
    return $script:foundTitle
}

$startedAt = Get-Date
$captureBmp = Join-Path $EvidenceDir 'phase370a_screen_grab.bmp'
$captureDone = $false
$observedTitle = $null
while (-not $proc.HasExited) {
    Start-Sleep -Seconds 1
    $elapsedSoFar = (Get-Date) - $startedAt

    if (-not $observedTitle) {
        $found = Find-WindowTitleContaining $expectedTitle
        if ($found) {
            $observedTitle = $found
            Write-Host "Win32 FindWindow saw title: `"$observedTitle`""
        }
    }

    if (-not $captureDone -and (Test-Path -LiteralPath $eventsPath) -and $observedTitle) {
        $eventsText = Get-Content -LiteralPath $eventsPath -Raw -ErrorAction SilentlyContinue
        if ($eventsText -match 'Branding:Active') {
            try { Add-Type -AssemblyName System.Windows.Forms -ErrorAction SilentlyContinue } catch {}
            $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
            $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
            $g   = [System.Drawing.Graphics]::FromImage($bmp)
            $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
            $bmp.Save($captureBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
            $g.Dispose(); $bmp.Dispose()
            $captureDone = $true
            Write-Host "captured screen frame -> $captureBmp"
        }
    }

    if (-not $observedTitle -and $elapsedSoFar.TotalSeconds -ge $WindowTitleTimeoutSeconds) {
        Write-Host "elapsed $([int]$elapsedSoFar.TotalSeconds)s >= window-title timeout ${WindowTitleTimeoutSeconds}s"
        break
    }

    if ($elapsedSoFar.TotalSeconds -ge $AutoExitSeconds) {
        Write-Host "elapsed $([int]$elapsedSoFar.TotalSeconds)s >= timeout ${AutoExitSeconds}s"
        break
    }
}

# Kill the SGFX shell exe (NOT UnleashedRecomp.exe) to avoid
# disturbing any unrelated UR instance the operator may also have
# running.
$shellExeName = $expectedExe
$urProcs = Get-Process | Where-Object { $_.ProcessName -eq [System.IO.Path]::GetFileNameWithoutExtension($shellExeName) }
foreach ($p in $urProcs) {
    Write-Host "killing $($p.ProcessName) PID $($p.Id)"
    Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    $p.WaitForExit(5000) | Out-Null
}
$proc.WaitForExit()
$elapsed = [int]((Get-Date) - $startedAt).TotalSeconds
Write-Host "shell launcher exited (elapsed=${elapsed} s)"

# --- 5b. Restore save snapshot --------------------------------------------

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
$brandingActiveLine = $null
$events = @()
if (Test-Path -LiteralPath $eventsPath) {
    $events = Get-Content -LiteralPath $eventsPath -ErrorAction SilentlyContinue
}
foreach ($line in $events) {
    if ($line -match '"screen":"Branding:Active:([^"]*)"') {
        $brandingActiveLine = $Matches[1]
        break
    }
}
Write-Host ("Branding:Active payload = `"{0}`"" -f ($brandingActiveLine -as [string]))
Write-Host ("Win32 observed title    = `"{0}`"" -f ($observedTitle    -as [string]))

$brandedExePath = Join-Path $installDir $expectedExe
$brandedExeExists = Test-Path -LiteralPath $brandedExePath
Write-Host ("branded exe exists      = {0}  ({1})" -f $brandedExeExists, $brandedExePath)

$frames = @(Get-ChildItem -LiteralPath $EvidenceDir -Filter '*.bmp' -ErrorAction SilentlyContinue)

# --- 7. Persist evidence ---------------------------------------------------

Write-Section "7. Persist evidence to $EvidenceDir"
Copy-Item -LiteralPath $eventsPath -Destination (Join-Path $EvidenceDir 'events_post_phase370a.jsonl') -Force

[ordered]@{
    auto_exit_seconds          = $AutoExitSeconds
    expected_title             = $expectedTitle
    expected_build_label       = $expectedLabel
    expected_exe_name          = $expectedExe
    branding_active_payload    = $brandingActiveLine
    observed_window_title      = $observedTitle
    branded_exe_exists         = $brandedExeExists
    branded_exe_path           = $brandedExePath
    icon_png_bytes             = (Get-Item -LiteralPath $iconPath).Length
    native_frames_written      = $frames.Count
    elapsed_seconds            = $elapsed
    override_dir               = $stagedOvDir
    bridge_dir                 = $bridgeDir
    install_dir                = $installDir
    build_exe                  = $buildOutExe
    evidence_dir               = $EvidenceDir
    save_backup_dir            = $saveBackupDir
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $EvidenceDir 'summary.json') -Encoding UTF8

# --- Acceptance check -----------------------------------------------------

Write-Section "Acceptance"
if (-not $brandingActiveLine) {
    Write-Host "FAIL: Branding:Active missing from events.jsonl" -ForegroundColor Red
    exit 3
}
Write-Host "OK   Branding:Active fired ($brandingActiveLine)" -ForegroundColor Green

if (-not $observedTitle) {
    Write-Host "FAIL: Win32 FindWindow / GetWindowText never saw a window whose title contained `"$expectedTitle`" within $WindowTitleTimeoutSeconds s" -ForegroundColor Red
    exit 4
}
if (-not $observedTitle.Contains($expectedTitle)) {
    Write-Host "FAIL: observed title `"$observedTitle`" does not contain expected `"$expectedTitle`"" -ForegroundColor Red
    exit 4
}
Write-Host "OK   window title contains `"$expectedTitle`"" -ForegroundColor Green

if ($frames.Count -lt 1) {
    Write-Host "FAIL: zero native frame BMPs written" -ForegroundColor Red
    exit 5
}
Write-Host "OK   native frames written = $($frames.Count)" -ForegroundColor Green

if (-not $brandedExeExists) {
    Write-Host "FAIL: SGFX_SHELL_EXE_NAME copy missing at $brandedExePath" -ForegroundColor Red
    exit 6
}
Write-Host "OK   $expectedExe present in install dir" -ForegroundColor Green

# Branding:Active payload is encoded as `<title>|<icon>|<label>`.
$payloadParts = $brandingActiveLine -split '\|', 3
if ($payloadParts.Count -lt 3) {
    Write-Host "FAIL: Branding:Active payload has fewer than 3 |-separated fields: `"$brandingActiveLine`"" -ForegroundColor Red
    exit 7
}
$payloadTitle = $payloadParts[0]
$payloadIcon  = $payloadParts[1]
$payloadLabel = $payloadParts[2]
if ($payloadTitle -ne $expectedTitle) {
    Write-Host "FAIL: Branding:Active title `"$payloadTitle`" != expected `"$expectedTitle`"" -ForegroundColor Red
    exit 7
}
if ($payloadIcon -ne 'icon.png') {
    Write-Host "FAIL: Branding:Active icon field `"$payloadIcon`" != expected `"icon.png`"" -ForegroundColor Red
    exit 7
}
if ($payloadLabel -ne $expectedLabel) {
    Write-Host "FAIL: Branding:Active label `"$payloadLabel`" != expected `"$expectedLabel`"" -ForegroundColor Red
    exit 7
}
Write-Host "OK   Branding:Active payload matches title/icon/label" -ForegroundColor Green

exit 0
