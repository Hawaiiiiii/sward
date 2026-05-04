param(
    # Historical contract marker:
    # [ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]
    [ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-owner-setter-probe", "native-owner-setter-status", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]
    [string]$Command = "state",
    [string]$Target = "",
    [string]$GlobalName = "",
    [string]$GlobalValue = "1",
    [string]$Project = "",
    [string]$Scene = "",
    [string]$Frame = "",
    [string]$MotionFrame = "",
    [string]$PipeName = "sward_ui_lab_live",
    [int]$TimeoutMilliseconds = 3000,
    [switch]$Raw,
    [switch]$AsJson
)

$ErrorActionPreference = "Stop"

function Get-UiLabBridgeCommandText(
    [string]$Command,
    [string]$Target,
    [string]$GlobalName,
    [string]$GlobalValue,
    [string]$Project,
    [string]$Scene,
    [string]$Frame,
    [string]$MotionFrame
) {
    switch ($Command) {
        "route" {
            if ([string]::IsNullOrWhiteSpace($Target)) {
                throw "route <target> requires -Target."
            }

            return "route $Target"
        }
        "set-global" {
            if ([string]::IsNullOrWhiteSpace($GlobalName)) {
                throw "set-global <name> <0|1> requires -GlobalName."
            }

            return "set-global $GlobalName $GlobalValue"
        }
        "native-make-observe" {
            if ([string]::IsNullOrWhiteSpace($Project) -or [string]::IsNullOrWhiteSpace($Scene)) {
                throw "native-make-observe <project> <scene> [frame] requires -Project and -Scene."
            }

            if ([string]::IsNullOrWhiteSpace($Frame)) {
                return "native-make-observe $Project $Scene"
            }

            return "native-make-observe $Project $Scene $Frame"
        }
        "native-motion-scrub" {
            if ([string]::IsNullOrWhiteSpace($MotionFrame)) {
                throw "native-motion-scrub <frame> requires -MotionFrame."
            }

            return "native-motion-scrub $MotionFrame"
        }
        default {
            return $Command
        }
    }
}

function Read-UiLabBridgeResponse(
    [System.IO.Pipes.NamedPipeClientStream]$Pipe,
    [int]$TimeoutMilliseconds
) {
    try {
        $Pipe.ReadTimeout = $TimeoutMilliseconds
    }
    catch {
        # Some PowerShell/.NET pipe wrappers reject timeout assignment. The
        # connect timeout still keeps this operator tool from hanging forever.
    }

    $buffer = New-Object byte[] 65536
    $builder = [System.Text.StringBuilder]::new()

    do {
        $read = $Pipe.Read($buffer, 0, $buffer.Length)
        if ($read -le 0) {
            break
        }

        [void]$builder.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $read))
    } while ($Pipe.IsMessageComplete -eq $false)

    return $builder.ToString()
}

function Invoke-UiLabBridgeCommand(
    [string]$CommandText,
    [string]$PipeName = "sward_ui_lab_live",
    [int]$TimeoutMilliseconds = 3000
) {
    # PipeOptions.None keeps the client a plain repo-safe operator probe.
    $pipe = [System.IO.Pipes.NamedPipeClientStream]::new(
        ".",
        $PipeName,
        [System.IO.Pipes.PipeDirection]::InOut,
        [System.IO.Pipes.PipeOptions]::None)

    try {
        $pipe.Connect($TimeoutMilliseconds)
        $pipe.ReadMode = [System.IO.Pipes.PipeTransmissionMode]::Message

        $bytes = [System.Text.Encoding]::UTF8.GetBytes($CommandText)
        $pipe.Write($bytes, 0, $bytes.Length)
        $pipe.Flush()

        return Read-UiLabBridgeResponse $pipe $TimeoutMilliseconds
    }
    finally {
        $pipe.Dispose()
    }
}

$commandText = Get-UiLabBridgeCommandText $Command $Target $GlobalName $GlobalValue $Project $Scene $Frame $MotionFrame
$response = Invoke-UiLabBridgeCommand $commandText $PipeName $TimeoutMilliseconds

if ($Raw) {
    Write-Output $response
    return
}

try {
    $parsed = $response | ConvertFrom-Json

    if ($AsJson) {
        $parsed | ConvertTo-Json -Depth 16
    }
    else {
        $parsed
    }
}
catch {
    Write-Output $response
}
