# SentinelOS ESP32-S3 Upload + Monitor Helper
# Run from the firmware project directory.
#
# Workflow:
#   1. Put the LilyGO into ROM download mode:
#        Hold BOOT -> tap RESET -> release BOOT
#   2. Run:
#        .\flash-monitor.ps1
#
# The script:
#   - Detects the current Espressif upload COM port
#   - Uploads SentinelOS
#   - Waits for the TinyUSB CDC + MSC application device
#   - Detects its COM port
#   - Opens the PlatformIO serial monitor

$ErrorActionPreference = "Stop"

function Get-EspressifPorts {
    $ports = Get-PnpDevice -PresentOnly -Class Ports -ErrorAction SilentlyContinue |
        Where-Object {
            $_.InstanceId -like "*VID_303A*"
        }

    foreach ($port in $ports) {
        if ($port.FriendlyName -match '\((COM\d+)\)') {
            [PSCustomObject]@{
                Port       = $Matches[1]
                Name       = $port.FriendlyName
                InstanceId = $port.InstanceId
            }
        }
    }
}

function Test-SentinelCompositeDevice {
    $msc = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object {
            $_.InstanceId -like "*VID_303A*" -and
            $_.FriendlyName -eq "USB Mass Storage Device"
        }

    return ($null -ne $msc)
}

Write-Host ""
Write-Host "SentinelOS Upload + Monitor"
Write-Host "==========================="
Write-Host ""

Write-Host "Looking for ESP32-S3 download port..."

$uploadPorts = @(Get-EspressifPorts)

if ($uploadPorts.Count -eq 0) {
    Write-Host ""
    Write-Host "No Espressif COM port found."
    Write-Host ""
    Write-Host "Put the board into ROM download mode:"
    Write-Host "  1. Hold BOOT"
    Write-Host "  2. Press and release RESET"
    Write-Host "  3. Release BOOT"
    Write-Host ""
    exit 1
}

if ($uploadPorts.Count -gt 1) {
    Write-Host "Multiple Espressif ports detected:"
    $uploadPorts | Format-Table Port, Name

    Write-Host ""
    Write-Host "Unable to safely determine the upload port."
    exit 1
}

$uploadPort = $uploadPorts[0].Port

Write-Host "Upload port found: $uploadPort"
Write-Host ""
Write-Host "Uploading SentinelOS..."
Write-Host ""

python -m platformio run `
    --target upload `
    --upload-port $uploadPort

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "UPLOAD FAILED."
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Upload complete."
Write-Host ""
Write-Host "=========================================================="
Write-Host "PRESS RESET ON THE LILYGO NOW"
Write-Host "=========================================================="
Write-Host ""
Write-Host "Waiting for SentinelOS USB CDC..."
Write-Host ""

$monitorPort = $null
$timeoutSeconds = 60
$startTime = Get-Date

while (((Get-Date) - $startTime).TotalSeconds -lt $timeoutSeconds) {

    Start-Sleep -Milliseconds 100

    # SentinelOS TinyUSB CDC is interface MI_01.
    # ROM downloader / USB-Serial-JTAG uses a different interface.
    $cdcDevice = Get-PnpDevice -PresentOnly -Class Ports `
        -ErrorAction SilentlyContinue |
        Where-Object {
            $_.InstanceId -like "*VID_303A*" -and
            $_.InstanceId -like "*MI_01*"
        } |
        Select-Object -First 1

    if ($cdcDevice -and
        $cdcDevice.FriendlyName -match '\((COM\d+)\)') {

        $monitorPort = $Matches[1]
        break
    }
}

if (-not $monitorPort) {
    Write-Host ""
    Write-Host "SentinelOS CDC port was not detected."
    Write-Host ""
    python -m serial.tools.list_ports -v
    exit 1
}

Write-Host "SentinelOS CDC detected on $monitorPort"
Write-Host "Opening serial monitor immediately..."
Write-Host ""

python -m platformio device monitor `
    --port $monitorPort `
    --baud 115200