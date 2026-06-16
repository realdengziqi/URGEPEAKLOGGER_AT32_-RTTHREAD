param(
    [string]$Target = "surge_rtthread_base_ac5",
    [switch]$Build
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root "firmware\project\mdk_v5\surge_rtthread_base.uvprojx"
$Log = Join-Path $Root "firmware\project\mdk_v5\$Target`_build.log"
$Uv4Candidates = @(
    "C:\Keil_v5\UV4\UV4.exe",
    "C:\Keil\UV4\UV4.exe"
)

$Uv4 = $Uv4Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Uv4) {
    throw "UV4.exe not found. Install Keil MDK or update tools/build.ps1."
}

if (-not (Test-Path $Project)) {
    throw "Project file not found: $Project"
}

$Action = if ($Build) { "-b" } else { "-r" }

Write-Host "Project: $Project"
Write-Host "Target : $Target"
Write-Host "UV4    : $Uv4"

$Arguments = @($Action, $Project, "-t", $Target, "-o", $Log)
$Process = Start-Process -FilePath $Uv4 -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
$ExitCode = $Process.ExitCode

if (-not (Test-Path $Log)) {
    throw "Keil build log was not generated. ExitCode=$ExitCode Log=$Log"
}

Get-Content $Log | Select-Object -Last 80

if ($ExitCode -gt 1) {
    throw "Keil build failed. ExitCode=$ExitCode Log=$Log"
}

Write-Host "Build completed. Log=$Log"
