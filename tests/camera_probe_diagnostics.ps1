#requires -Version 7.0
param([Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$script = Join-Path $projectRoot 'scripts/diagnose-camera.ps1'
$output = Join-Path $BuildDirectory 'icm/01a/fixtures'
$null = New-Item -ItemType Directory -Force -Path $output
$parseErrors = $null; $tokens = $null
$null = [Management.Automation.Language.Parser]::ParseFile($script,[ref]$tokens,[ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
$probe = Join-Path $BuildDirectory 'visionflow_camera_probe.exe'
$valid = Join-Path $BuildDirectory 'gentl_test_producer.cti'
$invalid = Join-Path $output 'invalid.cti'
[IO.File]::WriteAllText($invalid,'not a binary')
$x86 = Join-Path $output 'x86.cti'
$bytes = [IO.File]::ReadAllBytes($valid)
$offset = [BitConverter]::ToInt32($bytes,0x3c)
$bytes[$offset+4] = 0x4c; $bytes[$offset+5] = 0x01
[IO.File]::WriteAllBytes($x86,$bytes)
$spacePath = Join-Path $output 'path with spaces'
$null = New-Item -ItemType Directory -Force -Path $spacePath
$spaceProducer = Join-Path $spacePath 'fixture producer.cti'
Copy-Item -LiteralPath $valid -Destination $spaceProducer -Force
$cases = @(
    @('missing',(Join-Path $output 'missing.cti'),'MISSING_PRODUCER',$probe,0),
    @('invalid',$invalid,'INVALID_PRODUCER_BINARY',$probe,0),
    @('x86',$x86,'WRONG_PRODUCER_ARCHITECTURE',$probe,0),
    @('missing-probe',$valid,'MISSING_PROBE',(Join-Path $output 'missing.exe'),0),
    @('invalid-probe',$valid,'INVALID_PROBE_BINARY',$invalid,0),
    @('incomplete',(Join-Path $BuildDirectory 'gentl_incomplete_producer.cti'),'PROBE_ERROR',$probe,0),
    @('empty',(Join-Path $BuildDirectory 'gentl_empty_producer.cti'),'NO_DEVICES',$probe,0),
    @('discovered',$spaceProducer,'DEVICES_DISCOVERED',$probe,1)
)
$environmentBefore = @($env:PATH,$env:GENICAM_GENTL64_PATH,$env:GENICAM_GENTL32_PATH) | ConvertTo-Json
$results = foreach ($case in $cases) {
    $directory = Join-Path $output $case[0]
    & (Join-Path $PSHOME 'pwsh.exe') -NoProfile -File $script -ProducerPath $case[1] -ProbePath $case[3] -OutputDirectory $directory -OpenCheck -EvidenceKind synthetic-fixture
    $code = $LASTEXITCODE
    $report = Get-Content -LiteralPath (Join-Path $directory 'diagnostic.json') -Raw | ConvertFrom-Json
    $expectedExit = if ($case[2] -in @('NO_DEVICES','DEVICES_DISCOVERED')) { 0 } else { 1 }
    if ($report.status -ne $case[2] -or $code -ne $expectedExit -or $report.openCheckAttemptedCount -ne $case[4]) {
        throw "Case $($case[0]): unexpected status/exit/open-check: $($report.status)/$code/$($report.openCheckAttemptedCount)"
    }
    if ($case[0] -eq 'empty' -and ($report.probe.status -ne 'DONE' -or @($report.probe.devices).Count -ne 0)) { throw 'Empty discovery oracle failed' }
    if ($case[0] -eq 'discovered' -and $report.probe.devices[0].readOnlyOpenClose -ne 'ok') { throw 'Fixture open/close failed' }
    [ordered]@{case=$case[0]; status=$report.status; exitCode=$code; openCheckAttemptedCount=$report.openCheckAttemptedCount}
}
$environmentAfter = @($env:PATH,$env:GENICAM_GENTL64_PATH,$env:GENICAM_GENTL32_PATH) | ConvertTo-Json
if ($environmentBefore -cne $environmentAfter) { throw 'Parent environment changed' }
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding utf8
Write-Output "PASS: $($cases.Count) diagnostic cases; synthetic evidence only"
