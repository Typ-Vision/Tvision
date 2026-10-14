param(
    [string]$OpenCVRoot = 'C:/OpenCV/5.0.0/x64/mingw',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$dependencyRoot = Join-Path $projectRoot 'build/deps'
$buildDirectory = Join-Path $projectRoot 'build/zxing-probe'
$previousPath = $env:PATH
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed: $LASTEXITCODE" }
}
try {
    $env:PATH = "$OpenCVRoot/bin;$CompilerBin;" + $previousPath
    Invoke-Checked 'cmake' @('-S', "$projectRoot/tests/zxing-probe", '-B', $buildDirectory,
        '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe",
        "-DOpenCV_DIR=$OpenCVRoot/lib", "-DVISIONFLOW_ZXING_PREFIX=$dependencyRoot/zxing-prefix",
        "-DVISIONFLOW_CODE_FIXTURES=$dependencyRoot/code-fixtures")
    Invoke-Checked 'cmake' @('--build', $buildDirectory, '-j', "$Jobs")
    Invoke-Checked 'ctest' @('--test-dir', $buildDirectory, '--output-on-failure')
    $files = @(Get-ChildItem -LiteralPath "$dependencyRoot/zxing-prefix" -Recurse -File)
    $files += Get-ChildItem -LiteralPath "$dependencyRoot/code-fixtures" -File
    $files += Get-Item -LiteralPath "$dependencyRoot/fixture-build/code_fixture_generator.exe", "$buildDirectory/zxing_reader_probe.exe"
    $inventory = foreach ($file in $files) {
        [ordered]@{ path = $file.FullName; bytes = $file.Length; sha256 = (Get-FileHash -LiteralPath $file.FullName).Hash }
    }
    $inventory | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath "$buildDirectory/zxing-artifact-inventory.json" -Encoding UTF8
    & "$CompilerBin/g++.exe" -dumpmachine | Set-Content -LiteralPath "$buildDirectory/compiler-target.txt" -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'Compiler target query failed' }
    Invoke-Checked "$CompilerBin/objdump.exe" @('-p', "$buildDirectory/zxing_reader_probe.exe") |
        Set-Content -LiteralPath "$buildDirectory/pe-imports.txt" -Encoding UTF8
} finally { $env:PATH = $previousPath }
