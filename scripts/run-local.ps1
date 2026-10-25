param(
    [switch]$SmokeTest,
    [string]$BuildDirectory = 'build/reference-ui',
    [string]$QtRoot = 'C:/Qt/6.12.0/mingw_64',
    [string]$OpenCVRoot = 'C:/OpenCV/5.0.0/x64/mingw',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $projectRoot $BuildDirectory }
$executable = Join-Path $buildPath 'visionflow.exe'
if (-not (Test-Path -LiteralPath $executable)) { throw "Executable not found: $executable. Build using the README instructions first." }
$previousPath = $env:PATH
$previousPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$OpenCVRoot/bin;$QtRoot/bin;$CompilerBin;" + $previousPath
    if ($SmokeTest) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        & $executable --smoke-test
    } else { & $executable }
    if ($LASTEXITCODE -ne 0) { throw "VisionFlow exit code: $LASTEXITCODE" }
} finally {
    $env:PATH = $previousPath
    $env:QT_QPA_PLATFORM = $previousPlatform
}
