param(
    [string]$QtRoot = 'C:/Qt/6.12.0/mingw_64',
    [string]$OpenCVRoot = 'C:/OpenCV/5.0.0/x64/mingw',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$previousPath = $env:PATH
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed: $LASTEXITCODE" }
}
try {
    $env:PATH = "$OpenCVRoot/bin;$QtRoot/bin;$CompilerBin;" + $previousPath
    foreach ($mode in @('core', 'ui')) {
        $buildDirectory = Join-Path $projectRoot "build/a9-$mode"
        $uiEnabled = if ($mode -eq 'ui') { 'ON' } else { 'OFF' }
        Invoke-Checked 'cmake' @('-S', $projectRoot, '-B', $buildDirectory, '-G', 'Ninja',
            '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_TESTING=ON', "-DVISIONFLOW_BUILD_UI=$uiEnabled",
            "-DCMAKE_PREFIX_PATH=$QtRoot", "-DOpenCV_DIR=$OpenCVRoot/lib")
        Invoke-Checked 'cmake' @('--build', $buildDirectory, '-j', "$Jobs")
        Invoke-Checked 'ctest' @('--test-dir', $buildDirectory, '--output-on-failure')
    }
} finally { $env:PATH = $previousPath }
