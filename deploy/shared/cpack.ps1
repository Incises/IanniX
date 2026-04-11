param(
    [Parameter(Mandatory = $true)]
    [string]$Generator,
    [string]$OutDir = "",
    [string]$BuildDir = ""
)

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build"
}
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $RepoRoot ("dist\" + $Generator.ToLowerInvariant())
}

$Config = $env:IANNIX_BUILD_CONFIG
if ([string]::IsNullOrWhiteSpace($Config)) {
    $Config = "Release"
}

cmake -S $RepoRoot -B $BuildDir
cmake --build $BuildDir --config $Config
cpack --config (Join-Path $BuildDir "CPackConfig.cmake") -G $Generator -B $OutDir -C $Config

Write-Host ("Generated {0} package artifacts in {1}" -f $Generator, $OutDir)
