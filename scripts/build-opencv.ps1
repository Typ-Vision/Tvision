param(
    [string]$Source = 'C:/OpenCV/sources',
    [string]$InstallPrefix = 'C:/OpenCV/5.0.0',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [string]$CMakeBin = 'C:/Qt/Tools/CMake_64/bin',
    [string]$NinjaBin = 'C:/Qt/Tools/Ninja',
    [ValidateRange(1, 64)][int]$Jobs = 6,
    [switch]$Install
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot 'build/opencv-mingw'
$previousPath = $env:PATH
function Invoke-Checked([string[]]$Arguments, [string]$LogName) {
    & "$CMakeBin/cmake.exe" @Arguments 2>&1 | Tee-Object (Join-Path $buildDirectory $LogName)
    if ($LASTEXITCODE -ne 0) { throw "CMake failed ($LogName): $LASTEXITCODE" }
}
try {
    foreach ($required in @("$Source/CMakeLists.txt", "$CompilerBin/g++.exe",
                            "$CompilerBin/gcc.exe", "$CMakeBin/cmake.exe", "$NinjaBin/ninja.exe")) {
        if (!(Test-Path -LiteralPath $required)) { throw "Required file missing: $required" }
    }
    $versionHeader = [IO.File]::ReadAllText("$Source/modules/core/include/opencv2/core/version.hpp")
    foreach ($version in @(@('MAJOR', '5'), @('MINOR', '0'), @('REVISION', '0'))) {
        if ($versionHeader -notmatch "(?m)^#define CV_VERSION_$($version[0])\s+$($version[1])\s*$") {
            throw 'This build configuration requires OpenCV 5.0.0 sources.'
        }
    }
    $env:PATH = "$CompilerBin;$NinjaBin;$CMakeBin;" + $previousPath
    New-Item -ItemType Directory -Force $buildDirectory | Out-Null
    Invoke-Checked @('-S', $Source, '-B', $buildDirectory, '-G', 'Ninja',
        '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_C_COMPILER=$CompilerBin/gcc.exe",
        "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe", "-DCMAKE_INSTALL_PREFIX=$InstallPrefix",
        '-DBUILD_LIST=core,imgproc,imgcodecs,objdetect,calib,geometry,features',
        '-DBUILD_SHARED_LIBS=ON', '-DBUILD_TESTS=OFF', '-DBUILD_PERF_TESTS=OFF',
        '-DBUILD_EXAMPLES=OFF', '-DBUILD_opencv_apps=OFF', '-DBUILD_JAVA=OFF',
        '-DBUILD_opencv_python3=OFF', '-DWITH_IPP=OFF', '-DWITH_ITT=OFF',
        '-DWITH_CUDA=OFF', '-DWITH_OPENCL=OFF', '-DWITH_FFMPEG=OFF',
        '-DWITH_GSTREAMER=OFF', '-DWITH_QT=OFF', '-DWITH_OPENEXR=OFF',
        '-DWITH_OPENJPEG=OFF', '-DWITH_AVIF=OFF',
        "-DOPENCV_DOWNLOAD_PATH=$projectRoot/build/opencv-downloads") 'configure.log'
    Invoke-Checked @('--build', $buildDirectory, '-j', "$Jobs") 'build.log'
    if ($Install) {
        Invoke-Checked @('--install', $buildDirectory) 'install.log'
    } else {
        Write-Output "Build complete; install separately: cmake --install $buildDirectory"
    }
} finally { $env:PATH = $previousPath }
