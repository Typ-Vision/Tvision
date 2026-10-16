param(
    [string]$BuildDirectory = 'build/reference-ui',
    [string]$QtRoot = 'C:/Qt/6.12.0/mingw_64',
    [string]$OpenCVRoot = 'C:/OpenCV/5.0.0/x64/mingw',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [string]$CMakeBin = 'C:/Qt/Tools/CMake_64/bin',
    [int]$Jobs = 6
)
# Prepare an already configured Release build for direct Explorer launch.
# This is a local development deployment, not an installer/distribution package.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $projectRoot $BuildDirectory }
$buildPath = (Resolve-Path -LiteralPath $buildPath).Path
$cachePath = Join-Path $buildPath 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $cachePath)) { throw 'Configure the build using README first.' }
$cache = Get-Content -LiteralPath $cachePath -Raw
if ($cache -notmatch '(?m)^CMAKE_BUILD_TYPE:STRING=Release\r?$' -or
    $cache -notmatch '(?m)^VISIONFLOW_BUILD_UI:BOOL=ON\r?$') {
    throw 'A Release build with VISIONFLOW_BUILD_UI=ON is required.'
}
$deployTool = Join-Path $QtRoot 'bin/windeployqt.exe'
$objdump = Join-Path $CompilerBin 'objdump.exe'
$cmake = Join-Path $CMakeBin 'cmake.exe'
foreach ($tool in @($deployTool, $objdump, $cmake)) {
    if (-not (Test-Path -LiteralPath $tool)) { throw "Missing tool: $tool" }
}
$previousPath = $env:PATH
try {
    $env:PATH = "$QtRoot/bin;$OpenCVRoot/bin;$CompilerBin;$CMakeBin;" + $previousPath
    & $cmake --build $buildPath --config Release --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw "Release build failed: $LASTEXITCODE" }
    $executable = Join-Path $buildPath 'visionflow.exe'
    & $deployTool --release --force --compiler-runtime --include-plugins qoffscreen --dir $buildPath $executable
    if ($LASTEXITCODE -ne 0) { throw "Qt deployment failed: $LASTEXITCODE" }
    $opencvDlls = @(Get-ChildItem -LiteralPath (Join-Path $OpenCVRoot 'bin') -Filter 'libopencv_*.dll')
    if ($opencvDlls.Count -eq 0) { throw 'No MinGW OpenCV runtime DLLs found.' }
    foreach ($dll in $opencvDlls) { Copy-Item -LiteralPath $dll.FullName -Destination $buildPath -Force }
    # Resolve transitive PE imports, including those of Qt plugins and FFmpeg.
    # Do not rely on the invoking shell's PATH to declare a dependency available.
    $searchDirectories = @($CompilerBin, (Join-Path $OpenCVRoot 'bin'), (Join-Path $QtRoot 'bin'))
    $queue = [Collections.Generic.Queue[string]]::new()
    # Test/probe executables share this development build folder (e.g. Qt6Test).
    foreach ($exe in Get-ChildItem -LiteralPath $buildPath -Filter '*.exe') { $queue.Enqueue($exe.FullName) }
    foreach ($dll in Get-ChildItem -LiteralPath $buildPath -Filter '*.dll' -Recurse) { $queue.Enqueue($dll.FullName) }
    $seen = @{}
    $imports = [Collections.Generic.List[object]]::new()
    while ($queue.Count -gt 0) {
        $binary = $queue.Dequeue()
        if ($seen.ContainsKey($binary)) { continue }
        $seen[$binary] = $true
        $headers = & $objdump -p $binary
        if ($LASTEXITCODE -ne 0) { throw "Cannot inspect PE imports: $binary" }
        foreach ($line in $headers) {
            if ($line -notmatch 'DLL Name:\s*(\S+)') { continue }
            $name = $Matches[1]
            $local = Join-Path $buildPath $name
            $resolved = $null
            if (Test-Path -LiteralPath $local) { $resolved = $local }
            else {
                foreach ($directory in $searchDirectories) {
                    $candidate = Join-Path $directory $name
                    if (Test-Path -LiteralPath $candidate) {
                        Copy-Item -LiteralPath $candidate -Destination $local -Force
                        $resolved = $local
                        break
                    }
                }
            }
            if ($resolved) { $queue.Enqueue($resolved) }
            elseif ($name -match '^(api-ms-|ext-ms-)' -or (Test-Path -LiteralPath (Join-Path "$env:SystemRoot/System32" $name))) {
                $resolved = 'Windows'
            } else { throw "Unresolved dependency '$name' imported by '$binary'" }
            $imports.Add([pscustomobject]@{ Binary = $binary.Substring($buildPath.Length + 1); Import = $name; Resolved = $resolved })
        }
    }
    # Keep plugin lookup relative to the executable, independent of the Qt SDK.
    Set-Content -LiteralPath (Join-Path $buildPath 'qt.conf') -Value "[Paths]`nPrefix=.`nPlugins=." -Encoding ascii
    $files = @($executable) + @(Get-ChildItem -LiteralPath $buildPath -Filter '*.dll' -Recurse | ForEach-Object FullName)
    $manifest = [ordered]@{
        CreatedUtc = [DateTime]::UtcNow.ToString('o')
        Executable = $executable
        Files = @($files | ForEach-Object { [pscustomobject]@{ Path = $_.Substring($buildPath.Length + 1); SHA256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash } })
        Imports = $imports.ToArray()
    }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $buildPath 'local-deployment.json') -Encoding utf8
    Write-Host "Ready for direct launch: $executable"
    Write-Host "Verified PE dependency closure for $($seen.Count) binaries."
} finally { $env:PATH = $previousPath }
