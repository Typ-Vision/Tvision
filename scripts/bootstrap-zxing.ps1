param(
    [switch]$Download,
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [int]$Jobs = 6
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$dependencyRoot = Join-Path $projectRoot 'build/deps'
$manifest = Get-Content -LiteralPath (Join-Path $projectRoot 'third_party/manifest.json') -Raw | ConvertFrom-Json
$previousPath = $env:PATH
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed: $LASTEXITCODE" }
}
try {
    $env:PATH = "$CompilerBin;" + $previousPath
    New-Item -ItemType Directory -Force "$dependencyRoot/downloads", "$dependencyRoot/src" | Out-Null
    foreach ($dependency in $manifest.dependencies) {
        $archive = Join-Path $dependencyRoot "downloads/$($dependency.id).zip"
        if (!(Test-Path -LiteralPath $archive)) {
            if (!$Download) { throw "Missing $archive. Use -Download explicitly or restore the pinned archive from offline storage." }
            Invoke-WebRequest -Uri $dependency.archiveUrl -OutFile $archive
        }
        $actualHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
        if ($actualHash -ne $dependency.sha256) { throw "Source SHA-256 mismatch: $archive" }
        # Re-extract verified bytes on every bootstrap, not a mutable source cache.
        Expand-Archive -LiteralPath $archive -DestinationPath "$dependencyRoot/src" -Force
        $source = Join-Path $dependencyRoot "src/$($dependency.sourceDirectory)"
        $license = Join-Path $source 'LICENSE'
        $recordedLicense = Join-Path $projectRoot "third_party/$($dependency.licenseFile)"
        # Git may check out the recorded text with CRLF on Windows. Compare
        # exact license text after only CRLF normalization; archive SHA-256
        # above still verifies every downloaded source byte.
        $upstreamLicenseText = [IO.File]::ReadAllText($license).Replace("`r`n", "`n")
        $recordedLicenseText = [IO.File]::ReadAllText($recordedLicense).Replace("`r`n", "`n")
        if ($upstreamLicenseText -cne $recordedLicenseText) {
            throw "Recorded license differs from pinned source: $($dependency.id)"
        }
        if ($dependency.id -eq 'zxing') {
            $arguments = @('-S', $source, '-B', "$dependencyRoot/zxing-build", '-G', 'Ninja',
                '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_STANDARD=20', '-DCMAKE_CXX_STANDARD_REQUIRED=ON',
                '-DCMAKE_CXX_EXTENSIONS=OFF', '-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON',
                "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe",
                "-DCMAKE_INSTALL_PREFIX=$dependencyRoot/zxing-prefix", '-DCMAKE_INSTALL_LIBDIR=lib')
            foreach ($property in $dependency.buildOptions.PSObject.Properties) {
                $arguments += "-D$($property.Name)=$($property.Value)"
            }
            Invoke-Checked 'cmake' $arguments
            Invoke-Checked 'cmake' @('--build', "$dependencyRoot/zxing-build", '-j', "$Jobs")
            Invoke-Checked 'cmake' @('--install', "$dependencyRoot/zxing-build")
            $licenseDestination = "$dependencyRoot/zxing-prefix/share/licenses/ZXing"
            New-Item -ItemType Directory -Force $licenseDestination | Out-Null
            Copy-Item -LiteralPath $recordedLicense -Destination "$licenseDestination/LICENSE"
            Copy-Item -LiteralPath (Join-Path $projectRoot "third_party/$($dependency.embeddedNoticesFile)") -Destination "$licenseDestination/EMBEDDED-NOTICES.txt"
        }
    }
    $zint = $manifest.dependencies | Where-Object { $_.id -eq 'zint' }
    Invoke-Checked 'cmake' @('-S', "$projectRoot/tests/dependency-fixtures", '-B', "$dependencyRoot/fixture-build",
        '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_C_COMPILER=$CompilerBin/gcc.exe",
        "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe", "-DVISIONFLOW_ZINT_SOURCE=$dependencyRoot/src/$($zint.sourceDirectory)")
    Invoke-Checked 'cmake' @('--build', "$dependencyRoot/fixture-build", '--target', 'code_fixture_generator', '-j', "$Jobs")
    Invoke-Checked "$dependencyRoot/fixture-build/code_fixture_generator.exe" @(
        "$projectRoot/tests/dependency-fixtures/codes.tsv", "$dependencyRoot/code-fixtures")
    Invoke-Checked "$dependencyRoot/fixture-build/code_fixture_generator.exe" @(
        "$projectRoot/tests/dependency-fixtures/codes-t3.tsv", "$dependencyRoot/code-fixtures-t3")
    Write-Output "Bootstrap complete. Reader prefix: $dependencyRoot/zxing-prefix"
} finally { $env:PATH = $previousPath }
