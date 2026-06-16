param(
    [string]$Target = "surge_rtthread_base_ac5"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root "firmware\project\mdk_v5\surge_rtthread_base.uvprojx"
$Log = Join-Path $Root "firmware\project\mdk_v5\$Target`_flash.log"
$Uv4Candidates = @(
    "C:\Keil_v5\UV4\UV4.exe",
    "C:\Keil\UV4\UV4.exe"
)

$Uv4 = $Uv4Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Uv4) {
    throw "UV4.exe not found. Install Keil MDK or update tools/flash.ps1."
}

if (-not (Test-Path $Project)) {
    throw "Project file not found: $Project"
}

$Project = (Resolve-Path $Project).Path
$LogDirectory = Split-Path -Parent $Log
if (-not (Test-Path $LogDirectory)) {
    New-Item -ItemType Directory -Path $LogDirectory | Out-Null
}
$Log = Join-Path (Resolve-Path $LogDirectory).Path (Split-Path -Leaf $Log)

Write-Host "Project: $Project"
Write-Host "Target : $Target"
Write-Host "UV4    : $Uv4"
Write-Host "Log    : $Log"

$Arguments = @("-f", $Project, "-t", $Target, "-o", $Log)
$Process = Start-Process -FilePath $Uv4 -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
$ExitCode = $Process.ExitCode

if (-not (Test-Path $Log)) {
    throw "Keil flash log was not generated. ExitCode=$ExitCode Log=$Log"
}

$LogContent = Get-Content $Log -Raw
$LogContent -split "`r?`n" | Select-Object -Last 80

if ($ExitCode -gt 1) {
    throw "Keil flash failed. ExitCode=$ExitCode Log=$Log"
}

if ($LogContent -match "(?i)flash download failed|error") {
    throw "Keil flash log reports an error. ExitCode=$ExitCode Log=$Log"
}

if ($LogContent -notmatch "Verify OK" -or $LogContent -notmatch "Flash Load finished") {
    throw "Keil flash did not report Verify OK and Flash Load finished. ExitCode=$ExitCode Log=$Log"
}

Write-Host "Flash completed. Log=$Log"
