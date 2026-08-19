$ErrorActionPreference = "Stop"

$environment = "sentinelos_lilygo_amoled"

Write-Host ""
Write-Host "SentinelOS Flash"
Write-Host "================"
Write-Host ""

Write-Host "Looking for ESP32-S3 upload port..."

$device = Get-CimInstance Win32_PnPEntity |
    Where-Object {
        $_.Name -match "\(COM\d+\)" -and
        $_.PNPDeviceID -match "VID_303A"
    } |
    Select-Object -First 1

if ($null -eq $device)
{
    Write-Host ""
    Write-Host "ESP32-S3 download port was not detected." -ForegroundColor Red
    Write-Host ""
    Write-Host "Put the device into download mode:"
    Write-Host "  1. Hold BOOT"
    Write-Host "  2. Tap RESET"
    Write-Host "  3. Release BOOT"
    Write-Host ""
    exit 1
}

$match = [regex]::Match(
    $device.Name,
    "\(COM(\d+)\)"
)

if (!$match.Success)
{
    Write-Host "Unable to determine COM port." -ForegroundColor Red
    exit 1
}

$uploadPort = "COM" + $match.Groups[1].Value

Write-Host "Upload port found: $uploadPort"
Write-Host ""
Write-Host "Uploading SentinelOS..."
Write-Host ""

python -m platformio run `
    -e $environment `
    --target upload `
    --upload-port $uploadPort

if ($LASTEXITCODE -ne 0)
{
    Write-Host ""
    Write-Host "SentinelOS upload FAILED." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "Upload complete." -ForegroundColor Green
Write-Host ""
Write-Host "Tap RESET to start SentinelOS."
Write-Host ""