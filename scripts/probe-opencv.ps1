param(
    [string]$OpenCVRoot = 'C:/OpenCV/5.0.0/x64/mingw',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot 'build/tool-probes'
$previousPath = $env:PATH
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed: $LASTEXITCODE" }
}
try {
    $env:PATH = "$OpenCVRoot/bin;$CompilerBin;" + $previousPath
    Invoke-Checked 'cmake' @('-S', (Join-Path $projectRoot 'tests/probes'), '-B', $buildDirectory,
        '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DOpenCV_DIR=$OpenCVRoot/lib")
    Invoke-Checked 'cmake' @('--build', $buildDirectory, '-j', "$Jobs")

    # Hash the installed import libraries/DLLs; successful execution separately
    # proves that the linked APIs load with this process PATH.
    # OpenCV's package root can differ from its architecture/compiler prefix.
    $linkInventory = Get-Content -LiteralPath (Join-Path $buildDirectory 'opencv-link-inventory.txt')
    $packageDirectory = (($linkInventory | Where-Object { $_ -match '^OpenCV_DIR=' }) -split '=', 2)[1]
    $packageConfig = Join-Path $packageDirectory 'OpenCVConfig.cmake'
    $files = @($packageConfig)
    $includeDirectory = (($linkInventory | Where-Object { $_ -match '^includes=' }) -split '=', 2)[1]
    foreach ($header in @('core.hpp', 'imgproc.hpp', 'imgcodecs.hpp', 'objdetect.hpp', 'calib.hpp', 'geometry.hpp', 'geometry/2d.hpp', 'geometry/3d.hpp')) {
        $files += Join-Path $includeDirectory "opencv2/$header"
    }
    $files += Get-ChildItem -LiteralPath "$OpenCVRoot/lib" -Filter '*opencv*.dll.a' | ForEach-Object { $_.FullName }
    $files += Get-ChildItem -LiteralPath "$OpenCVRoot/bin" -Filter '*opencv*.dll' | ForEach-Object { $_.FullName }
    $inventory = foreach ($file in $files) {
        $item = Get-Item -LiteralPath $file
        [ordered]@{ path = $item.FullName; bytes = $item.Length; sha256 = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash }
    }
    $inventory | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $buildDirectory 'opencv-binary-inventory.json') -Encoding UTF8
    & "$CompilerBin/g++.exe" -dumpmachine | Set-Content -LiteralPath (Join-Path $buildDirectory 'compiler-target.txt') -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'Compiler target query failed' }
    Invoke-Checked 'ctest' @('--test-dir', $buildDirectory, '--output-on-failure')
} finally { $env:PATH = $previousPath }
