param([switch]$SmokeTest)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$executable = Join-Path $projectRoot 'build\visionflow.exe'
if (-not (Test-Path -LiteralPath $executable)) { throw 'Önce README içindeki derleme komutlarını çalıştırın.' }
$previousPath = $env:PATH
$previousPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $previousPath
    if ($SmokeTest) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        & $executable --smoke-test
    } else { & $executable }
    if ($LASTEXITCODE -ne 0) { throw "VisionFlow exit code: $LASTEXITCODE" }
} finally {
    $env:PATH = $previousPath
    $env:QT_QPA_PLATFORM = $previousPlatform
}
