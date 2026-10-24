param(
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$ProbePath = 'C:/Projects/Tvision/build/reference-ui/oecs_experimental_probe.exe',
    [string]$RuntimeDll = 'C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvCameraControl.dll',
    [string]$ProducerPath = 'C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64/MvProducerU3V.cti',
    [double]$ExposureUs = 56000
)
$ErrorActionPreference = 'Stop'
if (-not [IO.Path]::IsPathFullyQualified($OutputDirectory)) { throw 'Evidence directory must be absolute so JSONL timing is enabled.' }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Choose a new evidence directory.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$timingBefore = $env:VISIONFLOW_ACQUISITION_TIMING
$discoveryBefore = $env:VISIONFLOW_TIMING_PROBE_PRODUCER
try {
    $env:VISIONFLOW_TIMING_PROBE_PRODUCER = $ProducerPath
    $environment = [ordered]@{
        dateUtc = [DateTime]::UtcNow.ToString('o')
        operatingSystem = [Environment]::OSVersion.VersionString
        processorCount = [Environment]::ProcessorCount
        powershellVersion = $PSVersionTable.PSVersion.ToString()
        probePath = $ProbePath
        probeSha256 = (Get-FileHash -LiteralPath $ProbePath -Algorithm SHA256).Hash
        runtimeDll = $RuntimeDll
        runtimeSha256 = (Get-FileHash -LiteralPath $RuntimeDll -Algorithm SHA256).Hash
        producerPath = $ProducerPath
        producerSha256 = (Get-FileHash -LiteralPath $ProducerPath -Algorithm SHA256).Hash
        exposureUs = $ExposureUs
        countPerRun = 20
        freshProcessesPerFlowAndMode = 3
        scope = 'Application service paths including real GenTL discovery and exact SDK enumeration; UI click/paint excluded'
    }
    $environment | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'environment.json') -Encoding utf8
    foreach ($round in 1..3) {
        foreach ($enabled in @($false,$true)) {
            foreach ($flow in @('editor','runtime','calibration')) {
                $label = "$flow-$round-$enabled"
                $evidence = Join-Path $OutputDirectory $label
                if ($enabled) { $env:VISIONFLOW_ACQUISITION_TIMING = Join-Path $OutputDirectory "$label-trace.jsonl" }
                else { Remove-Item Env:VISIONFLOW_ACQUISITION_TIMING -ErrorAction SilentlyContinue }
                & $ProbePath $RuntimeDll $evidence $ExposureUs 20 "--$flow-run" > (Join-Path $OutputDirectory "$label-report.json")
                $exitCode = $LASTEXITCODE
                [ordered]@{ label=$label; exitCode=$exitCode } | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $OutputDirectory 'runs.jsonl') -Encoding utf8
                if ($exitCode -ne 0) { throw "Baseline failed: $label (exit $exitCode). Failed sample is not successful timing evidence." }
            }
        }
    }
} finally {
    if ($null -eq $timingBefore) { Remove-Item Env:VISIONFLOW_ACQUISITION_TIMING -ErrorAction SilentlyContinue }
    else { $env:VISIONFLOW_ACQUISITION_TIMING = $timingBefore }
    if ($null -eq $discoveryBefore) { Remove-Item Env:VISIONFLOW_TIMING_PROBE_PRODUCER -ErrorAction SilentlyContinue }
    else { $env:VISIONFLOW_TIMING_PROBE_PRODUCER = $discoveryBefore }
}
