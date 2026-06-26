param(
    [string]$Preset = "gcc-debug",
    [string]$Target = "surge_backend_serial"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

cmake --preset $Preset -S $Root
cmake --build --preset $Preset --target $Target

