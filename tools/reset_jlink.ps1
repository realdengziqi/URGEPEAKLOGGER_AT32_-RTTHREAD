param(
    [string]$Script = "tools\jlink\reset_backend_serial.jlink",
    [string]$JLinkExe = "",
    [int]$DelaySeconds = 1
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$ScriptPath = Join-Path $Root $Script

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

if ($DelaySeconds -gt 0) {
    Start-Sleep -Seconds $DelaySeconds
}

Push-Location $Root
try {
    Write-Host "Using J-Link: $JLinkExe"
    & $JLinkExe -NoGui 1 -CommanderScript $ScriptPath
}
finally {
    Pop-Location
}
