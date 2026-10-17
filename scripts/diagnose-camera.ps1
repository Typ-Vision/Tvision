#requires -Version 7.0
<#
.SYNOPSIS
Records local SDK/PE/dependency metadata and an explicit GenTL probe run.
.DESCRIPTION
Does not install drivers, reboot, grab frames, or change persistent environment.
Report status NO_DEVICES is a completed discovery with no camera evidence.
Dependency candidates are a static inventory, not proof of Windows loader resolution.
#>
param(
    [string]$ProducerPath = 'C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvProducerU3V.cti',
    [string]$ProbePath = 'build/reference-ui/visionflow_camera_probe.exe',
    [string]$OutputDirectory = 'build/icm/01a/local',
    [string]$SdkRoot = 'C:/Program Files (x86)/OECS',
    [string]$QtBin = 'C:/Qt/6.12.0/mingw_64/bin',
    [string]$CompilerBin = 'C:/Qt/Tools/mingw1310_64/bin',
    [ValidateRange(1,60000)][int]$TimeoutMs = 5000,
    [ValidateRange(1,60000)][int]$ScanWaitMs = 250,
    [ValidateRange(100,120000)][int]$ProcessTimeoutMs = 15000,
    [switch]$OpenCheck,
    [ValidateSet('local-sdk-probe','synthetic-fixture')][string]$EvidenceKind = 'local-sdk-probe'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
function AbsolutePath([string]$Path) {
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
}
function FileMetadata([string]$Path) {
    $result = [ordered]@{ path=$Path; exists=(Test-Path -LiteralPath $Path -PathType Leaf) }
    if ($result.exists) {
        $file = Get-Item -LiteralPath $Path
        $result.length = $file.Length
        $result.sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
        $result.fileVersion = $file.VersionInfo.FileVersion
        $result.productVersion = $file.VersionInfo.ProductVersion
        $stream = [IO.File]::OpenRead($Path)
        $reader = [IO.BinaryReader]::new($stream)
        try {
            $result.machine = 'INVALID_PE'
            if ($stream.Length -ge 64 -and $reader.ReadUInt16() -eq 0x5a4d) {
                $stream.Position = 0x3c
                $offset = $reader.ReadUInt32()
                if ($offset -le ($stream.Length - 6)) {
                    $stream.Position = $offset
                    if ($reader.ReadUInt32() -eq 0x4550) {
                        $machine = $reader.ReadUInt16()
                        $result.machine = switch ($machine) { 0x8664 {'x64'} 0x14c {'x86'} default {'0x{0:x4}' -f $machine} }
                    }
                }
            }
        } finally { $reader.Dispose() }
    }
    return $result
}
$ProducerPath = AbsolutePath $ProducerPath
$ProbePath = AbsolutePath $ProbePath
$OutputDirectory = AbsolutePath $OutputDirectory
$null = New-Item -ItemType Directory -Force -Path $OutputDirectory
# Reused output folders must not retain raw output from an earlier probe run.
foreach ($name in @('probe.stdout.json','probe.stderr.txt','producer-objdump.txt')) {
    Set-Content -LiteralPath (Join-Path $OutputDirectory $name) -Value '' -Encoding utf8
}
$producer = FileMetadata $ProducerPath
$probe = FileMetadata $ProbePath
$childPath = "$(Split-Path -Parent $ProducerPath);$QtBin;$CompilerBin;" + $env:PATH
$environment = foreach ($name in @('GENICAM_GENTL64_PATH','GENICAM_GENTL32_PATH','PATH')) {
    $processValue = [Environment]::GetEnvironmentVariable($name,'Process')
    $machineValue = [Environment]::GetEnvironmentVariable($name,'Machine')
    [ordered]@{ name=$name; process=$processValue; machine=$machineValue; differs=($processValue -cne $machineValue) }
}
$sdkFiles = foreach ($relative in @('Applications/Win64/OECS.exe','Development/Includes/MvCameraControl.h','Development/Libraries/win64/MvCameraControl.lib')) {
    FileMetadata (Join-Path $SdkRoot $relative)
}
$arguments = @('--producer',$ProducerPath,'--timeout',"$TimeoutMs",'--scan-wait',"$ScanWaitMs")
if ($OpenCheck) { $arguments += '--open-check' }
$report = [ordered]@{
    schema='visionflow.camera-diagnostic1'; utc=[DateTime]::UtcNow.ToString('o')
    evidenceKind=$EvidenceKind; producer=$producer; probeBinary=$probe
    sdkRoot=$SdkRoot; sdkFiles=@($sdkFiles); environment=@($environment)
    childPath=$childPath; command=@($ProbePath)+$arguments; processTimeoutMs=$ProcessTimeoutMs
    status='NOT_RUN'; exitCode=$null; probe=$null; error=$null
    openCheckRequested=[bool]$OpenCheck; openCheckAttemptedCount=0
    acquisitionImplemented=$false; physicalAcceptance=$false
    sdkRuntime=(FileMetadata (Join-Path (Split-Path -Parent $ProducerPath) 'MvCameraControl.dll'))
    dependencies=@(); dependencyInspection='unavailable'
}
# Inspect only direct imports. API sets and delay/dynamic imports need loader/runtime evidence.
$objdump = Join-Path $CompilerBin 'objdump.exe'
if ($producer.exists -and (Test-Path -LiteralPath $objdump)) {
    $dump = & $objdump -p $ProducerPath 2>&1
    $dumpExit = $LASTEXITCODE
    $dump | Set-Content -LiteralPath (Join-Path $OutputDirectory 'producer-objdump.txt') -Encoding utf8
    $report.dependencyInspection = if ($dumpExit -eq 0) { 'direct-imports-only' } else { 'objdump-error' }
    $report.dependencies = @(foreach ($line in $dump) {
        if ("$line" -match 'DLL Name:\s*(\S+)') {
            $name = $Matches[1]
            $candidates = @(foreach ($directory in @((Split-Path -Parent $ProbePath),"$env:SystemRoot/System32") + $childPath.Split(';')) {
                if ($directory) {
                    $candidate = Join-Path $directory $name
                    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $candidate }
                }
            }) | Select-Object -Unique
            [ordered]@{ name=$name; candidates=@($candidates); loaderResolutionVerified=$false }
        }
    })
}
if (-not $producer.exists) { $report.status='MISSING_PRODUCER' }
elseif ($producer.machine -eq 'x86') { $report.status='WRONG_PRODUCER_ARCHITECTURE' }
elseif ($producer.machine -ne 'x64') { $report.status='INVALID_PRODUCER_BINARY' }
elseif (-not $probe.exists) { $report.status='MISSING_PROBE' }
elseif ($probe.machine -ne 'x64') { $report.status='INVALID_PROBE_BINARY' }
else {
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = [Diagnostics.ProcessStartInfo]::new($ProbePath)
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $process.StartInfo.Environment['PATH'] = $childPath
    foreach ($argument in $arguments) { $process.StartInfo.ArgumentList.Add($argument) }
    try {
        $null = $process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($ProcessTimeoutMs)) {
            $process.Kill($true)
            $process.WaitForExit()
            $report.status='PROCESS_TIMEOUT'
        }
        $report.exitCode = $process.ExitCode
        $out = $stdout.GetAwaiter().GetResult()
        $err = $stderr.GetAwaiter().GetResult()
        $out | Set-Content -LiteralPath (Join-Path $OutputDirectory 'probe.stdout.json') -Encoding utf8
        $err | Set-Content -LiteralPath (Join-Path $OutputDirectory 'probe.stderr.txt') -Encoding utf8
        if ($report.status -ne 'PROCESS_TIMEOUT') {
            try {
                $result = ConvertFrom-Json -InputObject $out -ErrorAction Stop
                if ($null -eq $result -or $result.status -notin @('DONE','ERROR','NO_PRODUCER') -or
                    $null -eq $result.devices -or $null -eq $result.errors) { throw 'Invalid probe schema' }
                $report.probe = $result
                $report.openCheckAttemptedCount = @($result.devices | Where-Object { $_.readOnlyOpenClose -in @('ok','error') }).Count
                $report.status = if ($report.exitCode -ne 0 -or $result.status -ne 'DONE' -or @($result.errors).Count -gt 0) { 'PROBE_ERROR' }
                    elseif (@($result.devices).Count -eq 0) { 'NO_DEVICES' } else { 'DEVICES_DISCOVERED' }
            } catch { $report.status='INVALID_PROBE_OUTPUT'; $report.error=$_.Exception.Message }
        }
    } catch { $report.status='PROCESS_ERROR'; $report.error=$_.Exception.Message }
    finally { $process.Dispose() }
}
$report | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'diagnostic.json') -Encoding utf8
Write-Output "$($report.status): $(Join-Path $OutputDirectory 'diagnostic.json')"
if ($report.status -in @('NO_DEVICES','DEVICES_DISCOVERED')) { exit 0 }
exit 1
