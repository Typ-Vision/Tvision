#!/usr/bin/env python3
"""COMM4-25 independent TCP/robot fixture acceptance; emits incremental JSON/JSONL evidence."""
import argparse
import contextlib
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import time
import uuid

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from communication_v4_simulator import Peer, CONTRACT, ZERO, decode, frame, identity, put32, u32


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def eventually(predicate, seconds=8):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.01)
    raise TimeoutError('fixture condition deadline')


def read_json(path):
    # QSaveFile atomically replaces the local fixture counters. Windows can
    # briefly deny open during replacement; no observation is invented.
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (PermissionError, FileNotFoundError):
        return None


class Acceptance:
    def __init__(self, output):
        self.output = output
        self.rows = []

    @contextlib.contextmanager
    def case(self, name, layer='independent TCP E2E'):
        row = {'name': name, 'layer': layer, 'result': 'running', 'start': time.time()}
        self.rows.append(row)
        self.save()
        try:
            yield row
            row['result'] = 'passed'
        except BaseException as error:
            row.update(result='failed', error=str(error))
            raise
        finally:
            row['end'] = time.time()
            self.save()

    def save(self):
        data = {'schema': 1, 'physical': 'BLOCKED', 'exactlyOnce': False,
                'cases': self.rows, 'passed': sum(r['result'] == 'passed' for r in self.rows),
                'failed': sum(r['result'] == 'failed' for r in self.rows)}
        temporary = self.output / 'result.tmp'
        temporary.write_text(json.dumps(data, indent=2), encoding='utf-8')
        temporary.replace(self.output / 'result.json')


@contextlib.contextmanager
def server(executable, root):
    root.mkdir(parents=True)
    with (root / 'server.log').open('w', encoding='utf-8') as log:
        process = subprocess.Popen([str(executable), str(root)], stdout=log, stderr=subprocess.STDOUT)
        try:
            def ready():
                if process.poll() is not None:
                    raise RuntimeError(f'fixture exited {process.returncode}; see {root / "server.log"}')
                path = root / 'ready.json'
                return json.loads(path.read_text(encoding='utf-8')) if path.exists() else None
            yield eventually(ready, 20)
        finally:
            (root / 'quit').touch()
            try:
                code = process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
                raise RuntimeError('fixture failed bounded shutdown')
            if code:
                raise RuntimeError(f'fixture exited {code}')


def catalogue(peer, row):
    body = bytearray(384)
    put32(body, 0, 1)
    put32(body, 4, row['station'])
    raw = peer.make(3, body=body)
    peer.send(raw)
    reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {103})
    row.update(recipe=u32(reply['body'], 220), template=u32(reply['body'], 224),
               activation=reply['body'][232:248], camera_set=reply['body'][248:264])
    return reply


def request(peer, row, kind, body=None, request_id=None):
    return peer.make(kind, request=request_id, station=row['station'], recipe=row['recipe'],
                     template=row['template'], activation=row['activation'], body=body)


def command(peer, row, kind, body=None):
    raw = request(peer, row, kind, body)
    peer.send(raw)
    sequence = int.from_bytes(raw[24:32], 'big')
    peer.wait(sequence, {110})
    terminal = peer.wait(sequence, {111})
    require(terminal['reason'] == 0, f'control command {kind} failed')
    if kind == 4:
        row.update(recipe=u32(terminal['raw'], 80), template=u32(terminal['raw'], 84),
                   activation=terminal['raw'][88:104])
    peer.ack(terminal)
    return terminal


def enable(peer, row):
    body = bytearray(384)
    put32(body, 0, 1)
    terminal = command(peer, row, 5, body)
    require(u32(terminal['body'], 756) == 1, 'SET_AUTO terminal must be READY')


def trigger(peer, row):
    body = bytearray(384)
    body[:16] = row['camera_set']
    return request(peer, row, 10, body)


