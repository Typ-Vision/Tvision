"""Opt-in local SDK experiment; never installs drivers or changes persistent PATH.

Build tests/industrial-camera-probe first. Reports remain under build/icm/02.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from datetime import datetime, timezone


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, default=Path('build/icm/02/abi/industrial_backend_probe.exe'))
    parser.add_argument('--runtime', type=Path, default=Path('C:/Program Files (x86)/Common Files/OECS/Runtime/Win64_x64'))
    parser.add_argument('--sdk', type=Path, default=Path('C:/Program Files (x86)/OECS'))
    parser.add_argument('--output', type=Path, default=Path('build/icm/02'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env['PATH'] = ';'.join([str(args.runtime), 'C:/Qt/6.12.0/mingw_64/bin',
                            'C:/Qt/Tools/mingw1310_64/bin', env.get('PATH', '')])
    records = []

    def run(name, mode, library):
        command = [str(args.probe.resolve()), mode, str(library.resolve())]
        result = subprocess.run(command, env=env, capture_output=True, timeout=20)
        (args.output / f'{name}.stdout.json').write_bytes(result.stdout)
        (args.output / f'{name}.stderr.txt').write_bytes(result.stderr)
        report = json.loads(result.stdout.decode('utf-8-sig'))
        if report['exitCode'] != result.returncode:
            raise RuntimeError(f'{name}: JSON/process exit mismatch')
        records.append(dict(name=name, command=command, exitCode=result.returncode,
                            status=report['status']))
        return report

    run('gentl', '--gentl', args.runtime / 'MvProducerU3V.cti')
    run('oecs', '--oecs', args.runtime / 'MvCameraControl.dll')
    # Negative checks must fail cleanly with structured diagnostics, never crash.
    absent = run('missing-runtime', '--oecs', args.output / 'absent-runtime.dll')
    assert absent['status'] == 'UNAVAILABLE' and absent['exitCode'] == 1
    wrong = run('wrong-api', '--oecs', args.runtime / 'MvProducerU3V.cti')
    assert wrong['status'] == 'ERROR' and 'missing export' in wrong['error'] and wrong['exitCode'] == 1
    files = [args.probe, args.runtime / 'MvProducerU3V.cti', args.runtime / 'MvCameraControl.dll']
    files += sorted((args.sdk / 'Development/Includes').glob('*.h'))
    files += sorted((args.sdk / 'License').rglob('*License*'))
    files += sorted((args.runtime.parent.parent / 'Licenses').rglob('*License*'))
    metadata = []
    for path in files:
        if path.is_file():
            metadata.append(dict(path=str(path.resolve()), bytes=path.stat().st_size,
                                 sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    summary = dict(utc=datetime.now(timezone.utc).isoformat(), runs=records, files=metadata,
                   negativeChecksPassed=2, physicalAcceptance=False,
                   licenseRedistributionApproved=False)
    (args.output / 'experiment.json').write_text(json.dumps(summary, indent=2), encoding='utf-8')
    print(json.dumps(records, indent=2))


if __name__ == '__main__':
    main()
