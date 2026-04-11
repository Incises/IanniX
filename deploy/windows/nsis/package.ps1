param(
    [string]$BuildDir = "",
    [string]$OutDir = ""
)

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build"
}
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $RepoRoot "dist\windows\nsis"
}

cmake -S $RepoRoot -B $BuildDir
cmake --build $BuildDir --config Release
cpack --config (Join-Path $BuildDir "CPackConfig.cmake") -G NSIS -B $OutDir -C Release