def production(args, acceptance):
    root = acceptance.output / 'production'
    with server(args.server, root) as manifest, (root / 'peer.jsonl').open('w', encoding='utf-8') as log:
        peer = Peer('127.0.0.1', manifest['port'], manifest['peer'], manifest['epoch'], log)
        try:
            peer.connect()
            transport = peer.session
            rows = manifest['stations']
            with acceptance.case('PLC_SELECT_RECIPE_SET_AUTO_READY_TRIGGER_ACK_four_stations') as result:
                for row in rows:
                    catalogue(peer, row)
                    body = bytearray(384)
                    put32(body, 0, row['recipe'])
                    put32(body, 4, row['template'])
                    command(peer, row, 4, body)
                    catalogue(peer, row)
                    enable(peer, row)
                result['stations'] = len(rows)
                require(len(rows) >= 4, 'fourth station without product constants')
            with acceptance.case('same_stream_three_admissions_fairness_pending_remote_STOP') as result:
                # Explicitly pause owner completion publication; all three real
                # Runtime jobs must be admitted before any terminal is published.
                (root / 'pause-poll').touch()
                triggers = [trigger(peer, row) for row in rows[:3]]
                peer.send(b''.join(triggers))
                ids = [int.from_bytes(raw[24:32], 'big') for raw in triggers]
                for sequence in ids:
                    peer.wait(sequence, {110})
                (root / 'pause-poll').unlink()
                terminals = [peer.wait(sequence, {111}) for sequence in ids]
                for index, terminal in enumerate(terminals):
                    require(terminal['station'] == rows[index]['station'], 'station correlation')
                    require(u32(terminal['body'], 72) == 1 and u32(terminal['body'], 80) == 0x8000,
                            'valid Number15')
                    require(struct.unpack_from('>i', terminal['body'], 212)[0] == 40 + index, 'recipe output isolation')
                    require(u32(terminal['body'], 84) == 0x80000000 and u32(terminal['body'], 88) == 0, 'false valid Flag31')
                    require(u32(terminal['body'], 68) == 0, 'no fabricated part decision')
                peer.ack(terminals[1])
                peer.ack(terminals[2])
                command(peer, rows[0], 6)
                status = peer.make(2, station=rows[0]['station'])
                peer.send(status)
                reply = peer.wait(int.from_bytes(status[24:32], 'big'), {102})
                require(u32(reply['body'], 764) == 1, 'STOP ACK must retain production pending')
                result.update(admitted=3, pendingAfterStop=1,
                              overlapEvidence='station_execution_contract controlled acquisition barriers; separate layer')
            with acceptance.case('consumer_crash_after_effect_before_ACK_duplicate_terminal_no_exactly_once') as result:
                external = root / 'consumer-effects.jsonl'
                peer.close()
                time.sleep(0.1)
                crash_config = root / 'crash-consumer.json'
                crash_config.write_text(json.dumps(dict(port=manifest['port'], peer=manifest['peer'], epoch=manifest['epoch'], request=str(ids[0]),
                    digest=terminals[0]['body'][32:64].hex(), effects=str(external))), encoding='utf-8')
                with (root / 'crash-consumer.log').open('w', encoding='utf-8') as crash_log:
                    crashed = subprocess.run([sys.executable, __file__, '--consumer-crash', str(crash_config)],
                                             stdout=crash_log, stderr=subprocess.STDOUT, timeout=12)
                require(crashed.returncode == 77, 'consumer must really terminate after fsynced effect, before ACK')
                require(len(external.read_text().splitlines()) == 1, 'crashed consumer committed one external effect')
                peer.event('consumer_process_crash_after_effect_before_ACK', exit=crashed.returncode)
                time.sleep(0.1)
                peer.connect()
                require(peer.session != transport, 'reconnect must use fresh session')
                replay = peer.wait(ids[0], {111})
                require(replay['body'] == terminals[0]['body'], 'sealed terminal must be byte-exact')
                with external.open('a') as effects:
                    effects.write(json.dumps({'effect': replay['body'][:16].hex()}) + '\n')
                peer.ack(replay)
                require(len(external.read_text().splitlines()) == 2, 'external effect duplicated without consumer dedup')
                result.update(terminalDigest=replay['body'][32:64].hex(), consumerEffects=2, exactlyOnce=False)
                transport = peer.session
            with acceptance.case('fourth_station_runtime_cycle'):
                raw = trigger(peer, rows[3]);peer.send(raw)
                terminal = peer.wait(int.from_bytes(raw[24:32], 'big'), {111})
                require(struct.unpack_from('>i', terminal['body'], 212)[0] == 43, 'fourth station output')
                peer.ack(terminal)
            with acceptance.case('recipe_A_B_A_no_socket_cut_stale_activation'):
                first = rows[0]['activation']
                body = bytearray(384);put32(body, 0, rows[1]['recipe']);put32(body, 4, rows[1]['template'])
                command(peer, rows[0], 4, body);catalogue(peer, rows[0]);enable(peer, rows[0])
                second = rows[0]['activation']
                raw = trigger(peer, rows[0]);peer.send(raw)
                terminal = peer.wait(int.from_bytes(raw[24:32], 'big'), {111})
                require(struct.unpack_from('>i', terminal['body'], 212)[0] == 41, 'B result on S1')
                peer.ack(terminal);command(peer, rows[0], 6)
                # Original A key is carried by the first production request.
                old_recipe = u32(triggers[0], 80)
                put32(body, 0, old_recipe)
                command(peer, rows[0], 4, body);catalogue(peer, rows[0])
                require(len({first, second, rows[0]['activation']}) == 3, 'A-B-A must not revive activation')
                stale = bytearray(trigger(peer, rows[0]));stale[88:104] = first
                peer.send(stale)
                reply = peer.wait(int.from_bytes(stale[24:32], 'big'), {190})
                require(reply['reason'] == 4, 'stale activation reason')
                require(peer.session == transport, 'A/B changed transport session')
            with acceptance.case('local_Stop_fences_pending_remote_Auto'):
                (root / 'pause-poll').touch()
                body = bytearray(384);put32(body, 0, 1)
                raw = request(peer, rows[0], 5, body);peer.send(raw)
                sequence = int.from_bytes(raw[24:32], 'big');peer.wait(sequence, {110})
                (root / 'local-stop').touch()
                eventually(lambda: (root / 'local-stop-done.json').exists())
                (root / 'pause-poll').unlink()
                terminal = peer.wait(sequence, {111})
                require(terminal['reason'] == 20 and not u32(terminal['body'], 756), 'late Auto must remain fenced')
                peer.ack(terminal)
            with acceptance.case('TCP_split_coalesced_slow_reader_uint64'):
                for split in (1, 31, 127, 128, 255, 511):
                    raw = peer.make(2, request=2**53 + split)
                    peer.send(raw, split=split, delay=0.002)
                    peer.wait(2**53 + split, {102})
                requests = [peer.make(2) for _ in range(4)]
                peer.send(b''.join(requests))
                time.sleep(0.15)
                for raw in requests:
                    peer.wait(int.from_bytes(raw[24:32], 'big'), {102})
            with acceptance.case('partial_trickle_deadline_disconnect_and_recovery'):
                raw = peer.make(2)
                peer.socket.sendall(raw[:1]);time.sleep(1.0)
                peer.socket.sendall(raw[1:2]);time.sleep(1.25)
                disconnected = False
                try:
                    while peer.socket.recv(1024):
                        pass
                    disconnected = True
                except (ConnectionResetError, ConnectionAbortedError):
                    disconnected = True
                require(disconnected, 'partial/trickle must not extend 2s deadline')
                peer.close();time.sleep(0.05);peer.connect()
            with acceptance.case('normal_composition_has_no_calibration_fixture_capability'):
                raw = peer.make(25, station=rows[0]['station'])
                peer.send(raw)
                reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {190})
                require(reply['reason'] == 20, 'normal composition cannot grant fixture calibration')
        finally:
            (root / 'pause-poll').unlink(missing_ok=True)
            peer.close()


