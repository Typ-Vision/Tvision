param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$evidence = Join-Path $root 'build/ccux/ccux-24i'
New-Item -ItemType Directory -Force "$evidence/tmp", "$evidence/suite" | Out-Null
$env:PATH = 'C:\OpenCV\5.0.0\x64\mingw\bin;C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;' + $env:PATH
$env:TEMP = "$evidence/tmp"
$env:TMP = $env:TEMP
$env:VISIONFLOW_TEST_ARTIFACTS = "$evidence/suite"
$shared = @(
    'output_visibility_policy', 'output_visibility_preferences', 'active_calibration_contract',
    'calibration_save_use', 'scoped_active_calibration', 'calibrated_circle_contract',
    'experimental_metric_runtime_contract', 'geometry_measurement_service_contract',
    'measurement_setup_contract', 'measurement_setup_migration', 'framed_calibration_plate_contract',
    'calibration_camera_sessions_contract', 'camera_acquisition_sessions_contract',
    'camera_settings', 'oecs_camera_settings', 'reference_world_contract',
    'world_tracking_dag_contract', 'world_tracking_contract', 'tool_creation_policy'
)
$ui = @(
    'ui_editor_usability', 'ui_shell_smoke', 'ui_input_precision', 'ui_geometry_smoke',
    'ui_roi_tool', 'ui_pose_tracking_smoke', 'ui_circle_smoke', 'ui_calibration_wizard',
    'ui_calibration_overview', 'ui_scoped_calibration_activation', 'ui_camera_manager',
    'roi_viewer_interaction', 'image_refresh_contract', 'ui_runtime_overlays',
    'ui_runtime_visibility', 'ui_runtime_failures', 'ui_multi_source_editor',
    'ui_multi_source_runtime', 'ui_restart_runtime'
)
function Invoke-Recorded([string]$Name, [string[]]$Arguments) {
    $program = if ($Name -like 'ctest*') { 'ctest' } else { 'cmake' }
    ($program + ' ' + ($Arguments -join ' ')) | Set-Content "$evidence/$Name.command.txt"
    & $program @Arguments *> "$evidence/$Name.log"
    $code = $LASTEXITCODE
    "$Name exit=$code" | Tee-Object -FilePath "$evidence/results.txt" -Append
    if ($code -ne 0) { throw "$Name failed; inspect $evidence/$Name.log" }
}
('Started ' + (Get-Date -Format o)) | Set-Content "$evidence/results.txt"
git rev-parse HEAD | Set-Content "$evidence/base-commit.txt"
git diff --name-only 989e645..HEAD | Set-Content "$evidence/package-files.txt"
foreach ($variant in @('ui', 'off')) {
    $build = if ($variant -eq 'ui') { 'build/reference-ui' } else { 'build/kmc9-core-off' }
    $names = if ($variant -eq 'ui') { @($shared + $ui) } else { $shared }
    $regex = '^(' + ($names -join '|') + ')$'
    $names | Set-Content "$evidence/tests-$variant.txt"
    if (!$SkipBuild) {
        $enabled = if ($variant -eq 'ui') { 'ON' } else { 'OFF' }
        Invoke-Recorded "configure-$variant" @('-S', '.', '-B', $build, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64', '-DOpenCV_DIR=C:/OpenCV/5.0.0/x64/mingw/lib', "-DVISIONFLOW_BUILD_UI=$enabled", "-DVISIONFLOW_WITH_ZXING=$enabled", "-DVISIONFLOW_ZXING_PREFIX=$root/build/deps/zxing-prefix")
        Invoke-Recorded "build-$variant" @('--build', $build, '-j', '6')
    }
    Invoke-Recorded "ctest-$variant-list" @('--test-dir', $build, '-N')
    $listing = Get-Content "$evidence/ctest-$variant-list.log" -Raw
    foreach ($name in $names) {
        if ($listing -notmatch ('(?m)Test\s+#\d+:\s+' + [regex]::Escape($name) + '\r?$')) { throw "Missing registered test: $name in $build" }
    }
    Invoke-Recorded "ctest-$variant" @('--test-dir', $build, '-R', $regex, '--output-on-failure')
    Copy-Item "$build/Testing/Temporary/LastTest.log" "$evidence/last-test-$variant.log"
    if ($variant -eq 'ui') {
        # This test deliberately overrides VISIONFLOW_TEST_ARTIFACTS in CMake.
        Copy-Item "$build/int06-restart" "$evidence/restart" -Recurse -Force
    }
}
('Finished ' + (Get-Date -Format o)) | Add-Content "$evidence/results.txt"
