"""VF-07: independent client against a normal product-created station, no server fixture."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time
import uuid

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from communication_v4_simulator import Peer, RequestState, discover, load_profile, station_status, terminal_summary, u32, put32


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser()
    parser.add_argument('--profile', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    profile = load_profile(args.profile)
    state_path = args.output / 'requests.json'
    simulator = Path(__file__).resolve().parents[1] / 'scripts' / 'communication_v4_simulator.py'
    cases = []

    def cli(name, *options):
        time.sleep(0.15)  # prior socket close is observed by the asynchronous owner
        result = subprocess.run([sys.executable, str(simulator), '--profile', str(args.profile),
                                 '--state', str(state_path), '--log', str(args.output / (name+'.jsonl')),
                                 *options], capture_output=True, text=True, encoding='utf-8', timeout=20)
        (args.output / (name+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
        require(result.returncode == 0, name + ': ' + result.stderr)
        cases.append(name)

    cli('discover', '--stations')
    cli('surface_A_OK_exact_ACK', '--station', 'İstasyon 1', '--trigger', '--expect-decision', 'OK')
    require(json.loads(state_path.read_text())['last'] == '1', 'first durable request is retained')
    with (args.output / 'negative-peer.jsonl').open('w', encoding='utf-8') as log:
        time.sleep(0.15)
        peer = Peer(profile['address'], profile['port'], profile['peer'], profile['epoch'], log, timeout=2)
        try:
            peer.connect()
            rows, _ = discover(peer)
            require(len(rows) == 1, 'one actual product station')
            station = rows[0]
            with RequestState(state_path, profile) as state:
                raw = state.reserve(peer, station)
                # Four stale fields are rejected without a production admission.
                for index, offset in enumerate((80, 84, 88, 128)):
                    stale = bytearray(raw)
                    stale[24:32] = (1000+index).to_bytes(8, 'big')
                    if offset in (80, 84):
                        put32(stale, offset, u32(stale, offset)+1)
                    else:
                        stale[offset:offset+16] = uuid.uuid4().bytes
                    peer.send(stale)
                    reply = peer.wait(1000+index, {190}, station['key'])
                    require(reply['reason'] in {4, 10}, 'stale selection rejection')
                    status = station_status(peer, station)
                    require(status['ready'] and not status['busy'] and not status['pending'], 'stale request did not start work')
                cases.append('four_stale_selection_fields_no_cycle')
                peer.send(raw)
                peer.wait(2, {110}, station['key'])
                terminal = peer.wait(2, {111}, station['key'])
                summary = terminal_summary(terminal, raw)
                require(summary['valid'] and summary['decision'] == 'OK', 'real surface output valid OK')
                for name, station_key, corrupt in [('digest', station['key'], 16), ('result', station['key'], 0), ('station', station['key']+1, None)]:
                    body = bytearray(terminal['raw'][128:144] + terminal['raw'][160:192] + bytes(336))
                    if corrupt is not None:
                        body[corrupt] ^= 1
                    ack = peer.make(11, request=2, station=station_key, recipe=station['recipe'],
                                    template=station['template'], activation=station['activation'], body=body)
                    peer.send(ack)
                    reply = peer.wait(2, {190}, station_key)
                    require(reply['reason'] != 0, 'wrong '+name+' ACK rejected')
                    require(station_status(peer, station)['pending'] == 1, 'wrong ACK did not close delivery')
                cases.append('wrong_digest_result_station_ACK_retains_pending')
                # No ACK: bounded wait times out; no new mutation is sent.
                try:
                    peer.wait(2, {112}, station['key'])
                    raise AssertionError('ACKED invented')
                except TimeoutError:
                    pass
                require(state.state['last'] == '2' and state.state['pending'], 'timeout retains exact request')
                cases.append('timeout_keeps_unresolved_request')
            sealed = summary['result'], summary['digest']
        finally:
            peer.close()
    cli('disconnect_recover_same_result_exact_ACK', '--recover', '--expect-decision', 'OK')
    records = [json.loads(line) for line in (args.output/'disconnect_recover_same_result_exact_ACK.jsonl').read_text().splitlines()]
    delivery = next(r for r in records if r['event'] == 'delivery')
    require((delivery['result'], delivery['digest']) == sealed, 'recovery never recomputes terminal')
    state = json.loads(state_path.read_text())
    require(state['last'] == '2' and state['pending'] is None, 'recovery never creates request 3')
    result = dict(cases=cases, passed=len(cases), station='İstasyon 1', expected='OK',
                  actual='OK', valid=True, delivery='ACKED', mutations=2, physical='BLOCKED', exactlyOnce=False)
    (args.output/'result.json').write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
