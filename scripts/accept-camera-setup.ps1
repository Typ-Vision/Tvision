param([string]$EvidenceDirectory = ('build/ccux/ccux-14/run-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')))
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
Push-Location $repo
$savedArtifacts = $env:VISIONFLOW_TEST_ARTIFACTS
$savedTemp = $env:TEMP
$savedTmp = $env:TMP
try {
    $evidence = [IO.Path]::GetFullPath((Join-Path $repo $EvidenceDirectory))
    New-Item -ItemType Directory -Force $evidence | Out-Null
    # This is the CCUX-01..14 union, not the entire product suite.
    $core = @(
        'camera_setup_acceptance', 'measurement_setup_contract',
        'measurement_environment_persistence', 'measurement_setup_migration',
        'industrial_discovery_contract', 'camera_transport', 'industrial_configuration',
        'camera_settings', 'industrial_provider_contract', 'industrial_oecs_experimental',
        'camera_worker_lease_contract', 'camera_worker_stress', 'scoped_active_calibration',
        'industrial_scoped_calibration', 'scoped_calibration_editor', 'scoped_calibration_runtime',
        'system_calibration_contract', 'industrial_runtime_admission_contract',
        'independent_calibration_capture', 'industrial_calibration_capture',
        'calibration_wizard_contract', 'calibration_plate_contract', 'active_calibration_contract',
        'calibration_save_use', 'industrial_source_tool_contract', 'multi_source_target_only',
        'project_persistence', 'production_recipe_contract', 'multi_source_production_contract',
        'multi_source_terminal_trace', 'metrology_runtime_contract', 'runtime_persistence',
        'experimental_metric_runtime_contract'
    )
    $ui = @('ui_camera_manager', 'ui_calibration_overview', 'ui_independent_calibration_wizard',
        'ui_scoped_calibration_activation', 'ui_industrial_calibration_activation',
        'ui_multi_source_editor', 'ui_multi_source_runtime', 'experimental_metric_ui_fixture',
        'ui_recipe_fields', 'ui_shell_smoke')
    $summary = @()
    foreach ($configuration in @(@{Name='ui'; Build='build/reference-ui'; Tests=$core+$ui},
                                 @{Name='core-off'; Build='build/kmc9-core-off'; Tests=$core})) {
        $output = Join-Path $evidence $configuration.Name
        New-Item -ItemType Directory -Force $output | Out-Null
        $env:VISIONFLOW_TEST_ARTIFACTS = $output.Replace('\','/')
        $temporary = Join-Path $output 'tmp'
        New-Item -ItemType Directory -Force $temporary | Out-Null
        $env:TEMP=$temporary; $env:TMP=$temporary
        $cache = Get-Content (Join-Path $configuration.Build 'CMakeCache.txt') -Raw
        if ($cache -notmatch '(?m)^CMAKE_BUILD_TYPE:STRING=Release\r?$') { throw 'Release build required' }
        & cmake --build $configuration.Build -j 6 *> (Join-Path $output 'build.log')
        $buildExit = $LASTEXITCODE
        if ($buildExit -ne 0) { throw "Build failed ($buildExit): $output/build.log" }
        # The shell fixture resolves its local test producer and child simulator here.
        if ($configuration.Name -eq 'ui') {
            Copy-Item -LiteralPath (Join-Path $configuration.Build 'gentl_test_producer.cti') -Destination $output
            Copy-Item -LiteralPath (Join-Path $configuration.Build 'visionflow_plc_simulator.exe') -Destination $output
            $fixtureDestination = Join-Path $evidence 'deps'
            New-Item -ItemType Directory -Force $fixtureDestination | Out-Null
            Copy-Item -LiteralPath (Join-Path $repo 'build/deps/code-fixtures') -Destination $fixtureDestination -Recurse -Force
        }
        $regex = '^(' + ($configuration.Tests -join '|') + ')$'
        & ctest --test-dir $configuration.Build -N -R $regex *> (Join-Path $output 'inventory.log')
        if ($LASTEXITCODE -ne 0) { throw 'CTest inventory failed' }
        $inventory = & ctest --test-dir $configuration.Build --show-only=json-v1 -R $regex | ConvertFrom-Json
        $names = @($inventory.tests | ForEach-Object { $_.name })
        $missing = @($configuration.Tests | Where-Object { $_ -notin $names })
        if ($missing.Count -or $names.Count -ne $configuration.Tests.Count) { throw "Missing tests: $missing" }
        Write-Host "$($configuration.Name): Release built; running $($names.Count) selected tests"
        & ctest --test-dir $configuration.Build -R $regex --output-on-failure --timeout 180 --output-junit (Join-Path $output 'tests.xml') *> (Join-Path $output 'tests.log')
        $testExit = $LASTEXITCODE
        Copy-Item -LiteralPath (Join-Path $configuration.Build 'Testing/Temporary/LastTest.log') -Destination (Join-Path $output 'LastTest.log')
        $summary += [ordered]@{configuration=$configuration.Name; build=$configuration.Build; buildExit=$buildExit; testExit=$testExit; count=$names.Count; tests=$names; regex=$regex}
        $summary | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $evidence 'run-summary.json')
        Write-Host "$($configuration.Name): tests exit $testExit ($output/tests.log)"
    }
    if (@($summary | Where-Object {$_.testExit -ne 0}).Count) { exit 1 }
} finally {
    $env:VISIONFLOW_TEST_ARTIFACTS = $savedArtifacts
    $env:TEMP = $savedTemp; $env:TMP = $savedTmp
    Pop-Location
}
