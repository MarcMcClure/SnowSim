$ErrorActionPreference = "Stop"

# Ensure we run relative to the repository root (one level up from tests/)
Set-Location (Split-Path -Path $PSScriptRoot -Parent)

$buildDir = Join-Path -Path "." -ChildPath "build"
if (-not (Test-Path $buildDir)) {
    Write-Error "Build directory '$buildDir' not found. Run CMake configure/build first."
}

$exePath = Join-Path $buildDir "Debug\snow_sim_unit_tests.exe"
if (-not (Test-Path $exePath)) {
    Write-Error "Executable '$exePath' not found. Build the Debug configuration first."
}

$width = 36  # tweak as needed
& $exePath --reporter compact --success --colour-mode ansi |
    ForEach-Object {
        if ($_ -match '^(?<ansi>\x1B\[[0-9;]*m)[A-Za-z]:.*[\\/](?<file>[^\\/]+\(\d+\)):(?<tail>.*)$') {
            $padded = ($matches['file'] + ":").PadRight($width)
            $matches['ansi'] + $padded + $matches['tail'].TrimStart()
        } else {
            $_
        }
    }