def robot_body(manifest, session, mode):
    body = bytearray(384);body[:16] = session
    put32(body, 16, mode);put32(body, 20, manifest['camera']);body[24:40] = identity(manifest['cameraRevision'])
    for offset, key in ((40, 'intrinsic'), (44, 'plate'), (52, 'adapter')):
        put32(body, offset, manifest[key])
    for offset, key in ((112, 'target'), (128, 'targetRevision'), (144, 'source'), (160, 'sourceRevision')):
        body[offset:offset+16] = identity(manifest[key])
    put32(body, 176, 2)
    return body


def sample_body(manifest, session, sample, role):
    body = bytearray(384);body[:16] = session;body[16:32] = sample
    put32(body, 32, manifest['camera']);put32(body, 36, 1);body[40:56] = identity(manifest['cameraRevision'])
    struct.pack_into('>QQII7d', body, 56, 2**53+1, 123, 1, 1, 42., 3., -7., 1., 0., 0., 0.)
    for offset, key in ((136, 'target'), (152, 'targetRevision'), (168, 'source'), (184, 'sourceRevision')):
        body[offset:offset+16] = identity(manifest[key])
    put32(body, 200, 1);put32(body, 208, manifest['profile']);put32(body, 212, role)
    return body


def robot(args, acceptance):
    root = acceptance.output / 'robot'
    with server(args.robot_server, root) as manifest, (root / 'peer.jsonl').open('w', encoding='utf-8') as log:
        peer = Peer('127.0.0.1', manifest['port'], manifest['peer'], manifest['epoch'], log)
        try:
            peer.connect()
            for mode in (1, 2):
                with acceptance.case(f'robot_mode_{mode}_BEGIN_SAMPLE_100_duplicates_holdout_actual_pose_handshake') as result:
                    session = uuid.uuid4().bytes
                    raw = peer.make(20, station=manifest['station'], body=robot_body(manifest, session, mode))
                    peer.send(raw);peer.wait(int.from_bytes(raw[24:32], 'big'), {110})
                    for role in (1, 2):
                        sample = uuid.uuid4().bytes
                        body = sample_body(manifest, session, sample, role)
                        raw = peer.make(21, station=manifest['station'], body=body)
                        sequence = int.from_bytes(raw[24:32], 'big')
                        peer.send(raw)
                        accepted = peer.wait(sequence, {110})
                        require(u32(accepted['body'], 100) == 1, 'ACCEPTED is not robot release')
                        peer.event('robot_wait_stationary', mode=mode, role=role, request=str(sequence))
                        for _ in range(25):
                            peer.send(raw * 4)
                        terminal = peer.wait(sequence, {111})
                        require(terminal['reason'] == 0 and u32(terminal['body'], 100) == 6, 'CAPTURED release required')
                        require(terminal['body'][120:136] == sample and terminal['body'][136:152] != ZERO,
                                'exact sample/fresh frame identity')
                        require(u32(terminal['body'], 820) == manifest['camera'] and u32(terminal['body'], 824) == 21,
                                'exact camera/origin')
                        # Drain 100 explicit duplicate replies; every one is
                        # ACCEPTED or the same sealed terminal, never a new job.
                        for _ in range(100):
                            duplicate = peer.wait(sequence, {110, 111})
                            if duplicate['kind'] == 111:
                                require(duplicate['body'] == terminal['body'], 'duplicate changed sealed sample')
                        peer.event('robot_release_after_capture_terminal', mode=mode, role=role)
                        peer.ack(terminal)
                    counters = eventually(lambda: read_json(root / 'counters.json'))
                    require(counters['captures'] == mode * 2 and counters['dispatches'] == mode * 2,
                            '100 duplicate SAMPLE requests must not capture again')
                    result.update(captures=counters['captures'], dispatches=counters['dispatches'])
                with acceptance.case(f'robot_mode_{mode}_stale_camera_frame_profile_revisions_capture_zero'):
                    before = counters['captures']
                    for offset in (40, 152, 184, 208):
                        body = sample_body(manifest, session, uuid.uuid4().bytes, 1);body[offset] ^= 1
                        raw = peer.make(21, station=manifest['station'], body=body);peer.send(raw)
                        reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {190})
                        require(reply['reason'] in {4, 13, 20}, 'explicit stale revision rejection')
                    time.sleep(0.05)
                    require(eventually(lambda: read_json(root / 'counters.json'))['captures'] == before, 'stale revision acquired frame')
                with acceptance.case(f'robot_mode_{mode}_ABORT_drain_timeout_retry_control_ACK'):
                    (root / 'fail-drain').touch()
                    body = bytearray(384);body[:16] = session;put32(body, 16, 1)
                    raw = peer.make(25, station=manifest['station'], body=body)
                    sequence = int.from_bytes(raw[24:32], 'big');peer.send(raw)
                    first = peer.wait(sequence, {110, 190})
                    require(first['kind'] != 111, 'failed drain cannot release robot')
                    require(eventually(lambda: read_json(root / 'counters.json'))['leases'] == 1, 'lease retained until real drain')
                    (root / 'fail-drain').unlink();peer.send(raw)
                    terminal = peer.wait(sequence, {111});peer.ack(terminal)
                    eventually(lambda: eventually(lambda: read_json(root / 'counters.json'))['leases'] == 0)
        finally:
            peer.close()


