param(
    [string]$Script = "tools\jlink\flash_backend_serial.jlink",
    [string]$Firmware = "",
    [string]$JLinkExe = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$ScriptPath = Join-Path $Root $Script
$TempScriptPath = $null

if ([string]::IsNullOrWhiteSpace($JLinkExe)) {
    $Candidates = @(
        "C:\Program Files\SEGGER\JLink_V946\JLink.exe",
        "C:\Program Files\SEGGER\JLink_V892\JLink.exe",
        "C:\Program Files\SEGGER\JLink_V848\JLink.exe",
        "C:\Program Files\SEGGER\JLink\JLink.exe"
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path $Candidate) {
            $JLinkExe = $Candidate
            break
        }
    }
}

if (-not (Test-Path $JLinkExe)) {
    throw "JLink.exe not found: $JLinkExe"
}

if (-not (Test-Path $ScriptPath)) {
    throw "J-Link script not found: $ScriptPath"
}

Push-Location $Root
try {
    if (-not [string]::IsNullOrWhiteSpace($Firmware)) {
        $FirmwarePath = Join-Path $Root $Firmware
        if (-not (Test-Path $FirmwarePath)) {
            throw "Firmware not found: $FirmwarePath"
        }

        $FirmwareForJLink = $Firmware -replace '\\', '/'
        $TempScriptPath = Join-Path $env:TEMP ("flash_backend_serial_{0}.jlink" -f ([Guid]::NewGuid().ToString("N")))
        $ScriptLines = Get-Content $ScriptPath
        $ScriptLines = $ScriptLines | ForEach-Object {
            if ($_ -match '^\s*loadfile\s+') {
                "loadfile $FirmwareForJLink"
            }
            else {
                $_
            }
        }
        Set-Content -Path $TempScriptPath -Value $ScriptLines -Encoding ASCII
        $ScriptPath = $TempScriptPath
    }

    Write-Host "Using J-Link: $JLinkExe"
    if (-not [string]::IsNullOrWhiteSpace($Firmware)) {
        Write-Host "Using firmware: $Firmware"
    }
    & $JLinkExe -NoGui 1 -CommanderScript $ScriptPath
}
finally {
    if ($TempScriptPath -and (Test-Path $TempScriptPath)) {
        Remove-Item $TempScriptPath -Force
    }
    Pop-Location
}
