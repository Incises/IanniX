param(
    [string]$BuildDir = "",
    [string]$OutDir = ""
)

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $RepoRoot "dist\windows\archive"
}

& (Join-Path $RepoRoot "deploy\shared\cpack.ps1") -Generator ZIP -BuildDir $BuildDir -OutDir $OutDir