def golden(args, acceptance):
    fixtures = args.source / 'tests/fixtures/communication-v4/golden'
    with acceptance.case('independent_peer_frozen_golden_bytes_and_digest', 'independent fixture oracle') as result:
        count = 0
        manifest = json.loads((fixtures.parent / 'manifest.json').read_text())
        valid = {case['name'] for case in manifest['cases'] if case['outcome'] == 'valid'}
        for path in fixtures.glob('*.bin'):
            if path.stem not in valid:
                continue
            raw = path.read_bytes()
            if path.stem.startswith(('bad_', 'catalog_')) or len(raw) not in (512, 1024):
                continue
            if len(raw) == 512:
                expected = frame(u32(raw, 8), int.from_bytes(raw[24:32], 'big'), raw[48:64], raw[64:80],
                                 raw[32:48], u32(raw, 20), u32(raw, 80), u32(raw, 84), raw[88:104], raw[128:])
                require(expected == raw, f'golden request mismatch {path.name}')
            else:
                decode(raw)
            count += 1
        require(count > 25, 'frozen golden coverage')
        for value in (0, 2**64, -1):
            try:
                frame(2, value, uuid.uuid4().bytes, uuid.uuid4().bytes)
            except ValueError:
                continue
            raise AssertionError('request zero/wrap accepted')
        result['vectors'] = count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', required=True, type=Path)
    parser.add_argument('--robot-server', required=True, type=Path)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    # Each CTest invocation has a unique root; no prior fixture state is inherited.
    args.output = Path(tempfile.mkdtemp(prefix='run-', dir=args.output))
    acceptance = Acceptance(args.output)
    try:
        golden(args, acceptance)
        production(args, acceptance)
        robot(args, acceptance)
    finally:
        acceptance.save()
        print(json.dumps({'report': str(args.output / 'result.json'),
                          'passed': sum(r['result'] == 'passed' for r in acceptance.rows),
                          'failed': sum(r['result'] == 'failed' for r in acceptance.rows)}), flush=True)


def crash_consumer(config):
    data = json.loads(Path(config).read_text(encoding='utf-8'))
    peer = Peer('127.0.0.1', data['port'], data['peer'], data['epoch'])
    peer.connect()
    terminal = peer.wait(int(data['request']), {111})
    require(terminal['body'][32:64].hex() == data['digest'], 'consumer exact terminal')
    with Path(data['effects']).open('a', encoding='utf-8') as effects:
        effects.write(json.dumps({'effect': terminal['body'][:16].hex(), 'pid': os.getpid()}) + '\n')
        effects.flush()
        os.fsync(effects.fileno())
    # Abrupt process loss: no cleanup, ACK, or graceful socket shutdown.
    os._exit(77)


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--consumer-crash':
        crash_consumer(sys.argv[2])
    else:
        main()
